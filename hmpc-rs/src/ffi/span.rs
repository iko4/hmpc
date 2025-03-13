use core::slice;
use std::collections::BTreeSet;
use std::ptr::NonNull;

use crate::vec::Vec2d;

/// Wrapper for array/vector data coming from foreign code.
#[repr(C)]
#[derive(Debug)]
pub struct Span<T>
{
    data: *const T,
    len: usize,
}

impl<T: Clone> Span<T>
{
    /// Constructs a(n ordered) set from pointer and length.
    ///
    /// If `len` is zero, the value of the pointer is ignored and an empty set is returned.
    ///
    /// # Safety
    /// The `data` pointer has to be valid.
    /// (The function does *not* check for `nullptr`. Use [`Self::try_to_set`] instead.)
    /// The `data` pointer has to point to a region of (at least) `len` valid objects of type `T`.
    #[must_use]
    pub unsafe fn to_set(self) -> BTreeSet<T>
    where
        T: Eq + Ord,
    {
        if self.len == 0
        {
            return BTreeSet::new();
        }

        let data = unsafe { slice::from_raw_parts(self.data, self.len) };
        data.iter().cloned().collect()
    }

    /// Constructs a vector from pointer and length.
    ///
    /// If `len` is zero, the value of the pointer is ignored and an empty vector is returned.
    ///
    /// # Safety
    /// The `data` pointer has to be valid.
    /// (The function does *not* check for `nullptr`. Use [`Self::try_to_vec`] instead.)
    /// The `data` pointer has to point to a region of (at least) `len` valid objects of type `T`.
    #[must_use]
    pub unsafe fn to_vec(self) -> Vec<T>
    {
        if self.len == 0
        {
            return Vec::new();
        }

        let data = unsafe { slice::from_raw_parts(self.data, self.len) };
        data.to_vec()
    }

    /// Constructs a(n ordered) set from pointer and length, returning `None` if the pointer is null (unless the length is also zero).
    ///
    /// # Safety
    /// The `data` pointer has to be valid.
    /// (The function only checks for `nullptr`.)
    /// The `data` pointer has to point to a region of (at least) `len` valid objects of type `T`.
    #[must_use]
    pub unsafe fn try_to_set(self) -> Option<BTreeSet<T>>
    where
        T: Eq + Ord,
    {
        if self.len == 0
        {
            return Some(BTreeSet::new());
        }

        if self.data.is_null()
        {
            return None;
        }

        unsafe { Some(self.to_set()) }
    }

    /// Constructs a vector from pointer and length, returning `None` if the pointer is null (unless the length is also zero).
    ///
    /// # Safety
    /// The `data` pointer has to be valid.
    /// (The function only checks for `nullptr`.)
    /// The `data` pointer has to point to a region of (at least) `len` valid objects of type `T`.
    #[must_use]
    pub unsafe fn try_to_vec(self) -> Option<Vec<T>>
    {
        if self.len == 0
        {
            return Some(Vec::new());
        }

        if self.data.is_null()
        {
            return None;
        }

        unsafe { Some(self.to_vec()) }
    }
}

impl<T> Span<Option<NonNull<T>>>
{
    /// Constructs a vector from pointer and length.
    /// If any pointer inside the vector would be null, `None` is returned as well.
    /// See [`Self::to_vec`].
    ///
    /// # Safety
    /// See [`Self::to_vec`].
    #[must_use]
    pub unsafe fn try_to_non_null_vec(self) -> Option<Vec<NonNull<T>>>
    {
        if self.len == 0
        {
            return Some(Vec::new());
        }

        if self.data.is_null()
        {
            return None;
        }

        let v = unsafe { self.to_vec() };

        if v.iter().any(Option::is_none)
        {
            None
        }
        else
        {
            Some(v.into_iter().map(Option::unwrap).collect())
        }
    }
}

/// Wrapper for 2d array/vector data coming from C ode.
#[allow(clippy::module_name_repetitions)]
#[repr(C)]
#[derive(Debug)]
pub struct Span2d<T>
{
    data: *const T,
    outer_extent: usize,
    inner_extent: usize,
}

impl<T: Clone> Span2d<T>
{
    /// Constructs a 2d vector from pointer and lengths.
    ///
    /// If any extent is zero, the value of the pointer is ignored and an empty vector is returned.
    ///
    /// # Safety
    /// The `data` pointer has to be valid.
    /// (The function does *not* check for `nullptr`. Use [`Self::try_to_vec`] instead.)
    /// The `data` pointer has to point to a region of (at least) `inner_extent * outer_extent` valid objects of type `T`.
    #[must_use]
    pub unsafe fn to_vec(self) -> Vec2d<T>
    {
        if self.inner_extent == 0 || self.outer_extent == 0
        {
            return Vec2d::new();
        }

        let data = unsafe { slice::from_raw_parts(self.data, self.inner_extent * self.outer_extent) };
        Vec2d::from(data, self.outer_extent, self.inner_extent)
    }

    /// Constructs a 2d vector from pointer and lengths, returning `None` if the pointer is null (unless the extents are also zero).
    ///
    /// # Safety
    /// The `data` pointer has to be valid.
    /// (The function only checks for `nullptr`.)
    /// The `data` pointer has to point to a region of (at least) `inner_extent * outer_extent` valid objects of type `T`.
    #[must_use]
    pub unsafe fn try_to_vec(self) -> Option<Vec2d<T>>
    {
        if self.inner_extent == 0 || self.outer_extent == 0
        {
            return Some(Vec2d::new());
        }

        if self.data.is_null()
        {
            return None;
        }

        unsafe { Some(self.to_vec()) }
    }
}

impl<T> Span2d<Option<NonNull<T>>>
{
    /// Constructs a 2d vector from pointer and lengths.
    /// If any pointer inside the vector would be null, `None` is returned as well.
    /// See [`Self::to_vec`].
    ///
    /// # Safety
    /// See [`Self::to_vec`].
    #[must_use]
    pub unsafe fn try_to_non_null_vec(self) -> Option<Vec2d<NonNull<T>>>
    {
        if self.inner_extent == 0 || self.outer_extent == 0
        {
            return Some(Vec2d::new());
        }

        if self.data.is_null()
        {
            return None;
        }

        let inner_extent = self.inner_extent;
        let outer_extent = self.outer_extent;

        let v = unsafe { self.to_vec() };
        debug_assert_eq!(v.len(), inner_extent * outer_extent);

        if v.iter_elements().any(Option::is_none)
        {
            None
        }
        else
        {
            Some(Vec2d::from(
                v.into_iter_elements()
                    .map(Option::unwrap)
                    .collect::<Vec<_>>(),
                outer_extent,
                inner_extent,
            ))
        }
    }
}
