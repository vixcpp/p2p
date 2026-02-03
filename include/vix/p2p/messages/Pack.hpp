/**
 *
 *  @file Pack.hpp
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
#ifndef VIX_PACK_HPP
#define VIX_PACK_HPP

#include <cstdint>
#include <vector>
#include <span>
#include <array>

#include <vix/p2p/Protocol.hpp>
#include <vix/p2p/messages/Envelope.hpp>
#include <vix/p2p/Crypto.hpp>

namespace vix::p2p::pack
{
  /**
   * @brief Generate the next message identifier.
   *
   * Message identifiers are used for tracking, deduplication, and
   * optional acknowledgement workflows.
   *
   * @return Monotonically increasing MessageId.
   */
  inline MessageId next_message_id()
  {
    static MessageId g = 1;
    return g++;
  }

  /**
   * @brief Build AEAD additional authenticated data for an envelope.
   *
   * The AAD binds the encryption to immutable header fields, ensuring
   * that any tampering of version, type, msg_id, or flags is detected.
   *
   * Layout (little-endian encoding):
   *   ver.major(u16) | ver.minor(u16) | type(u16) | msg_id(u64) | flags(u32)
   *
   * @param e Envelope source (nonce, tag, payload are not included).
   * @return AAD bytes.
   */
  inline std::vector<std::uint8_t> make_aad(const Envelope &e)
  {
    std::vector<std::uint8_t> aad;
    aad.reserve(2 + 2 + 2 + 8 + 4);

    auto push_u16 = [&](std::uint16_t v)
    {
      aad.push_back(static_cast<std::uint8_t>(v & 0xFF));
      aad.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    };

    auto push_u32 = [&](std::uint32_t v)
    {
      aad.push_back(static_cast<std::uint8_t>(v & 0xFF));
      aad.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
      aad.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
      aad.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
    };

    auto push_u64 = [&](std::uint64_t v)
    {
      for (std::size_t i = 0; i < 8; ++i)
        aad.push_back(static_cast<std::uint8_t>((v >> (8u * i)) & 0xFFu));
    };

    push_u16(e.version.major);
    push_u16(e.version.minor);
    push_u16(static_cast<std::uint16_t>(e.type));
    push_u64(e.msg_id);
    push_u32(e.flags);

    return aad;
  }

  /**
   * @brief Create an encrypted envelope using AEAD.
   *
   * The envelope is created with the Encrypted flag set, a fresh message
   * id, and a nonce derived from a caller-provided counter.
   *
   * Nonce layout:
   *   nonce12 = [u64 counter (LE)] + [u32 zeros]
   *
   * @param type Protocol message type.
   * @param plaintext Plaintext payload bytes.
   * @param session_key32 Session key (32 bytes).
   * @param crypto Crypto implementation used for AEAD.
   * @param nonce_counter Monotonic counter used to build the nonce.
   * @param extra_flags Additional envelope flags to OR with Encrypted.
   * @return Encrypted envelope.
   */
  inline Envelope make_envelope_secure(
      MessageType type,
      std::span<const std::uint8_t> plaintext,
      std::span<const std::uint8_t> session_key32,
      Crypto &crypto,
      std::uint64_t nonce_counter,
      std::uint32_t extra_flags = 0)
  {
    Envelope e;
    e.version = ProtocolVersion{1, 0};
    e.type = type;
    e.msg_id = next_message_id();
    e.flags = static_cast<std::uint32_t>(EnvelopeFlag::Encrypted) | extra_flags;

    for (std::size_t i = 0; i < 8; ++i)
      e.nonce[i] = static_cast<std::uint8_t>((nonce_counter >> (8u * i)) & 0xFFu);

    for (std::size_t i = 8; i < e.nonce.size(); ++i)
      e.nonce[i] = 0;

    const auto aad = make_aad(e);

    std::array<std::uint8_t, 16> tag{};
    e.payload = crypto.aead_encrypt(
        session_key32,
        std::span<const std::uint8_t>(e.nonce.data(), e.nonce.size()),
        aad,
        plaintext,
        tag);
    e.tag = tag;

    return e;
  }

  /**
   * @brief Create a plaintext envelope from an encodable message.
   *
   * The message is encoded as plaintext payload and stored in the envelope.
   *
   * @tparam Msg Encodable message type providing encode().
   * @param type Protocol message type.
   * @param m Message object.
   * @param flags Envelope flags bitmask.
   * @return Plaintext envelope.
   */
  template <class Msg>
  Envelope make_envelope(MessageType type, const Msg &m, std::uint32_t flags = 0)
  {
    Envelope e;
    e.version = ProtocolVersion{1, 0};
    e.type = type;
    e.msg_id = next_message_id();
    e.flags = flags;
    e.payload = m.encode();
    return e;
  }

} // namespace vix::p2p::pack

#endif // VIX_PACK_HPP
