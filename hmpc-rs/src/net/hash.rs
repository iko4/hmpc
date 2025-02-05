use ring::digest::{Context, Digest, SHA256};

pub fn hash(data: &[u8]) -> Digest
{
    let mut sha = Context::new(&SHA256);

    sha.update(data);

    sha.finish()
}
