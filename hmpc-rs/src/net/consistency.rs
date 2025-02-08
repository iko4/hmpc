use std::collections::hash_map::{Entry, Keys};
use std::collections::HashMap;

use log::{debug, error, warn};

use super::errors::{ConsistencyCheckError, SendError};
use super::{metadata, Communicator, Config, ConsistencyCheckCommand, ConsistencyCheckMessage, MessageDatatype, MessageID, MessageKind, NetChannel, NetCommand, PartyID, SendMessage, MESSAGE_FEATURE_FLAGS, MESSAGE_FORMAT_VERSION};
#[cfg(feature = "sessions")]
use super::SessionID;
use super::hash::Hash;
use super::sign::{PublicKey, Signature};

struct ConsistencyCheckData
{
    hash: Hash,
    signature: Signature,
}

#[derive(Debug, PartialEq, Eq, Hash, Clone)]
struct ConsistencyCheckKey
{
    kind: MessageKind,
    datatype: MessageDatatype,
    sender: PartyID,
    id: MessageID,
}
impl From<metadata::ConsistencyCheck> for (ConsistencyCheckKey, Communicator)
{
    fn from(value: metadata::ConsistencyCheck) -> Self
    {
        (ConsistencyCheckKey { kind: value.kind, datatype: value.datatype, sender: value.sender, id: value.id }, value.receivers)
    }
}
impl From<ConsistencyCheckMessage> for (ConsistencyCheckKey, PartyID, ConsistencyCheckData)
{
    fn from(value: ConsistencyCheckMessage) -> Self
    {
        // PANICS: Only MessageKind that is check is sent from calling code
        (ConsistencyCheckKey { kind: value.metadata.kind.try_from_check().unwrap(), datatype: value.metadata.datatype, sender: value.metadata.sender, id: value.metadata.id }, value.metadata.receiver, ConsistencyCheckData { hash: value.hash, signature: value.signature })
    }
}
impl From<ConsistencyCheckMessage> for (ConsistencyCheckKey, ConsistencyCheckData)
{
    fn from(value: ConsistencyCheckMessage) -> Self
    {
        (ConsistencyCheckKey { kind: value.metadata.kind, datatype: value.metadata.datatype, sender: value.metadata.sender, id: value.metadata.id }, ConsistencyCheckData { hash: value.hash, signature: value.signature })
    }
}

struct ConsistencyCheckValue
{
    received: Option<ConsistencyCheckData>,
    receivers: HashMap<PartyID, Option<ConsistencyCheckData>>,
    registered: bool,
}
impl ConsistencyCheckValue
{
    fn new_request(receivers: Communicator) -> Self
    {
        Self { received: None, receivers: receivers.iter().map(|&party| (party, None)).collect(), registered: true }
    }

    fn new_received(receiver: PartyID, check: ConsistencyCheckData) -> Self
    {
        Self { received: None, receivers: HashMap::from([(receiver, Some(check))]), registered: false }
    }

    fn new_received_data(check: ConsistencyCheckData) -> Self
    {
        Self { received: Some(check), receivers: HashMap::new(), registered: false }
    }

    fn register(&mut self, receivers: &Communicator) -> Result<bool, ConsistencyCheckError>
    {
        if self.registered
        {
            return Err(ConsistencyCheckError::MultipleRequests);
        }
        if !receivers.is_superset(&self.receivers.keys().copied().collect())
        {
            self.receivers = receivers.iter().map(|&receiver| (receiver, self.receivers.remove(&receiver).unwrap_or(None))).collect();

            return Err(ConsistencyCheckError::UnknownCheck);
        }

        for &receiver in receivers
        {
            self.receivers.entry(receiver).or_insert(None);
        }

        self.registered = true;
        Ok(self.received.is_some())
    }

    fn receive(&mut self, receiver: PartyID, check: ConsistencyCheckData) -> Result<(), ConsistencyCheckError>
    {
        match self.receivers.entry(receiver)
        {
            Entry::Occupied(mut entry) =>
            {
                let value = entry.get();
                if value.is_some()
                {
                    return Err(ConsistencyCheckError::MultipleChecks);
                }

                entry.insert(Some(check));
            },
            Entry::Vacant(entry) =>
            {
                if self.registered
                {
                    return Err(ConsistencyCheckError::UnknownCheck);
                }

                entry.insert(Some(check));
            },
        }
        Ok(())
    }

    fn receive_data(&mut self, check: ConsistencyCheckData) -> Result<bool, ConsistencyCheckError>
    {
        if self.received.is_some()
        {
            return Err(ConsistencyCheckError::MultipleMessages);
        }

        self.received = Some(check);
        Ok(self.registered)
    }

