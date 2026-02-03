/**
 *
 *  @file HelloFinish.hpp
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
#ifndef VIX_HELLO_FINISH_HPP
#define VIX_HELLO_FINISH_HPP

#include <span>
#include <vector>
#include <cstdint>

#include <vix/p2p/messages/Binary.hpp>

namespace vix::p2p::msg
{
  /**
   * @brief Handshake finalization message (initiator to responder).
   *
   * HelloFinish is the last message of the handshake v2 sequence.
   * It proves possession of the initiator private key and confirms
   * that both parties derived the same session parameters.
   */
  struct HelloFinish
  {
    /**
     * @brief Initiator nonce echoed from Hello.
     */
    std::uint64_t nonce_a{0};

    /**
     * @brief Responder nonce echoed from HelloAck.
     */
    std::uint64_t nonce_b{0};

    /**
     * @brief Signature over the handshake transcript.
     *
     * In secure modes, this is expected to be an Ed25519 signature.
     * In NullCrypto or development modes, this may be empty.
     */
    std::vector<std::uint8_t> signature;

    /**
     * @brief Encode the HelloFinish message into binary wire format.
     *
     * Encoding order:
     *   nonce_a(var_u64) | nonce_b(var_u64) | signature(bytes_var)
     *
     * @return Encoded bytes.
     */
    std::vector<std::uint8_t> encode() const
    {
      bin::Writer w;
      w.reserve(64);

      w.var_u64(nonce_a);
      w.var_u64(nonce_b);
      w.bytes_var(signature);

      return std::move(w.out);
    }

    /**
     * @brief Decode a HelloFinish message from binary wire bytes.
     *
     * @param bytes Input bytes.
     * @return Decoded HelloFinish message.
     *
     * @throws bin::Error if the input is malformed or contains trailing bytes.
     */
    static HelloFinish decode_or_throw(std::span<const std::uint8_t> bytes)
    {
      bin::Reader r(bytes);

      HelloFinish f;
      f.nonce_a = r.var_u64();
      f.nonce_b = r.var_u64();
      f.signature = r.bytes_var();

      if (r.remaining() != 0)
        throw bin::Error("HelloFinish: trailing bytes");

      return f;
    }
  };

} // namespace vix::p2p::msg

#endif // VIX_HELLO_FINISH_HPP
