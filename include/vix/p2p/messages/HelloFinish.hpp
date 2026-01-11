#ifndef HELLO_FINISH_HPP
#define HELLO_FINISH_HPP

#include <span>
#include <vector>
#include <cstdint>

#include <vix/p2p/messages/Binary.hpp>

namespace vix::p2p::msg
{
    // HelloFinish (A -> B)
    struct HelloFinish
    {
        std::uint64_t nonce_a{0};
        std::uint64_t nonce_b{0};

        // signature bytes (ed25519 later); for now can be empty in NullCrypto mode
        std::vector<std::uint8_t> signature;

        std::vector<std::uint8_t> encode() const
        {
            bin::Writer w;
            w.reserve(64);
            w.var_u64(nonce_a);
            w.var_u64(nonce_b);
            w.bytes_var(signature);
            return std::move(w.out);
        }

        static HelloFinish decode_or_throw(std::span<const std::uint8_t> bytes)
        {
            bin::Reader r(bytes);
            HelloFinish f;
            f.nonce_a = r.var_u64();
            f.nonce_b = r.var_u64();
            f.signature = r.bytes_var();

            if (r.remaining() != 0)
                throw bin::Error("HelloFinish: trailing bytes");
            return f;
        }
    };
} // namespace vix::p2p::msg

#endif