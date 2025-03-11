use std::collections::HashMap;
use std::mem::size_of;

use itertools::izip;
use log::error;
use tokio::runtime::Handle;
use tokio::task::JoinSet;

use super::config::CHANNEL_CAPACITY;
use super::errors::{QueueError, ReceiveError, SendError, SendReceiveError, ServerError, SizeMismatchError};
use super::hash::Hash;
use super::metadata::{self, AllGather, AllToAll, BaseMessageID, Broadcast, Gather, ToMessage};
use super::ptr::{NonNullData, ReadData, WriteData};
use super::{client, message_buffer, server, Communicator, Config, DataChannel, DataCommand, MessageDatatype, MessageID, MessageKind, MessageSize, NetChannel, NetCommand, PartyID, SendMessage};
#[cfg(feature = "collective-consistency")]
use super::{consistency, ConsistencyCheckChannel, ConsistencyCheckCommand};
use crate::vec::Vec2d;

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub struct NetworkStatistics
{
    sent: MessageSize,
    received: MessageSize,
    rounds: MessageSize,
}

impl NetworkStatistics
{
    #[must_use]
    pub fn new() -> Self
    {
        NetworkStatistics { sent: 0, received: 0, rounds: 0 }
    }
}
impl Default for NetworkStatistics
{
    fn default() -> Self
    {
        Self::new()
    }
}

fn check_size(x: usize, y: MessageSize) -> Result<(), SizeMismatchError>
{
    let x = x as MessageSize;
    if x == y
    {
        Ok(())
    }
    else
    {
        Err(SizeMismatchError(x, y))
    }
}

async fn collect_tasks<E>(mut tasks: JoinSet<Result<(), E>>) -> Result<(), SendReceiveError>
where
    SendReceiveError: From<E>,
    E: 'static + std::fmt::Debug,
{
    let mut result = Ok(());
    while let Some(task_result) = tasks.join_next().await
    {
        let new_result = match task_result
        {
            Ok(Ok(v)) => Ok(v),
            Err(e) =>
            {
                error!("Join failed: {:?}", e);
                Err(SendReceiveError::Join(e))
            },
            Ok(Err(e)) =>
            {
                error!("Task failed: {:?}", e);
                Err(e.into())
            },
        };

        if let Ok(()) = result
        {
            result = new_result;
        }
        else
        {
            result = Err(SendReceiveError::Multiple);
        }
    }
    result
}

type CounterKey = Hash;

#[derive(Debug)]
struct QueueState
{
    id: PartyID,
    data_channel: DataChannel,
    net_channel: NetChannel,
    #[cfg(feature = "collective-consistency")]
    consistency_channel: ConsistencyCheckChannel,
    counters: HashMap<(MessageKind, MessageDatatype, CounterKey), MessageID>,
    #[cfg(feature = "statistics")]
    network_statistics: NetworkStatistics,
}

impl QueueState
{
    fn update_counter(&mut self, kind: MessageKind, datatype: MessageDatatype, key: CounterKey) -> MessageID
    {
        const MESSAGE_ID_SIZE: usize = size_of::<MessageID>();
        const _: () = assert!(MESSAGE_ID_SIZE <= size_of::<CounterKey>());

        // PANICS: Assert above checks that array is large enough
        let id = key.first_chunk::<MESSAGE_ID_SIZE>().unwrap();
        let id = MessageID::from_le_bytes(*id);

        let counter = *self.counters.entry((kind, datatype, key))
            .and_modify(|c| *c += 1)
            .or_insert(1);

        id + counter
    }

    fn next_message_id<Message: BaseMessageID>(&mut self, message: Message, communicator: &Communicator) -> MessageID
    {
        self.extended_next_message_id(message, communicator, communicator)
    }

    fn extended_next_message_id<Message: BaseMessageID>(&mut self, message: Message, senders: &Communicator, receivers: &Communicator) -> MessageID
    {
        let key = message.hash(senders, receivers).as_ref().try_into().unwrap();

        self.update_counter(Message::KIND, message.datatype(), key)
    }

