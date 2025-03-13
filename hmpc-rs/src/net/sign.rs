use ring::signature::{ED25519, Ed25519KeyPair, UnparsedPublicKey};

pub(crate) const SIGNATURE_SIZE: usize = 64; // see https://ed25519.cr.yp.to/

/// Bytes obtained by signing.
pub type Signature = [u8; SIGNATURE_SIZE];

/// Wrapper for a public signature verification key.
#[derive(Debug)]
pub struct PublicKey(UnparsedPublicKey<Vec<u8>>);
impl PublicKey
{
    pub fn new(bytes: Vec<u8>) -> Self
    {
        Self(UnparsedPublicKey::new(&ED25519, bytes))
    }

    /// Verify a signature.
    pub fn verify(&self, message: &[u8], signature: &[u8]) -> Result<(), ()>
    {
        self.0.verify(message, signature).or(Err(()))
    }
}

/// Wrapper for a private signing key.
#[derive(Debug)]
pub struct PrivateKey(Ed25519KeyPair);
impl PrivateKey
{
    pub fn new(bytes: &[u8]) -> Result<Self, ring::error::KeyRejected>
    {
        Ed25519KeyPair::from_pkcs8(bytes).map(Self)
    }

    /// Sign a sequence of bytes.
    pub fn sign(&self, message: &[u8]) -> Signature
    {
        // PANICS: With the selected signature algorithm, the size should always match
        self.0.sign(message).as_ref().try_into().unwrap()
    }
}
