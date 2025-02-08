use log::{debug, error, info, warn};

use super::errors::SendReceiveErrc;
use super::{check_communicator, check_mut_pointer, check_non_null_vec, check_pointer, check_size_eq, check_span, check_vec, free_nullable, nullable, set_logger, Communicator, Config, NetworkStatistics, NullableData, PartyID, Queue, Span, Span2d};
use crate::net::metadata::{AllGather, AllToAll, Broadcast, Gather};
use crate::net::ptr::Nullable;

/// Construct a `Queue` object for network operations
///
/// Note that this *frees the config object* if it is given.
/// Also sets the global logger to a (defaulted) `env_logger`.
/// # Safety
/// If `config` is not null, this function frees the passed pointer with `Box::from_raw`.
/// As a caller, ensure that the pointer actually comes from a `Box` and is not freed multiple times.
/// This function only checks for `nullptr` but cannot do any other checks.
#[no_mangle]
#[must_use]
pub unsafe extern "C" fn hmpc_ffi_net_queue_init(id: PartyID, config: Nullable<Config>) -> Nullable<Queue>
{
    set_logger();

    debug!("Initializing queue");

    let config = if let Some(config) = config
    {
        let boxed = unsafe { Box::from_raw(config.as_ptr()) };
        *boxed
    }
    else
    {
        warn!("Config was null. Consider constructing a config. Trying to load config from default path.");
        match Config::read(None)
        {
            Ok(config) => config,
            Err(e) =>
            {
                error!("Error reading config from default path: {e:?}");
                return None;
            },
        }
    };

    match Queue::new(id, config)
    {
        Ok(queue) => nullable(queue),
        Err(e) =>
        {
            error!("Error initializing queue: {:?}", e);
            None
        },
    }
}

/// Drop a `Queue` object and free its memory
/// # Safety
/// This function frees the passed pointer with `Box::from_raw`.
/// As a caller, ensure that the pointer actually comes from a `Box` and is not freed multiple times.
/// This function only checks for `nullptr` but cannot do any other checks.
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_free(queue: Nullable<Queue>)
{
    debug!("Freeing queue");
    free_nullable!(queue);
}

/// Perform a broadcast network operation via the `Queue` object
/// # Safety
/// The `queue` pointer has to be valid. (The function only checks for `nullptr`.)
///
/// The `communicator` has to be valid. (The function only checks for `nullptr`.)
///
/// The `data` pointer has to be valid. (The function only checks for `nullptr`.)
/// The `data` pointer has to point to a region of (at least) `message.size` valid bytes.
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_broadcast(queue: Nullable<Queue>, message: Broadcast, communicator: Communicator, data: NullableData) -> SendReceiveErrc
{
    info!("Broadcast");

    check_mut_pointer!(queue, SendReceiveErrc::InvalidHandle);
    check_pointer!(data, SendReceiveErrc::InvalidPointer);
    check_communicator!(communicator);

    let queue = unsafe { queue.as_mut() };

    if let Err(e) = queue.broadcast_blocking(message, &communicator, data)
    {
        error!("Send error: {:?}", e);
        e.into()
    }
    else
    {
        SendReceiveErrc::Ok
    }
}

/// Perform multiple broadcast network operations via the `Queue` object
/// # Safety
/// The `queue` pointer has to be valid. (The function only checks for `nullptr`.)
///
/// The `messages` span has to be valid. (The function only checks for `nullptr`.)
///
/// The `communicator` has to be valid. (The function only checks for `nullptr`.)
///
/// The `data` span has to be valid. (The function only checks for `nullptr`.)
/// Each `data[i]` pointer has to be valid. (The function only checks for `nullptr`.)
/// Each `data[i]` pointer has to point to a region of (at least) `messages[i].size` valid bytes.
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_multi_broadcast(queue: Nullable<Queue>, messages: Span<Broadcast>, communicator: Communicator, data: Span<NullableData>) -> SendReceiveErrc
{
    info!("Multi broadcast");

    check_mut_pointer!(queue, SendReceiveErrc::InvalidHandle);
    check_vec!(messages);
    check_communicator!(communicator);
    check_non_null_vec!(data);
    check_size_eq!(messages.len(), data.len());

    let queue = unsafe { queue.as_mut() };

    if let Err(e) = queue.multi_broadcast_blocking(messages, &communicator, data)
    {
        error!("Send error: {:?}", e);
        e.into()
    }
    else
    {
        SendReceiveErrc::Ok
    }
}

