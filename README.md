# ⚡ Vix.cpp — P2P Module

High-performance **peer-to-peer & edge networking module** for **Vix.cpp**.  
Designed for **offline-first systems**, **unstable networks**, and **secure distributed runtimes**.

This module powers Vix’s **sync engine (WAL / outbox)** and forms the **transport backbone** of the ecosystem.

---

## ✨ What this module provides

✅ TCP peer connections  
✅ Secure handshake lifecycle  
✅ Per-peer session state  
✅ Peer discovery (UDP broadcast / multicast)  
✅ HTTP bootstrap (registry pull / announce)  
✅ Runtime peer & handshake statistics  
✅ Clean shutdown & signal handling  
✅ Modular, production-shaped architecture  

This is **not** a toy P2P example.

---

## 📂 Repository layout

```
modules/p2p/
├── include/            # Public P2P API (Node, Peer, Protocol, Crypto, …)
├── src/                # Implementation
├── tests/
│   ├── manual/         # Manual integration tests
│   │   ├── main.cpp    # p2p_demo (real runtime demo)
│   │   └── registry.py # HTTP bootstrap registry (test server)
│   └── CMakeLists.txt
├── build/              # Build output
│   └── tests/
│       └── p2p_demo    # Compiled demo binary
├── CMakeLists.txt
├── README.md
├── CHANGELOG.md
└── LICENSE
```

---

## 🧪 Demo: `p2p_demo`

The **real executable demo** lives here after build:

```bash
build/tests/p2p_demo
```

It spins up a full P2P node with:
- listening socket
- optional outbound connections
- discovery
- bootstrap
- live stats

---

## 🚀 Run the demo

### Terminal A
```bash
./build/tests/p2p_demo --id A --listen 9001
```

### Terminal B
```bash
./build/tests/p2p_demo --id B --listen 9002 --connect 127.0.0.1:9001
```

### Delayed connect (handshake timeout test)
```bash
./build/tests/p2p_demo \
  --id B \
  --listen 9002 \
  --connect 127.0.0.1:9001 \
  --connect-delay 8000
```

### Auto stop after 20 seconds
```bash
./build/tests/p2p_demo --id A --listen 9001 --run 20
```

---

## 📊 Runtime statistics

Printed periodically while running:

```
peers_total=2
peers_connected=1
handshakes_started=1
handshakes_completed=1
```

Final stats are always printed on exit.

---

## 🌐 Discovery & Bootstrap (optional)

### UDP discovery
```bash
--discovery on
--disc-mode broadcast | multicast
--disc-port 37020
```

### HTTP bootstrap registry
```bash
--bootstrap on
--registry http://127.0.0.1:8080/p2p/v1
--announce on
```

A minimal test registry is provided in:

```
tests/manual/registry.py
```

---

## 🔐 Security model (high-level)

- Explicit handshake state machine  
- Per-peer session lifecycle  
- Control messages remain plaintext (safe bootstrapping)  
- Designed for AEAD encrypted payloads  
- Ready for secure WAL / sync traffic  

---

## 🎯 Who this is for

- C++ backend engineers  
- Distributed systems developers  
- Offline-first / local-first builders  
- Edge & P2P networking enthusiasts  
- Anyone who wants **real** P2P, not fake examples  

---

## ⭐ Why star this module?

- Clean **production-oriented P2P design**
- Minimal but **realistic**
- Built for **unstable networks**
- Reusable as a base for:
  - sync engines
  - mesh networks
  - decentralized runtimes
- Actively used inside **Vix.cpp**

---

## 🧭 Status

✔️ Actively developed  
✔️ Manually tested  
✔️ Used internally  
🚧 Continuously evolving  

---

If this module helped you understand **how real P2P in C++ should look**,  
**leave a ⭐ it genuinely helps the project grow.**
