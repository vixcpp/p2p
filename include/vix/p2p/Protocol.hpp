/**
 *
 *  @file Protocol.hpp
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
#ifndef VIX_PROTOCOL_HPP
#define VIX_PROTOCOL_HPP

#include <cstdint>
#include <string_view>

namespace vix::p2p
{
  /**
   * @brief P2P protocol version descriptor.
   *
   * Compatibility is defined by the major version.
   */
  struct ProtocolVersion
  {
    /// Major protocol version (breaking changes)
    std::uint16_t major{1};

    /// Minor protocol version (backward compatible changes)
    std::uint16_t minor{0};

    /// Equality comparison
    constexpr bool operator==(const ProtocolVersion &) const = default;

    /**
     * @brief Check compatibility with another version.
     *
     * Two versions are considered compatible if they share
     * the same major version.
     *
     * @param other Other protocol version.
     * @return true if compatible.
     */
    constexpr bool is_compatible_with(const ProtocolVersion &other) const
    {
      return major == other.major;
    }
  };

  /**
   * @brief P2P message type identifiers.
   *
   * These numeric values are part of the wire format.
   */
  enum class MessageType : std::uint16_t
  {
    Unknown = 0,

    // Handshake v2
    Hello = 1,
    Ping = 2,
    Pong = 3,
    HelloAck = 4,
    HelloFinish = 5,

    // Sync
    WalPush = 10,
    WalAck = 11,
    OutboxPull = 12,
  };

  /**
   * @brief Convert a message type to a stable string label.
   *
   * Intended for logs and diagnostics.
   *
   * @param t Message type.
   * @return String label.
   */
  inline constexpr std::string_view to_string(MessageType t)
  {
    switch (t)
    {
    case MessageType::Hello:
      return "Hello";
    case MessageType::HelloAck:
      return "HelloAck";
    case MessageType::HelloFinish:
      return "HelloFinish";
    case MessageType::Ping:
      return "Ping";
    case MessageType::Pong:
      return "Pong";
    case MessageType::WalPush:
      return "WalPush";
    case MessageType::WalAck:
      return "WalAck";
    case MessageType::OutboxPull:
      return "OutboxPull";
    default:
      return "Unknown";
    }
  }

} // namespace vix::p2p

#endif // VIX_PROTOCOL_HPP
