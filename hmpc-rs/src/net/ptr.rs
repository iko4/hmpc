use std::ffi::c_void;
use std::ptr::NonNull;

pub type Nullable<T> = Option<NonNull<T>>;

pub type Data = *mut c_void;
pub type NonNullData = NonNull<c_void>;
pub type NullableData = Nullable<c_void>;

#[derive(Debug, Clone)]
pub struct ReadData
{
    ptr: *const u8,
}
impl ReadData
{
    #[must_use]
    pub fn new_unchecked(ptr: *const u8) -> Self
    {
        Self { ptr }
    }

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

#[derive(Debug)]
pub struct WriteData
{
    ptr: *mut u8,
}
impl WriteData
{
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
