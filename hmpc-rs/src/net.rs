use std::collections::BTreeSet;
#[cfg(feature = "signing")]
use std::collections::HashMap;
use std::mem::size_of;
use std::net::{SocketAddr, ToSocketAddrs};

use log::info;
use quinn::{RecvStream, SendStream, WriteError};

mod client;
pub mod config;
#[cfg(feature = "collective-consistency")]
pub(crate) mod consistency;
pub(crate) mod errors;
pub(crate) mod hash;
mod message_buffer;
pub mod metadata;
pub mod ptr;
pub(crate) mod queue;
mod server;
#[cfg(feature = "signing")]
pub(crate) mod sign;

pub use self::config::Config;
#[cfg(feature = "collective-consistency")]
use self::errors::ConsistencyCheckError;
use self::errors::{ClientError, FromPrimitiveError, ServerError};
#[cfg(feature = "signing")]
use self::hash::hash;
#[cfg(feature = "collective-consistency")]
use self::hash::{Hash, HASH_SIZE};
pub use self::queue::Queue;
#[cfg(feature = "collective-consistency")]
use self::sign::Signature;
#[cfg(feature = "signing")]
use self::sign::{PrivateKey, PublicKey, SIGNATURE_SIZE};

const SESSION_ID_SIZE: usize = 128 / 8; // 128 bit => [u8; 16]

const MESSAGE_SIZE_LIMIT: usize = 2 << 32;

const MESSAGE_FORMAT_VERSION: u8 = 0;

#[rustfmt::skip]
const MESSAGE_FEATURE_FLAGS: u8 = {
    let session_flag = cfg!(feature = "sessions") as u8;
    let signing_flag = cfg!(feature = "signing") as u8;
    session_flag
        | (signing_flag << 1)
};

pub type PartyID = u16;
#[repr(u8)]
#[derive(PartialEq, Eq, Hash, Debug, Clone, Copy)]
pub enum MessageKind
{
    /// Send a fixed message from one party to one party
    /// ### Example
    /// Sender(s): i
    /// Receiver(s): j
    /// Value(s): x
    ///
    /// Before: Pi holds x
    /// After: Pj holds x
    Send = 1,
    /// Send a fixed message from one party to all parties
    /// ### Example
    /// Sender(s): i
    /// Receiver(s): [1..n]
    /// Value(s): x
    ///
    /// Before: Pi holds x
    /// After: for j in [1..n]: Pj holds x
    Broadcast = 2,
    /// Send different messages from one party to all parties
    /// ### Example
    /// Sender(s): i
    /// Receiver(s): [1..n]
    /// Value(s): [x1..xn]
    ///
    /// Before: for j in [1..n]: Pi holds xj
    /// After: for j in [1..n]: Pj holds xj
    Scatter = 3,
    /// Send different messages from all parties to one party
    /// ### Example
    /// Sender(s): [1..n]
    /// Receiver(s): i
    /// Value(s): [x1..xn]
    ///
    /// Before: for j in [1..n]: Pj holds xj
    /// After: for j in [1..n]: Pi holds xj
    Gather = 4,
    /// Send a fixed message from all parties to all parties
    /// ### Example
    /// Sender(s): [1..n]
    /// Receiver(s): [1..n]
    /// Value(s): [x1..xn]
    ///
    /// Before: for j in [1..n]: Pj holds xj
    /// After: for i in [1..n] for j in [1..n]: Pi holds xj
    AllGather = 5,
    /// Send different messages from all parties to all parties
    /// ### Example
    /// Sender(s): [1..n]
    /// Receiver(s): [1..n]
    /// Value(s): [x11..xnn]
    ///
    /// Before: for i in [1..n] for j in [1..n]: Pi holds xij
    /// After: for i in [1..n] for j in [1..n]: Pi holds xji
    AllToAll = 6,
    /// Check collective consistency of `Broadcast`
    ConsistencyCheckBroadcast = 18,
    /// Check collective consistency of `AllGather`
    ConsistencyCheckAllGather = 21,
}

type MessageKindUnderlying = u8;

impl TryFrom<MessageKindUnderlying> for MessageKind
{
    type Error = FromPrimitiveError;

