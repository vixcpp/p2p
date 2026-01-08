# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
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
