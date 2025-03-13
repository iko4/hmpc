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
use rustls::pki_types::{CertificateDer, PrivateKeyDer};
use serde::de::{self, MapAccess, Visitor};
use serde::{Deserialize, Deserializer};
use thiserror::Error;

use super::hash::hash;
#[cfg(feature = "signing")]
use super::sign::{PrivateKey, PublicKey};
use super::{PartyID, SESSION_ID_SIZE, SessionID};

pub type Port = u16;

/// Name of environment variable that contains a path for a config file.
const CONFIG_PATH_ENVIRONMENT_VARIABLE: &str = "HMPC_CONFIG";
/// Name of environment variable that contains the session value.
const SESSION_ID_VALUE_ENVIRONMENT_VARIABLE: &str = "HMPC_SESSION_VALUE";
/// Name of environment variable that contains the session string to be hashed.
const SESSION_ID_STRING_ENVIRONMENT_VARIABLE: &str = "HMPC_SESSION_STRING";
/// Default path for config file.
const DEFAULT_PATH: &str = "mpc.yaml";
/// Default port if none is given.
pub(crate) const DEFAULT_PORT: Port = 5000;
/// Timeout for connections.
pub(crate) const DEFAULT_TIMEOUT: Duration = Duration::from_secs(10 * 60);
/// Capacity for [`tokio::sync::mpsc`] channels.
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

/// Session ID in a [`Config`].
///
/// Can be given as value ([`Session::Value`]) or string.
/// For strings, the string can be either a string representation of a value ([`Session::Parse`]) or something that is hashed ([`Session::String`]) to get the session ID.
#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "snake_case")]
pub enum Session
{
    /// Session is given as value that can be directly used as-is.
    /// TODO: Currently, deserializing u128 values does not work with the [`config`] crate. Use [`Session::Parse`] instead.
    Value(SessionID),
    /// Session is given as value that can be directly used as-is (after parsing).
    Parse(String),
    /// Session is given as string that will be hashed.
    String(String),
}
impl Session
{
    /// Try to get the a [`Session`] from the environment.
    ///
    /// First tries a value representation ([`SESSION_ID_VALUE_ENVIRONMENT_VARIABLE`]) and then a string representation ([`SESSION_ID_STRING_ENVIRONMENT_VARIABLE`]).
    pub fn try_from_env() -> Option<Session>
    {
        env::var(SESSION_ID_VALUE_ENVIRONMENT_VARIABLE)
            .map(Session::Parse)
            .ok()
            .or_else(|| env::var(SESSION_ID_STRING_ENVIRONMENT_VARIABLE).map(Session::String).ok())
    }
}
impl TryInto<SessionID> for Session
{
    type Error = ParseIntError;

    fn try_into(self) -> Result<SessionID, Self::Error>
    {
        match self
        {
            Session::Value(value) => Ok(value),
            Session::Parse(s) => s.parse(),
            Session::String(s) =>
            {
                let digest = hash(s.as_bytes());
                let digest = digest.as_ref();

                assert!(digest.len() >= SESSION_ID_SIZE);

                // PANICS: Cannot panic because of above assert (that should always be true for SHA256 and u128)
                let value = digest
                    .first_chunk::<SESSION_ID_SIZE>()
                    .map(|bytes| SessionID::from_le_bytes(*bytes))
                    .unwrap();
                Ok(value)
            },
        }
    }
}

/// Raw configuration for networking (to be parsed from files).
///
/// Compared to [`Config`], more members are optional that get replaced by default values if not given.
///
/// This includes:
/// - all parties (including party IDs and matching [`PartyOrigin`])
/// - a default networking port (if none is given in a [`PartyOrigin`])
/// - a directory to look for certificates
/// - a directory to look for certificate keys
/// - a directory to look for signature verification keys
/// - a directory to look for signing keys
/// - a [`Session`]
#[derive(Debug, Deserialize, Clone)]
pub struct RawConfig
{
    pub parties: BTreeMap<PartyID, PartyOrigin>,
    pub port: Option<Port>,
    pub cert_dir: Option<PathBuf>,
    pub cert_keys_dir: Option<PathBuf>,
    pub sign_verify_dir: Option<PathBuf>,
    pub sign_keys_dir: Option<PathBuf>,
    pub session: Option<Session>,
}

