use super::ptr::ReadData;
use super::{Communicator, MessageDatatype, MessageID, MessageKind, MessageSize, PartyID};

/// Trait to get all information for deriving a message ID (message king, message datatype, communicator, and relevant metadata).
pub(crate) trait ToMessageID
{
    /// The message kind.
    const KIND: MessageKind;

    /// The message datatype.
    fn datatype(&self) -> MessageDatatype;

    /// Hash all remaining metadata for this message type.
    fn hash_metadata(&self, hash: &mut ring::digest::Context);

    /// Hash a communicator.
    fn hash_communicator(communicator: &Communicator, hash: &mut ring::digest::Context)
    {
        let len = communicator.len() as u64;
        hash.update(len.to_le_bytes().as_slice());
        for party in communicator
        {
            hash.update(party.to_le_bytes().as_slice());
        }
    }

    /// Hash `self` and communicators to derive the message ID.
    fn hash(&self, senders: &Communicator, receivers: &Communicator) -> ring::digest::Digest
    {
        let mut sha = ring::digest::Context::new(&ring::digest::SHA256);
        Self::hash_communicator(senders, &mut sha);
        self.hash_metadata(&mut sha);
        Self::hash_communicator(receivers, &mut sha);

        sha.finish()
    }
}

/// Trait to convert to a generic [`Message`].
pub(crate) trait ToMessage
{
    /// Perform the conversion.
    fn to_message(self, sender: PartyID, receiver: PartyID, id: MessageID) -> Message;
}

/// Trait to convert to the corresponding [`ConsistencyCheck`].
#[cfg(feature = "collective-consistency")]
pub(crate) trait ToConsistencyCheck
{
    /// Perform the conversion.
    fn to_consistency_check(&self, sender: PartyID, receivers: Communicator, id: MessageID) -> ConsistencyCheck;
}

#[cfg(feature = "collective-consistency")]
impl<T> ToConsistencyCheck for T
where
    T: ToMessageID,
{
    fn to_consistency_check(&self, sender: PartyID, receivers: Communicator, id: MessageID) -> ConsistencyCheck
    {
        ConsistencyCheck { kind: Self::KIND, datatype: self.datatype(), sender, receivers, id }
    }
}

macro_rules! message
{
    ($t:ident $(, $($member:ident),+ $(,)?)? ) =>
    {
        impl ToMessageID for &$t
        {
            const KIND: MessageKind = MessageKind::$t;

            fn datatype(&self) -> MessageDatatype
            {
                self.datatype
            }

            fn hash_metadata(&self, _hash: &mut ring::digest::Context)
            {
                _hash.update(Self::KIND.to_le_bytes().as_slice());
                _hash.update(self.datatype.to_le_bytes().as_slice());
                $(
                    $(
                        _hash.update(self.$member.to_le_bytes().as_slice());
                    )+
                )?
            }
        }

        impl ToMessage for &$t
        {
            fn to_message(self, sender: PartyID, receiver: PartyID, id: MessageID) -> Message
            {
                Message { kind: <Self as ToMessageID>::KIND, datatype: self.datatype, sender, receiver, id, size: self.size }
            }
        }
    };
}

/// Generic message metadata.
#[derive(PartialEq, Eq, Hash, Debug, Clone)]
pub struct Message
{
    pub(crate) kind: MessageKind,
    pub(crate) datatype: MessageDatatype,
    pub(crate) sender: PartyID,
    pub(crate) receiver: PartyID,
    pub(crate) id: MessageID,
    pub(crate) size: MessageSize,
}

impl Message
{
    #[must_use]
    pub fn new(kind: MessageKind, datatype: MessageDatatype, sender: PartyID, receiver: PartyID, id: MessageID, size: MessageSize) -> Self
    {
        Self { kind, datatype, sender, receiver, id, size }
    }

    /// Build usable slice from data pointer.
    ///
    /// # Panics
    /// If message is too long for the current hardware platform (size cannot be represented in `usize`).
    #[must_use]
    pub fn data_as_slice(&self, ptr: &ReadData) -> &[u8]
    {
        unsafe { std::slice::from_raw_parts(ptr.as_ptr(), usize::try_from(self.size).expect("Message too long for this platform")) }
    }
}

/// Broadcast operation metadata.
#[repr(C)]
#[derive(PartialEq, Eq, Hash, Debug, Clone, Copy)]
pub struct Broadcast
{
    pub(crate) datatype: MessageDatatype,
    pub(crate) sender: PartyID,
    pub(crate) size: MessageSize,
}
impl Broadcast
{
    #[must_use]
    pub fn new(datatype: MessageDatatype, sender: PartyID, size: MessageSize) -> Self
    {
        Self { datatype, sender, size }
    }
}
message!(Broadcast, sender);

/// Gather operation metadata.
#[repr(C)]
#[derive(PartialEq, Eq, Hash, Debug, Clone, Copy)]
pub struct Gather
{
    pub(crate) datatype: MessageDatatype,
    pub(crate) receiver: PartyID,
    pub(crate) size: MessageSize,
}
impl Gather
{
    #[must_use]
    pub fn new(datatype: MessageDatatype, receiver: PartyID, size: MessageSize) -> Self
    {
        Self { datatype, receiver, size }
    }
}
message!(Gather, receiver);

/// All-gather operation metadata.
#[repr(C)]
#[derive(PartialEq, Eq, Hash, Debug, Clone, Copy)]
pub struct AllGather
{
    pub(crate) datatype: MessageDatatype,
    pub(crate) size: MessageSize,
}
impl AllGather
{
    #[must_use]
    pub fn new(datatype: MessageDatatype, size: MessageSize) -> Self
    {
        Self { datatype, size }
    }
}
message!(AllGather);

/// All-to-all operation metadata.
#[repr(C)]
#[derive(PartialEq, Eq, Hash, Debug, Clone, Copy)]
pub struct AllToAll
{
    pub(crate) datatype: MessageDatatype,
    pub(crate) size: MessageSize,
}
impl AllToAll
{
    #[must_use]
    pub fn new(datatype: MessageDatatype, size: MessageSize) -> Self
    {
        Self { datatype, size }
    }
}
message!(AllToAll);

/// Consistency check message metadata.
#[cfg(feature = "collective-consistency")]
#[derive(PartialEq, Eq, Hash, Debug, Clone)]
pub struct ConsistencyCheck
{
    pub(crate) kind: MessageKind,
    pub(crate) datatype: MessageDatatype,
    pub(crate) sender: PartyID,
    pub(crate) receivers: Communicator,
    pub(crate) id: MessageID,
}