    fn try_from(value: MessageKindUnderlying) -> Result<Self, Self::Error>
    {
        match value
        {
            x if x == MessageKind::Send as MessageKindUnderlying => Ok(MessageKind::Send),
            x if x == MessageKind::Broadcast as MessageKindUnderlying => Ok(MessageKind::Broadcast),
            x if x == MessageKind::Scatter as MessageKindUnderlying => Ok(MessageKind::Scatter),
            x if x == MessageKind::Gather as MessageKindUnderlying => Ok(MessageKind::Gather),
            x if x == MessageKind::AllGather as MessageKindUnderlying => Ok(MessageKind::AllGather),
            x if x == MessageKind::AllToAll as MessageKindUnderlying => Ok(MessageKind::AllToAll),
            x if x == MessageKind::ConsistencyCheckBroadcast as MessageKindUnderlying => Ok(MessageKind::ConsistencyCheckBroadcast),
            x if x == MessageKind::ConsistencyCheckAllGather as MessageKindUnderlying => Ok(MessageKind::ConsistencyCheckAllGather),
            x => Err(FromPrimitiveError::FromU8(x)),
        }
    }
}
impl MessageKind
{
    const BYTE_SIZE: usize = size_of::<MessageKindUnderlying>();

    fn to_le_bytes(self) -> [u8; Self::BYTE_SIZE]
    {
        (self as MessageKindUnderlying).to_le_bytes()
    }

    fn try_from_le_bytes(bytes: [u8; Self::BYTE_SIZE]) -> Result<Self, FromPrimitiveError>
    {
        let value = MessageKindUnderlying::from_le_bytes(bytes);
        value.try_into()
    }

    fn needs_check(self) -> bool
    {
        match self
        {
            MessageKind::Broadcast | MessageKind::AllGather => true,
            _ => false,
        }
    }

    fn is_consistency_check(self) -> bool
    {
        match self
        {
            MessageKind::ConsistencyCheckBroadcast | MessageKind::ConsistencyCheckAllGather => true,
            _ => false,
        }
    }

    fn try_to_check(self) -> Result<Self, ()>
    {
        match self
        {
            MessageKind::Broadcast => Ok(MessageKind::ConsistencyCheckBroadcast),
            MessageKind::AllGather => Ok(MessageKind::ConsistencyCheckAllGather),
            _ => Err(()),
        }
    }

    fn try_from_check(self) -> Result<Self, ()>
    {
        match self
        {
            MessageKind::ConsistencyCheckBroadcast => Ok(MessageKind::Broadcast),
            MessageKind::ConsistencyCheckAllGather => Ok(MessageKind::AllGather),
            _ => Err(()),
        }
    }
}

pub type MessageFormat = u8;
pub type MessageFlags = u8;
pub type MessageDatatype = u8;
pub type MessageID = u64;
pub type MessageSize = u64;
pub type OwnedData = Vec<u8>;
pub type SessionID = u128;
const _: () = assert!(SESSION_ID_SIZE == size_of::<SessionID>());

type Communicator = BTreeSet<PartyID>;

#[derive(Debug)]
pub struct SendMessage
{
    metadata: metadata::Message,
    data: ptr::ReadData,
}
impl SendMessage
{
    async fn write_to(self, #[cfg(feature = "sessions")] session: SessionID, #[cfg(feature = "signing")] signing_key: &PrivateKey, stream: &mut SendStream) -> Result<(), WriteError>
    {
        let data = self.metadata.data_as_slice(&self.data);

        #[cfg(feature = "signing")]
        let signature =
        {
            let metadata =
            [
                MESSAGE_FORMAT_VERSION.to_le_bytes().as_slice(),
                MESSAGE_FEATURE_FLAGS.to_le_bytes().as_slice(),
                self.metadata.kind.to_le_bytes().as_slice(),
                self.metadata.datatype.to_le_bytes().as_slice(),
                self.metadata.sender.to_le_bytes().as_slice(),
                self.metadata.receiver.to_le_bytes().as_slice(),
                self.metadata.id.to_le_bytes().as_slice(),
                #[cfg(feature = "sessions")]
                session.to_le_bytes().as_slice(),
                hash(data).as_ref(),
            ]
            .concat();

            signing_key.sign(&metadata)
        };

        stream.write_all(MESSAGE_FORMAT_VERSION.to_le_bytes().as_slice()).await?;
        stream.write_all(MESSAGE_FEATURE_FLAGS.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.kind.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.datatype.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.sender.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.receiver.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.id.to_le_bytes().as_slice()).await?;
        #[cfg(feature = "sessions")]
        stream.write_all(session.to_le_bytes().as_slice()).await?;
        stream.write_all(data).await?;

        #[cfg(feature = "signing")]
        {
            assert_eq!(signature.len(), SIGNATURE_SIZE);

            stream.write_all(signature.as_slice()).await?;
        }

        Ok(())
    }
}
unsafe impl Send for SendMessage {}

#[cfg(not(feature = "collective-consistency"))]
type ReadResult = ReceiveMessage;
#[cfg(feature = "collective-consistency")]
type ReadResult = (ReceiveMessage, Signature);

#[derive(Debug)]
pub struct ReceiveMessage
{
    metadata: metadata::Message,
    data: OwnedData,
}
impl ReceiveMessage
{
    fn new(message_kind: MessageKind, datatype: MessageDatatype, sender: PartyID, receiver: PartyID, message_id: MessageID, data: OwnedData) -> Self
    {
        Self { metadata: metadata::Message::new(message_kind, datatype, sender, receiver, message_id, data.len() as MessageSize), data }
    }

