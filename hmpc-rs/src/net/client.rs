use std::collections::hash_map::Entry;
use std::sync::Arc;

use log::{debug, error, info};
use quinn::crypto::rustls::QuicClientConfig;
use quinn::{Connection, TransportConfig};

#[cfg(feature = "sessions")]
use super::SessionID;
use super::config::DEFAULT_TIMEOUT;
use super::errors::ClientError;
#[cfg(feature = "signing")]
use super::sign::PrivateKey;
use super::{Config, NetCommand, PartyID, SendMessage, make_addr};

/// Make client endpoint.
async fn make_client(config: &Config) -> quinn::Endpoint
{
    // TODO: Use other wildcard address to bind to
    // TODO: Could make IPv4/IPv6 configurable
    let mut endpoint = quinn::Endpoint::client((std::net::Ipv6Addr::UNSPECIFIED, 0).into()).expect("Could not create client endpoint");
    endpoint.set_default_client_config(make_client_config(config).await);
    endpoint
}

/// Make client config (for [`make_client`]).
async fn make_client_config(config: &Config) -> quinn::ClientConfig
{
    let crypto = rustls::ClientConfig::builder()
        .with_root_certificates(config.certs().await.expect("Could not get certificates"))
        .with_no_client_auth();

    let mut transport_config = TransportConfig::default();
    transport_config.max_idle_timeout(Some(DEFAULT_TIMEOUT.try_into().expect("Invalid idle timeout")));

    let quic_config = QuicClientConfig::try_from(crypto).expect("Could not create QUIC config");

    let mut client_config = quinn::ClientConfig::new(Arc::new(quic_config));
    client_config.transport_config(Arc::new(transport_config));
    client_config
}

/// Open or reuse connection.
async fn make_connection(id: PartyID, receiver: PartyID, config: &Config, endpoint: &quinn::Endpoint, outgoing: &mut std::collections::HashMap<PartyID, quinn::Connection>) -> Result<quinn::Connection, ClientError>
{
    match outgoing.entry(receiver)
    {
        Entry::Occupied(entry) =>
        {
            let connection = entry.get();
            info!("[Party {}] Outgoing connection: to {} reused", id, connection.remote_address());
            Ok(connection.clone())
        },
        Entry::Vacant(entry) =>
        {
            info!("[Party {}] Outgoing connection: trying to connect new client", id);
            let connection = connect_client(endpoint, receiver, config).await?;
            info!("[Party {}] Outgoing connection: to {} established", id, connection.remote_address());
            Ok(entry.insert(connection).clone())
        },
    }
}

/// Open new connection.
async fn connect_client(client: &quinn::Endpoint, id: PartyID, config: &Config) -> Result<quinn::Connection, ClientError>
{
    client.connect(make_addr(id, config), config.name(id))?.await.map_err(Into::into)
}

/// Try sending message over network.
async fn handle_connection(id: PartyID, #[cfg(feature = "sessions")] session: SessionID, #[cfg(feature = "signing")] signing_key: Arc<PrivateKey>, connection: Connection, message: SendMessage, answer_channel: tokio::sync::oneshot::Sender<Result<(), ClientError>>)
{
    debug!("[Party {}] Outgoing stream: to {}", id, connection.remote_address());
    let mut stream = match connection.open_uni().await
    {
        Ok(stream) => stream,
        Err(e) =>
        {
            answer_channel.send(Err(ClientError::Connection(e))).expect("No longer waiting for result");
            return;
        },
    };
    info!("[Party {}] Outgoing stream: to {} established", id, connection.remote_address());
    if let Err(e) = message.write_to(
        #[cfg(feature = "sessions")]
        session,
        #[cfg(feature = "signing")]
        &signing_key,
        &mut stream,
    )
    .await
    {
        answer_channel.send(Err(ClientError::Write(e))).expect("No longer waiting for result");
        return;
    };
    debug!("[Party {}] Finished writing stream", id);
    if let Err(e) = stream.finish()
    {
        answer_channel.send(Err(ClientError::Write(e.into()))).expect("No longer waiting for result");
        return;
    };
    answer_channel.send(Ok(())).expect("No longer waiting for result");
}

/// Create loop for sending messages over the network, instructed by [`NetCommand`]s.
pub(crate) async fn run(id: PartyID, config: Config, mut receive_channel: tokio::sync::mpsc::Receiver<NetCommand>)
{
    let mut outgoing = std::collections::HashMap::<PartyID, quinn::Connection>::new();
    let endpoint = make_client(&config).await;
    #[cfg(feature = "sessions")]
    let session = config.session.clone();
    #[cfg(feature = "signing")]
    let signing_key = Arc::new(config.signing_key(id).await.expect("Could not get signing key"));

    while let Some(command) = receive_channel.recv().await
    {
        match command
        {
            NetCommand::Send(message, answer_channel) =>
            {
                match make_connection(id, message.metadata.receiver, &config, &endpoint, &mut outgoing).await
                {
                    Err(e) =>
                    {
                        if let Err(_) = answer_channel.send(Err(e))
                        {
                            error!("[Party {}] No longer waiting to send", id);
                        }
                    },
                    Ok(connection) =>
                    {
                        tokio::spawn(
                            handle_connection(
                                id,
                                #[cfg(feature = "sessions")]
                                session,
                                #[cfg(feature = "signing")]
                                signing_key.clone(),
                                connection,
                                message,
                                answer_channel
                            )
                        );
                    },
                }
            },
            #[cfg(feature = "collective-consistency")]
            NetCommand::SendCheck(message, receiver, answer_channel) =>
            {
                match make_connection(id, receiver, &config, &endpoint, &mut outgoing).await
                {
                    Err(e) =>
                    {
                        if let Err(_) = answer_channel.send(Err(e))
                        {
                            error!("[Party {}] No longer waiting to send consistency check", id);
                        }
                    },
                    Ok(connection) =>
                    {
                        tokio::spawn(
                            handle_connection(
                                id,
                                #[cfg(feature = "sessions")]
                                session,
                                #[cfg(feature = "signing")]
                                signing_key.clone(),
                                connection,
                                message,
                                answer_channel
                            )
                        );
                    },
                }
            },
        }
    }
}