    #[cfg(feature = "collective-consistency")]
    async fn check_consistency(&mut self, check: metadata::ConsistencyCheck)
    {
        self.consistency_channel
            .send(ConsistencyCheckCommand::Request(check))
            .await
            .expect("Consistency check unavailable");
    }

    #[cfg(feature = "collective-consistency")]
    async fn ensure_consistency(&mut self) -> Result<(), ReceiveError>
    {
        let (send_answer, receive_answer) = tokio::sync::oneshot::channel();
        self.consistency_channel
            .send(ConsistencyCheckCommand::Wait(send_answer))
            .await
            .expect("Consistency check unavailable");
        receive_answer.await??;
        Ok(())
    }

    async fn send_message(net_channel: tokio::sync::mpsc::Sender<NetCommand>, message: metadata::Message, data: ReadData) -> Result<(), SendError>
    {
        let (send_answer, receive_answer) = tokio::sync::oneshot::channel();
        net_channel
            .send(NetCommand::Send(SendMessage { metadata: message, data }, send_answer))
            .await?;
        receive_answer.await??;
        Ok(())
    }

    async fn receive_message(data_channel: tokio::sync::mpsc::Sender<DataCommand>, message: metadata::Message, data: WriteData) -> Result<(), ReceiveError>
    {
        let (send_answer, receive_answer) = tokio::sync::oneshot::channel();
        let message_size = message.size;
        data_channel.send(DataCommand::Receive(message, send_answer)).await?;
        let result = receive_answer.await?;
        check_size(result.len(), message_size).map_err(ServerError::SizeMismatch)?;
        unsafe { std::ptr::copy_nonoverlapping(result.as_ptr(), data.as_ptr(), result.len()) };
        Ok(())
    }

    fn send_on<E>(&mut self, tasks: &mut JoinSet<Result<(), E>>, handle: &Handle, message: metadata::Message, data: NonNullData)
    where
        E: From<SendError> + Send + 'static,
    {
        #[cfg(feature = "statistics")]
        {
            self.network_statistics.sent += message.size;
        }

        let net_channel = self.net_channel.clone();
        let data = data.into();
        tasks.spawn_on(async
        {
            Self::send_message(net_channel, message, data).await.map_err(Into::into)
        }, handle);
    }

    fn receive_on<E>(&mut self, tasks: &mut JoinSet<Result<(), E>>, handle: &Handle, message: metadata::Message, data: NonNullData)
    where
        E: From<ReceiveError> + Send + 'static,
    {
        #[cfg(feature = "statistics")]
        {
            self.network_statistics.received += message.size;
        }

        let data_channel = self.data_channel.clone();
        let data = data.into();
        tasks.spawn_on(async
        {
            Self::receive_message(data_channel, message, data).await.map_err(Into::into)
        }, handle);
    }

    pub(crate) async fn broadcast(&mut self, handle: &Handle, message: Broadcast, communicator: &Communicator, data: NonNullData) -> Result<(), SendReceiveError>
    {
        self.multi_broadcast(handle, vec![message], communicator, vec![data]).await
    }

    pub(crate) async fn multi_broadcast(&mut self, handle: &Handle, messages: Vec<Broadcast>, communicator: &Communicator, data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        assert_eq!(messages.len(), data.len());
        #[cfg(feature = "collective-consistency")]
        if messages.iter().any(|message| message.sender == self.id)
        {
            self.ensure_consistency().await?;
        }
        #[cfg(feature = "statistics")]
        {
            self.network_statistics.rounds += 1;
        }

        let id = self.id; // to prevent borrowing self later as immutable and mutable
        let mut tasks: JoinSet<Result<(), SendReceiveError>> = JoinSet::new();
        for (message, data) in messages.into_iter().zip(data)
        {
            let message_id = self.next_message_id(&message, communicator);

            if message.sender == id
            {
                for &receiver in communicator.iter().filter(|&&party| party != id)
                {
                    let message = message.to_message(id, receiver, message_id);
                    self.send_on(&mut tasks, handle, message, data);
                }
            }
            else
            {
                #[cfg(feature = "collective-consistency")]
                {
                    let receivers = communicator
                        .iter()
                        .filter(|&&party| party != id && party != message.sender)
                        .copied()
                        .collect();
                    let check = (&message).to_consistency_check(message.sender, receivers, message_id);
                    self.check_consistency(check).await;
                }

                let message = message.to_message(message.sender, id, message_id);
                self.receive_on(&mut tasks, handle, message, data);
            }
        }
        collect_tasks(tasks).await
    }