/// Perform a gather network operation via the `Queue` object
/// # Safety
/// The `queue` pointer has to be valid. (The function only checks for `nullptr`.)
///
/// The `communicator` has to be valid. (The function only checks for `nullptr`.)
///
/// The `data` span has to be valid. (The function only checks for `nullptr`.)
/// If `message.receiver == queue.id`:
/// - `data` and `communicator` have to have the same length.
/// - Each `data[i]` pointer has to be valid. (The function only checks for `nullptr`.)
/// - Each `data[i]` pointer has to point to a region of (at least) `message.size` valid bytes.
///
/// Otherwise:
/// - `data` has length 1.
/// - The `data[0]` pointer has to be valid. (The function only checks for `nullptr`.)
/// - The `data[0]` pointer has to point to a region of (at least) `message.size` valid bytes.
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_gather(queue: Nullable<Queue>, message: Gather, communicator: Communicator, data: Span<NullableData>) -> SendReceiveErrc
{
    info!("Gather");

    check_mut_pointer!(queue, SendReceiveErrc::InvalidHandle);
    check_communicator!(communicator);
    check_non_null_vec!(data);

    let queue = unsafe { queue.as_mut() };

    if queue.id() == message.receiver
    {
        check_size_eq!(communicator.len(), data.len());
    }
    if queue.id() != message.receiver
    {
        check_size_eq!(data.len(), 1);
    }

    if let Err(e) = queue.gather_blocking(message, &communicator, data)
    {
        error!("Send error: {:?}", e);
        e.into()
    }
    else
    {
        SendReceiveErrc::Ok
    }
}

/// Perform multiple gather network operations via the `Queue` object
/// # Safety
/// The `queue` pointer has to be valid. (The function only checks for `nullptr`.)
///
/// The `messages` span has to be valid. (The function only checks for `nullptr`.)
/// This includes that
/// - all `messages[i].receiver == queue.id` or
/// - all `messages[i].receiver != queue.id`
///
/// The `communicator` has to be valid. (The function only checks for `nullptr`.)
///
/// The `data` span has to be valid. (The function only checks for `nullptr`.)
/// If all `message[s].receiver == queue.id`:
/// - `data[i]` and `communicator` have to have the same length.
/// - Each `data[i][j]` pointer has to be valid. (The function only checks for `nullptr`.)
/// - Each `data[i][j]` pointer has to point to a region of (at least) `messages[i].size` valid bytes.
///
/// Otherwise:
/// - `data[i]` has length 1.
/// - The `data[i][0]` pointer has to be valid. (The function only checks for `nullptr`.)
/// - The `data[i][0]` pointer has to point to a region of (at least) `messages[i].size` valid bytes.
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_multi_gather(queue: Nullable<Queue>, messages: Span<Gather>, communicator: Communicator, data: Span2d<NullableData>) -> SendReceiveErrc
{
    info!("Multi gather");

    check_mut_pointer!(queue, SendReceiveErrc::InvalidHandle);
    check_vec!(messages);
    check_communicator!(communicator);
    check_non_null_vec!(data);
    check_size_eq!(messages.len(), data.outer_extent());

    let queue = unsafe { queue.as_mut() };

    let is_always_receiver = messages.iter().all(|message| message.receiver == queue.id());
    let is_never_receiver = messages.iter().all(|message| message.receiver != queue.id());

    if !(is_always_receiver || is_never_receiver)
    {
        error!("Has to be either always or never the receiver");
        return SendReceiveErrc::InvalidMetadata;
    }
    if is_always_receiver
    {
        check_size_eq!(data.inner_extent(), communicator.len());
    }
    if is_never_receiver
    {
        check_size_eq!(data.inner_extent(), 1);
    }

    if let Err(e) = queue.multi_gather_blocking(messages, &communicator, data)
    {
        error!("Send error: {:?}", e);
        e.into()
    }
    else
    {
        SendReceiveErrc::Ok
    }
}

/// Perform an all-gather network operation via the `Queue` object
/// # Safety
/// The `queue` pointer has to be valid. (The function only checks for `nullptr`.)
///
/// The `communicator` has to be valid. (The function only checks for `nullptr`.)
///
/// The `data` span has to be valid. (The function only checks for `nullptr`.)
/// Each `data[i]` pointer has to be valid. (The function only checks for `nullptr`.)
/// Each `data[i]` pointer has to point to a region of (at least) `message.size` valid bytes.
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_all_gather(queue: Nullable<Queue>, message: AllGather, communicator: Communicator, data: Span<NullableData>) -> SendReceiveErrc
{
    info!("All-gather");

    check_mut_pointer!(queue, SendReceiveErrc::InvalidHandle);
    check_communicator!(communicator);
    check_non_null_vec!(data);
    check_size_eq!(communicator.len(), data.len());

    let queue = unsafe { queue.as_mut() };
    if let Err(e) = queue.all_gather_blocking(message, &communicator, data)
    {
        error!("Receive error: {:?}", e);
        e.into()
    }
    else
    {
        SendReceiveErrc::Ok
    }
}

