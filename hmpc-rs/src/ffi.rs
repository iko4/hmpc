use std::ptr::NonNull;

use log::debug;

mod config;
mod errors;
mod queue;
mod span;

pub use self::span::{Span, Span2d};
pub use crate::net::ptr::{Nullable, NullableData};
pub use crate::net::queue::NetworkStatistics;
pub use crate::net::{Config, MessageDatatype, MessageID, MessageKind, PartyID, Queue};

pub type Size = u64;
pub type Communicator = Span<PartyID>;

macro_rules! check_pointer
{
    ($id:ident $(, $result:expr)?) =>
    {
        let Some($id) = $id
        else
        {
            const NAME: &str = stringify!($id);
            error!("{NAME} pointer is null");
            return $($result)?;
        };
    };
}

macro_rules! check_mut_pointer
{
    ($id:ident $(, $result:expr)?) =>
    {
        let Some(mut $id) = $id
        else
        {
            const NAME: &str = stringify!($id);
            error!("{NAME} pointer is null");
            return $($result)?;
        };
    };
}

macro_rules! check_span
{
    ($id:ident, $conversion:ident) =>
    {
        let Some($id) = (unsafe { $id.$conversion() })
        else
        {
            const NAME: &str = stringify!($id);
            error!("{NAME} span is null");
            return SendReceiveErrc::InvalidPointer;
        };
    };
}

macro_rules! check_communicator
{
    ($id:ident) =>
    {
        check_span!($id, try_to_set);
    };
}

macro_rules! check_vec
{
    ($id:ident) =>
    {
        check_span!($id, try_to_vec);
    };
}

macro_rules! check_non_null_vec
{
    ($id:ident) =>
    {
        check_span!($id, try_to_non_null_vec);
    };
}

macro_rules! check_size_eq
{
    ($left:expr, $right:expr) =>
    {
        if $left != $right
        {
            const LEFT_NAME: &str = stringify!($left);
            const RIGHT_NAME: &str = stringify!($right);
            let left = $left;
            let right = $right;
            error!("Sizes of {LEFT_NAME} and {RIGHT_NAME} do not match: {left} != {right}");
            return SendReceiveErrc::InvalidSize;
        }
    };
}

#[allow(clippy::unnecessary_wraps)]
fn nullable<T>(x: T) -> Nullable<T>
{
    let ptr = Box::into_raw(Box::new(x));
    // SAFETY: The result of `Box::into_raw` is non-null
    unsafe { Some(NonNull::new_unchecked(ptr)) }
}

macro_rules! free_nullable
{
    ($id:ident) =>
    {
        check_pointer!($id);

        let boxed = unsafe { Box::from_raw($id.as_ptr()) };
        drop(boxed);
    };
}

pub(crate) use {check_communicator, check_mut_pointer, check_pointer, check_non_null_vec, check_size_eq, check_span, check_vec, free_nullable};

fn set_logger()
{
    #[rustfmt::skip]
    let logger = env_logger::builder()
        .build();
    let logger = Box::new(logger);
    if let Ok(()) = log::set_boxed_logger(logger)
    {
        log::set_max_level(log::STATIC_MAX_LEVEL);
        debug!("Set logger. Set log_max_level to {}", log::STATIC_MAX_LEVEL);
    }
    else
    {
        debug!("Not setting logger (already set)");
    }
}