    pub(crate) async fn gather(&mut self, handle: &Handle, message: Gather, communicator: &Communicator, data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.multi_gather(handle, vec![message], communicator, Vec2d::from_inner(data)).await
    }

    pub(crate) async fn multi_gather(&mut self, handle: &Handle, messages: Vec<Gather>, communicator: &Communicator, data: Vec2d<NonNullData>) -> Result<(), SendReceiveError>
    {
        assert_eq!(messages.len(), data.outer_extent());
        #[cfg(feature = "collective-consistency")]
        if messages.iter().any(|message| message.receiver != self.id)
        {
            self.ensure_consistency().await?;
        }
        #[cfg(feature = "statistics")]
        {
            self.network_statistics.rounds += 1;
        }

        let id = self.id; // to prevent borrowing self later as immutable and mutable
        let mut tasks: JoinSet<Result<(), SendReceiveError>> = JoinSet::new();
        for (message, data) in messages.into_iter().zip(&data)
        {
            let message_id = self.next_message_id(&message, communicator);

            if message.receiver == id
            {
                assert_eq!(communicator.len(), data.len());

                for (&party, &data) in communicator.iter().zip(data).filter(|&(&party, _)| party != id)
                {
                    let message = message.to_message(party, id, message_id);
                    self.receive_on(&mut tasks, handle, message, data);
                }
            }
            else
            {
                assert_eq!(data.len(), 1);
                let message = message.to_message(id, message.receiver, message_id);
                self.send_on(&mut tasks, handle, message, data[0]);
            }
        }
        collect_tasks(tasks).await
    }

    pub(crate) async fn all_gather(&mut self, handle: &Handle, message: AllGather, communicator: &Communicator, data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.extended_all_gather(handle, message, communicator, communicator, data).await
    }

    pub(crate) async fn multi_all_gather(&mut self, handle: &Handle, messages: Vec<AllGather>, communicator: &Communicator, data: Vec2d<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.extended_multi_all_gather(handle, messages, communicator, communicator, data).await
    }

    pub(crate) async fn extended_all_gather(&mut self, handle: &Handle, message: AllGather, senders: &Communicator, receivers: &Communicator, data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.extended_multi_all_gather(handle, vec![message], senders, receivers, Vec2d::from_inner(data)).await
    }

    pub(crate) async fn extended_multi_all_gather(&mut self, handle: &Handle, messages: Vec<AllGather>, senders: &Communicator, receivers: &Communicator, data: Vec2d<NonNullData>) -> Result<(), SendReceiveError>
    {
        assert_eq!(senders.len(), data.inner_extent());
        assert_eq!(messages.len(), data.outer_extent());
        assert!(senders.is_subset(receivers));
        assert!(receivers.contains(&self.id));
        #[cfg(feature = "collective-consistency")]
        if senders.iter().any(|&sender| sender == self.id)
        {
            self.ensure_consistency().await?;
        }
        #[cfg(feature = "statistics")]
        {
            self.network_statistics.rounds += 1;
        }

        let id = self.id; // to prevent borrowing self later as immutable and mutable
        let mut tasks: JoinSet<Result<(), SendReceiveError>> = JoinSet::new();
        for (message, data) in messages.into_iter().zip(&data)
        {
            let message_id = self.extended_next_message_id(&message, senders, receivers);

            for (&sender, &data) in senders.iter().zip(data)
            {
                if sender == id
                {
                    for &receiver in receivers.iter().filter(|&&party| party != id)
                    {
                        let message = message.to_message(sender, receiver, message_id);
                        self.send_on(&mut tasks, handle, message, data);
                    }
                }
                else
                {
                    #[cfg(feature = "collective-consistency")]
                    {
                        let receivers = receivers
                            .iter()
                            .filter(|&&party| party != id && party != sender)
                            .copied()
                            .collect();
                        let check = (&message).to_consistency_check(sender, receivers, message_id);
                        self.check_consistency(check).await;
                    }

                    let message = message.to_message(sender, id, message_id);
                    self.receive_on(&mut tasks, handle, message, data);
                }
            }
        }

        collect_tasks(tasks).await
    }