/// Perform multiple all-gather network operation via the `Queue` object
/// # Safety
/// The `queue` pointer has to be valid. (The function only checks for `nullptr`.)
///
/// The `messages` span has to be valid. (The function only checks for `nullptr`.)
///
/// The `communicator` has to be valid. (The function only checks for `nullptr`.)
///
/// The `data` span has to be valid. (The function only checks for `nullptr`.)
// TODO: clarify validity for data
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_multi_all_gather(queue: Nullable<Queue>, messages: Span<AllGather>, communicator: Communicator, data: Span2d<NullableData>) -> SendReceiveErrc
{
    info!("Multi all-gather");

    check_mut_pointer!(queue, SendReceiveErrc::InvalidHandle);
    check_vec!(messages);
    check_communicator!(communicator);
    check_non_null_vec!(data);
    check_size_eq!(communicator.len(), data.inner_extent());
    check_size_eq!(messages.len(), data.outer_extent());

    let queue = unsafe { queue.as_mut() };

    if let Err(e) = queue.multi_all_gather_blocking(messages, &communicator, data)
    {
        error!("Receive error: {:?}", e);
        e.into()
    }
    else
    {
        SendReceiveErrc::Ok
    }
}

/// Perform an extended all-gather network operation via the `Queue` object
/// # Safety
/// The `queue` pointer has to be valid. (The function only checks for `nullptr`.)
/// The `senders` communicator has to be valid. (The function only checks for `nullptr`.)
/// The `receivers` communicator has to be valid. (The function only checks for `nullptr`.)
///
/// The `data` span has to be valid. (The function only checks for `nullptr`.)
/// Each `data[i]` pointer has to be valid. (The function only checks for `nullptr`.)
/// Each `data[i]` pointer has to point to a region of (at least) `message.size` valid bytes.
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_extended_all_gather(queue: Nullable<Queue>, message: AllGather, senders: Communicator, receivers: Communicator, data: Span<NullableData>) -> SendReceiveErrc
{
    info!("Extended all-gather");

    check_mut_pointer!(queue, SendReceiveErrc::InvalidHandle);
    check_communicator!(senders);
    check_communicator!(receivers);
    check_non_null_vec!(data);
    check_size_eq!(senders.len(), data.len());
    if !senders.is_subset(&receivers)
    {
        error!("Senders are not a subset of receivers");
        return SendReceiveErrc::InvalidCommunicator;
    }

    let queue = unsafe { queue.as_mut() };

    if let Err(e) = queue.extended_all_gather_blocking(message, &senders, &receivers, data)
    {
        error!("Receive error: {:?}", e);
        e.into()
    }
    else
    {
        SendReceiveErrc::Ok
    }
}

/// Perform multiple extended all-gather network operation via the `Queue` object
/// # Safety
/// The `queue` pointer has to be valid. (The function only checks for `nullptr`.)
///
/// The `messages` span has to be valid. (The function only checks for `nullptr`.)
///
/// The `senders` communicator has to be valid. (The function only checks for `nullptr`.)
///
/// The `receivers` communicator has to be valid. (The function only checks for `nullptr`.)
///
/// The `data` span has to be valid. (The function only checks for `nullptr`.)
// TODO: clarify validity for data
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_extended_multi_all_gather(queue: Nullable<Queue>, messages: Span<AllGather>, senders: Communicator, receivers: Communicator, data: Span2d<NullableData>) -> SendReceiveErrc
{
    info!("Extended multi all-gather");

    check_mut_pointer!(queue, SendReceiveErrc::InvalidHandle);
    check_vec!(messages);
    check_communicator!(senders);
    check_communicator!(receivers);
    check_non_null_vec!(data);
    check_size_eq!(senders.len(), data.inner_extent());
    check_size_eq!(messages.len(), data.outer_extent());
    if !senders.is_subset(&receivers)
    {
        error!("Senders are not a subset of receivers");
        return SendReceiveErrc::InvalidCommunicator;
    }

    let queue = unsafe { queue.as_mut() };

    if let Err(e) = queue.extended_multi_all_gather_blocking(messages, &senders, &receivers, data)
    {
        error!("Receive error: {:?}", e);
        e.into()
    }
    else
    {
        SendReceiveErrc::Ok
    }
}

