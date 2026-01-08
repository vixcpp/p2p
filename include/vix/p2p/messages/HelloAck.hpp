#pragma once
#include <span>
#include <vector>
#include <cstdint>

#include "Binary.hpp"

namespace vix::p2p::msg
{
    // HelloAck (B -> A)
    struct HelloAck
    {
        std::uint64_t nonce_a{0}; // echo
        std::uint64_t nonce_b{0}; // challenge

        std::vector<std::uint8_t> encode() const
        {
            bin::Writer w;
            w.reserve(24);
            w.var_u64(nonce_a);
            w.var_u64(nonce_b);
            return std::move(w.out);
        }

        static HelloAck decode_or_throw(std::span<const std::uint8_t> bytes)
        {
            bin::Reader r(bytes);
            HelloAck a;
            a.nonce_a = r.var_u64();
            a.nonce_b = r.var_u64();

            if (r.remaining() != 0)
                throw bin::Error("HelloAck: trailing bytes");
            return a;
        }
    };
} // namespace vix::p2p::msg