    pub(crate) async fn all_to_all(&mut self, handle: &Handle, message: AllToAll, communicator: &Communicator, send_data: Vec<NonNullData>, receive_data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.multi_all_to_all(handle, vec![message], communicator, Vec2d::from_inner(send_data), Vec2d::from_inner(receive_data)).await
    }

    pub(crate) async fn multi_all_to_all(&mut self, handle: &Handle, messages: Vec<AllToAll>, communicator: &Communicator, send_data: Vec2d<NonNullData>, receive_data: Vec2d<NonNullData>) -> Result<(), SendReceiveError>
    {
        assert_eq!(communicator.len(), send_data.inner_extent());
        assert_eq!(communicator.len(), receive_data.inner_extent());
        assert_eq!(messages.len(), send_data.outer_extent());
        assert_eq!(messages.len(), receive_data.outer_extent());
        assert!(communicator.contains(&self.id));
        #[cfg(feature = "collective-consistency")]
        self.ensure_consistency().await?;
        #[cfg(feature = "statistics")]
        {
            self.network_statistics.rounds += 1;
        }

        let id = self.id; // to prevent borrowing self later as immutable and mutable
        let mut tasks: JoinSet<Result<(), SendReceiveError>> = JoinSet::new();
        for (message, send_data, receive_data) in izip!(messages, &send_data, &receive_data)
        {
            let message_id = self.next_message_id(&message, communicator);

            for (&party, &send_data, &receive_data) in izip!(communicator.iter(), send_data, receive_data)
            {
                if party != id
                {
                    self.send_on(&mut tasks, handle, message.to_message(id, party, message_id), send_data);
                    self.receive_on(&mut tasks, handle, message.to_message(party, id, message_id), receive_data);
                }
            }
        }

        collect_tasks(tasks).await
    }
}

#[derive(Debug)]
pub struct Queue
{
    runtime: tokio::runtime::Runtime,
    state: QueueState,
}

impl Queue
{
    /// Construct a `Queue` managing a `tokio` runtime
    ///
    /// This also spawns multiple tasks to handle incoming and outgoing messages.
    /// # Errors
    /// Fails if the `tokio` runtime cannot be constructed.
    /// Then, the errors are simply forwarded to the caller.
    pub fn new(id: PartyID, mut config: Config) -> Result<Self, QueueError>
    {
        #[rustfmt::skip]
        let runtime = tokio::runtime::Builder::new_multi_thread()
            .enable_all()
            .build()?;


        #[cfg(feature = "sessions")]
        {
            use super::errors::SessionError;
            if let Some(session) = config.session.as_mut()
            {
                session.try_assign_value().map_err(Into::<SessionError>::into)?;
            }
            else
            {
                return Err(SessionError::NotFound.into());
            }
        }

        let (data_channel, receive_channel) = tokio::sync::mpsc::channel(CHANNEL_CAPACITY);

        // thread to handle data access
        runtime.spawn(message_buffer::run(id, receive_channel));

        let (net_channel, receive_channel) = tokio::sync::mpsc::channel(CHANNEL_CAPACITY);

        #[cfg(feature = "collective-consistency")]
        let consistency_channel =
        {
            let (consistency_channel, receive_channel) = tokio::sync::mpsc::channel(CHANNEL_CAPACITY);

            // thread to handle consistency checks
            runtime.spawn(consistency::run(id, config.clone(), receive_channel, net_channel.clone()));

            consistency_channel
        };

        // thread to handle receiving data
        // spawns new threads for each connection
        runtime.spawn(server::run(
            id,
            config.clone(),
            data_channel.clone(),
            #[cfg(feature = "collective-consistency")]
            consistency_channel.clone(),
        ));

        // thread to handle sending data
        // spawns new threads for each connection
        runtime.spawn(client::run(id, config, receive_channel));

        Ok(Self {
            runtime,
            state: QueueState
            {
                id,
                data_channel,
                net_channel,
                #[cfg(feature = "collective-consistency")]
                consistency_channel,
                counters: HashMap::new(),
                #[cfg(feature = "statistics")]
                network_statistics: NetworkStatistics::new(),
            },
        })
    }

