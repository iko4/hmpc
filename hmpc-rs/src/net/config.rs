use std::collections::BTreeMap;
#[cfg(feature = "signing")]
use std::collections::HashMap;
use std::fmt::{Debug, Display};
use std::num::ParseIntError;
use std::path::{Path, PathBuf};
use std::str::FromStr;
use std::time::Duration;
use std::{env, fs};

use config::{ConfigError, File};
use rustls::pki_types::pem::PemObject;
use rustls::pki_types::{CertificateDer, PrivateKeyDer};
use serde::de::{self, MapAccess, Visitor};
use serde::{Deserialize, Deserializer};
use thiserror::Error;

#[cfg(feature = "signing")]
use super::sign::{PublicKey, PrivateKey};
use super::PartyID;

pub type Port = u16;

/// Name of environment variable that contains a path for a config file
const ENVIRONMENT_VARIABLE: &str = "HMPC_CONFIG";
/// Default path for config file
const DEFAULT_PATH: &str = "mpc.yaml";
/// Default port if none is given
pub(crate) const DEFAULT_PORT: Port = 5000;
/// Timeout for connections
pub(crate) const DEFAULT_TIMEOUT: Duration = Duration::from_secs(10 * 60);
/// Capacity for `tokio::sync::mpsc` channels
pub(crate) const CHANNEL_CAPACITY: usize = 10;

#[derive(Debug, Error)]
pub struct ParseNameError(String);
impl Display for ParseNameError
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result
    {
        write!(f, "Could not parse name: {}", self.0)
    }
}

