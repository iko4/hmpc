# Networking Documentation

## Config

The parties involved in the communication are managed with a configuration file, by default "mpc.yaml".
The keys in the "parties" dictionary are used as party identifier and have to be representable by 16 bit unsigned integers.
The values for these dictionary keys correspond to the network address (host name or IP address; with or without port), while also a default port can be specified.

The certificates and private key of the current party are by default located in a ".mpc" directory next to "mpc.yaml".

### Environment Variables

- `HMPC_CONFIG`: Determines the path to the config file.
- `HMPC_SESSION_VALUE`: Determines the session ID (parsed as unsigned integer).
- `HMPC_SESSION_STRING`: Determines the session ID (arbitrary string that is hashed to get the session ID).

Note: `HMPC_SESSION_VALUE` takes precedence over `HMPC_SESSION_STRING`.

### Example Config

```yaml
parties:
  0: example.com:8000
  1: mpc.local
  2: 127.0.0.0
port: 5000
```

## Communication Operations

Currently, communication operations similar to the ones in [MPI](https://www.mpi-forum.org/) are supported.

### Send

[TODO]

### Broadcast

[TODO]

### Scatter

[TODO]

### Gather

[TODO]

### All Gather

[TODO]

### All To All

[TODO]


## Message Format

For all types of communication, a message has the same format.
First, the necessary metadata is encoded, then the message payload.
The following components make up the message (in order).
Note that we give the bit length of each metadata member (interpreted as little-endian unsigned integer), as well as a one character symbol used in the following illustration.

- message format version (8 bit, `v`):
    The current version of the format is "0".

- feature flags (8 bit, `f`):
    Specifies which networking features are enabled.
    These add additional metadata members to a message.

- message kind (8 bit, `k`):
    Specifies which kind of communication operation is performed.

- datatype tag (8 bit, `t`):
    Specifies which underlying datatype is used.

- sender ID (16 bit, `s`):
    Identifier of the sending party.

- receiver ID (16 bit, `r`):
    Identifier of the receiving party.

- message ID (64 bit, `m`):
    Identifier of the current message.

- optional, enabled with "sessions" feature:
    session ID (128 bit, `i`).

- data payload (remaining bytes, `d`)

- optional, enabled with "signing" feature:
    message signature (512 bit, `z`).


### Visualizations

The following visualizations show the format for all supported modes.
Each character symbol stands for 8 bit of (meta)data.
On the left, we show the current offset in bits, counted from the begin of the message.

```
    no features enabled
    +-----------------+
  0 | v f k t s s r r |
 64 | m m m m m m m m |
128 | d d d d d d d d |
... |       ...       |
    | d d d d d d d d |
    +-----------------+

    session feature enabled
    +-----------------+
  0 | v f k t s s r r |
 64 | m m m m m m m m |
128 | i i i i i i i i |
192 | i i i i i i i i |
256 | d d d d d d d d |
... |       ...       |
    | d d d d d d d d |
    +-----------------+

    signing feature enabled
    +-----------------+
  0 | v f k t s s r r |
 64 | m m m m m m m m |
128 | d d d d d d d d |
... |       ...       |
    | d d d d d d d d |
    | z z z z z z z z |
    | z z z z z z z z |
    | z z z z z z z z |
    | z z z z z z z z |
    | z z z z z z z z |
    | z z z z z z z z |
    | z z z z z z z z |
    | z z z z z z z z |
    +-----------------+

    singing and session feature enabled
    +-----------------+
  0 | v f k t s s r r |
 64 | m m m m m m m m |
128 | i i i i i i i i |
192 | i i i i i i i i |
256 | d d d d d d d d |
... |       ...       |
    | d d d d d d d d |
    | z z z z z z z z |
    | z z z z z z z z |
    | z z z z z z z z |
    | z z z z z z z z |
    | z z z z z z z z |
    | z z z z z z z z |
    | z z z z z z z z |
    | z z z z z z z z |
    +-----------------+
```

### Message Format Version

The current format is version "0".
After version 0, the version number will be incremented once the format changes.
> ⚠️ For version 0, the format could change even without incrementing the version number!

### Feature Flags

Features that change the message format are indicated by bits of this metadata member.
Currently, "sessions" and "signing" are supported features.
The bits of this member indicate if the feature is enabled (1) or not (0).
The feature bits are the following (in order):

- "sessions" feature
- "signing" feature
- reserved for future use
- reserved for future use
- reserved for future use
- reserved for future use
- reserved for future use
- reserved for future use

### Message Kind

The above communication operations are encoded as follows:

- Send = 1

- Broadcast = 2

- Scatter = 3

- Gather = 4

- All Gather = 5

- All To All = 6

### Datatype Tag

The underlying datatype of the message is encoded with a tag as follows:

1. The bit width of the type is encoded as 8 bit unsigned integer.

2. The endianness is encoded as 0x1 for little-endian types and 0x0 otherwise.

3. The result is the bitwise OR of the above values.

#### Examples

- uint1: `0x1 | 0x1 = 0x1`
- uint8: `0x8 | 0x1 = 0x9`
- uint16:
    - for little-endian: `0x10 | 0x1 = 0x11`
    - for big-endian: `0x10 | 0x0 = 0x10`

### Sender and Receiver ID

Unique (but not necessarily sequential) integers are used to identify the parties.
These integers are used as identifiers in messages and map to the same identifiers used in the config.

### Message ID

The communicator of the involved parties is hashed to get an initial message ID.
This is to avoid a collision for messages with the same metadata (sender, receiver, etc.) if the overall communication pattern is different, e.g., a broadcast from party 0 to parties {1, 2} and a broadcast from party 0 to parties {1, 2, 3, 4} at the same time.
The initial message IDs are incremented for every communication operation, *not* for every message, e.g., not twice for the first broadcast example in the last sentence.

### (Optional) Session ID

Enabled with the "sessions" feature.
Session identifiers avoid the reuse of messages across messages.
As an alternative, new keys and certificates could be used for every protocol run.

### Data payload

The data payload is stored byte-by-byte as the last part (or second-to-last part, if signing is enabled) of the message.
The length of the payload or overall message is not stored; the end of the stream is used to signal the end of the message.

### (Optional) Signature

Enabled with the "signing" feature.
This makes messages identifiable.
In other words, the receiver can show the message to somebody else, in case the sender misbehaved and the message can be used as proof of this.

All other (meta)data (except the signature itself) is signed to create the signature.
When verifying the signature, the sender ID is used to determine which verification key to use.
