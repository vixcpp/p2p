# vix/p2p

Peer-to-peer engine for Vix.cpp

**Durable. Offline-first. Deterministic.**

---

## Overview

vix/p2p is the networking layer of Vix.cpp.

It provides everything needed to build a real-world distributed system:

- peer discovery (LAN + registry)
- secure handshake
- message framing
- transport abstraction
- WAL replication
- offline-first sync

Designed for unreliable networks. Built for the real world.

---

## Core Philosophy

- Local-first → nodes work without network
- Eventually consistent → sync happens later
- Deterministic → same inputs → same state
- Transport-agnostic → TCP, QUIC, future-ready
- Failure-tolerant → restart-safe, replay-safe

---

## Architecture

```text
Discovery  → finds peers (UDP / registry)
Transport  → sends frames (TCP / QUIC)
Framing    → splits byte streams into messages
Envelope   → protocol-level message container
Dispatch   → routes message types
EdgeSync   → WAL replication + outbox
Router     → optional multi-hop routing
```

---

## Protocol Layers

### 1. Framing

```cpp
Frame encode(payload);
FrameDecodeResult decode(buffer);
```

Example:

LengthPrefixVarint → compact framing using varint length

---

### 2. Envelope

Every message is wrapped in an envelope:

```
magic | version | type | msg_id | flags | [crypto] | payload
```

Features:

- versioning
- encryption (AEAD)
- message tracking
- compatibility checks

---

### 3. Messages

Supported message types:

- Hello
- HelloAck
- HelloFinish

- Ping
- Pong

- WalPush
- WalAck
- OutboxPull

---

### 4. Dispatch

```cpp
AnyMessage decode_payload_or_throw(type, payload);
```

Maps raw payload → typed message (std::variant)

---

### 5. Sync (EdgeSync)

- WalPush → send WAL batch
- WalAck → confirm apply
- OutboxPull → request pending ops

Core of offline-first replication.

---

## Quick Start

### Basic message roundtrip
```bash
vix run examples/p2p/01_envelope_and_framing_basic.cpp
```

### Handshake messages
```bash
vix run examples/p2p/02_hello_handshake_messages.cpp
```

### Discovery JSON
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

---

## Runtime Examples

⚠️ Important rule with vix run:

- `--` = compiler flags
- `--run` = runtime arguments (argv)

### Manual connect (TCP)

Terminal 1:
```bash
vix run examples/p2p/08_runtime_manual_connect.cpp --run server
```

Terminal 2:
```bash
vix run examples/p2p/08_runtime_manual_connect.cpp --run client 127.0.0.1 9101
```

---

### UDP Discovery (LAN)

Terminal 1:
```bash
vix run examples/p2p/09_udp_discovery_basic.cpp --run node-a 9201
```

Terminal 2:
```bash
vix run examples/p2p/09_udp_discovery_basic.cpp --run node-b 9202
```

---

### HTTP Bootstrap (Registry)

Start a registry returning:

```json
{
  "peers": [
    { "host": "127.0.0.1", "tcp_port": 9301, "node_id": "node-x" }
  ]
}
```

Run:
```bash
vix run examples/p2p/10_bootstrap_http_basic.cpp --run http://127.0.0.1:8080/peers
```

---

## CLI Alternative (Recommended)

Instead of writing code, you can directly run a node:

```bash
vix p2p --id A --listen 9001
```

Connect:

```bash
vix p2p --id B --listen 9002 --connect 127.0.0.1:9001
```

This uses the same engine internally.

---

## Key Concepts

### Message Flow

```
encode(message)
→ envelope
→ framing
→ transport
→ network
→ decode
→ dispatch
```

---

### Handshake (v2)

```
A → Hello
B → HelloAck
A → HelloFinish
```

After that:

- session key established
- encryption enabled
- peer becomes "Connected"

---

### Offline Sync

```
local write → WAL
WAL → WalPush
peer → WalAck
retry until convergence
```

---

## Why it exists

Real systems fail:

- network drops
- partial writes
- restarts
- duplicates

Traditional systems:

- lose data
- become inconsistent

Vix P2P ensures:

> No data is lost. Ever.
> Sync happens later. Safely.

---

## Summary

vix/p2p is not just networking.

It is:

- a sync protocol
- a replication engine
- a failure recovery system
- a foundation for distributed apps

---

## License

MIT License
Copyright (c) 2025 Gaspard Kirira

