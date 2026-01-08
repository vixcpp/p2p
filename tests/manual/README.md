# Vix P2P — Manual Demo (Phase 3)

This demo validates the first end-to-end TCP path:

- Node A listens
- Node B connects
- HELLO handshake
- PING/PONG roundtrip

## Build

```bash
cmake -S . -B build -DVIX_P2P_BUILD_TESTS=ON
cmake --build build -j
```

Run (two terminals)
Terminal A
./build/tests/p2p_demo --id A --listen 9001

Terminal B
./build/tests/p2p_demo --id B --listen 9002 --connect 127.0.0.1:9001

Expected output

handshakes_started increases

handshakes_completed increases

peers_connected becomes 1

You should see logs for:

HELLO recv

PING recv

PONG recv

---

## 2) Logs visibles dans `src/Node.cpp`

Ajoute ces `std::cout` (ou `std::cerr`) dans les handlers, **sans framework logging** (Phase 3 simple).

### Dans `on_new_socket(...)` après création `pid` :

```cpp
std::cout << "[p2p] new socket peer_id=" << pid
          << " inbound=" << (is_inbound ? "yes" : "no")
          << "\n";

Dans send_hello(...) juste avant send_envelope(...) :
std::cout << "[p2p] -> HELLO to " << peer_id << "\n";

Dans on_hello(...) au tout début :
std::cout << "[p2p] <- HELLO from " << peer_id
          << " node_id=" << h.node_id
          << " caps=" << h.capabilities.size()
          << "\n";

Dans on_ping(...) au début :
std::cout << "[p2p] <- PING from " << peer_id
          << " nonce=" << p.nonce
          << "\n";


Et avant d’envoyer PONG :

std::cout << "[p2p] -> PONG to " << peer_id
          << " nonce=" << p.nonce
          << "\n";

Dans on_pong(...) :
std::cout << "[p2p] <- PONG from " << peer_id << "\n";
```
