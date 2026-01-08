#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <limits>
#include <optional>

namespace vix::p2p::bin
{

    struct Error : std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    struct Reader
    {
        std::span<const std::uint8_t> s;
        std::size_t off{0};

        explicit Reader(std::span<const std::uint8_t> bytes) : s(bytes) {}

        std::size_t remaining() const { return s.size() - off; }

        void require(std::size_t n)
        {
            if (remaining() < n)
                throw Error("bin::Reader: buffer underflow");
        }

        std::uint8_t u8()
        {
            require(1);
            return s[off++];
        }

        std::uint16_t u16_le()
        {
            require(2);
            std::uint16_t v = (std::uint16_t)s[off] | ((std::uint16_t)s[off + 1] << 8);
            off += 2;
            return v;
        }

        std::uint32_t u32_le()
        {
            require(4);
            std::uint32_t v = (std::uint32_t)s[off] | ((std::uint32_t)s[off + 1] << 8) | ((std::uint32_t)s[off + 2] << 16) | ((std::uint32_t)s[off + 3] << 24);
            off += 4;
            return v;
        }

        std::uint64_t u64_le()
        {
            require(8);
            std::uint64_t v = 0;
            for (int i = 0; i < 8; ++i)
                v |= (std::uint64_t)s[off + i] << (8 * i);
            off += 8;
            return v;
        }

        // unsigned LEB128-style varint
        std::uint64_t var_u64()
        {
            std::uint64_t result = 0;
            int shift = 0;
            for (int i = 0; i < 10; ++i)
            {
                auto byte = u8();
                result |= (std::uint64_t)(byte & 0x7F) << shift;
                if ((byte & 0x80) == 0)
                    return result;
                shift += 7;
            }
            throw Error("bin::Reader: varint too long");
        }

        std::vector<std::uint8_t> bytes_n(std::size_t n)
        {
            require(n);
            std::vector<std::uint8_t> out(s.begin() + (std::ptrdiff_t)off,
                                          s.begin() + (std::ptrdiff_t)off + (std::ptrdiff_t)n);
            off += n;
            return out;
        }

        std::vector<std::uint8_t> bytes_var()
        {
            auto n = (std::size_t)var_u64();
            if (n > remaining())
                throw Error("bin::Reader: bytes_var too large");
            return bytes_n(n);
        }

        std::string str_var()
        {
            auto n = (std::size_t)var_u64();
            if (n > remaining())
                throw Error("bin::Reader: str_var too large");
            std::string out((const char *)s.data() + off, n);
            off += n;
            return out;
        }
    };

    struct Writer
    {
        std::vector<std::uint8_t> out;

        void reserve(std::size_t n) { out.reserve(n); }

        void u8(std::uint8_t v) { out.push_back(v); }

        void u16_le(std::uint16_t v)
        {
            out.push_back((std::uint8_t)(v & 0xFF));
            out.push_back((std::uint8_t)((v >> 8) & 0xFF));
        }

        void u32_le(std::uint32_t v)
        {
            out.push_back((std::uint8_t)(v & 0xFF));
            out.push_back((std::uint8_t)((v >> 8) & 0xFF));
            out.push_back((std::uint8_t)((v >> 16) & 0xFF));
            out.push_back((std::uint8_t)((v >> 24) & 0xFF));
        }

        void u64_le(std::uint64_t v)
        {
            for (int i = 0; i < 8; ++i)
                out.push_back((std::uint8_t)((v >> (8 * i)) & 0xFF));
        }

        void var_u64(std::uint64_t v)
        {
            while (true)
            {
                std::uint8_t byte = (std::uint8_t)(v & 0x7F);
                v >>= 7;
                if (v == 0)
                {
                    out.push_back(byte);
                    return;
                }
                out.push_back((std::uint8_t)(byte | 0x80));
            }
        }

        void bytes(std::span<const std::uint8_t> b)
        {
            out.insert(out.end(), b.begin(), b.end());
        }

        void bytes_var(std::span<const std::uint8_t> b)
        {
            var_u64((std::uint64_t)b.size());
            bytes(b);
        }

        void str_var(std::string_view s)
        {
            var_u64((std::uint64_t)s.size());
            out.insert(out.end(), s.begin(), s.end());
        }
    };

} // namespace vix::p2p::bin
