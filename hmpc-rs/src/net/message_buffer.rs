use std::collections::HashMap;

use log::{debug, error, warn};

use super::{DataCommand, OwnedData, PartyID};

/// Create loop for handling requested and received messages, instructed by [`DataCommand`]s.
pub(crate) async fn run(id: PartyID, mut receive_channel: tokio::sync::mpsc::Receiver<DataCommand>)
{
    let mut message_buffer = HashMap::<_, OwnedData>::new();
    let mut open_requests = HashMap::new();

    while let Some(command) = receive_channel.recv().await
    {
        match command
        {
            DataCommand::Receive(message, answer_channel) =>
            {
                if let Some(data) = message_buffer.remove(&message)
                {
                    debug!("[Party {}] Requested message {:?}. [{} bytes] from message buffer", id, message, data.len());
                    if let Err(data) = answer_channel.send(data)
                    {
                        warn!("[Party {}] Waiting thread for {:?} [{} bytes] no longer available", id, message, data.len());
                    }
                }
                else
                {
                    debug!("[Party {}] Requested message {:?}. Waiting", id, message);
                    if open_requests.insert(message.clone(), answer_channel).is_some()
                    {
                        error!("[Party {}] Somebody was already waiting for message {:?}", id, message);
                    }
                }
            },
            DataCommand::Received(message) =>
            {
                if let Some(request_channel) = open_requests.remove(&message.metadata)
                {
                    debug!("[Party {}] Received message {:?} [{} bytes]. Send to waiting thread", id, message.metadata, message.data.len());
                    if let Err(data) = request_channel.send(message.data)
                    {
                        warn!("[Party {}] Waiting thread for {:?} [{} bytes] no longer available", id, message.metadata, data.len());
                    }
                }
                else
                {
                    debug!("[Party {}] Received message {:?} [{} bytes]. Store in message buffer", id, message.metadata, message.data.len());
                    let metadata = message.metadata.clone();
                    if let Some(data) = message_buffer.insert(message.metadata, message.data)
                    {
                        error!("[Party {}] There was already some data for message {:?} [{} bytes]", id, metadata, data.len());
                    }
                }
            },
        }
    }
}
