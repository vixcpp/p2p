/**
 *
 *  @file Protocol.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
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
  struct ProtocolVersion
  {
    std::uint16_t major{1};
    std::uint16_t minor{0};

    constexpr bool operator==(const ProtocolVersion &) const = default;

    constexpr bool is_compatible_with(const ProtocolVersion &other) const
    {
      return major == other.major;
    }
  };

  enum class MessageType : std::uint16_t
  {
    Unknown = 0,
    // Handshake v2
    Hello = 1, // HelloInit
    Ping = 2,
    Pong = 3,
    HelloAck = 4,    // B -> A
    HelloFinish = 5, // A -> B
    // Sync
    WalPush = 10,
    WalAck = 11,
    OutboxPull = 12,
  };

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

#endif
