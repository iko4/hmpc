use std::ffi::c_void;
use std::ptr::NonNull;

pub type Nullable<T> = Option<NonNull<T>>;

/// Pointer to data from foreign code.
pub type Data = *mut c_void;
/// Non-null pointer to data from foreign code.
pub type NonNullData = NonNull<c_void>;
/// Pointer to data from foreign code.
pub type NullableData = Nullable<c_void>;

/// Pointer to read-only data.
#[derive(Debug, Clone)]
pub struct ReadData
{
    ptr: *const u8,
}
impl ReadData
{
    /// Create new [`ReadData`] pointer without checking the input for null or any other kind of validity.
    #[must_use]
    pub fn new_unchecked(ptr: *const u8) -> Self
    {
        Self { ptr }
    }

    /// Get the contained raw pointer.
    #[must_use]
    pub fn as_ptr(&self) -> *const u8
    {
        self.ptr
    }
}
unsafe impl Send for ReadData {}
impl From<NonNullData> for ReadData
{
    fn from(value: NonNullData) -> Self
    {
        Self { ptr: value.as_ptr() as _ }
    }
}

/// Pointer to write-only data.
#[derive(Debug)]
pub struct WriteData
{
    ptr: *mut u8,
}
impl WriteData
{
    /// Get the contained raw pointer.
    #[must_use]
    pub fn as_ptr(&self) -> *mut u8
    {
        self.ptr
    }
}
unsafe impl Send for WriteData {}
impl From<NonNullData> for WriteData
{
    fn from(value: NonNullData) -> Self
    {
        Self { ptr: value.as_ptr().cast() }
    }
}
