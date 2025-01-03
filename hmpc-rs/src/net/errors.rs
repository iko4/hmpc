use config::ConfigError;
use quinn::{ConnectionError, ReadExactError, ReadToEndError, WriteError};
use thiserror::Error;
use tokio::sync::oneshot::error::RecvError;
use tokio::task::JoinError;

use super::{DataCommand, MessageSize, NetCommand};


#[derive(Debug, Error, Clone, PartialEq, Eq)]
pub enum FromPrimitiveError
{
    FromU16(u16),
}
impl std::fmt::Display for FromPrimitiveError
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result
    {
        match *self
        {
            Self::FromU16(value) => write!(f, "FromPrimitiveError(Could not convert from u16 value={value})"),
        }
    }
}
impl From<u16> for FromPrimitiveError
{
    fn from(value: u16) -> Self
    {
        FromPrimitiveError::FromU16(value)
    }
}

#[derive(Debug, Error, Clone, PartialEq, Eq)]
pub struct SizeMismatchError(pub MessageSize, pub MessageSize);
impl std::fmt::Display for SizeMismatchError
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result
    {
        let x = self.0;
        let y = self.1;
        write!(f, "SizeMismatchError({x} != {y})")
    }
}

#[derive(Debug, Error)]
pub enum QueueError
{
    #[error(transparent)]
    Config(#[from] ConfigError),
    #[error(transparent)]
    IO(#[from] std::io::Error),
}

#[derive(Debug, Error, Clone, PartialEq, Eq)]
pub enum ClientError
{
    #[error(transparent)]
    Connection(#[from] ConnectionError),
    #[error(transparent)]
    Write(#[from] WriteError),
}

#[derive(Debug, Error, Clone, PartialEq, Eq)]
pub enum ServerError
{
    #[error(transparent)]
    ReadExact(#[from] ReadExactError),
    #[error(transparent)]
    ReadToEnd(#[from] ReadToEndError),
    #[error(transparent)]
    FromPrimitive(#[from] FromPrimitiveError),
    #[error(transparent)]
    SizeMismatch(#[from] SizeMismatchError),
    #[cfg(feature = "signing")]
    #[error("Signature verification failed")]
    SignatureVerification,
    #[cfg(feature = "signing")]
    #[error("Unknown sender")]
    UnknownSender,
}

#[derive(Debug, Error)]
pub enum SendError
{
    #[error(transparent)]
    Send(#[from] tokio::sync::mpsc::error::SendError<NetCommand>),
    #[error(transparent)]
    Receive(#[from] RecvError),
    #[error(transparent)]
    Client(#[from] ClientError),
}

#[derive(Debug, Error)]
pub enum ReceiveError
{
    #[error(transparent)]
    Send(#[from] tokio::sync::mpsc::error::SendError<DataCommand>),
    #[error(transparent)]
    Receive(#[from] RecvError),
    #[error(transparent)]
    Server(#[from] ServerError),
}

#[derive(Debug, Error)]
pub enum SendReceiveError
{
    #[error(transparent)]
    Send(#[from] SendError),
    #[error(transparent)]
    Receive(#[from] ReceiveError),
    #[error(transparent)]
    Join(#[from] JoinError),
    #[error("MultipleErrors(check logs)")]
    Multiple,
}
