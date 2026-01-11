#ifndef HELLO_HPP
#define HELLO_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <span>
#include <cstdint>

#include <vix/p2p/messages/Binary.hpp>

namespace vix::p2p::msg
{
    // Hello = HelloInit (A -> B)
    struct Hello
    {
        // anti-replay / handshake v2
        std::uint64_t nonce_a{0};
        std::uint64_t ts_ms{0};

        // identity
        std::string node_id;

        // capabilities
        std::unordered_map<std::string, std::string> capabilities;

        // crypto
        std::vector<std::uint8_t> public_key;

        std::vector<std::uint8_t> encode() const
        {
            bin::Writer w;
            w.reserve(96);

            w.var_u64(nonce_a);
            w.var_u64(ts_ms);

            w.str_var(node_id);

            w.var_u64((std::uint64_t)capabilities.size());
            for (const auto &[k, v] : capabilities)
            {
                w.str_var(k);
                w.str_var(v);
            }

            w.bytes_var(public_key);
            return std::move(w.out);
        }

        static Hello decode_or_throw(std::span<const std::uint8_t> bytes)
        {
            bin::Reader r(bytes);

            Hello h;
            h.nonce_a = r.var_u64();
            h.ts_ms = r.var_u64();

            h.node_id = r.str_var();

            auto n = (std::size_t)r.var_u64();
            for (std::size_t i = 0; i < n; ++i)
            {
                auto k = r.str_var();
                auto v = r.str_var();
                h.capabilities.emplace(std::move(k), std::move(v));
            }

            h.public_key = r.bytes_var();

            if (r.remaining() != 0)
                throw bin::Error("Hello: trailing bytes");
            return h;
        }
    };
} // namespace vix::p2p::msg

#endif