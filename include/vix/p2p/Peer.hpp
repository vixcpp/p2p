/**
 *
 *  @file Peer.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#ifndef VIX_PEER_HPP
#define VIX_PEER_HPP

#include <cstdint>
#include <string>
#include <chrono>
#include <optional>
#include <unordered_map>
#include <vector>

namespace vix::p2p
{
  /**
   * @brief Unique identifier for a peer.
   *
   * Usually derived from a node identity (e.g. public key fingerprint).
   */
  using PeerId = std::string;

  /**
   * @brief Connection lifecycle state of a peer.
   */
  enum class PeerState : std::uint8_t
  {
    Disconnected = 0,
    Connecting,
    Handshaking,
    Connected,
    Stale,
    Closed
  };

  /**
   * @brief Network endpoint information for a peer.
   */
  struct PeerEndpoint
  {
    /// IP address or hostname
    std::string host;

    /// Network port
    std::uint16_t port{0};

    /// Transport scheme (e.g. "tcp", "quic")
    std::string scheme;
  };

  /**
   * @brief Runtime metadata attached to a peer.
   *
   * Stores capabilities, timestamps, and cryptographic context.
   */
  struct PeerMetadata
  {
    /// Peer advertised capabilities (features, versions, protocol flags, etc.)
    std::unordered_map<std::string, std::string> capabilities;

    /// Last observed activity timestamp
    std::chrono::steady_clock::time_point last_seen{};

    /// Remote peer public key bytes
    std::vector<std::uint8_t> public_key;

    /// Indicates whether the connection is secured (handshake completed)
    bool secure{false};

    /// Symmetric session key (32 bytes)
    std::vector<std::uint8_t> session_key_32;

    /// Monotonic send nonce counter (starts at 1)
    std::uint64_t send_nonce_counter{1};

    /// Highest accepted receive-side nonce counter
    std::uint64_t recv_nonce_max{0};

    /// 64-bit sliding window bitmap for anti-replay
    std::uint64_t recv_nonce_window{0};
  };

  /**
   * @brief Handshake state machine for a peer.
   */
  struct HandshakeState
  {
    /**
     * @brief Handshake progression stages.
     */
    enum class Stage
    {
      None,
      HelloSent,
      HelloReceived,
      AckSent,
      AckReceived,
      Finished
    };

    /// Current handshake stage
    Stage stage{Stage::None};

    /// Local nonce
    std::uint64_t nonce_a{0};

    /// Remote nonce
    std::uint64_t nonce_b{0};

    /// Handshake timestamp in milliseconds
    std::uint64_t ts_ms{0};

    /// When the handshake started (steady clock)
    std::chrono::steady_clock::time_point started_at{};
  };

  /**
   * @brief Peer representation within the P2P network.
   *
   * Holds identity, connection state, endpoint, metadata, and handshake context.
   */
  struct Peer
  {
    /// Unique peer identifier
    PeerId id;

    /// Current peer connection state
    PeerState state{PeerState::Disconnected};

    /// Last known endpoint (if available)
    std::optional<PeerEndpoint> endpoint;

    /// Runtime metadata and crypto context
    PeerMetadata meta;

    /// Current handshake state (if handshaking)
    std::optional<HandshakeState> handshake;

    /**
     * @brief Check whether the peer is fully connected.
     *
     * @return true if the peer state is Connected.
     */
    bool is_connected() const { return state == PeerState::Connected; }
  };

} // namespace vix::p2p

#endif // VIX_PEER_HPP
