/**
 *
 *  @file OutboxPull.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
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

  struct OutboxPull
  {
    std::string target_node_id;
    std::uint32_t max_items{128};

    std::vector<std::uint8_t> encode() const
    {
      bin::Writer w;
      w.str_var(target_node_id);
      w.u32_le(max_items);
      return std::move(w.out);
    }

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

#endif