/// Configuration for networking.
///
/// This includes:
/// - all parties (including party IDs and matching [`PartyOrigin`])
/// - a default networking port (if none is given in a [`PartyOrigin`])
/// - a directory to look for certificates
/// - a directory to look for certificate keys
/// - a directory to look for signature verification keys
/// - a directory to look for signing keys
/// - a [`Session`]
#[derive(Debug, Clone)]
pub struct Config
{
    pub parties: BTreeMap<PartyID, PartyOrigin>,
    pub port: Option<Port>,
    pub cert_dir: PathBuf,
    pub cert_keys_dir: PathBuf,
    pub sign_verify_dir: PathBuf,
    pub sign_keys_dir: PathBuf,
    #[cfg(feature = "sessions")]
    pub session: SessionID,
}
impl Config
{
    /// Read a [`Config`] from file.
    ///
    /// Uses default path (see [`default_path`]) if `config` is `None`.
    /// See [`Self::read_file`] for more details.
    ///
    /// # Errors
    /// See [`Self::read_file`].
    pub fn read(config: Option<PathBuf>) -> Result<Self, config::ConfigError>
    {
        let config = config.unwrap_or_else(default_path);

        Self::read_file(config)
    }

    /// Read a config from file.
    ///
    /// Uses environment path of the config if available.
    /// If not, checks if the given input is available.
    /// Otherwise, uses the fallback path.
    /// See [`Self::read_file`] for more details.
    ///
    /// # Errors
    /// See [`Self::read_file`].
    pub fn read_env(config: Option<PathBuf>) -> Result<Self, config::ConfigError>
    {
        let config = path_from_env().or(config).unwrap_or_else(fallback_path);

        Self::read_file(config)
    }

    /// Read a config from a given file.
    ///
    /// The cert/keys directories are adjusted to default paths relative to the config file if they are not given.
    ///
    /// # Errors
    /// - [`ConfigError`] when parsing config fails
    /// - if the "sessions" feature is enabled: [`ConfigError::NotFound`] when a session cannot be determined
    /// - if the "sessions" feature is enabled: [`ConfigError::Foreign`] when a session cannot be parsed
    pub fn read_file<P>(path: P) -> Result<Self, ConfigError>
    where
        P: AsRef<Path> + Debug,
    {
        let config: RawConfig = config::Config::builder()
            .add_source(File::from(path.as_ref()))
            .build()?
            .try_deserialize()?;
        let base_dir = path.as_ref().parent().map_or(".mpc".into(), |dir| dir.join(".mpc"));

        let cert_dir = config.cert_dir.unwrap_or_else(|| base_dir.join("cert"));
        let cert_keys_dir = config.cert_keys_dir.unwrap_or_else(|| base_dir.join("cert-keys"));
        let sign_verify_dir = config.sign_verify_dir.unwrap_or_else(|| base_dir.join("sign-verify"));
        let sign_keys_dir = config.sign_keys_dir.unwrap_or_else(|| base_dir.join("sign-keys"));

        #[cfg(feature = "sessions")]
        let session = Session::try_from_env()
            .or(config.session)
            .ok_or(ConfigError::NotFound(format!("No session available")))?
            .try_into()
            .map_err(|e: ParseIntError| ConfigError::Foreign(e.into()))?;

        Ok(Self {
            parties: config.parties,
            port: config.port,
            cert_dir,
            cert_keys_dir,
            sign_verify_dir,
            sign_keys_dir,
            #[cfg(feature = "sessions")]
            session,
        })
    }

    /// Port of party `id`.
    ///
    /// Tries to find the port in the following order:
    /// - port given for this party ([`PartyOrigin::port`])
    /// - port given for this config ([`Config::port`])
    /// - the default port ([`DEFAULT_PORT`])
    #[must_use]
    pub fn port(&self, id: PartyID) -> Port
    {
        self.parties
            .get(&id)
            .and_then(|party| party.port)
            .or(self.port)
            .unwrap_or(DEFAULT_PORT)
    }

    /// Server name of party `id`.
    ///
    /// # Panics
    /// If `id` is not in config.
    #[must_use]
    pub fn name(&self, id: PartyID) -> &str
    {
        self.parties.get(&id).unwrap().name.as_ref()
    }

