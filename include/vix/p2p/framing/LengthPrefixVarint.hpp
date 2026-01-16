/**
 *
 *  @file LengthPrefixVarint.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#ifndef VIX_LENGTH_PREFIX_VARINT_HPP
#define VIX_LENGTH_PREFIX_VARINT_HPP

#include <cstdint>
#include <span>
#include <vector>
#include <vix/p2p/Framing.hpp>
#include <vix/p2p/messages/Binary.hpp>

namespace vix::p2p::framing
{

  class LengthPrefixVarint final : public vix::p2p::Framing
  {
  public:
    Frame encode(std::span<const std::uint8_t> payload) override
    {
      vix::p2p::bin::Writer w;
      w.reserve(payload.size() + 10);
      w.var_u64((std::uint64_t)payload.size());
      w.bytes(payload);
      Frame f;
      f.bytes = std::move(w.out);
      return f;
    }

    FrameDecodeResult decode(std::span<const std::uint8_t> buffer) override
    {
      FrameDecodeResult res;

      std::vector<std::uint8_t> work(buffer.begin(), buffer.end());
      std::size_t off = 0;

      while (true)
      {
        std::uint64_t len = 0;
        int shift = 0;
        std::size_t i = 0;

        bool done = false;
        for (; i < 10; ++i)
        {
          if (off + i >= work.size())
          {
            done = false;
            break;
          }
          std::uint8_t byte = work[off + i];
          len |= (std::uint64_t)(byte & 0x7F) << shift;
          if ((byte & 0x80) == 0)
          {
            done = true;
            break;
          }
          shift += 7;
        }

        if (!done)
        {
          res.remaining.assign(work.begin() + (std::ptrdiff_t)off, work.end());
          return res;
        }

        const std::size_t varint_len = i + 1;
        if (len > (std::uint64_t)std::numeric_limits<std::size_t>::max())
        {
          throw vix::p2p::bin::Error("LengthPrefixVarint: length overflow");
        }

        const std::size_t needed = varint_len + (std::size_t)len;
        if (off + needed > work.size())
        {
          res.remaining.assign(work.begin() + (std::ptrdiff_t)off, work.end());
          return res;
        }

        Frame f;
        auto payload_begin = work.begin() + (std::ptrdiff_t)(off + varint_len);
        auto payload_end = payload_begin + (std::ptrdiff_t)len;
        f.bytes.assign(payload_begin, payload_end);
        res.frames.push_back(std::move(f));

        off += needed;
        if (off >= work.size())
          return res;
      }
    }
  };

} // namespace vix::p2p::framing

#endif
