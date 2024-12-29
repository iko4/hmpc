use quinn::{ConnectionError, ReadError, WriteError};

use crate::net::errors::{ClientError, ReceiveError, SendError, SendReceiveError, ServerError};

#[repr(u8)]
#[derive(Debug)]
#[must_use]
pub enum SendErrc
{
    Ok = 0,
    InvalidHandle,
    InvalidPointer,
    InvalidSize,
    ChannelCouldNotReceive,
    ChannelCouldNotSend,
    ConnectionVersionMismatch,
    ConnectionTransportError,
    ConnectionClosed,
    ConnectionReset,
    ConnectionTimedOut,
    ConnectionLocallyClosed,
    ApplicationClosed,
    StreamStopped,
    StreamUnknown,
    StreamRejected,
}

impl From<ConnectionError> for SendErrc
{
    fn from(value: ConnectionError) -> Self
    {
        match value
        {
            ConnectionError::VersionMismatch => Self::ConnectionVersionMismatch,
            ConnectionError::TransportError(_) => Self::ConnectionTransportError,
            ConnectionError::ConnectionClosed(_) => Self::ConnectionClosed,
            ConnectionError::ApplicationClosed(_) => Self::ApplicationClosed,
            ConnectionError::Reset => Self::ConnectionReset,
            ConnectionError::TimedOut => Self::ConnectionTimedOut,
            ConnectionError::LocallyClosed => Self::ConnectionLocallyClosed,
        }
    }
}

impl From<SendError> for SendErrc
{
    fn from(value: SendError) -> Self
    {
        match value
        {
            SendError::Send(_) => Self::ChannelCouldNotSend,
            SendError::Receive(_) => Self::ChannelCouldNotReceive,
            SendError::Client(ClientError::Connection(connection)) => connection.into(),
            SendError::Client(ClientError::Write(write)) => match write
            {
                WriteError::Stopped(_) => Self::StreamStopped,
                WriteError::ConnectionLost(connection) => connection.into(),
                WriteError::UnknownStream => Self::StreamUnknown,
                WriteError::ZeroRttRejected => Self::StreamRejected,
            },
        }
    }
}

#[repr(u8)]
#[derive(Debug)]
#[must_use]
pub enum ReceiveErrc
{
    Ok = 0,
    InvalidHandle,
    InvalidPointer,
    ChannelCouldNotReceive,
    ChannelCouldNotSend,
    ConnectionVersionMismatch,
    ConnectionTransportError,
    ConnectionClosed,
    ConnectionReset,
    ConnectionTimedOut,
    ConnectionLocallyClosed,
    ApplicationClosed,
    StreamFinishedEarly,
    StreamReset,
    StreamUnknown,
    StreamIllegalOrderedRead,
    StreamRejected,
    StreamTooLong,
    InvalidEnumValue,
    SizeMismatch,
    SignatureVerification,
    UnknownSender,
}

impl From<ConnectionError> for ReceiveErrc
{
    fn from(value: ConnectionError) -> Self
    {
        match value
        {
            ConnectionError::VersionMismatch => Self::ConnectionVersionMismatch,
            ConnectionError::TransportError(_) => Self::ConnectionTransportError,
            ConnectionError::ConnectionClosed(_) => Self::ConnectionClosed,
            ConnectionError::ApplicationClosed(_) => Self::ApplicationClosed,
            ConnectionError::Reset => Self::ConnectionReset,
            ConnectionError::TimedOut => Self::ConnectionTimedOut,
            ConnectionError::LocallyClosed => Self::ConnectionLocallyClosed,
        }
    }
}

impl From<ReadError> for ReceiveErrc
{
    fn from(value: ReadError) -> Self
    {
        match value
        {
            ReadError::Reset(_) => Self::StreamReset,
            ReadError::ConnectionLost(connection) => connection.into(),
            ReadError::UnknownStream => Self::StreamUnknown,
            ReadError::IllegalOrderedRead => Self::StreamIllegalOrderedRead,
            ReadError::ZeroRttRejected => Self::StreamRejected,
        }
    }
}

impl From<ReceiveError> for ReceiveErrc
{
    fn from(value: ReceiveError) -> Self
    {
        match value
        {
            ReceiveError::Send(_) => Self::ChannelCouldNotSend,
            ReceiveError::Receive(_) => Self::ChannelCouldNotReceive,
            ReceiveError::Server(ServerError::ReadExact(read)) => match read
            {
                quinn::ReadExactError::FinishedEarly => Self::StreamFinishedEarly,
                quinn::ReadExactError::ReadError(read) => read.into(),
            },
            ReceiveError::Server(ServerError::ReadToEnd(read)) => match read
            {
                quinn::ReadToEndError::Read(read) => read.into(),
                quinn::ReadToEndError::TooLong => Self::StreamTooLong,
            },
            ReceiveError::Server(ServerError::FromPrimitive(_)) => Self::InvalidEnumValue,
            ReceiveError::Server(ServerError::SizeMismatch(_)) => Self::SizeMismatch,
            #[cfg(feature = "signing")]
            ReceiveError::Server(ServerError::SignatureVerification) => Self::SignatureVerification,
            #[cfg(feature = "signing")]
            ReceiveError::Server(ServerError::UnknownSender) => Self::UnknownSender,
        }
    }
}

