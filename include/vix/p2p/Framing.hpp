/**
 *
 *  @file Framing.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
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

  struct Frame
  {
    std::vector<std::uint8_t> bytes;
  };

  struct FrameDecodeResult
  {
    std::vector<Frame> frames;
    std::vector<std::uint8_t> remaining;
  };

  class Framing
  {
  public:
    virtual ~Framing() = default;
    virtual Frame encode(std::span<const std::uint8_t> payload) = 0;
    virtual FrameDecodeResult decode(std::span<const std::uint8_t> buffer) = 0;
  };

} // namespace vix::p2p

#endif
