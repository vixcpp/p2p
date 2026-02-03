/**
 *
 *  @file Dispatch.hpp
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
#ifndef VIX_DISPATCH_HPP
#define VIX_DISPATCH_HPP

#include <variant>
#include <span>

#include <vix/p2p/Protocol.hpp>
#include <vix/p2p/messages/Envelope.hpp>
#include <vix/p2p/messages/Hello.hpp>
#include <vix/p2p/messages/HelloAck.hpp>
#include <vix/p2p/messages/HelloFinish.hpp>
#include <vix/p2p/messages/Ping.hpp>
#include <vix/p2p/messages/Pong.hpp>
#include <vix/p2p/messages/WalPush.hpp>
#include <vix/p2p/messages/WalAck.hpp>
#include <vix/p2p/messages/OutboxPull.hpp>

namespace vix::p2p::msg
{
  /**
   * @brief Variant holding any supported P2P message payload.
   *
   * This type is the canonical runtime representation for decoded
   * protocol messages after dispatch.
   */
  using AnyMessage = std::variant<
      Hello,
      HelloAck,
      HelloFinish,
      Ping,
      Pong,
      WalPush,
      WalAck,
      OutboxPull>;

  /**
   * @brief Decode a message payload according to its protocol type.
   *
   * The function selects the appropriate message decoder based on
   * the provided MessageType and decodes the raw payload bytes into
   * a strongly-typed message structure.
   *
   * @param type Protocol message type.
   * @param payload Raw payload bytes.
   * @return Decoded message wrapped in AnyMessage.
   *
   * @throws bin::Error if the message type is unknown or decoding fails.
   */
  inline AnyMessage decode_payload_or_throw(
      MessageType type,
      std::span<const std::uint8_t> payload)
  {
    switch (type)
    {
    case MessageType::Hello:
      return Hello::decode_or_throw(payload);
    case MessageType::HelloAck:
      return HelloAck::decode_or_throw(payload);
    case MessageType::HelloFinish:
      return HelloFinish::decode_or_throw(payload);

    case MessageType::Ping:
      return Ping::decode_or_throw(payload);
    case MessageType::Pong:
      return Pong::decode_or_throw(payload);

    case MessageType::WalPush:
      return WalPush::decode_or_throw(payload);
    case MessageType::WalAck:
      return WalAck::decode_or_throw(payload);
    case MessageType::OutboxPull:
      return OutboxPull::decode_or_throw(payload);

    default:
      break;
    }

    throw bin::Error("Dispatch: unknown message type");
  }

} // namespace vix::p2p::msg

#endif // VIX_DISPATCH_HPP
