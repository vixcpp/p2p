/**
 *
 *  @file Ping.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
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

  struct Ping
  {
    std::uint64_t nonce{0};

    std::vector<std::uint8_t> encode() const
    {
      bin::Writer w;
      w.u64_le(nonce);
      return std::move(w.out);
    }

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

#endif