    pub fn id(&self) -> PartyID
    {
        self.state.id
    }

    /// Broadcast
    /// # Errors
    /// Forwards errors from sending or receiving
    pub async fn broadcast(&mut self, message: Broadcast, communicator: &Communicator, data: NonNullData) -> Result<(), SendReceiveError>
    {
        self.state.broadcast(self.runtime.handle(), message, communicator, data).await
    }

    /// Broadcast (blocking)
    /// # Errors
    /// Forwards errors from sending or receiving
    pub fn broadcast_blocking(&mut self, message: Broadcast, communicator: &Communicator, data: NonNullData) -> Result<(), SendReceiveError>
    {
        self.runtime.block_on(self.state.broadcast(self.runtime.handle(), message, communicator, data))
    }

    /// Multi broadcast
    /// # Errors
    /// Forwards errors from sending or receiving
    pub async fn multi_broadcast(&mut self, messages: Vec<Broadcast>, communicator: &Communicator, data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.state.multi_broadcast(self.runtime.handle(), messages, communicator, data).await
    }

    /// Multi broadcast (blocking)
    /// # Errors
    /// Forwards errors from sending or receiving
    pub fn multi_broadcast_blocking(&mut self, messages: Vec<Broadcast>, communicator: &Communicator, data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.runtime.block_on(self.state.multi_broadcast(self.runtime.handle(), messages, communicator, data))
    }

    /// Gather
    /// # Errors
    /// Forwards errors from sending or receiving
    pub async fn gather(&mut self, message: Gather, communicator: &Communicator, data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.state.gather(self.runtime.handle(), message, communicator, data).await
    }

    /// Gather (blocking)
    /// # Errors
    /// Forwards errors from sending or receiving
    pub fn gather_blocking(&mut self, message: Gather, communicator: &Communicator, data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.runtime.block_on(self.state.gather(self.runtime.handle(), message, communicator, data))
    }

    /// Multi gather
    /// # Errors
    /// Forwards errors from sending or receiving
    pub async fn multi_gather(&mut self, messages: Vec<Gather>, communicator: &Communicator, data: Vec2d<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.state.multi_gather(self.runtime.handle(), messages, communicator, data).await
    }

    /// Multi gather (blocking)
    /// # Errors
    /// Forwards errors from sending or receiving
    pub fn multi_gather_blocking(&mut self, messages: Vec<Gather>, communicator: &Communicator, data: Vec2d<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.runtime.block_on(self.state.multi_gather(self.runtime.handle(), messages, communicator, data))
    }

    /// All-gather
    /// # Errors
    /// Forwards errors from sending and/or receiving
    pub async fn all_gather(&mut self, message: AllGather, communicator: &Communicator, data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.state.all_gather(self.runtime.handle(), message, communicator, data).await
    }

    /// All-gather (blocking)
    /// # Errors
    /// Forwards errors from sending and/or receiving
    pub fn all_gather_blocking(&mut self, message: AllGather, communicator: &Communicator, data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.runtime.block_on(self.state.all_gather(self.runtime.handle(), message, communicator, data))
    }

    /// Multi all-gather
    /// # Errors
    /// Forwards errors from sending and/or receiving
    pub async fn multi_all_gather(&mut self, messages: Vec<AllGather>, communicator: &Communicator, data: Vec2d<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.state.multi_all_gather(self.runtime.handle(), messages, communicator, data).await
    }

