use ring::digest::{Context, SHA256, SHA256_OUTPUT_LEN};

pub(crate) const HASH_SIZE: usize = SHA256_OUTPUT_LEN;

pub(crate) fn hash(data: &[u8]) -> Hash
{
    let mut sha = Context::new(&SHA256);

    sha.update(data);

    // PANICS: Hash length should match by construction
    sha.finish().as_ref().try_into().unwrap()
}

pub(crate) type Hash = [u8; HASH_SIZE];
