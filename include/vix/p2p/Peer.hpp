/**
 *
 *  @file Peer.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
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

  using PeerId = std::string;

  enum class PeerState : std::uint8_t
  {
    Disconnected = 0,
    Connecting,
    Handshaking,
    Connected,
    Stale,
    Closed
  };

  struct PeerEndpoint
  {
    std::string host; // ip/hostname
    std::uint16_t port{0};
    std::string scheme; // "tcp" => "quic"
  };

  struct PeerMetadata
  {
    std::unordered_map<std::string, std::string> capabilities;
    std::chrono::steady_clock::time_point last_seen{};
    std::vector<std::uint8_t> public_key;
    bool secure{false};
    std::vector<std::uint8_t> session_key_32; // 32 bytes
    std::uint64_t send_nonce_counter{1};      // start at 1
    // Anti-replay (recv side) for AEAD packets
    std::uint64_t recv_nonce_max{0};    // highest accepted counter
    std::uint64_t recv_nonce_window{0}; // 64-bit window bitmap
  };

  struct HandshakeState
  {
    enum class Stage
    {
      None,
      HelloSent,
      HelloReceived,
      AckSent,
      AckReceived,
      Finished
    };

    Stage stage{Stage::None};

    std::uint64_t nonce_a{0};
    std::uint64_t nonce_b{0};
    std::uint64_t ts_ms{0};

    std::chrono::steady_clock::time_point started_at{};
  };

  struct Peer
  {
    PeerId id;
    PeerState state{PeerState::Disconnected};
    std::optional<PeerEndpoint> endpoint;
    PeerMetadata meta;
    std::optional<HandshakeState> handshake;

    bool is_connected() const { return state == PeerState::Connected; }
  };

} // namespace vix::p2p

#endif
