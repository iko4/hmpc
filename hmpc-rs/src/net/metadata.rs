use super::ptr::ReadData;
use super::{MessageDatatype, MessageID, MessageKind, MessageSize, PartyID};

pub(crate) trait BaseMessageID
{
    const KIND: MessageKind;

    fn hash(self, hash: &mut ring::digest::Context);
}

pub(crate) trait ToMessage
{
    fn to_message(self, sender: PartyID, receiver: PartyID, id: MessageID) -> Message;
}

macro_rules! message
{
    ($t:ident $(, $($member:ident),+ $(,)?)? ) =>
    {
        impl BaseMessageID for &$t
        {
            const KIND: MessageKind = MessageKind::$t;

            fn hash(self, _hash: &mut ring::digest::Context)
            {
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
                Message { kind: <Self as BaseMessageID>::KIND, datatype: self.datatype, sender, receiver, id, size: self.size }
            }
        }
    };
}

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

    /// Build usable slice from data pointer
    ///
    /// # Panics
    /// If message is too long for the current hardware platform (size cannot be represented in `usize`)
    #[must_use]
    pub fn data_as_slice(&self, ptr: &ReadData) -> &[u8]
    {
        unsafe { std::slice::from_raw_parts(ptr.as_ptr(), usize::try_from(self.size).expect("Message too long for this platform")) }
    }
}

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

