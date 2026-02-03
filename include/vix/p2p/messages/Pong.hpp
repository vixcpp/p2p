/**
 *
 *  @file Pong.hpp
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
#ifndef VIX_PONG_HPP
#define VIX_PONG_HPP

#include <cstdint>
#include <span>

#include <vix/p2p/messages/Binary.hpp>

namespace vix::p2p::msg
{
  /**
   * @brief Pong message used as a response to Ping.
   *
   * Pong echoes the nonce received in a Ping message and is used
   * to confirm peer liveness and optionally measure round-trip time.
   */
  struct Pong
  {
    /**
     * @brief Nonce echoed from the corresponding Ping message.
     */
    std::uint64_t nonce{0};

    /**
     * @brief Encode the Pong message into binary wire format.
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
     * @brief Decode a Pong message from binary wire bytes.
     *
     * @param bytes Input bytes.
     * @return Decoded Pong message.
     *
     * @throws bin::Error if the input is malformed or contains trailing bytes.
     */
    static Pong decode_or_throw(std::span<const std::uint8_t> bytes)
    {
      bin::Reader r(bytes);

      Pong p;
      p.nonce = r.u64_le();

      if (r.remaining() != 0)
        throw bin::Error("Pong: trailing bytes");

      return p;
    }
  };

} // namespace vix::p2p::msg

#endif // VIX_PONG_HPP
