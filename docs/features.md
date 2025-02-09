# Optional Features

The following features are available

- [sessions](#sessions)
- [signing](#signing)
- [collective consistency](#collective-consistency)
- [statistics](#statistics)

There are corresponding features with the same names available in hmpc-rs.
For CMake, there is a corresponding option `HMPC_ENABLE_<FEATURE>` that can be set, for example, `HMPC_ENABLE_SESSIONS=ON`.
All features are enabled by default in CMake.


## Sessions

When enabled, each message contains a [session ID](networking.md#optional-session-id).
Session identifiers avoid the reuse of messages across protocol runs.
The session ID is inferred from [environment variables](networking.md#environment-variables) (`HMPC_SESSION_VALUE` or `HMPC_SESSION_STRING`); from the [config](networking.md#config) key "session"; or can be specified directly.


## Signing

When enabled, each message contains a [signature](networking.md#optional-signature) to make messages identifiable.
In other words, the receiver can show the message to somebody else, in case the sender misbehaved and the message can be used as proof of this.


## Collective Consistency

When enabled, all collective communication operations that require all parties to receive the same data are checked.
This is done by sending extra messages to all other receivers and checking consistency by comparing if the same messages arrived.

The [signing](#signing) feature needs to be enabled for this feature to work.

The extra messages have the following differences to normal messages in [message format](networking.md#message-format):

- The [message kind](networking.md#message-kind) indicates that the message is for a consistency check (and the kind of the original message).
- The [sender and receiver IDs](networking.md#sender-and-receiver-id) are the ones of the original received message, *not* of the consistency check message.
- The [payload](networking.md#data-payload) consists of a hash of the original data payload and the [signature](networking.md#optional-signature) of the original message.

Note the consistency check message is also signed by its sender.
Therefore, the signature of the original message is followed by another [signature](networking.md#optional-signature), signed by the sender of the consistency check.

With this information, a receiver of a consistency check message can compare the data they received by comparing the hashes.
Additionally, they know that the original message contained data with the same hash, as the corresponding signature is also included.


## Statistics

When enabled, statistics about the behavior of the application are recorded.
Currently, this includes

- the overall number of data payload bytes send
- the overall number of data payload bytes received
- the overall number of calls to communication operations, i.e., the number of logical communication rounds
