use ring::signature::{Ed25519KeyPair, UnparsedPublicKey, ED25519};

pub(crate) const SIGNATURE_SIZE: usize = 64; // see https://ed25519.cr.yp.to/

pub type Signature = [u8; SIGNATURE_SIZE];

#[derive(Debug)]
pub struct PublicKey(UnparsedPublicKey<Vec<u8>>);
#[derive(Debug)]
pub struct PrivateKey(Ed25519KeyPair);

impl PublicKey
{
    pub fn new(bytes: Vec<u8>) -> Self
    {
        Self(UnparsedPublicKey::new(&ED25519, bytes))
    }

    pub fn verify(&self, message: &[u8], signature: &[u8]) -> Result<(), ()>
    {
        self.0.verify(message, signature).or(Err(()))
    }
}

impl PrivateKey
{
    pub fn new(bytes: &[u8]) -> Result<Self, ring::error::KeyRejected>
    {
        Ed25519KeyPair::from_pkcs8(bytes).map(Self)
    }

    pub fn sign(&self, message: &[u8]) -> Signature
    {
        // PANICS: With the selected signature algorithm, the size should always match
        self.0.sign(message).as_ref().try_into().unwrap()
    }
}