#[derive(Debug, Error)]
pub enum ParseError
{
    #[error(transparent)]
    ParseInt(#[from] ParseIntError),
    #[error(transparent)]
    ParseName(#[from] ParseNameError),
}

#[derive(Debug, Error)]
pub enum CertificateError
{
    #[error(transparent)]
    AddToStore(#[from] rustls::Error),
    #[error(transparent)]
    File(#[from] tokio::io::Error),
}

#[derive(Debug, Deserialize)]
struct DeserializablePartyOrigin
{
    pub name: String,
    pub port: Option<Port>,
}

#[derive(Debug, Clone)]
pub struct PartyOrigin
{
    pub name: String,
    pub port: Option<Port>,
}

impl From<DeserializablePartyOrigin> for PartyOrigin
{
    fn from(value: DeserializablePartyOrigin) -> Self
    {
        Self { name: value.name, port: value.port }
    }
}

impl FromStr for PartyOrigin
{
    type Err = ParseError;

    fn from_str(s: &str) -> Result<Self, Self::Err>
    {
        let parts: Vec<_> = s.split(':').collect();
        match parts.len()
        {
            1 => Ok(Self { name: parts[0].into(), port: None }),
            2 =>
            {
                let port = parts[1].parse::<Port>()?;
                Ok(Self { name: parts[0].into(), port: Some(port) })
            },
            _ => Err(ParseError::ParseName(ParseNameError(s.into()))),
        }
    }
}

impl<'de> Deserialize<'de> for PartyOrigin
{
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct V();
        impl<'de> Visitor<'de> for V
        {
            type Value = PartyOrigin;

            fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result
            {
                write!(formatter, "\"<name>\" or \"<name>:<port>\" or struct")
            }

            fn visit_str<E>(self, value: &str) -> Result<PartyOrigin, E>
            where
                E: de::Error,
            {
                value.parse().map_err(de::Error::custom)
            }

            fn visit_map<M>(self, map: M) -> Result<PartyOrigin, M::Error>
            where
                M: MapAccess<'de>,
            {
                let full_struct: Result<DeserializablePartyOrigin, _> = Deserialize::deserialize(de::value::MapAccessDeserializer::new(map));
                full_struct.map(Into::into)
            }
        }

        deserializer.deserialize_any(V())
    }
}

#[derive(Debug, Deserialize, Clone)]
pub struct Config
{
    pub parties: BTreeMap<PartyID, PartyOrigin>,
    pub port: Option<Port>,
    pub cert_dir: Option<PathBuf>,
    pub cert_keys_dir: Option<PathBuf>,
    pub sign_verify_dir: Option<PathBuf>,
    pub sign_keys_dir: Option<PathBuf>,
}
impl Config
{
    /// Read a config from file
    ///
    /// Uses default path (see `default_path`) if `config` is `None`.
    /// See `read_file` for more details.
    /// # Errors
    /// See `read_file`
    pub fn read(config: Option<PathBuf>) -> Result<Self, config::ConfigError>
    {
        let config = config.unwrap_or_else(default_path);

        Self::read_file(config)
    }

    /// Read a config from file
    ///
    /// Uses environment path of the config if available.
    /// If not, checks if the given input is available.
    /// Otherwise, uses the fallback path.
    /// See `read_file` for more details.
    /// # Errors
    /// See `read_file`
    pub fn read_env(config: Option<PathBuf>) -> Result<Self, config::ConfigError>
    {
        let config = path_from_env().or(config).unwrap_or_else(fallback_path);

        Self::read_file(config)
    }

    /// Read a config from a given file
    ///
    /// The cert/keys directories are adjusted to default paths relative to the config file if they are not given.
    /// # Errors
    /// - `std::io::Error` when opening file fails
    /// - `std::io::Error` (`NotFound`) when the cert/keys directories cannot be determined
    /// - `serde_yaml::Error` when parsing config fails
    pub fn read_file<P>(path: P) -> Result<Self, ConfigError>
    where
        P: AsRef<Path> + Debug,
    {
        let mut config: Self = config::Config::builder()
            .add_source(File::from(path.as_ref()))
            .build()?
            .try_deserialize()?;
        if config.cert_dir.is_none() || config.cert_keys_dir.is_none() || config.sign_verify_dir.is_none() || config.sign_keys_dir.is_none()
        {
            if let Some(dir) = path.as_ref().parent()
            {
                let base_dir = dir.join(".mpc");
                config.cert_dir.get_or_insert_with(|| base_dir.join("cert"));
                config.cert_keys_dir.get_or_insert_with(|| base_dir.join("cert-keys"));
                config.sign_verify_dir.get_or_insert_with(|| base_dir.join("sign-verify"));
                config.sign_keys_dir.get_or_insert_with(|| base_dir.join("sign-keys"));
            }
            else
            {
                return Err(config::ConfigError::NotFound(format!("Could not determine parent directory of {path:?}")));
            }
        }
        Ok(config)
    }

    /// Port of party `id`
    ///
    /// Tries to find the port in the following order
    /// - Port given for this party (`PartyOrigin::port`)
    /// - Port given for this config (`Config::port`)
    /// - The default port (`DEFAULT_PORT`)
    #[must_use]
    pub fn port(&self, id: PartyID) -> Port
    {
        self.parties
            .get(&id)
            .and_then(|party| party.port)
            .or(self.port)
            .unwrap_or(DEFAULT_PORT)
    }

    /// Server name of party `id`
    ///
    /// # Panics
    /// If `id` is not in config
    #[must_use]
    pub fn name(&self, id: PartyID) -> &str
    {
        &self.parties.get(&id).unwrap().name
    }

    /// Filename of `id`'s certificate
    /// # Panics
    /// If `cert_dir` is `None`
    #[must_use]
    pub fn cert_filename(&self, id: PartyID) -> PathBuf
    {
        self.cert_dir.as_ref().unwrap().join(format!("{id}.der"))
    }

    /// Filename of `id`'s certificate private key
    /// # Panics
    /// If `cert_keys_dir` is `None`
    #[must_use]
    pub fn cert_key_filename(&self, id: PartyID) -> PathBuf
    {
        self.cert_keys_dir.as_ref().unwrap().join(format!("{id}.key"))
    }

    /// Filename of `id`'s signature verification key
    /// # Panics
    /// If `sign_verify_dir` is `None`
    #[must_use]
    pub fn verification_key_filename(&self, id: PartyID) -> PathBuf
    {
        self.sign_verify_dir.as_ref().unwrap().join(format!("{id}.der"))
    }

    /// Filename of `id`'s signing key
    /// # Panics
    /// If `sign_keys_dir` is `None`
    #[must_use]
    pub fn signing_key_filename(&self, id: PartyID) -> PathBuf
    {
        self.sign_keys_dir.as_ref().unwrap().join(format!("{id}.key"))
    }

    /// Load the certificate of party `id` from disk
    /// # Errors
    /// If the file could not be read
    /// # Panics
    /// If `cert_dir` is `None`
    pub async fn cert(&self, id: PartyID) -> Result<CertificateDer, tokio::io::Error>
    {
        let cert = tokio::fs::read(self.cert_filename(id)).await?;
        Ok(CertificateDer::from(cert))
    }

    /// Load all certificates from disk
    ///
    /// See `cert`.
    /// # Errors
    /// See `cert`.
    /// # Panics
    /// See `cert`.
    pub async fn certs(&self) -> Result<rustls::RootCertStore, CertificateError>
    {
        let mut certs = rustls::RootCertStore::empty();
        for &id in self.parties.keys()
        {
            certs.add(self.cert(id).await?)?;
        }
        Ok(certs)
    }

    /// Load the certificate and private key of party `id` from disk
    /// # Errors
    /// If the file could not be read
    /// # Panics
    /// If `cert_dir` or `cert_keys_dir` is `None`
    pub async fn cert_and_key(&self, id: PartyID) -> Result<(CertificateDer<'static>, PrivateKeyDer<'static>), tokio::io::Error>
    {
        let cert = tokio::fs::read(self.cert_filename(id)).await?;
        let key = tokio::fs::read(self.cert_key_filename(id)).await?;
        let cert = CertificateDer::from(cert);
        let key = PrivateKeyDer::Pkcs8(key.into());
        Ok((cert, key))
    }

    /// Load the certificate and private key of party `id` from disk
    /// # Errors
    /// If the file could not be read
    /// # Panics
    /// If `cert_dir` or `cert_keys_dir` is `None`
    pub fn cert_and_key_blocking(&self, id: PartyID) -> Result<(CertificateDer<'static>, PrivateKeyDer<'static>), std::io::Error>
    {
        let cert = fs::read(self.cert_filename(id))?;
        let key = fs::read(self.cert_key_filename(id))?;
        let cert = CertificateDer::from(cert);
        let key = PrivateKeyDer::Pkcs8(key.into());
        Ok((cert, key))
    }

    /// Load the verification key of party `id` from disk
    /// # Errors
    /// If the file could not be read
    /// # Panics
    /// If `sign_verify_dir` is `None`
    #[cfg(feature = "signing")]
    pub async fn verification_key(&self, id: PartyID) -> Result<PublicKey, tokio::io::Error>
    {
        let key = tokio::fs::read(self.verification_key_filename(id)).await?;
        Ok(PublicKey::new(key))
    }

    /// Load the verification keys from disk
    /// # Errors
    /// See `verification_key`.
    /// # Panics
    /// See `verification_key`.
    #[cfg(feature = "signing")]
    pub async fn verification_keys(&self) -> Result<HashMap<PartyID, PublicKey>, tokio::io::Error>
    {
        let parties = self.parties.keys();
        let mut keys = HashMap::with_capacity(parties.len());
        for &id in parties
        {
            keys.insert(id, self.verification_key(id).await?);
        }
        Ok(keys)
    }

    /// Load the signing key of party `id` from disk
    /// # Errors
    /// If the file could not be read or the key is rejected
    /// # Panics
    /// If `sign_keys_dir` is `None`
    #[cfg(feature = "signing")]
    pub async fn signing_key(&self, id: PartyID) -> Result<PrivateKey, tokio::io::Error>
    {
        let key = tokio::fs::read(self.signing_key_filename(id)).await?;
        PrivateKey::new(&key).map_err(tokio::io::Error::other)
    }
}


/// Returns the config path from the environment (if available)
#[must_use]
pub fn path_from_env() -> Option<PathBuf>
{
    env::var(ENVIRONMENT_VARIABLE)
        .map_or(None, |config| Some(PathBuf::from(config)))
}

/// Returns the fallback config path
#[must_use]
pub fn fallback_path() -> PathBuf
{
    PathBuf::from(DEFAULT_PATH)
}

/// Returns the config path from the environment (if available) or the fallback path
#[must_use]
pub fn default_path() -> PathBuf
{
    path_from_env().unwrap_or_else(fallback_path)
}
