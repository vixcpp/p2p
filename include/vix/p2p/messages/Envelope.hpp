#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <string>
#include <optional>

#include "../Protocol.hpp"
#include "Binary.hpp"

namespace vix::p2p
{

    enum class EnvelopeFlag : std::uint32_t
    {
        None = 0,
        Encrypted = 1u << 0,
        Compressed = 1u << 1,
        AckReq = 1u << 2,
    };

    inline constexpr std::uint32_t operator|(EnvelopeFlag a, EnvelopeFlag b)
    {
        return (std::uint32_t)a | (std::uint32_t)b;
    }
    inline constexpr bool has_flag(std::uint32_t flags, EnvelopeFlag f)
    {
        return (flags & (std::uint32_t)f) != 0;
    }

    // 64-bit
    using MessageId = std::uint64_t;

    struct Envelope
    {
        ProtocolVersion version{1, 0};
        MessageType type{MessageType::Unknown};
        MessageId msg_id{0};
        std::uint32_t flags{0};
        std::vector<std::uint8_t> payload;

        // magic "VP2P" (4) | ver.major(u16) | ver.minor(u16) | type(u16)
        // | msg_id(u64) | flags(u32) | payload_len(varint) | payload(bytes)
        static inline constexpr std::uint32_t kMagic = 0x50325056; // 'V''P''2''P' little-endian

        std::vector<std::uint8_t> encode() const
        {
            bin::Writer w;
            w.reserve(64 + payload.size());
            w.u32_le(kMagic);
            w.u16_le(version.major);
            w.u16_le(version.minor);
            w.u16_le((std::uint16_t)type);
            w.u64_le(msg_id);
            w.u32_le(flags);
            w.bytes_var(payload);
            return std::move(w.out);
        }

        static Envelope decode_or_throw(std::span<const std::uint8_t> bytes)
        {
            bin::Reader r(bytes);

            auto magic = r.u32_le();
            if (magic != kMagic)
                throw bin::Error("Envelope: bad magic");

            Envelope e;
            e.version.major = r.u16_le();
            e.version.minor = r.u16_le();
            e.type = (MessageType)r.u16_le();
            e.msg_id = r.u64_le();
            e.flags = r.u32_le();
            e.payload = r.bytes_var();

            ProtocolVersion expected{1, 0};
            if (!e.version.is_compatible_with(expected))
                throw bin::Error("Envelope: incompatible version");

            if (r.remaining() != 0)
                throw bin::Error("Envelope: trailing bytes");
            return e;
        }
    };

} // namespace vix::p2p
