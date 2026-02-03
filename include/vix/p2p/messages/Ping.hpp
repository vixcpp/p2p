/**
 *
 *  @file Ping.hpp
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
#ifndef VIX_PING_HPP
#define VIX_PING_HPP

#include <cstdint>
#include <span>

#include <vix/p2p/messages/Binary.hpp>

namespace vix::p2p::msg
{
  /**
   * @brief Ping message used for liveness and latency checks.
   *
   * Ping is a lightweight control message exchanged between peers
   * to verify connectivity and measure round-trip time. It is
   * typically answered with a corresponding Pong message that
   * echoes the nonce.
   */
  struct Ping
  {
    /**
     * @brief Random nonce used to correlate Ping and Pong messages.
     */
    std::uint64_t nonce{0};

    /**
     * @brief Encode the Ping message into binary wire format.
     *
     * Encoding order:
     *   nonce(u64_le)
     *
     * @return Encoded bytes.
     */
    std::vector<std::uint8_t> encode() const
    {
      bin::Writer w;
      w.u64_le(nonce);
      return std::move(w.out);
    }

    /**
     * @brief Decode a Ping message from binary wire bytes.
     *
     * @param bytes Input bytes.
     * @return Decoded Ping message.
     *
     * @throws bin::Error if the input is malformed or contains trailing bytes.
     */
    static Ping decode_or_throw(std::span<const std::uint8_t> bytes)
    {
      bin::Reader r(bytes);

      Ping p;
      p.nonce = r.u64_le();

      if (r.remaining() != 0)
        throw bin::Error("Ping: trailing bytes");

      return p;
    }
  };

} // namespace vix::p2p::msg

#endif // VIX_PING_HPP
