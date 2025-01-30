use std::collections::BTreeSet;
#[cfg(feature = "signing")]
use std::collections::HashMap;
use std::mem::size_of;
use std::net::{SocketAddr, ToSocketAddrs};

use log::info;
use quinn::{RecvStream, SendStream, WriteError};

mod client;
pub mod config;
pub(crate) mod errors;
mod message_buffer;
pub mod metadata;
#[cfg(feature = "signing")]
pub(crate) mod sign;
pub mod ptr;
pub(crate) mod queue;
mod server;

pub use self::config::Config;
#[cfg(feature = "signing")]
use self::sign::{PrivateKey, PublicKey};
use self::errors::{ClientError, FromPrimitiveError, ServerError};
pub use self::queue::Queue;

const SESSION_ID_SIZE: usize = 128 / 8; // 128 bit => [u8; 16]

#[cfg(feature = "signing")]
const SIGNATURE_SIZE: usize = 64; // see https://ed25519.cr.yp.to/

const MESSAGE_SIZE_LIMIT: usize = 2 << 32;

const MESSAGE_FORMAT_VERSION: u8 = 0;

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
}

type MessageDatatypeUnderlying = u8;

impl TryFrom<MessageDatatypeUnderlying> for MessageKind
{
    type Error = FromPrimitiveError;

    fn try_from(value: MessageDatatypeUnderlying) -> Result<Self, Self::Error>
    {
        match value
        {
            x if x == MessageKind::Send as MessageDatatypeUnderlying => Ok(MessageKind::Send),
            x if x == MessageKind::Broadcast as MessageDatatypeUnderlying => Ok(MessageKind::Broadcast),
            x if x == MessageKind::Scatter as MessageDatatypeUnderlying => Ok(MessageKind::Scatter),
            x if x == MessageKind::Gather as MessageDatatypeUnderlying => Ok(MessageKind::Gather),
            x if x == MessageKind::AllGather as MessageDatatypeUnderlying => Ok(MessageKind::AllGather),
            x if x == MessageKind::AllToAll as MessageDatatypeUnderlying => Ok(MessageKind::AllToAll),
            x => Err(FromPrimitiveError::FromU8(x)),
        }
    }
}
impl MessageKind
{
    const BYTE_SIZE: usize = size_of::<MessageDatatypeUnderlying>();

    fn to_le_bytes(self) -> [u8; Self::BYTE_SIZE]
    {
        (self as MessageDatatypeUnderlying).to_le_bytes()
    }

    fn try_from_le_bytes(bytes: [u8; Self::BYTE_SIZE]) -> Result<Self, FromPrimitiveError>
    {
        let value = MessageDatatypeUnderlying::from_le_bytes(bytes);
        value.try_into()
    }
}

pub type MessageFormat = u8;
pub type MessageFlags = u8;
pub type MessageDatatype = u16;
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
    #[cfg(feature = "signing")]
    async fn write_to(self, #[cfg(feature = "sessions")] session: SessionID, signing_key: &PrivateKey, stream: &mut SendStream) -> Result<(), WriteError>
    {
        let message =
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
            self.metadata.data_as_slice(&self.data),
        ].concat();

        let signature = signing_key.sign(&message);
        let signature = signature.as_ref();

        assert_eq!(signature.len(), SIGNATURE_SIZE);

        stream.write_all(&message).await?;
        stream.write_all(signature).await
    }

    #[cfg(not(feature = "signing"))]
    async fn write_to(self, #[cfg(feature = "sessions")] session: SessionID, stream: &mut SendStream) -> Result<(), WriteError>
    {
        stream.write_all(MESSAGE_FORMAT_VERSION.to_le_bytes().as_slice()).await?;
        stream.write_all(MESSAGE_FEATURE_FLAGS.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.kind.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.datatype.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.sender.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.receiver.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.id.to_le_bytes().as_slice()).await?;
        #[cfg(feature = "sessions")]
        stream.write_all(session.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.data_as_slice(&self.data)).await
    }
}
unsafe impl Send for SendMessage {}

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
    async fn read_from(#[cfg(feature = "sessions")] session: SessionID, verification_keys: &HashMap<PartyID, PublicKey>, stream: &mut RecvStream) -> Result<Self, ServerError>
    {
        let full_message = stream.read_to_end(MESSAGE_SIZE_LIMIT).await?;
        let full_message = full_message.as_slice();

        let (message, signature) = full_message.split_last_chunk::<SIGNATURE_SIZE>()
            .ok_or(ServerError::ReadExact(quinn::ReadExactError::FinishedEarly(full_message.len())))?;

        let result = Self::build_from(#[cfg(feature = "sessions")] session, message)?;

        verification_keys.get(&result.metadata.sender)
            .map_or(Err(ServerError::UnknownSender), Ok)?
            .verify(message, signature)
            .or(Err(ServerError::SignatureVerification))?;

        Ok(result)
    }

    #[cfg(not(feature = "signing"))]
    async fn read_from(#[cfg(feature = "sessions")] session: SessionID, stream: &mut RecvStream) -> Result<Self, ServerError>
    {
        let message = stream.read_to_end(MESSAGE_SIZE_LIMIT).await?;

        Self::build_from(#[cfg(feature = "sessions")] session, message.as_slice())
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

#[derive(Debug)]
pub enum NetCommand
{
    Send(SendMessage, tokio::sync::oneshot::Sender<Result<(), ClientError>>),
}

fn make_addr(id: PartyID, config: &Config) -> SocketAddr
{
    let name = config.name(id);
    let port = config.port(id);
    let mut addrs = (name, port).to_socket_addrs().unwrap();
    let addr = addrs.next().unwrap();
    info!("Resolved party {id} (aka {name}:{port}) as {addr}");
    addr
}
