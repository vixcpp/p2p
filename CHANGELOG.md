# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.4.0] 2026-01-08

feat(p2p): phase 6.5 – secure envelopes with AEAD and session keys

This commit completes Phase 6.5 of the P2P protocol by introducing
authenticated encryption at the Envelope layer.

Key changes:

- Added AEAD support (encrypt + authenticate) using per-peer session keys.
- Extended Envelope format with nonce (96-bit) and authentication tag (128-bit).
- Derived and stored a 32-byte session key at the end of the handshake.
- Enabled secure channels per peer with send-side nonce counters.
- Implemented decrypt-before-dispatch logic in Node to transparently handle
  encrypted payloads.
- Kept handshake and control messages (Hello, Ping/Pong) in plaintext to
  preserve backward compatibility and simplify bootstrapping.
- Provided a NullCrypto AEAD implementation for development and testing.

This lays the foundation for fully secure WAL / Outbox synchronization and
future replay protection (Phase 6.6).

## [0.3.0] - 2026-01-08

This PR introduces Phase 5 of the P2P roadmap.

- HTTP bootstrap registry (pull + optional announce)
- Cooldown and exponential backoff
- Safe async HTTP client (timeouts, size limits)
- Integration with TcpNode lifecycle
- Manual registry for local testing

This enables peer discovery beyond LAN and prepares WAN scenarios.

## [0.2.0] - 2026-01-08

### Added

-

### Changed

-

### Removed

-

p2p: phase 4 LAN discovery via UDP (broadcast/multicast)

- Add UDP-based peer discovery (broadcast/multicast)
- Introduce DiscoveryAnnounce message (lightweight JSON, size-limited)
- Implement DiscoveryUdp with announce loop and receive loop
- Add deduplication, TTL, and connect cooldown
- Integrate discovery into TcpNode lifecycle
- Reserve peer slot before async connect to prevent duplicate connects
- Add tcp_connect_async failure callback for cleanup
- Extend p2p_demo with discovery CLI options (--discovery, --disc-port, --disc-mode, --disc-interval, --no-connect)
- Harden discovery against self-announces, spam, and invalid packets"

## [0.1.1] - 2026-01-08

### Added

-

### Changed

-

### Removed

-

p2p: hardening tcp transport and node lifecycle (timeouts, heartbeat, demo)

- Add backpressure and safety limits to TCP transport
  - max queued frames and bytes
  - idempotent close and atomic closed flag
- Add heartbeat loop (ping / stale detection)
- Add handshake timeout protection
- Improve node shutdown ordering and safety
- Extend manual p2p_demo with hardening scenarios
  - connect delay
  - auto-stop (--run)
  - configurable stats interval
- Remove obsolete manual README

## [0.1.0] - 2026-01-08

p2p: phase 3 tcp transport, stable peer id, manual demo

- Introduce TCP transport based on Asio standalone
- Add public tcp_accept / tcp_connect_async API
- Implement HELLO handshake with Ping/Pong validation
- Re-key peer identifiers using Hello.node_id (stable identity)
- Keep TcpSession fully private to transport implementation
- Provide manual p2p_demo for end-to-end validation

This completes Phase 3:

- end-to-end TCP path
- framed envelopes
- protocol dispatch
- no sync or crypto logic yet

### Added

-

### Changed

-

### Removed

-

## [0.0.1] - 2025-12-30

### Added

-

### Changed

-

### Removed

-

-

### Added

- Initial project scaffolding for the `vixcpp/p2p` module.
- CMake build system:
  - STATIC vs header-only build depending on `src/` contents.
  - Integration with `vix::core` and optional JSON backend.
  - Support for sanitizers via `VIX_ENABLE_SANITIZERS`.
- Basic repository structure:
  - `include/vix/p2p/` for public p2p API.
  - `src/` for implementation files.
- Release workflow:
  - `Makefile` with `release`, `commit`, `push`, `merge`, and `tag` targets.
  - `changelog` target wired to `scripts/update_changelog.sh`.
