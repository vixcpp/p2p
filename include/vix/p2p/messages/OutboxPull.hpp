/**
 *
 *  @file OutboxPull.hpp
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
#ifndef VIX_OUTBOX_PULL_HPP
#define VIX_OUTBOX_PULL_HPP

#include <cstdint>
#include <string>
#include <span>

#include <vix/p2p/messages/Binary.hpp>

namespace vix::p2p::msg
{
  /**
   * @brief Request message for pulling outbox entries from a peer.
   *
   * OutboxPull is used during synchronization to request pending
   * operations destined for a specific target node.
   */
  struct OutboxPull
  {
    /**
     * @brief Identifier of the target node whose outbox is requested.
     */
    std::string target_node_id;

    /**
     * @brief Maximum number of items to return.
     *
     * Acts as a flow-control and batching hint.
     */
    std::uint32_t max_items{128};

    /**
     * @brief Encode the OutboxPull message into binary wire format.
     *
     * Encoding order:
     *   target_node_id(str_var) | max_items(u32_le)
     *
     * @return Encoded bytes.
     */
    std::vector<std::uint8_t> encode() const
    {
      bin::Writer w;
      w.str_var(target_node_id);
      w.u32_le(max_items);
      return std::move(w.out);
    }

    /**
     * @brief Decode an OutboxPull message from binary wire bytes.
     *
     * @param bytes Input bytes.
     * @return Decoded OutboxPull message.
     *
     * @throws bin::Error if the input is malformed or contains trailing bytes.
     */
    static OutboxPull decode_or_throw(std::span<const std::uint8_t> bytes)
    {
      bin::Reader r(bytes);

      OutboxPull m;
      m.target_node_id = r.str_var();
      m.max_items = r.u32_le();

      if (r.remaining() != 0)
        throw bin::Error("OutboxPull: trailing bytes");

      return m;
    }
  };

} // namespace vix::p2p::msg

#endif // VIX_OUTBOX_PULL_HPP
