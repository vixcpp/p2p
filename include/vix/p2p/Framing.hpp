/**
 *
 *  @file Framing.hpp
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
#ifndef VIX_FRAMING_HPP
#define VIX_FRAMING_HPP

#include <cstdint>
#include <span>
#include <vector>
#include <optional>

namespace vix::p2p
{
  /**
   * @brief Encoded transport frame.
   *
   * Represents a single framed payload ready for transmission.
   */
  struct Frame
  {
    /// Raw encoded frame bytes
    std::vector<std::uint8_t> bytes;
  };

  /**
   * @brief Result of a frame decoding operation.
   *
   * Contains fully decoded frames and any remaining
   * incomplete bytes that should be kept for the next decode call.
   */
  struct FrameDecodeResult
  {
    /// Successfully decoded frames
    std::vector<Frame> frames;

    /// Remaining undecoded bytes
    std::vector<std::uint8_t> remaining;
  };

  /**
   * @brief Framing abstraction for P2P transports.
   *
   * Responsible for turning raw payloads into framed byte streams
   * and extracting frames from a continuous input buffer.
   */
  class Framing
  {
  public:
    /// Virtual destructor
    virtual ~Framing() = default;

    /**
     * @brief Encode a payload into a transport frame.
     *
     * @param payload Raw payload bytes.
     * @return Encoded frame.
     */
    virtual Frame encode(std::span<const std::uint8_t> payload) = 0;

    /**
     * @brief Decode frames from an input buffer.
     *
     * Implementations may return zero or more frames and must
     * preserve any incomplete trailing bytes.
     *
     * @param buffer Input byte buffer.
     * @return Decoding result containing frames and remaining bytes.
     */
    virtual FrameDecodeResult decode(std::span<const std::uint8_t> buffer) = 0;
  };

} // namespace vix::p2p

#endif // VIX_FRAMING_HPP
