use ring::signature::{Ed25519KeyPair, Signature, UnparsedPublicKey, ED25519};

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
        self.0.sign(message)
    }
}
