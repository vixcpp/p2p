/**
 *
 *  @file Envelope.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#ifndef VIX_ENVELOPE_HPP
#define VIX_ENVELOPE_HPP

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
  /**
   * @brief Envelope feature flags.
   *
   * Flags are stored as a 32-bit bitmask on the wire.
   */
  enum class EnvelopeFlag : std::uint32_t
  {
    None = 0,
    Encrypted = 1u << 0,
    Compressed = 1u << 1,
    AckReq = 1u << 2,
  };

  /**
   * @brief Combine two envelope flags into a bitmask.
   *
   * @param a Flag A.
   * @param b Flag B.
   * @return Combined bitmask.
   */
  inline constexpr std::uint32_t operator|(EnvelopeFlag a, EnvelopeFlag b)
  {
    return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b);
  }

  /**
   * @brief Test whether a bitmask contains a given flag.
   *
   * @param flags Flag bitmask.
   * @param f Flag to test.
   * @return true if the flag is set.
   */
  inline constexpr bool has_flag(std::uint32_t flags, EnvelopeFlag f)
  {
    return (flags & static_cast<std::uint32_t>(f)) != 0;
  }

  /**
   * @brief Unique message identifier used for tracking and acknowledgments.
   */
  using MessageId = std::uint64_t;

  /**
   * @brief Protocol envelope for all P2P messages.
   *
   * The envelope is responsible for versioning, type tagging, optional
   * encryption metadata, and carrying the message payload bytes.
   *
   * Wire format:
   *   magic(u32) | ver.major(u16) | ver.minor(u16) | type(u16)
   *   | msg_id(u64) | flags(u32)
   *   | [nonce(12) | tag(16)] if Encrypted
   *   | payload_len(var_u64) | payload(bytes)
   *
   * If Encrypted is set, payload contains ciphertext and the AEAD fields
   * provide nonce and authentication tag.
   */
  struct Envelope
  {
    /// Protocol version
    ProtocolVersion version{1, 0};

    /// Message type identifier
    MessageType type{MessageType::Unknown};

    /// Message identifier
    MessageId msg_id{0};

    /// Feature flags bitmask
    std::uint32_t flags{0};

    /// AEAD nonce (96-bit), meaningful only when Encrypted is set
    std::array<std::uint8_t, 12> nonce{};

    /// AEAD tag (128-bit), meaningful only when Encrypted is set
    std::array<std::uint8_t, 16> tag{};

    /// Payload bytes (plaintext or ciphertext depending on flags)
    std::vector<std::uint8_t> payload;

    /**
     * @brief Wire magic constant ("VP2P" in little-endian).
     */
    static inline constexpr std::uint32_t kMagic = 0x50325056;

    /**
     * @brief Encode the envelope into wire bytes.
     *
     * @return Encoded bytes.
     */
    std::vector<std::uint8_t> encode() const
    {
      bin::Writer w;
      w.reserve(96 + payload.size());

      w.u32_le(kMagic);
      w.u16_le(version.major);
      w.u16_le(version.minor);
      w.u16_le(static_cast<std::uint16_t>(type));
      w.u64_le(msg_id);
      w.u32_le(flags);

      if (has_flag(flags, EnvelopeFlag::Encrypted))
      {
        w.bytes(std::span<const std::uint8_t>(nonce.data(), nonce.size()));
        w.bytes(std::span<const std::uint8_t>(tag.data(), tag.size()));
      }

      w.bytes_var(payload);
      return std::move(w.out);
    }

    /**
     * @brief Decode an envelope from wire bytes.
     *
     * The function validates magic, checks protocol compatibility,
     * and rejects trailing bytes.
     *
     * @param bytes Input wire bytes.
     * @return Decoded Envelope.
     *
     * @throws bin::Error on malformed input or incompatible version.
     */
    static Envelope decode_or_throw(std::span<const std::uint8_t> bytes)
    {
      bin::Reader r(bytes);

      const auto magic = r.u32_le();
      if (magic != kMagic)
        throw bin::Error("Envelope: bad magic");

      Envelope e;
      e.version.major = r.u16_le();
      e.version.minor = r.u16_le();
      e.type = static_cast<MessageType>(r.u16_le());
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

      const ProtocolVersion expected{1, 0};
      if (!e.version.is_compatible_with(expected))
        throw bin::Error("Envelope: incompatible version");

      if (r.remaining() != 0)
        throw bin::Error("Envelope: trailing bytes");

      return e;
    }
  };

} // namespace vix::p2p

#endif // VIX_ENVELOPE_HPP
