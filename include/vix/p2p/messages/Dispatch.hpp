#pragma once
#include <variant>
#include <span>

#include "../Protocol.hpp"
#include "Envelope.hpp"
#include "Hello.hpp"
#include "Ping.hpp"
#include "Pong.hpp"
#include "WalPush.hpp"
#include "WalAck.hpp"
#include "OutboxPull.hpp"

namespace vix::p2p::msg
{

    using AnyMessage = std::variant<
        Hello,
        Ping,
        Pong,
        WalPush,
        WalAck,
        OutboxPull>;

    inline AnyMessage decode_payload_or_throw(MessageType type, std::span<const std::uint8_t> payload)
    {
        switch (type)
        {
        case MessageType::Hello:
            return Hello::decode_or_throw(payload);
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