/// Perform an all-to-all network operation via the `Queue` object
/// # Safety
/// The `queue` pointer has to be valid. (The function only checks for `nullptr`.)
///
/// The `communicator` has to be valid. (The function only checks for `nullptr`.)
///
/// The `send_data` span has to be valid. (The function only checks for `nullptr`.)
///
/// The `receive_data` span has to be valid. (The function only checks for `nullptr`.)
// TODO: clarify validity for send_data and receive_data
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_all_to_all(queue: Nullable<Queue>, message: AllToAll, communicator: Communicator, send_data: Span<NullableData>, receive_data: Span<NullableData>) -> SendReceiveErrc
{
    info!("All-to-all");

    check_mut_pointer!(queue, SendReceiveErrc::InvalidHandle);
    check_communicator!(communicator);
    check_non_null_vec!(send_data);
    check_non_null_vec!(receive_data);
    check_size_eq!(communicator.len(), send_data.len());
    check_size_eq!(communicator.len(), receive_data.len());

    let queue = unsafe { queue.as_mut() };

    if let Err(e) = queue.all_to_all_blocking(message, &communicator, send_data, receive_data)
    {
        error!("Receive error: {:?}", e);
        e.into()
    }
    else
    {
        SendReceiveErrc::Ok
    }
}

/// Perform multiple all-to-all network operation via the `Queue` object
/// # Safety
/// The `queue` pointer has to be valid. (The function only checks for `nullptr`.)
///
/// The `messages` span has to be valid. (The function only checks for `nullptr`.)
///
/// The `communicator` has to be valid. (The function only checks for `nullptr`.)
///
/// The `send_data` span has to be valid. (The function only checks for `nullptr`.)
///
/// The `receive_data` span has to be valid. (The function only checks for `nullptr`.)
// TODO: clarify validity for send_data and receive_data
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_multi_all_to_all(queue: Nullable<Queue>, messages: Span<AllToAll>, communicator: Communicator, send_data: Span2d<NullableData>, receive_data: Span2d<NullableData>) -> SendReceiveErrc
{
    info!("Multi all-to-all");

    check_mut_pointer!(queue, SendReceiveErrc::InvalidHandle);
    check_vec!(messages);
    check_communicator!(communicator);
    check_non_null_vec!(send_data);
    check_non_null_vec!(receive_data);
    check_size_eq!(communicator.len(), send_data.inner_extent());
    check_size_eq!(communicator.len(), receive_data.inner_extent());
    check_size_eq!(messages.len(), send_data.outer_extent());
    check_size_eq!(messages.len(), receive_data.outer_extent());

    let queue = unsafe { queue.as_mut() };

    if let Err(e) = queue.multi_all_to_all_blocking(messages, &communicator, send_data, receive_data)
    {
        error!("Receive error: {:?}", e);
        e.into()
    }
    else
    {
        SendReceiveErrc::Ok
    }
}

/// Consistency checks for collective communication operations are disabled.
/// Enable the "collective-consistency" feature to add consistency checks.
#[cfg(not(feature = "collective-consistency"))]
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_wait(_queue: Nullable<Queue>) -> SendReceiveErrc
{
    warn!("The \"collective-consistency\" feature is not enabled");
    SendReceiveErrc::Ok
}

/// Wait for collective communication operations to finish
/// # Safety
/// The `queue` pointer has to be valid. (The function only checks for `nullptr`.)
#[cfg(feature = "collective-consistency")]
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_wait(queue: Nullable<Queue>) -> SendReceiveErrc
{
    info!("Wait");

    check_mut_pointer!(queue, SendReceiveErrc::InvalidHandle);

    let queue = unsafe { queue.as_mut() };

    if let Err(e) = queue.wait()
    {
        error!("Wait error: {:?}", e);
        e.into()
    }
    else
    {
        SendReceiveErrc::Ok
    }
}

/// Retrieving network statistics is disabled.
/// Enable the "statistics" feature to get estimates on the number of bytes sent and received.
#[cfg(not(feature = "statistics"))]
#[no_mangle]
pub extern "C" fn hmpc_ffi_net_queue_network_statistics(_queue: Nullable<Queue>) -> NetworkStatistics
{
    warn!("The \"statistics\" feature is not enabled");
    NetworkStatistics::new()
}

/// Retrieve network statistics from a `Queue` object.
/// This is only the estimated number of bytes send and received.
/// If messages did not yet arrive or errors occurred (or unexpected messages arrived), this number is off.
/// Currently, this only includes the payload of messages.
/// # Safety
/// The `queue` pointer has to be valid. (The function only checks for `nullptr`.)
#[cfg(feature = "statistics")]
#[no_mangle]
pub unsafe extern "C" fn hmpc_ffi_net_queue_network_statistics(queue: Nullable<Queue>) -> NetworkStatistics
{
    let Some(mut queue) = queue
    else
    {
        error!("Queue pointer is null");
        return NetworkStatistics::new();
    };

    let queue = unsafe { queue.as_mut() };

    queue.network_statistics()
}
