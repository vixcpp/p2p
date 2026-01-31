/**
 *
 *  @file HelloAck.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#ifndef VIX_HELLO_ACK_HPP
#define VIX_HELLO_ACK_HPP

#include <span>
#include <vector>
#include <cstdint>

#include <vix/p2p/messages/Binary.hpp>

namespace vix::p2p::msg
{
  // HelloAck (responder -> initiator)
  struct HelloAck
  {
    std::uint64_t nonce_a{0}; // echo
    std::uint64_t nonce_b{0}; // challenge

    // NEW: responder public key (needed by initiator to derive session key)
    std::vector<std::uint8_t> public_key;

    std::vector<std::uint8_t> encode() const
    {
      bin::Writer w;
      w.reserve(64);
      w.var_u64(nonce_a);
      w.var_u64(nonce_b);
      w.bytes_var(public_key);
      return std::move(w.out);
    }

    static HelloAck decode_or_throw(std::span<const std::uint8_t> bytes)
    {
      bin::Reader r(bytes);
      HelloAck a;
      a.nonce_a = r.var_u64();
      a.nonce_b = r.var_u64();
      a.public_key = r.bytes_var();

      if (r.remaining() != 0)
        throw bin::Error("HelloAck: trailing bytes");
      return a;
    }
  };
} // namespace vix::p2p::msg

#endif