#[repr(u8)]
#[derive(Debug)]
#[must_use]
pub enum SendReceiveErrc
{
    Ok = 0,
    InvalidHandle,
    InvalidPointer,
    InvalidSize,
    InvalidCommunicator,
    InvalidMetadata,
    ChannelCouldNotReceive,
    ChannelCouldNotSend,
    ConnectionVersionMismatch,
    ConnectionTransportError,
    ConnectionClosed,
    ConnectionReset,
    ConnectionTimedOut,
    ConnectionLocallyClosed,
    ApplicationClosed,
    StreamFinishedEarly,
    StreamReset,
    StreamStopped,
    StreamUnknown,
    StreamIllegalOrderedRead,
    StreamRejected,
    StreamTooLong,
    InvalidEnumValue,
    SizeMismatch,
    TaskCancelled,
    TaskPanicked,
    MultipleErrors,
    SignatureVerification,
    UnknownSender,
}

impl From<SendErrc> for SendReceiveErrc
{
    fn from(value: SendErrc) -> Self
    {
        match value
        {
            SendErrc::Ok => Self::Ok,
            SendErrc::InvalidHandle => Self::InvalidHandle,
            SendErrc::InvalidPointer => Self::InvalidPointer,
            SendErrc::InvalidSize => Self::InvalidSize,
            SendErrc::ChannelCouldNotReceive => Self::ChannelCouldNotReceive,
            SendErrc::ChannelCouldNotSend => Self::ChannelCouldNotSend,
            SendErrc::ConnectionVersionMismatch => Self::ConnectionVersionMismatch,
            SendErrc::ConnectionTransportError => Self::ConnectionTransportError,
            SendErrc::ConnectionClosed => Self::ConnectionClosed,
            SendErrc::ConnectionReset => Self::ConnectionReset,
            SendErrc::ConnectionTimedOut => Self::ConnectionTimedOut,
            SendErrc::ConnectionLocallyClosed => Self::ConnectionLocallyClosed,
            SendErrc::ApplicationClosed => Self::ApplicationClosed,
            SendErrc::StreamStopped => Self::StreamStopped,
            SendErrc::StreamUnknown => Self::StreamUnknown,
            SendErrc::StreamRejected => Self::StreamRejected,
        }
    }
}

impl From<ReceiveErrc> for SendReceiveErrc
{
    fn from(value: ReceiveErrc) -> Self
    {
        match value
        {
            ReceiveErrc::Ok => Self::Ok,
            ReceiveErrc::InvalidHandle => Self::InvalidHandle,
            ReceiveErrc::InvalidPointer => Self::InvalidPointer,
            ReceiveErrc::ChannelCouldNotReceive => Self::ChannelCouldNotReceive,
            ReceiveErrc::ChannelCouldNotSend => Self::ChannelCouldNotSend,
            ReceiveErrc::ConnectionVersionMismatch => Self::ConnectionVersionMismatch,
            ReceiveErrc::ConnectionTransportError => Self::ConnectionTransportError,
            ReceiveErrc::ConnectionClosed => Self::ConnectionClosed,
            ReceiveErrc::ConnectionReset => Self::ConnectionReset,
            ReceiveErrc::ConnectionTimedOut => Self::ConnectionTimedOut,
            ReceiveErrc::ConnectionLocallyClosed => Self::ConnectionLocallyClosed,
            ReceiveErrc::ApplicationClosed => Self::ApplicationClosed,
            ReceiveErrc::StreamFinishedEarly => Self::StreamFinishedEarly,
            ReceiveErrc::StreamReset => Self::StreamReset,
            ReceiveErrc::StreamUnknown => Self::StreamUnknown,
            ReceiveErrc::StreamIllegalOrderedRead => Self::StreamIllegalOrderedRead,
            ReceiveErrc::StreamRejected => Self::StreamRejected,
            ReceiveErrc::StreamTooLong => Self::StreamTooLong,
            ReceiveErrc::InvalidEnumValue => Self::InvalidEnumValue,
            ReceiveErrc::SizeMismatch => Self::SizeMismatch,
            ReceiveErrc::SignatureVerification => Self::SignatureVerification,
            ReceiveErrc::UnknownSender => Self::UnknownSender,
        }
    }
}

impl From<SendError> for SendReceiveErrc
{
    fn from(value: SendError) -> Self
    {
        let errc: SendErrc = value.into();
        errc.into()
    }
}

impl From<ReceiveError> for SendReceiveErrc
{
    fn from(value: ReceiveError) -> Self
    {
        let errc: ReceiveErrc = value.into();
        errc.into()
    }
}

impl From<SendReceiveError> for SendReceiveErrc
{
    fn from(value: SendReceiveError) -> Self
    {
        match value
        {
            SendReceiveError::Send(send) => send.into(),
            SendReceiveError::Receive(receive) => receive.into(),
            SendReceiveError::Join(join) =>
            {
                if join.is_cancelled()
                {
                    Self::TaskCancelled
                }
                else
                {
                    Self::TaskPanicked
                }
            },
            SendReceiveError::Multiple => Self::MultipleErrors,
        }
    }
}
