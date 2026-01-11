#ifndef WAL_PUSH_HPP
#define WAL_PUSH_HPP

#include <cstdint>
#include <vector>
#include <span>
#include <vix/p2p/messages/Binary.hpp>

namespace vix::p2p::msg
{

    struct WalPush
    {
        std::uint64_t seq_begin{0};
        std::uint64_t seq_end{0};
        std::vector<std::uint8_t> wal_bytes;

        std::vector<std::uint8_t> encode() const
        {
            bin::Writer w;
            w.u64_le(seq_begin);
            w.u64_le(seq_end);
            w.bytes_var(wal_bytes);
            return std::move(w.out);
        }

        static WalPush decode_or_throw(std::span<const std::uint8_t> bytes)
        {
            bin::Reader r(bytes);
            WalPush m;
            m.seq_begin = r.u64_le();
            m.seq_end = r.u64_le();
            m.wal_bytes = r.bytes_var();
            if (r.remaining() != 0)
                throw bin::Error("WalPush: trailing bytes");
            return m;
        }
    };

} // namespace vix::p2p::msg

#endif