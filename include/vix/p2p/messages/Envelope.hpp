#ifndef ENVELOPE_HPP
#define ENVELOPE_HPP

#include <cstdint>
#include <vector>
#include <span>
#include <string>
#include <optional>
#include <array>
#include <algorithm>

#include <vix/p2p/Protocol.hpp>
#include <vix/p2p/messages/Binary.hpp>

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

    using MessageId = std::uint64_t;

    struct Envelope
    {
        ProtocolVersion version{1, 0};
        MessageType type{MessageType::Unknown};
        MessageId msg_id{0};
        std::uint32_t flags{0};

        // AEAD fields (only meaningful if Encrypted)
        std::array<std::uint8_t, 12> nonce{}; // 96-bit
        std::array<std::uint8_t, 16> tag{};   // 128-bit

        // payload: plaintext if not Encrypted, ciphertext if Encrypted
        std::vector<std::uint8_t> payload;

        // magic "VP2P" (4) | ver.major(u16) | ver.minor(u16) | type(u16)
        // | msg_id(u64) | flags(u32)
        // | [nonce(12) | tag(16)] if Encrypted
        // | payload_len(varint) | payload(bytes)
        static inline constexpr std::uint32_t kMagic = 0x50325056; // 'V''P''2''P' LE

        std::vector<std::uint8_t> encode() const
        {
            bin::Writer w;
            w.reserve(96 + payload.size());

            w.u32_le(kMagic);
            w.u16_le(version.major);
            w.u16_le(version.minor);
            w.u16_le((std::uint16_t)type);
            w.u64_le(msg_id);
            w.u32_le(flags);

            if (has_flag(flags, EnvelopeFlag::Encrypted))
            {
                // Writer::bytes prend un span => on le construit à partir du pointeur + taille
                w.bytes(std::span<const std::uint8_t>(nonce.data(), nonce.size()));
                w.bytes(std::span<const std::uint8_t>(tag.data(), tag.size()));
            }

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

            if (has_flag(e.flags, EnvelopeFlag::Encrypted))
            {
                auto n = r.bytes_n(e.nonce.size());
                std::copy(n.begin(), n.end(), e.nonce.begin());

                auto t = r.bytes_n(e.tag.size());
                std::copy(t.begin(), t.end(), e.tag.begin());
            }

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

#endif