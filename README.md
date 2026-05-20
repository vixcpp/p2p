# Vix.cpp P2P Module

Peer-to-peer engine for Vix.cpp.

The P2P module provides the networking and replication foundation used to build offline-first, local-first, and distributed C++ systems with peer discovery, secure handshakes, message framing, transport abstraction, routing, WAL replication, and safe sync.

## Documentation

Full documentation will be available here:

https://docs.vixcpp.com/modules/p2p/

API reference:

https://docs.vixcpp.com/modules/p2p/api-reference

## What P2P provides

- Peer discovery
- LAN discovery
- Registry-based bootstrap
- Secure handshake
- Message framing
- Protocol envelopes
- Message dispatch
- Transport abstraction
- Peer routing
- WAL replication
- Outbox synchronization
- Offline-first sync
- Failure-tolerant replication model

## Public headers

```cpp
#include <vix/p2p.hpp>
```

Or include specific headers when needed:

```cpp
#include <vix/p2p/discovery.hpp>
#include <vix/p2p/framing.hpp>
#include <vix/p2p/envelope.hpp>
#include <vix/p2p/dispatch.hpp>
#include <vix/p2p/router.hpp>
#include <vix/p2p/sync.hpp>
```

## Basic idea

```text
local write
  -> WAL
  -> envelope
  -> framing
  -> transport
  -> peer
  -> apply
  -> ack
  -> retry until convergence
```

The network is not required for local correctness. Nodes can continue working offline and synchronize when connectivity returns.

## Protocol architecture

```text
Discovery
  -> finds peers

Transport
  -> sends bytes

Framing
  -> splits byte streams into messages

Envelope
  -> wraps protocol messages

Dispatch
  -> routes message types

EdgeSync
  -> WAL replication and outbox sync

Router
  -> optional peer routing
```

## Message flow

```text
encode(message)
  -> envelope
  -> framing
  -> transport
  -> network
  -> decode
  -> dispatch
  -> handler
```

## Handshake model

```text
A -> Hello
B -> HelloAck
A -> HelloFinish
```

After the handshake, the peer session can become connected and start exchanging protocol messages.

## Offline sync model

```text
local operation
  -> append to WAL
  -> push WAL batch
  -> peer applies operation
  -> peer sends acknowledgement
  -> retry pending operations until convergence
```

## Examples

### Envelope and framing

```bash
vix run examples/p2p/01_envelope_and_framing_basic.cpp
```

### Handshake messages

```bash
vix run examples/p2p/02_hello_handshake_messages.cpp
```

### Discovery announce JSON

```bash
vix run examples/p2p/03_discovery_announce_json.cpp
```

### Router

```bash
vix run examples/p2p/04_router_memory_basic.cpp
```

### Secure envelope

```bash
vix run examples/p2p/05_pack_secure_envelope.cpp
```

### Dispatch

```bash
vix run examples/p2p/06_dispatch_decode_basic.cpp
```

### WAL sync

```bash
vix run examples/p2p/07_wal_push_and_ack.cpp
```

## Runtime examples

### Manual TCP connection

Terminal 1:

```bash
vix run examples/p2p/08_runtime_manual_connect.cpp --run server
```

Terminal 2:

```bash
vix run examples/p2p/08_runtime_manual_connect.cpp --run client 127.0.0.1 9101
```

### UDP discovery

Terminal 1:

```bash
vix run examples/p2p/09_udp_discovery_basic.cpp --run node-a 9201
```

Terminal 2:

```bash
vix run examples/p2p/09_udp_discovery_basic.cpp --run node-b 9202
```

### HTTP bootstrap

```bash
vix run examples/p2p/10_bootstrap_http_basic.cpp --run http://127.0.0.1:8080/peers
```

## CLI usage

You can also run a P2P node directly from the Vix CLI:

```bash
vix p2p --id A --listen 9001
```

Connect another node:

```bash
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
```

## Runtime arguments

When using `vix run`, keep this rule:

```text
--     = compiler or linker flags
--run  = runtime arguments passed to the program
```

Example:

```bash
vix run examples/p2p/08_runtime_manual_connect.cpp --run client 127.0.0.1 9101
```

## Build

Contributors should use the Vix CLI to build this module.

Vix wraps the C++ build workflow with project detection, presets, Ninja builds, clean logs, caching, and focused diagnostics. This keeps the contributor workflow consistent and helps avoid hidden C++ build issues.

### Build the project

```bash
git clone https://github.com/vixcpp/vix.git
cd vix
vix build
```

### Build all targets

Use this before running the full test suite, install workflows, or release checks:

```bash
vix build --build-target all
```

### Clean rebuild

Use this when the local CMake cache or build directory may be stale:

```bash
vix build --clean
```

### Release build

```bash
vix build --preset release
```

## Tests

Build all targets first, then run tests:

```bash
vix build --build-target all
vix tests
```

Before opening a pull request, use:

```bash
vix fmt --check
vix build --build-target all
vix tests
```

## Useful links

- P2P documentation: https://docs.vixcpp.com/modules/p2p/
- P2P API reference: https://docs.vixcpp.com/modules/p2p/api-reference
- P2P CLI command: https://docs.vixcpp.com/cli/p2p
- Build command: https://docs.vixcpp.com/cli/build
- Tests command: https://docs.vixcpp.com/cli/tests
- Documentation: https://docs.vixcpp.com/
- Engineering notes: https://blog.vixcpp.com/
- Registry: https://registry.vixcpp.com/
- GitHub: https://github.com/vixcpp/vix

## License

MIT License.

See [`LICENSE`](../../LICENSE) for details.