    fn check(&mut self, metadata: &ConsistencyCheckKey, verification_keys: &HashMap<PartyID, PublicKey>, #[cfg(feature = "sessions")] session: SessionID) -> Result<bool, ConsistencyCheckError>
    {
        if !self.registered
        {
            return Ok(false) // not ready to check
        }
        let Some(received) = &self.received
        else
        {
            return Ok(false) // not ready to check
        };

        let mut valid_receivers = Vec::new();
        for (&receiver, check) in self.receivers.iter()
        {
            if let Some(check) = check
            {
                check_signature(metadata, receiver, check, verification_keys, #[cfg(feature = "sessions")] session)?;

                if received.hash != check.hash
                {
                    return Err(ConsistencyCheckError::InconsistentCollectiveCommunication);
                }

                valid_receivers.push(receiver);
            }
        }

        for receiver in valid_receivers
        {
            self.receivers.remove(&receiver);
        }

        Ok(true)
    }

    fn is_done(&self) -> bool
    {
        self.registered && self.received.is_some() && self.receivers.is_empty()
    }

    fn keys(&self) -> Keys<PartyID, Option<ConsistencyCheckData>>
    {
        self.receivers.keys()
    }

    /// Get the received data (hash and signature)
    /// # Panics
    /// If no data was received before
    fn data(&self) -> &ConsistencyCheckData
    {
        // PANICS: Called below only if it is clear that data was received
        self.received.as_ref().unwrap()
    }
}

fn check_signature(metadata: &ConsistencyCheckKey, receiver: PartyID, check: &ConsistencyCheckData, verification_keys: &HashMap<PartyID, PublicKey>, #[cfg(feature = "sessions")] session: SessionID) -> Result<(), ConsistencyCheckError>
{
    let data =
    [
        MESSAGE_FORMAT_VERSION.to_le_bytes().as_slice(),
        MESSAGE_FEATURE_FLAGS.to_le_bytes().as_slice(),
        metadata.kind.to_le_bytes().as_slice(),
        metadata.datatype.to_le_bytes().as_slice(),
        metadata.sender.to_le_bytes().as_slice(),
        receiver.to_le_bytes().as_slice(),
        metadata.id.to_le_bytes().as_slice(),
        #[cfg(feature = "sessions")]
        session.to_le_bytes().as_slice(),
        check.hash.as_slice(),
    ].concat();

    verification_keys.get(&metadata.sender)
        .map_or(Err(ConsistencyCheckError::UnknownSender), Ok)?
        .verify(&data, &check.signature)
        .or(Err(ConsistencyCheckError::InconsistentSignatureVerification))
}

async fn notify_parties<Receivers>(id: PartyID, metadata: &ConsistencyCheckKey, receivers: Receivers, data: &ConsistencyCheckData, net_channel: &NetChannel) -> Result<(), SendError>
where
    Receivers: Iterator<Item = PartyID>
{
    let buffer = [data.hash.as_slice(), data.signature.as_slice()].concat();
    let data = super::ptr::ReadData::new_unchecked(buffer.as_ptr());
    // PANICS: hash size and signature size are small enough to fit into the value range
    let size = buffer.len().try_into().unwrap();

    for receiver in receivers
    {
        let (send_answer, receive_answer) = tokio::sync::oneshot::channel();
        net_channel.send(
            NetCommand::SendCheck(
                SendMessage
                {
                    metadata: metadata::Message::new(
                        // PANICS: Only MessageKind that supports checking is registered in calling code
                        metadata.kind.try_to_check().unwrap(),
                        metadata.datatype,
                        metadata.sender,
                        id,
                        metadata.id,
                        size
                    ),
                    data: data.clone()
                },
                receiver,
                send_answer
            )
        ).await?;
        receive_answer.await??;
    }
    Ok(())
}

fn is_consistent(consistency: &HashMap<ConsistencyCheckKey, ConsistencyCheckValue>) -> bool
{
    consistency.values().all(|value| !value.registered)
}

fn notify_requests(open_requests: &mut Vec<tokio::sync::oneshot::Sender<Result<(), ConsistencyCheckError>>>)
{
    for request in open_requests.drain(..)
    {
        if let Err(_) = request.send(Ok(()))
        {
            error!("No longer waiting for consistency check");
        }
    }
}

pub(super) async fn run(id: PartyID, config: Config, mut receive_channel: tokio::sync::mpsc::Receiver<ConsistencyCheckCommand>, net_channel: NetChannel)
{
    // PANICS: Calling code checks that config.session is not None and contains a value
    #[cfg(feature = "sessions")]
    let session = config.session.clone().unwrap().unwrap();
    let verification_keys = config.verification_keys().await.unwrap();
    let mut consistency = HashMap::<ConsistencyCheckKey, ConsistencyCheckValue>::new();
    let mut open_requests = Vec::new();

    while let Some(command) = receive_channel.recv().await
    {
        match command
        {
            ConsistencyCheckCommand::Request(metadata) =>
            {
                let (key, receivers) = metadata.into();
                debug!("[Party {}] Requesting consistency check of {:?} from {:?}", id, key, receivers);
                match consistency.entry(key)
                {
                    Entry::Occupied(mut entry) =>
                    {
                        let metadata = entry.key().clone();
                        let value = entry.get_mut();
                        match value.register(&receivers)
                        {
                            Err(e) => error!("[Party {}] Error when registering check: {:?}", id, e),
                            Ok(false) => (), // not ready
                            Ok(true) => // got ready
                            {
                                if let Err(e) = notify_parties(id, &metadata, receivers.iter().copied(), value.data(), &net_channel).await
                                {
                                    warn!("[Party {}] Could not send consistency check to other parties. Reason: {:?}", id, e);
                                }
                            },
                        }

                        match value.check(&metadata, &verification_keys, #[cfg(feature = "sessions")] session)
                        {
                            Ok(false) => (), // not ready
                            Err(e) => error!("[Party {}] Error when checking: {:?}", id, e),
                            Ok(true) =>
                            {
                                if value.is_done()
                                {
                                    entry.remove();
                                }
                            },
                        }

                        if is_consistent(&consistency)
                        {
                            notify_requests(&mut open_requests);
                        }
                    },
                    Entry::Vacant(entry) =>
                    {
                        let value = ConsistencyCheckValue::new_request(receivers);
                        entry.insert(value);
                    },
                }
            },
            ConsistencyCheckCommand::ReceivedCheck(message) =>
            {
                let (key, receiver, check) = message.into();
                debug!("[Party {}] Received consistency check {:?} from {:?}", id, key, receiver);
                match consistency.entry(key)
                {
                    Entry::Occupied(mut entry) =>
                    {
                        let metadata = entry.key().clone(); // clone because we cannot borrow the value mutably and the key at the same time
                        let value = entry.get_mut();
                        if let Err(e) = value.receive(receiver, check)
                        {
                            error!("[Party {}] Error when receiving check: {:?}", id, e);
                        }

                        match value.check(&metadata, &verification_keys, #[cfg(feature = "sessions")] session)
                        {
                            Ok(false) => (), // not ready
                            Err(e) => error!("[Party {}] Error when checking: {:?}", id, e),
                            Ok(true) =>
                            {
                                if value.is_done()
                                {
                                    entry.remove();
                                }
                            },
                        }

                        if is_consistent(&consistency)
                        {
                            notify_requests(&mut open_requests);
                        }
                    },
                    Entry::Vacant(entry) =>
                    {
                        let value = ConsistencyCheckValue::new_received(receiver, check);
                        entry.insert(value);
                    }
                }
            },
            ConsistencyCheckCommand::ReceivedData(message) =>
            {
                let (key, check) = message.into();
                debug!("[Party {}] Received data for consistency check of {:?}", id, key);
                match consistency.entry(key)
                {
                    Entry::Occupied(mut entry) =>
                    {
                        let metadata = entry.key().clone(); // clone because we cannot borrow the value mutably and the key at the same time
                        let value = entry.get_mut();
                        match value.receive_data(check)
                        {
                            Err(e) => error!("[Party {}] Error when receiving data: {:?}", id, e),
                            Ok(false) => (), // not ready
                            Ok(true) => // got ready
                            {
                                if let Err(e) = notify_parties(id, &metadata, value.keys().copied(), value.data(), &net_channel).await
                                {
                                    warn!("[Party {}] Could not send consistency check to other parties. Reason: {:?}", id, e);
                                }
                            },
                        }

                        match value.check(&metadata, &verification_keys, #[cfg(feature = "sessions")] session)
                        {
                            Ok(false) => (), // not ready
                            Err(e) => error!("[Party {}] Error when checking: {:?}", id, e),
                            Ok(true) =>
                            {
                                if value.is_done()
                                {
                                    entry.remove();
                                }
                            },
                        }

                        if is_consistent(&consistency)
                        {
                            notify_requests(&mut open_requests);
                        }
                    },
                    Entry::Vacant(entry) =>
                    {
                        let value = ConsistencyCheckValue::new_received_data(check);
                        entry.insert(value);
                    }
                }
            }
            ConsistencyCheckCommand::Wait(answer_channel) =>
            {
                if is_consistent(&consistency)
                {
                    debug!("[Party {}] Consistency check finished", id);
                    if let Err(_) = answer_channel.send(Ok(()))
                    {
                        warn!("[Party {}] Waiting thread for consistency check no longer available", id);
                    }
                }
                else
                {
                    debug!("[Party {}] Waiting for consistency check to finish", id);
                    open_requests.push(answer_channel);
                }
            }
        }
    }
}