    /// Filename of party `id`'s certificate.
    #[must_use]
    pub fn cert_filename(&self, id: PartyID) -> PathBuf
    {
        self.cert_dir.join(format!("{id}.x509.cert.der"))
    }

    /// Filename of party `id`'s certificate private key.
    #[must_use]
    pub fn cert_key_filename(&self, id: PartyID) -> PathBuf
    {
        self.cert_keys_dir.join(format!("{id}.cert-private.key.der"))
    }

    /// Filename of party `id`'s signature verification key.
    #[must_use]
    pub fn verification_key_filename(&self, id: PartyID) -> PathBuf
    {
        self.sign_verify_dir.join(format!("{id}.ed25519-public.key.bin"))
    }

    /// Filename of party `id`'s signing key.
    #[must_use]
    pub fn signing_key_filename(&self, id: PartyID) -> PathBuf
    {
        self.sign_keys_dir.join(format!("{id}.ed25519-private.key.der"))
    }

    /// Load the certificate of party `id`.
    ///
    /// # Errors
    /// If the file could not be read.
    pub async fn cert(&self, id: PartyID) -> Result<CertificateDer, tokio::io::Error>
    {
        let cert = tokio::fs::read(self.cert_filename(id)).await?;
        Ok(CertificateDer::from(cert))
    }

    /// Load all certificates.
    ///
    /// See [`Self::cert`].
    ///
    /// # Errors
    /// See [`Self::cert`].
    pub async fn certs(&self) -> Result<rustls::RootCertStore, CertificateError>
    {
        let mut certs = rustls::RootCertStore::empty();
        for &id in self.parties.keys()
        {
            certs.add(self.cert(id).await?)?;
        }
        Ok(certs)
    }

    /// Load the certificate and private key of party `id`.
    ///
    /// # Errors
    /// If the file could not be read.
    pub async fn cert_and_key(&self, id: PartyID) -> Result<(CertificateDer<'static>, PrivateKeyDer<'static>), tokio::io::Error>
    {
        let cert = tokio::fs::read(self.cert_filename(id)).await?;
        let key = tokio::fs::read(self.cert_key_filename(id)).await?;
        let cert = CertificateDer::from(cert);
        let key = PrivateKeyDer::Pkcs8(key.into());
        Ok((cert, key))
    }

    /// Load the certificate and private key of party `id`.
    ///
    /// # Errors
    /// If the file could not be read.
    pub fn cert_and_key_blocking(&self, id: PartyID) -> Result<(CertificateDer<'static>, PrivateKeyDer<'static>), std::io::Error>
    {
        let cert = fs::read(self.cert_filename(id))?;
        let key = fs::read(self.cert_key_filename(id))?;
        let cert = CertificateDer::from(cert);
        let key = PrivateKeyDer::Pkcs8(key.into());
        Ok((cert, key))
    }

    /// Load the verification key of party `id`.
    ///
    /// # Errors
    /// If the file could not be read.
    #[cfg(feature = "signing")]
    pub async fn verification_key(&self, id: PartyID) -> Result<PublicKey, tokio::io::Error>
    {
        let key = tokio::fs::read(self.verification_key_filename(id)).await?;
        Ok(PublicKey::new(key))
    }

    /// Load all verification keys.
    ///
    /// # Errors
    /// See [`Self::verification_key`].
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

    /// Load the signing key of party `id`.
    ///
    /// # Errors
    /// If the file could not be read or the key is rejected.
    #[cfg(feature = "signing")]
    pub async fn signing_key(&self, id: PartyID) -> Result<PrivateKey, tokio::io::Error>
    {
        let key = tokio::fs::read(self.signing_key_filename(id)).await?;
        PrivateKey::new(&key).map_err(tokio::io::Error::other)
    }
}


/// Returns the config path from the environment (if available).
#[must_use]
pub fn path_from_env() -> Option<PathBuf>
{
    env::var(CONFIG_PATH_ENVIRONMENT_VARIABLE)
        .map_or(None, |config| Some(PathBuf::from(config)))
}

/// Returns the fallback config path ([`DEFAULT_PATH`]).
#[must_use]
pub fn fallback_path() -> PathBuf
{
    PathBuf::from(DEFAULT_PATH)
}

/// Returns the config path from the environment (if available) or the fallback path.
#[must_use]
pub fn default_path() -> PathBuf
{
    path_from_env().unwrap_or_else(fallback_path)
}