    /// Multi all-gather (blocking)
    /// # Errors
    /// Forwards errors from sending and/or receiving
    pub fn multi_all_gather_blocking(&mut self, messages: Vec<AllGather>, communicator: &Communicator, data: Vec2d<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.runtime.block_on(self.state.multi_all_gather(self.runtime.handle(), messages, communicator, data))
    }

    /// Extended all-gather
    /// # Errors
    /// Forwards errors from sending and/or receiving
    pub async fn extended_all_gather(&mut self, message: AllGather, senders: &Communicator, receivers: &Communicator, data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.state.extended_all_gather(self.runtime.handle(), message, senders, receivers, data).await
    }

    /// Extended all-gather (blocking)
    /// # Errors
    /// Forwards errors from sending and/or receiving
    pub fn extended_all_gather_blocking(&mut self, message: AllGather, senders: &Communicator, receivers: &Communicator, data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.runtime.block_on(self.state.extended_all_gather(self.runtime.handle(), message, senders, receivers, data))
    }

    /// Extended multi all-gather
    /// # Errors
    /// Forwards errors from sending and/or receiving
    pub async fn extended_multi_all_gather(&mut self, messages: Vec<AllGather>, senders: &Communicator, receivers: &Communicator, data: Vec2d<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.state.extended_multi_all_gather(self.runtime.handle(), messages, senders, receivers, data).await
    }

    /// Extended multi all-gather (blocking)
    /// # Errors
    /// Forwards errors from sending and/or receiving
    pub fn extended_multi_all_gather_blocking(&mut self, messages: Vec<AllGather>, senders: &Communicator, receivers: &Communicator, data: Vec2d<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.runtime.block_on(self.state.extended_multi_all_gather(self.runtime.handle(), messages, senders, receivers, data))
    }

    /// All-to-all
    /// # Errors
    /// Forwards errors from sending and/or receiving
    pub async fn all_to_all(&mut self, message: AllToAll, communicator: &Communicator, send_data: Vec<NonNullData>, receive_data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.state.all_to_all(self.runtime.handle(), message, communicator, send_data, receive_data).await
    }

    /// All-to-all (blocking)
    /// # Errors
    /// Forwards errors from sending and/or receiving
    pub fn all_to_all_blocking(&mut self, message: AllToAll, communicator: &Communicator, send_data: Vec<NonNullData>, receive_data: Vec<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.runtime.block_on(self.state.all_to_all(self.runtime.handle(), message, communicator, send_data, receive_data))
    }

    /// Multi all-to-all
    /// # Errors
    /// Forwards errors from sending and/or receiving
    pub async fn multi_all_to_all(&mut self, messages: Vec<AllToAll>, communicator: &Communicator, send_data: Vec2d<NonNullData>, receive_data: Vec2d<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.state.multi_all_to_all(self.runtime.handle(), messages, communicator, send_data, receive_data).await
    }

    /// Multi all-to-all (blocking)
    /// # Errors
    /// Forwards errors from sending and/or receiving
    pub fn multi_all_to_all_blocking(&mut self, messages: Vec<AllToAll>, communicator: &Communicator, send_data: Vec2d<NonNullData>, receive_data: Vec2d<NonNullData>) -> Result<(), SendReceiveError>
    {
        self.runtime.block_on(self.state.multi_all_to_all(self.runtime.handle(), messages, communicator, send_data, receive_data))
    }

    #[cfg(feature = "collective-consistency")]
    pub fn wait(&mut self) -> Result<(), SendReceiveError>
    {
        self.runtime.block_on(self.state.ensure_consistency()).map_err(Into::into)
    }

    #[cfg(feature = "statistics")]
    pub fn network_statistics(&mut self) -> NetworkStatistics
    {
        self.state.network_statistics
    }
}
#[cfg(feature = "collective-consistency")]
impl Drop for Queue
{
    fn drop(&mut self)
    {
        self.runtime.block_on(self.state.ensure_consistency()).unwrap()
    }
}