    fn build_from(#[cfg(feature = "sessions")] session: SessionID, message: &[u8]) -> Result<Self, ServerError>
    {
        const MESSAGE_FORMAT_VERSION_SIZE: usize = size_of::<MessageFormat>();
        const MESSAGE_FEATURE_FLAGS_SIZE: usize = size_of::<MessageFlags>();
        const MESSAGE_KIND_SIZE: usize = size_of::<MessageKind>();
        const MESSAGE_DATATYPE_SIZE: usize = size_of::<MessageDatatype>();
        const MESSAGE_ID_SIZE: usize = size_of::<MessageID>();
        const PARTY_ID_SIZE: usize = size_of::<PartyID>();

        fn too_short(slice: &[u8]) -> ServerError
        {
            ServerError::ReadExact(quinn::ReadExactError::FinishedEarly(slice.len()))
        }

        let (message_format_version, message) = message.split_first_chunk::<MESSAGE_FORMAT_VERSION_SIZE>()
            .ok_or(too_short(message))?;
        let message_format_version = MessageFormat::from_le_bytes(*message_format_version);
        if message_format_version != MESSAGE_FORMAT_VERSION
        {
            return Err(ServerError::VersionMismatch);
        }

        let (message_feature_flags, message) = message.split_first_chunk::<MESSAGE_FEATURE_FLAGS_SIZE>()
        .ok_or(too_short(message))?;
        let message_feature_flags = MessageFlags::from_le_bytes(*message_feature_flags);
        if message_feature_flags != MESSAGE_FEATURE_FLAGS
        {
            return Err(ServerError::FeatureMismatch);
        }

        let (message_kind, message) = message.split_first_chunk::<MESSAGE_KIND_SIZE>()
            .ok_or(too_short(message))?;
        let message_kind = MessageKind::try_from_le_bytes(*message_kind)?;

        let (datatype, message) = message.split_first_chunk::<MESSAGE_DATATYPE_SIZE>()
            .ok_or(too_short(message))?;
        let datatype = MessageDatatype::from_le_bytes(*datatype);

        let (sender, message) = message.split_first_chunk::<PARTY_ID_SIZE>()
            .ok_or(too_short(message))?;
        let sender = PartyID::from_le_bytes(*sender);

        let (receiver, message) = message.split_first_chunk::<PARTY_ID_SIZE>()
            .ok_or(too_short(message))?;
        let receiver = PartyID::from_le_bytes(*receiver);

        let (message_id, message) = message.split_first_chunk::<MESSAGE_ID_SIZE>()
            .ok_or(too_short(message))?;
        let message_id = MessageID::from_le_bytes(*message_id);

        #[cfg(feature = "sessions")]
        let (message_session, message) = message.split_first_chunk::<SESSION_ID_SIZE>()
            .ok_or(too_short(message))?;
        #[cfg(feature = "sessions")]
        {
            let message_session = SessionID::from_le_bytes(*message_session);
            if message_session != session
            {
                return Err(ServerError::SessionMismatch);
            }
        }

        let data = message.into();

        Ok(Self::new(message_kind, datatype, sender, receiver, message_id, data))
    }

    #[cfg(feature = "signing")]
    async fn read_from(#[cfg(feature = "sessions")] session: SessionID, verification_keys: &HashMap<PartyID, PublicKey>, stream: &mut RecvStream) -> Result<ReadResult, ServerError>
    {
        use errors::SignatureError;

        let full_message = stream.read_to_end(MESSAGE_SIZE_LIMIT).await?;
        let full_message = full_message.as_slice();

        let (message, signature) = full_message.split_last_chunk::<SIGNATURE_SIZE>()
            .ok_or(ServerError::ReadExact(quinn::ReadExactError::FinishedEarly(full_message.len())))?;

        let result = Self::build_from(
            #[cfg(feature = "sessions")]
            session,
            message,
        )?;
        let metadata =
        [
            MESSAGE_FORMAT_VERSION.to_le_bytes().as_slice(),
            MESSAGE_FEATURE_FLAGS.to_le_bytes().as_slice(),
            result.metadata.kind.to_le_bytes().as_slice(),
            result.metadata.datatype.to_le_bytes().as_slice(),
            result.metadata.sender.to_le_bytes().as_slice(),
            result.metadata.receiver.to_le_bytes().as_slice(),
            result.metadata.id.to_le_bytes().as_slice(),
            #[cfg(feature = "sessions")]
            session.to_le_bytes().as_slice(),
            hash(result.data.as_ref()).as_ref(),
        ]
        .concat();

        let sender = if result.metadata.kind.is_consistency_check()
        {
            result.metadata.receiver
        }
        else
        {
            result.metadata.sender
        };

        verification_keys.get(&sender)
            .map_or(Err(SignatureError::UnknownSender), Ok)?
            .verify(&metadata, signature)
            .or(Err(SignatureError::SignatureVerification))?;

        #[cfg(not(feature = "collective-consistency"))]
        {
            Ok(result)
        }
        #[cfg(feature = "collective-consistency")]
        {
            Ok((result, *signature))
        }
    }

    #[cfg(not(feature = "signing"))]
    async fn read_from(#[cfg(feature = "sessions")] session: SessionID, stream: &mut RecvStream) -> Result<ReadResult, ServerError>
    {
        let message = stream.read_to_end(MESSAGE_SIZE_LIMIT).await?;

        Self::build_from(
            #[cfg(feature = "sessions")]
            session,
            message.as_slice(),
        )
    }
}


#[cfg(feature = "collective-consistency")]
#[derive(Debug)]
pub(crate) struct ConsistencyCheckMessage
{
    metadata: metadata::Message,
    hash: Hash,
    signature: Signature,
}
#[cfg(feature = "collective-consistency")]
impl TryFrom<ReceiveMessage> for ConsistencyCheckMessage
{
    type Error = ServerError;

    fn try_from(value: ReceiveMessage) -> Result<Self, Self::Error>
    {
        if value.metadata.kind.is_consistency_check()
        {
            let (hash, data) = value
                .data
                .as_slice()
                .split_first_chunk::<HASH_SIZE>()
                .ok_or(ServerError::ReadExact(quinn::ReadExactError::FinishedEarly(value.data.len())))?;

            let signature = data
                .try_into()
                .map_err(|_| ServerError::ReadExact(quinn::ReadExactError::FinishedEarly(data.len())))?;

            Ok(Self { metadata: value.metadata, hash: *hash, signature })
        }
        else
        {
            Err(ServerError::FromPrimitive(FromPrimitiveError::FromU8(value.metadata.kind as MessageKindUnderlying)))
        }
    }
}
#[cfg(feature = "collective-consistency")]
impl TryFrom<(&ReceiveMessage, Signature)> for ConsistencyCheckMessage
{
    type Error = ();

    fn try_from(value: (&ReceiveMessage, Signature)) -> Result<Self, Self::Error>
    {
        let (value, signature) = value;
        if value.metadata.kind.needs_check()
        {
            Ok(Self { metadata: value.metadata.clone(), hash: hash(value.data.as_slice()), signature })
        }
        else
        {
            Err(())
        }
    }
}

#[derive(Debug)]
pub enum DataCommand
{
    /// Want to obtain data for this
    Receive(metadata::Message, tokio::sync::oneshot::Sender<OwnedData>),
    /// Received data from network
    Received(ReceiveMessage),
}
type DataChannel = tokio::sync::mpsc::Sender<DataCommand>;

#[derive(Debug)]
pub enum NetCommand
{
    Send(SendMessage, tokio::sync::oneshot::Sender<Result<(), ClientError>>),
    #[cfg(feature = "collective-consistency")]
    SendCheck(SendMessage, PartyID, tokio::sync::oneshot::Sender<Result<(), ClientError>>),
}
type NetChannel = tokio::sync::mpsc::Sender<NetCommand>;

#[cfg(feature = "collective-consistency")]
#[derive(Debug)]
pub(crate) enum ConsistencyCheckCommand
{
    /// Want to check consistency for this
    Request(metadata::ConsistencyCheck),
    /// Received check message from network
    ReceivedCheck(ConsistencyCheckMessage),
    /// Received data to check from network
    ReceivedData(ConsistencyCheckMessage),
    /// Wait for all consistency checks to be finished
    Wait(tokio::sync::oneshot::Sender<Result<(), ConsistencyCheckError>>),
}
#[cfg(feature = "collective-consistency")]
type ConsistencyCheckChannel = tokio::sync::mpsc::Sender<ConsistencyCheckCommand>;

fn make_addr(id: PartyID, config: &Config) -> SocketAddr
{
    let name = config.name(id);
    let port = config.port(id);
    let mut addrs = (name, port).to_socket_addrs().unwrap();
    let addr = addrs.next().unwrap();
    info!("Resolved party {id} (aka {name}:{port}) as {addr}");
    addr
}
