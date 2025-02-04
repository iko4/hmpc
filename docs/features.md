# Optional Features

The following features are available
- [sessions](#sessions)
- [signing](#signing)
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


## Statistics

When enabled, statistics about the behavior of the application are recorded.
Currently, this includes
- the overall number of data payload bytes send
- the overall number of data payload bytes received
- the overall number of calls to communication operations, i.e., the number of logical communication rounds
