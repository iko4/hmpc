#[cfg(feature = "signing")]
use std::collections::HashMap;
use std::sync::Arc;

use log::{error, info};
use quinn::Incoming;

use super::config::{Config, DEFAULT_TIMEOUT};
#[cfg(feature = "signing")]
use super::sign::PublicKey;
#[cfg(feature = "sessions")]
use super::SessionID;
use super::{make_addr, DataChannel, DataCommand, PartyID, ReceiveMessage};
#[cfg(feature = "collective-consistency")]
use super::{ConsistencyCheckChannel, ConsistencyCheckCommand};

async fn make_server(id: PartyID, config: &Config) -> quinn::Endpoint
{
    quinn::Endpoint::server(make_server_config(id, config).await, make_addr(id, config)).unwrap()
}

async fn make_server_config(id: PartyID, config: &Config) -> quinn::ServerConfig
{
    let (cert, key) = config.cert_and_key(id).await.unwrap();
    let cert_chain = vec![cert];

    let mut server_config = quinn::ServerConfig::with_single_cert(cert_chain, key).unwrap();
    let transport_config = Arc::get_mut(&mut server_config.transport).unwrap();
    transport_config.max_idle_timeout(Some(DEFAULT_TIMEOUT.try_into().unwrap()));
    server_config
}

async fn handle_connection(id: PartyID, #[cfg(feature = "sessions")] session: SessionID, #[cfg(feature = "signing")] verification_keys: Arc<HashMap<PartyID, PublicKey>>, data_channel: DataChannel, #[cfg(feature = "collective-consistency")] consistency_channel: ConsistencyCheckChannel, connecting: Incoming)
{
    let connection = connecting.await.unwrap();
    info!("[Party {}] Incoming connection: from {} established", id, connection.remote_address());
    while let Ok(mut stream) = connection.accept_uni().await
    {
        info!("[Party {}] Incoming stream: from {} established", id, connection.remote_address());
        match ReceiveMessage::read_from(
            #[cfg(feature = "sessions")]
            session,
            #[cfg(feature = "signing")]
            &verification_keys,
            &mut stream,
        )
        .await
        {
            Ok(message) =>
            {
                #[cfg(feature = "collective-consistency")]
                let (message, signature) = message;
                if message.metadata.kind.is_consistency_check()
                {
                    #[cfg(feature = "collective-consistency")]
                    consistency_channel.send(
                        // PANICS: We check above that MessageKind is for consistency check
                        ConsistencyCheckCommand::ReceivedCheck(message.try_into().unwrap())
                    ).await.unwrap();
                }
                else
                {
                    #[cfg(feature = "collective-consistency")]
                    if let Ok(consistency) = (&message, signature).try_into()
                    {
                        consistency_channel.send(ConsistencyCheckCommand::ReceivedData(consistency)).await.unwrap();
                    }
                    data_channel.send(DataCommand::Received(message)).await.unwrap();
                }
            },
            Err(e) =>
            {
                error!("[Party {}] Failed to read stream from {}. Reason: {:?}", id, connection.remote_address(), e);
            },
        }
    }
}

pub(crate) async fn run(id: PartyID, config: Config, data_channel: DataChannel, #[cfg(feature = "collective-consistency")] consistency_channel: ConsistencyCheckChannel)
{
    let endpoint = make_server(id, &config).await;
    // PANICS: Calling code checks that config.session is not None and contains a value
    #[cfg(feature = "sessions")]
    let session = config.session.clone().unwrap().unwrap();
    #[cfg(feature = "signing")]
    let verification_keys = Arc::new(config.verification_keys().await.unwrap());
    while let Some(connecting) = endpoint.accept().await
    {
        info!("[Party {}] Incoming connection: from {}", id, connecting.remote_address());
        tokio::spawn(
            handle_connection(
                id,
                #[cfg(feature = "sessions")]
                session,
                #[cfg(feature = "signing")]
                verification_keys.clone(),
                data_channel.clone(),
                #[cfg(feature = "collective-consistency")]
                consistency_channel.clone(),
                connecting
            )
        );
    }
}
