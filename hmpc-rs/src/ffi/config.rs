use std::ffi::{CStr, c_char};

use log::{debug, error};

use super::{check_mut_pointer, check_pointer, free_nullable, nullable};
use crate::ffi::set_logger;
use crate::net::config::{Config, Session};
use crate::net::ptr::Nullable;

macro_rules! path_from_string
{
    ($id:ident) =>
    {
        let $id = if $id.is_null()
        {
            debug!("Null string given as path");
            None
        }
        else
        {
            let str = unsafe { CStr::from_ptr($id) };
            if let Ok(str) = str.to_str()
            {
                debug!("Using path: {str}");
                Some(str.into())
            }
            else
            {
                error!("Could not convert char pointer to string");
                return None;
            }
        };
    };
}

/// Construct a [`Config`] object from file.
///
/// This tries to find a config path in the following order:
/// - the provided path (if not `nullptr`)
/// - the environment
/// - the hard-coded fallback config path
/// Also sets the global logger to a (defaulted) [`env_logger`].
///
/// # Safety
/// The `config` pointer has to be a valid null-terminated C-string or `nullptr`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn hmpc_ffi_net_config_read(config: *const c_char) -> Nullable<Config>
{
    set_logger();

    debug!("Read config");
    path_from_string!(config);

    match Config::read(config)
    {
        Ok(config) => nullable(config),
        Err(e) =>
        {
            error!("Reading config failed: {e:?}");
            None
        },
    }
}

/// Construct a [`Config`] object from file.
///
/// This tries to find a config path in the following order:
/// - the environment
/// - the provided path (if not `nullptr`)
/// - the hard-coded fallback config path
/// Also sets the global logger to a (defaulted) [`env_logger`].
///
/// # Safety
/// The `config` pointer has to be a valid null-terminated C-string or `nullptr`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn hmpc_ffi_net_config_read_env(config: *const c_char) -> Nullable<Config>
{
    set_logger();

    debug!("Read config with precedence from environment");
    path_from_string!(config);

    match Config::read_env(config)
    {
        Ok(config) => nullable(config),
        Err(e) =>
        {
            error!("Reading config failed: {e:?}");
            None
        },
    }
}

/// Drop a [`Config`] object and free its memory.
///
/// # Safety
/// This function frees the passed pointer with [`Box::from_raw`].
/// As a caller, ensure that the pointer actually comes from a [`Box`] and is not freed multiple times.
/// This function only checks for `nullptr` but cannot do any other checks.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn hmpc_ffi_net_config_free(config: Nullable<Config>)
{
    debug!("Freeing config");
    free_nullable!(config);
}
