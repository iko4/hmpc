use std::slice::ChunksExact;

#[allow(clippy::module_name_repetitions)]
#[derive(Debug)]
pub struct Vec2d<T>
{
    data: Vec<T>,
    extents: (usize, usize),
}

impl<T> Vec2d<T>
{
    pub fn new() -> Self
    {
        Self { data: Vec::new(), extents: (0, 0) }
    }

    pub fn from<U>(data: U, outer_extent: usize, inner_extent: usize) -> Self
    where
        U: Into<Vec<T>>,
    {
        let data = data.into();
        assert_eq!(data.len(), outer_extent * inner_extent);
        Self { data, extents: (outer_extent, inner_extent) }
    }

    pub fn from_inner(vec: Vec<T>) -> Self
    {
        let len = vec.len();
        Self { data: vec, extents: (1, len) }
    }

    pub fn len(&self) -> usize
    {
        self.data.len()
    }

    pub fn inner_extent(&self) -> usize
    {
        self.extents.1
    }

    pub fn outer_extent(&self) -> usize
    {
        self.extents.0
    }

    pub fn iter_elements(&self) -> <&Vec<T> as IntoIterator>::IntoIter
    {
        self.data.iter()
    }

    pub fn into_iter_elements(self) -> <Vec<T> as IntoIterator>::IntoIter
    {
        self.data.into_iter()
    }
}

impl<T> Default for Vec2d<T>
{
    fn default() -> Self
    {
        Self::new()
    }
}

impl<'a, T> IntoIterator for &'a Vec2d<T>
{
    type IntoIter = ChunksExact<'a, T>;
    type Item = &'a [T];

    fn into_iter(self) -> Self::IntoIter
    {
        self.data.as_slice().chunks_exact(self.inner_extent())
    }
}


#[cfg(test)]
mod tests
{
    use super::Vec2d;

    #[test]
    fn inner()
    {
        let v = Vec2d::from_inner(vec![1, 2, 3]);
        assert_eq!(v.len(), 3);
        assert_eq!(v.outer_extent(), 1);
        assert_eq!(v.inner_extent(), 3);
    }

    #[test]
    fn iter()
    {
        const EMPTY: &[i32] = &[];

        let v = Vec2d::from(vec![1, 2, 3, 4, 5, 6], 3, 2);
        assert_eq!(v.len(), 6);
        assert_eq!(v.inner_extent(), 2);
        assert_eq!(v.outer_extent(), 3);

        let mut iter = v.into_iter();
        assert_eq!(iter.next(), Some([1, 2].as_slice()));
        assert_eq!(iter.next(), Some([3, 4].as_slice()));
        assert_eq!(iter.next(), Some([5, 6].as_slice()));
        assert_eq!(iter.next(), None);
        assert_eq!(iter.remainder(), EMPTY);
    }
}
