/**
 *
 *  @file WalAck.hpp
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
#ifndef VIX_WAL_ACK_HPP
#define VIX_WAL_ACK_HPP

#include <cstdint>
#include <span>

#include <vix/p2p/messages/Binary.hpp>

namespace vix::p2p::msg
{
  /**
   * @brief Write-Ahead Log acknowledgment message.
   *
   * WalAck is sent in response to a WalPush message to confirm
   * the highest sequence number that has been successfully
   * applied by the receiver.
   */
  struct WalAck
  {
    /**
     * @brief Last applied WAL sequence number.
     *
     * Indicates that all WAL entries up to and including this
     * sequence have been processed.
     */
    std::uint64_t last_applied_seq{0};

    /**
     * @brief Encode the WalAck message into binary wire format.
     *
     * Encoding order:
     *   last_applied_seq(u64_le)
     *
     * @return Encoded bytes.
     */
    std::vector<std::uint8_t> encode() const
    {
      bin::Writer w;
      w.u64_le(last_applied_seq);
      return std::move(w.out);
    }

    /**
     * @brief Decode a WalAck message from binary wire bytes.
     *
     * @param bytes Input bytes.
     * @return Decoded WalAck message.
     *
     * @throws bin::Error if the input is malformed or contains trailing bytes.
     */
    static WalAck decode_or_throw(std::span<const std::uint8_t> bytes)
    {
      bin::Reader r(bytes);

      WalAck a;
      a.last_applied_seq = r.u64_le();

      if (r.remaining() != 0)
        throw bin::Error("WalAck: trailing bytes");

      return a;
    }
  };

} // namespace vix::p2p::msg

#endif // VIX_WAL_ACK_HPP
