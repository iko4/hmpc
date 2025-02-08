# Networking Documentation

## Config

The parties involved in the communication are managed with a configuration file, by default "mpc.yaml".
The keys in the "parties" dictionary are used as party identifier and have to be representable by 16 bit unsigned integers.
The values for these dictionary keys correspond to the network address (host name or IP address; with or without port), while also a default port can be specified.

The certificates and private key of the current party are by default located in a ".mpc" directory next to "mpc.yaml".
This can be overwritten by setting the following config keys:

- "cert_dir":
    Directory of certificates.
- "cert_keys_dir":
    Directory of certificate private keys.
- "sign_verify_dir":
    Enabled with the ["signing" feature](features.md#signing).
    Directory of signature verification keys.
- "sign_keys_dir":
    Enabled with the ["signing" feature](features.md#signing).
    Directory of private signing keys.

### Environment Variables

The following environment variables influence the behavior of an application using hmpc.

- `HMPC_CONFIG`:
    Determines the path to the config file.
    This includes the full filename.
    For example: "config/hmpc.json".
- `HMPC_SESSION_VALUE`:
    Enabled with the ["sessions" feature](features.md#sessions).
    Determines the session ID (parsed as unsigned integer).
- `HMPC_SESSION_STRING`:
    Enabled with the ["sessions" feature](features.md#sessions).
    Determines the session ID (arbitrary string that is hashed to get the session ID).

Note: `HMPC_SESSION_VALUE` takes precedence over `HMPC_SESSION_STRING`.

### Example Config

The following shows an example config for three parties.
Note that the current party identifier is not part of the config, so the same config file can be used for all parties.

```yaml
parties:
  0: example.com:8000
  1: mpc.local
  2: 127.0.0.0
port: 5000 # default port if not given per party
session:
    string: "example computation"
```

### Example Directory Structure

The following shows an example directory structure for an application using hmpc.
For the above three party [example config](#example-config), the directory structure for *party 1* could look like the following.
Note that certificates and signature verification keys (if the ["signing" feature](features.md#signing) is enable) are required for all parties.
The private certificate key and private signing key are required only for the current party (party 1).
At the other parties (party 0 and party 2), the structure looks similar but different private keys are expected.

```
<current working directory>
├── <your application>
├── mpc.yaml                         # config
└── .mpc                             # communication files
   ├── cert                          # party certificates
   │  ├── 0.x509.cert.der
   │  ├── 1.x509.cert.der
   │  └── 2.x509.cert.der
   ├── cert-keys                     # certificate private key
   │  └── 1.cert-private.key.der
   ├── sign-keys                     # (optional) signing key
   │  └── 1.ed25519-private.key.der
   └── sign-verify                   # (optional) signature verification keys
      ├── 0.ed25519-public.key.bin
      ├── 1.ed25519-public.key.bin
      └── 2.ed25519-public.key.bin
```


## Communicator

Similar to [MPI](https://www.mpi-forum.org/), several [communication operations](#communication-operations) are defined for so-called communicators.
In hmpc, a communicator is simply a collection of party IDs and used to determine which parties are involved in a communication operations.
For example, for a [broadcast](#broadcast), the communicator could be parties {0, 1, 2} and the sending party could be party 0.


## Communication Operations

The following communication operations similar to the ones in [MPI](https://www.mpi-forum.org/) are supported:

- [send](#send)
- (multi) [broadcast](#broadcast)
- [scatter](#scatter)
- (multi) [gather](#gather)
- (extended) (multi) [all gather](#all-gather)
- (multi) [all to all](#all-to-all)

Here, the "multi" variants simply support multiple concurrent operations of the same type, for example, two independent broadcasts with two (potentially different) senders.
The "extended" variants use a second [communicator](#communicator) to indicate the receiving parties, while the first communicator argument is used as in the normal version.
This can be used, for example, in a client-server setting to send information from all servers to all servers and also to all clients.

For the below examples, we give the (pseudo C++) code that could be executed at each party.
Assume that the following code is run before each example at each party.
```cpp
auto party0 = hmpc::party_constant_of<0>;
auto party1 = hmpc::party_constant_of<1>;
auto party2 = hmpc::party_constant_of<2>;
auto parties = hmpc::net::communicator{party0, party1, party2};

using T = hmpc::core::uint32 // or any other supported type
auto shape = hmpc::shape{2, 2, 2}; // or any other valid shape

auto net = hmpc::net::queue(...); // initialized with current party ID and config file
```

### Send

One party sends data to one party.

#### Example

```cpp
// TODO: Currently not implemented
```

### Broadcast

One party sends data to all parties.
All parties receive the same data.

#### Example

```cpp
// party 0
auto x = make_tensor<T>(shape, ...);
net.broadcast(parties, party0, x);
// party 1
auto x = net.broadcast<T>(parties, party0, shape);
// party 2
auto x = net.broadcast<T>(parties, party0, shape);
```

### Scatter

One party sends data to all parties.
Each party receives different data.

#### Example

```cpp
// TODO: Currently not implemented
```

### Gather

All parties send data to one party.

Note: The receiving party also "sends" data to itself (the data is only moved to the correct position of the result, no data is communicated for the own data).

#### Example

```cpp
// party 0
auto x0 = make_tensor<T>(shape, ...);
auto [x0, x1, x2] = net.gather(parties, party0, std::move(x0));
// party 1
auto x1 = make_tensor<T>(shape, ...);
net.gather(parties, party0, x1)
// party 2
auto x2 = make_tensor<T>(shape, ...);
net.gather(parties, party0, x2)
```

### All Gather

All parties send data to all parties.
All parties receive the same data.

#### Example

```cpp
// party 0
auto x0 = make_tensor<T>(shape, ...);
auto [x0, x1, x2] = net.all_gather(parties, std::move(x0));
// party 1
auto x1 = make_tensor<T>(shape, ...);
auto [x0, x1, x2] = net.all_gather(parties, std::move(x1));
// party 2
auto x2 = make_tensor<T>(shape, ...);
auto [x0, x1, x2] = net.all_gather(parties, std::move(x2));
```

### All To All

All parties send data to all parties.
Each party receives different data.

#### Example

```cpp
// party 0
auto [x0, y0, z0] = make_tensors<T>(shape, ...);
auto [x0, x1, x2] = net.all_to_all(parties, std::move(x0), std::move(y0), std::move(z0));
// party 1
auto [x1, y1, z1] = make_tensors<T>(shape, ...);
auto [y0, y1, y2] = net.all_to_all(parties, std::move(x1), std::move(y1), std::move(z1));
// party 2
auto [x2, y2, z2] = make_tensors<T>(shape, ...);
auto [z0, z1, z2] = net.all_to_all(parties, std::move(x2), std::move(y2), std::move(z2));
```



## Message Format

For all types of communication, a message has the same format.
First, the necessary metadata is encoded, then the message payload.
The following components make up the message (in order).
Note that we give the bit length of each metadata member (interpreted as little-endian unsigned integer), as well as a one character symbol used in the following illustration.

- [message format version](#message-format-version) (8 bit, `v`):
    The current version of the format is "0".
- [feature flags](#feature-flags) (8 bit, `f`):
    Specifies which networking features are enabled.
    These add additional metadata members to a message.
- [message kind](#message-kind) (8 bit, `k`):
    Specifies which kind of communication operation is performed.
- [datatype tag](#datatype-tag) (8 bit, `t`):
    Specifies which underlying datatype is used.
- [sender ID](#sender-and-receiver-id) (16 bit, `s`):
    Identifier of the sending party.
- [receiver ID](#sender-and-receiver-id) (16 bit, `r`):
    Identifier of the receiving party.
- [message ID](#message-id) (64 bit, `m`):
    Identifier of the current message.
- optional, enabled with ["sessions" feature](features.md#sessions):
    [session ID](#optional-session-id) (128 bit, `i`).
- [data payload](#data-payload) (remaining bytes, `d`)
- optional, enabled with ["signing" feature](features.md#signing):
    [message signature](#optional-signature) (512 bit, `z`).


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

    sessions feature enabled
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

    singing and sessions feature enabled
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

[Features](features.md) that change the message format are indicated by bits of this metadata member.
Currently, ["sessions"](features.md#sessions) and ["signing"](features.md#signing) are features that change the networking behavior.
The bits of this member indicate if the feature is enabled (`1`) or not (`0`).
The feature bits are the following (in order);

1. ["sessions" feature](features.md#sessions)
2. ["signing" feature](features.md#signing)
3. reserved for future use
4. reserved for future use
5. reserved for future use
6. reserved for future use
7. reserved for future use
8. reserved for future use

### Message Kind

The above communication operations are encoded as follows:

- [Send](#send) = 1
- [Broadcast](#broadcast) = 2
- [Scatter](#scatter) = 3
- [Gather](#gather) = 4
- [All Gather](#all-gather) = 5
- [All To All](#all-to-all) = 6

Additionally, if the ["collective-consistency" feature](features.md#collective-consistency) is enabled, the extra messages for checking consistency use these tags:

- Check consistency of [Broadcast](#broadcast) = 18
- Check consistency of [All Gather](#all-gather) = 21

Note: The message kind value for consistency check message is the original message kind value OR `0x10`.

### Datatype Tag

The underlying datatype of the message is encoded with a tag as follows:

1. The bit width of the type is encoded as 8 bit unsigned integer.
2. The endianness is encoded as `0x01` for little-endian types and `0x00` otherwise.
3. The result is the bitwise OR of the above values.

#### Examples

- uint1: `0x01 | 0x01 = 0x01`
- uint8: `0x08 | 0x01 = 0x09`
- uint16:
    - for little-endian: `0x10 | 0x01 = 0x11`
    - for big-endian: `0x10 | 0x00 = 0x10`

### Sender and Receiver ID

Unique (but not necessarily sequential) integers are used to identify the parties.
These integers are used as identifiers in messages and map to the same identifiers used in the config.

### Message ID

The [communicator](#communicator) of the involved parties is hashed to get an initial message ID.
This is to avoid a collision for messages with the same metadata (sender, receiver, etc.) if the overall communication pattern is different;
for example, a broadcast from party 0 to parties {1, 2} and a broadcast from party 0 to parties {1, 2, 3, 4} at the same time.
The initial message IDs are incremented for every communication operation, *not* for every message;
for example, not twice for the first broadcast example in the last sentence.

### (Optional) Session ID

Enabled with the ["sessions" feature](features.md#sessions).
Session identifiers avoid the reuse of messages across protocol runs.
As an alternative, new keys and certificates could be used for every protocol run.

### Data payload

The data payload is stored byte-by-byte as the last part (or second-to-last part, if signing is enabled) of the message.
The length of the payload or overall message is not stored; the end of the stream is used to signal the end of the message.

### (Optional) Signature

Enabled with the ["signing" feature](features.md#sessions).
This makes messages identifiable.
In other words, the receiver can show the message to somebody else, in case the sender misbehaved and the message can be used as proof of this.

All other (meta)data (except the signature itself) is signed to create the signature.
When verifying the signature, the sender ID is used to determine which verification key to use.

Note: The data is first hashed and then signed together with all metadata.
