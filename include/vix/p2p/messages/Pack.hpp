/**
 *
 *  @file Pack.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
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
  inline MessageId next_message_id()
  {
    static MessageId g = 1;
    return g++;
  }

  // AAD = version + type + msg_id + flags (nonce/tag/payload)
  inline std::vector<std::uint8_t> make_aad(const Envelope &e)
  {
    std::vector<std::uint8_t> aad;
    aad.reserve(2 + 2 + 2 + 8 + 4);

    auto push_u16 = [&](std::uint16_t v)
    {
      aad.push_back(std::uint8_t(v & 0xFF));
      aad.push_back(std::uint8_t((v >> 8) & 0xFF));
    };
    auto push_u32 = [&](std::uint32_t v)
    {
      aad.push_back(std::uint8_t(v & 0xFF));
      aad.push_back(std::uint8_t((v >> 8) & 0xFF));
      aad.push_back(std::uint8_t((v >> 16) & 0xFF));
      aad.push_back(std::uint8_t((v >> 24) & 0xFF));
    };
    auto push_u64 = [&](std::uint64_t v)
    {
      for (int i = 0; i < 8; ++i)
        aad.push_back(std::uint8_t((v >> (8 * i)) & 0xFF));
    };

    push_u16(e.version.major);
    push_u16(e.version.minor);
    push_u16(static_cast<std::uint16_t>(e.type));
    push_u64(e.msg_id);
    push_u32(e.flags);

    return aad;
  }

  // encrypted envelope (AEAD)
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
    e.flags = (std::uint32_t)EnvelopeFlag::Encrypted | extra_flags;

    // nonce12 = [u64 counter LE] + [u32 zeros]
    for (int i = 0; i < 8; ++i)
      e.nonce[i] = std::uint8_t((nonce_counter >> (8 * i)) & 0xFF);
    for (int i = 8; i < 12; ++i)
      e.nonce[i] = 0;

    auto aad = make_aad(e);

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

  template <class Msg>
  Envelope make_envelope(MessageType type, const Msg &m, std::uint32_t flags = 0)
  {
    Envelope e;
    e.version = ProtocolVersion{1, 0};
    e.type = type;
    e.msg_id = next_message_id();
    e.flags = flags;
    e.payload = m.encode(); // plaintext
    return e;
  }

} // namespace vix::p2p::pack

#endif
