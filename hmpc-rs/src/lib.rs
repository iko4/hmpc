#![deny(unsafe_op_in_unsafe_fn)]

/// FFI module.
///
/// Contains functions and data structures that interop with foreign code.
pub mod ffi;
/// Networking module.
pub mod net;
/// Vector utility module.
mod vec;
