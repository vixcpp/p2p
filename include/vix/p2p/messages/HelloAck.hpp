/**
 *
 *  @file HelloAck.hpp
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
#ifndef VIX_HELLO_ACK_HPP
#define VIX_HELLO_ACK_HPP

#include <span>
#include <vector>
#include <cstdint>

#include <vix/p2p/messages/Binary.hpp>

namespace vix::p2p::msg
{
  /**
   * @brief Handshake acknowledgement message (responder to initiator).
   *
   * HelloAck is the second message in the handshake v2 sequence.
   * It confirms receipt of the initiator Hello, echoes the initiator
   * nonce, introduces a responder challenge nonce, and provides the
   * responder public key required for session key derivation.
   */
  struct HelloAck
  {
    /**
     * @brief Echo of the initiator nonce.
     *
     * Used to bind the response to the original Hello message.
     */
    std::uint64_t nonce_a{0};

    /**
     * @brief Responder challenge nonce.
     *
     * Used by the initiator to prove liveness and complete the handshake.
     */
    std::uint64_t nonce_b{0};

    /**
     * @brief Responder public key bytes.
     *
     * Required by the initiator to derive the shared session key.
     */
    std::vector<std::uint8_t> public_key;

    /**
     * @brief Encode the HelloAck message into binary wire format.
     *
     * Encoding order:
     *   nonce_a(var_u64) | nonce_b(var_u64) | public_key(bytes_var)
     *
     * @return Encoded bytes.
     */
    std::vector<std::uint8_t> encode() const
    {
      bin::Writer w;
      w.reserve(64);

      w.var_u64(nonce_a);
      w.var_u64(nonce_b);
      w.bytes_var(public_key);

      return std::move(w.out);
    }

    /**
     * @brief Decode a HelloAck message from binary wire bytes.
     *
     * @param bytes Input bytes.
     * @return Decoded HelloAck message.
     *
     * @throws bin::Error if the input is malformed or contains trailing bytes.
     */
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

#endif // VIX_HELLO_ACK_HPP
