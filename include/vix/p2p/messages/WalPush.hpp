/**
 *
 *  @file WalPush.hpp
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
#ifndef VIX_WAL_PUSH_HPP
#define VIX_WAL_PUSH_HPP

#include <cstdint>
#include <vector>
#include <span>

#include <vix/p2p/messages/Binary.hpp>

namespace vix::p2p::msg
{
  /**
   * @brief Write-Ahead Log batch push message.
   *
   * WalPush transports a contiguous batch of WAL entries from one
   * peer to another. The batch is identified by an inclusive
   * sequence range and carries the serialized WAL bytes.
   *
   * This message is a core component of the offline-first
   * synchronization pipeline.
   */
  struct WalPush
  {
    /**
     * @brief First sequence number in the batch (inclusive).
     */
    std::uint64_t seq_begin{0};

    /**
     * @brief Last sequence number in the batch (inclusive).
     */
    std::uint64_t seq_end{0};

    /**
     * @brief Serialized WAL batch bytes.
     *
     * The encoding of these bytes is implementation-defined
     * and interpreted by the EdgeSync/WAL layer.
     */
    std::vector<std::uint8_t> wal_bytes;

    /**
     * @brief Encode the WalPush message into binary wire format.
     *
     * Encoding order:
     *   seq_begin(u64_le) | seq_end(u64_le) | wal_bytes(bytes_var)
     *
     * @return Encoded bytes.
     */
    std::vector<std::uint8_t> encode() const
    {
      bin::Writer w;
      w.u64_le(seq_begin);
      w.u64_le(seq_end);
      w.bytes_var(wal_bytes);
      return std::move(w.out);
    }

    /**
     * @brief Decode a WalPush message from binary wire bytes.
     *
     * @param bytes Input bytes.
     * @return Decoded WalPush message.
     *
     * @throws bin::Error if the input is malformed or contains trailing bytes.
     */
    static WalPush decode_or_throw(std::span<const std::uint8_t> bytes)
    {
      bin::Reader r(bytes);

      WalPush m;
      m.seq_begin = r.u64_le();
      m.seq_end = r.u64_le();
      m.wal_bytes = r.bytes_var();

      if (r.remaining() != 0)
        throw bin::Error("WalPush: trailing bytes");

      return m;
    }
  };

} // namespace vix::p2p::msg

#endif // VIX_WAL_PUSH_HPP
