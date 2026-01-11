#ifndef WAL_ACK_HPP
#define WAL_ACK_HPP

#include <cstdint>
#include <span>
#include <vix/p2p/messages/Binary.hpp>

namespace vix::p2p::msg
{

    struct WalAck
    {
        std::uint64_t last_applied_seq{0};

        std::vector<std::uint8_t> encode() const
        {
            bin::Writer w;
            w.u64_le(last_applied_seq);
            return std::move(w.out);
        }

        static WalAck decode_or_throw(std::span<const std::uint8_t> bytes)
        {
            bin::Reader r(bytes);
            WalAck a;
            a.last_applied_seq = r.u64_le();
            if (r.remaining() != 0)
                throw bin::Error("WalAck: trailing bytes");
            return a;
        }
    };

} // namespace vix::p2p::msg

#endif