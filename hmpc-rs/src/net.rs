use std::collections::BTreeSet;
#[cfg(feature = "signing")]
use std::collections::HashMap;
use std::mem::size_of;
use std::net::{SocketAddr, ToSocketAddrs};
#[cfg(feature = "signing")]
use std::sync::Arc;

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


#[cfg(feature = "signing")]
const SIGNATURE_SIZE: usize = 64; // see https://ed25519.cr.yp.to/

const MESSAGE_SIZE_LIMIT: usize = 2 << 32;

pub type PartyID = u16;
#[repr(u16)]
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

type MessageDatatypeUnderlying = u16;

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
            x => Err(x.into()),
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

    fn from_le_bytes(bytes: [u8; Self::BYTE_SIZE]) -> Result<Self, FromPrimitiveError>
    {
        let value = MessageDatatypeUnderlying::from_le_bytes(bytes);
        value.try_into()
    }
}

pub type MessageDatatype = u16;
pub type MessageID = u64;
pub type MessageSize = u64;
pub type OwnedData = Vec<u8>;


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
    async fn write_to(self, signing_key: &PrivateKey, stream: &mut SendStream) -> Result<(), WriteError>
    {
        let message =
        [
            self.metadata.kind.to_le_bytes().as_slice(),
            self.metadata.datatype.to_le_bytes().as_slice(),
            self.metadata.sender.to_le_bytes().as_slice(),
            self.metadata.receiver.to_le_bytes().as_slice(),
            self.metadata.id.to_le_bytes().as_slice(),
            self.metadata.data_as_slice(&self.data),
        ].concat();

        let signature = signing_key.sign(&message);
        let signature = signature.as_ref();

        assert_eq!(signature.len(), SIGNATURE_SIZE);

        stream.write_all(signature.as_ref()).await?;
        stream.write_all(&message).await
    }

    #[cfg(not(feature = "signing"))]
    async fn write_to(self, stream: &mut SendStream) -> Result<(), WriteError>
    {
        stream.write_all(self.metadata.kind.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.datatype.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.sender.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.receiver.to_le_bytes().as_slice()).await?;
        stream.write_all(self.metadata.id.to_le_bytes().as_slice()).await?;
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

    #[cfg(feature = "signing")]
    async fn read_from(verification_keys: &HashMap<PartyID, PublicKey>, stream: &mut RecvStream) -> Result<Self, ServerError>
    {

        const PARTY_ID_SIZE: usize = size_of::<PartyID>();
        const MESSAGE_KIND_SIZE: usize = size_of::<MessageKind>();
        const MESSAGE_DATATYPE_SIZE: usize = size_of::<MessageDatatype>();
        const MESSAGE_ID_SIZE: usize = size_of::<MessageID>();

        const TOO_SHORT: ServerError = ServerError::ReadExact(quinn::ReadExactError::FinishedEarly);

        let mut signature = [0u8; SIGNATURE_SIZE];
        stream.read_exact(signature.as_mut_slice()).await?;

        let full_message = stream.read_to_end(MESSAGE_SIZE_LIMIT).await?;

        let message = full_message.as_slice();

        let (message_kind, message) = message.split_at(MESSAGE_KIND_SIZE);
        let message_kind = MessageKind::from_le_bytes(message_kind.try_into().or(Err(TOO_SHORT))?)?;

        let (datatype, message) = message.split_at(MESSAGE_DATATYPE_SIZE);
        let datatype = MessageDatatype::from_le_bytes(datatype.try_into().or(Err(TOO_SHORT))?);

        let (sender, message) = message.split_at(PARTY_ID_SIZE);
        let sender = PartyID::from_le_bytes(sender.try_into().or(Err(TOO_SHORT))?);

        let (receiver, message) = message.split_at(PARTY_ID_SIZE);
        let receiver = PartyID::from_le_bytes(receiver.try_into().or(Err(TOO_SHORT))?);

        let (message_id, message) = message.split_at(MESSAGE_ID_SIZE);
        let message_id = MessageID::from_le_bytes(message_id.try_into().or(Err(TOO_SHORT))?);

        let data = message.into();
        verification_keys.get(&sender)
            .map_or(Err(ServerError::UnknownSender), Ok)?
            .verify(&full_message, &signature)
            .or(Err(ServerError::SignatureVerification))?;

        Ok(Self::new(message_kind, datatype, sender, receiver, message_id, data))
    }

    #[cfg(not(feature = "signing"))]
    async fn read_from(stream: &mut RecvStream) -> Result<Self, ServerError>
    {
        const PARTY_ID_SIZE: usize = size_of::<PartyID>();
        const MESSAGE_KIND_SIZE: usize = size_of::<MessageKind>();
        const MESSAGE_DATATYPE_SIZE: usize = size_of::<MessageDatatype>();
        const MESSAGE_ID_SIZE: usize = size_of::<MessageID>();

        let mut message_kind = [0u8; MESSAGE_KIND_SIZE];
        stream.read_exact(message_kind.as_mut_slice()).await?;
        let message_kind = MessageKind::from_le_bytes(message_kind)?;

        let mut datatype = [0u8; MESSAGE_DATATYPE_SIZE];
        stream.read_exact(datatype.as_mut_slice()).await?;
        let datatype = MessageDatatype::from_le_bytes(datatype);

        let mut sender = [0u8; PARTY_ID_SIZE];
        stream.read_exact(sender.as_mut_slice()).await?;
        let sender = PartyID::from_le_bytes(sender);

        let mut receiver = [0u8; PARTY_ID_SIZE];
        stream.read_exact(receiver.as_mut_slice()).await?;
        let receiver = PartyID::from_le_bytes(receiver);

        let mut message_id = [0u8; MESSAGE_ID_SIZE];
        stream.read_exact(message_id.as_mut_slice()).await?;
        let message_id = MessageID::from_le_bytes(message_id);

        let data = stream.read_to_end(MESSAGE_SIZE_LIMIT).await?;

        Ok(Self::new(message_kind, datatype, sender, receiver, message_id, data))
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
