#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <span>

#include "Binary.hpp"

namespace vix::p2p::msg
{

    struct Hello
    {
        std::string node_id;

        std::unordered_map<std::string, std::string> capabilities;
        std::vector<std::uint8_t> public_key;

        std::vector<std::uint8_t> encode() const
        {
            bin::Writer w;
            w.reserve(64);
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
