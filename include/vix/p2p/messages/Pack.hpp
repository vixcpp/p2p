#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include "../Protocol.hpp"
#include "Envelope.hpp"

namespace vix::p2p::pack
{

    inline MessageId next_message_id()
    {
        static MessageId g = 1;
        return g++;
    }

    template <class Msg>
    Envelope make_envelope(MessageType type, const Msg &m, std::uint32_t flags = 0)
    {
        Envelope e;
        e.version = ProtocolVersion{1, 0};
        e.type = type;
        e.msg_id = next_message_id();
        e.flags = flags;
        e.payload = m.encode();
        return e;
    }

} // namespace vix::p2p::pack
