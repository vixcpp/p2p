/**
 *
 *  @file Binary.hpp
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
#ifndef VIX_BINARY_HPP
#define VIX_BINARY_HPP

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <limits>
#include <optional>

namespace vix::p2p::bin
{
  /**
   * @brief Binary encoding/decoding error.
   */
  struct Error : std::runtime_error
  {
    using std::runtime_error::runtime_error;
  };

  /**
   * @brief Simple binary reader over a byte span.
   *
   * Supports:
   * - fixed-width little-endian integers
   * - unsigned LEB128-style varint (var_u64)
   * - length-prefixed byte arrays and strings (varint length)
   */
  struct Reader
  {
    /// Input buffer
    std::span<const std::uint8_t> s;

    /// Current read offset
    std::size_t off{0};

    /**
     * @brief Construct a reader over a byte buffer.
     *
     * @param bytes Input byte span.
     */
    explicit Reader(std::span<const std::uint8_t> bytes) : s(bytes) {}

    /**
     * @brief Number of remaining unread bytes.
     *
     * @return Remaining size in bytes.
     */
    std::size_t remaining() const { return s.size() - off; }

    /**
     * @brief Ensure at least n bytes are available.
     *
     * @param n Required number of bytes.
     * @throws Error if the buffer does not contain enough bytes.
     */
    void require(std::size_t n)
    {
      if (remaining() < n)
        throw Error("bin::Reader: buffer underflow");
    }

    /**
     * @brief Read an unsigned 8-bit integer.
     *
     * @return Read value.
     */
    std::uint8_t u8()
    {
      require(1);
      return s[off++];
    }

    /**
     * @brief Read an unsigned 16-bit little-endian integer.
     *
     * @return Read value.
     */
    std::uint16_t u16_le()
    {
      require(2);

      const std::uint16_t b0 = static_cast<std::uint16_t>(s[off]);
      const std::uint16_t b1 = static_cast<std::uint16_t>(s[off + 1]);

      const std::uint16_t v = static_cast<std::uint16_t>(b0 | static_cast<std::uint16_t>(b1 << 8));

      off += 2;
      return v;
    }

    /**
     * @brief Read an unsigned 32-bit little-endian integer.
     *
     * @return Read value.
     */
    std::uint32_t u32_le()
    {
      require(4);
      std::uint32_t v =
          (std::uint32_t)s[off] |
          ((std::uint32_t)s[off + 1] << 8) |
          ((std::uint32_t)s[off + 2] << 16) |
          ((std::uint32_t)s[off + 3] << 24);
      off += 4;
      return v;
    }

    /**
     * @brief Read an unsigned 64-bit little-endian integer.
     *
     * @return Read value.
     */
    std::uint64_t u64_le()
    {
      require(8);

      std::uint64_t v = 0;
      for (std::size_t i = 0; i < 8; ++i)
      {
        v |= (static_cast<std::uint64_t>(s[off + i]) << (8u * i));
      }

      off += 8;
      return v;
    }

    /**
     * @brief Read an unsigned LEB128-style varint.
     *
     * Maximum length is 10 bytes for 64-bit values.
     *
     * @return Decoded value.
     * @throws Error if the varint is too long.
     */
    std::uint64_t var_u64()
    {
      std::uint64_t result = 0;
      int shift = 0;
      for (int i = 0; i < 10; ++i)
      {
        auto byte = u8();
        result |= (std::uint64_t)(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0)
          return result;
        shift += 7;
      }
      throw Error("bin::Reader: varint too long");
    }

    /**
     * @brief Read exactly n bytes and return them in a vector.
     *
     * @param n Number of bytes to read.
     * @return Byte vector of size n.
     * @throws Error if not enough bytes remain.
     */
    std::vector<std::uint8_t> bytes_n(std::size_t n)
    {
      require(n);
      std::vector<std::uint8_t> out(
          s.begin() + static_cast<std::ptrdiff_t>(off),
          s.begin() + static_cast<std::ptrdiff_t>(off) + static_cast<std::ptrdiff_t>(n));
      off += n;
      return out;
    }

    /**
     * @brief Read a varint-length-prefixed byte array.
     *
     * Encoded format:
     *   [var_u64 length][length bytes]
     *
     * @return Byte vector.
     * @throws Error if the length exceeds remaining bytes.
     */
    std::vector<std::uint8_t> bytes_var()
    {
      auto n = static_cast<std::size_t>(var_u64());
      if (n > remaining())
        throw Error("bin::Reader: bytes_var too large");
      return bytes_n(n);
    }

    /**
     * @brief Read a varint-length-prefixed string.
     *
     * Encoded format:
     *   [var_u64 length][length bytes]
     *
     * @return Decoded string.
     * @throws Error if the length exceeds remaining bytes.
     */
    std::string str_var()
    {
      auto n = static_cast<std::size_t>(var_u64());
      if (n > remaining())
        throw Error("bin::Reader: str_var too large");
      std::string out(reinterpret_cast<const char *>(s.data()) + off, n);
      off += n;
      return out;
    }
  };

  /**
   * @brief Simple binary writer producing a byte vector.
   *
   * Supports:
   * - fixed-width little-endian integers
   * - unsigned LEB128-style varint (var_u64)
   * - writing raw bytes, varint-length-prefixed bytes and strings
   */
  struct Writer
  {
    /// Output byte buffer
    std::vector<std::uint8_t> out;

    /**
     * @brief Reserve output capacity.
     *
     * @param n Capacity to reserve.
     */
    void reserve(std::size_t n) { out.reserve(n); }

    /**
     * @brief Write an unsigned 8-bit integer.
     *
     * @param v Value.
     */
    void u8(std::uint8_t v) { out.push_back(v); }

    /**
     * @brief Write an unsigned 16-bit little-endian integer.
     *
     * @param v Value.
     */
    void u16_le(std::uint16_t v)
    {
      out.push_back(static_cast<std::uint8_t>(v & 0xFF));
      out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    }

    /**
     * @brief Write an unsigned 32-bit little-endian integer.
     *
     * @param v Value.
     */
    void u32_le(std::uint32_t v)
    {
      out.push_back(static_cast<std::uint8_t>(v & 0xFF));
      out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
      out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
      out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
    }

    /**
     * @brief Write an unsigned 64-bit little-endian integer.
     *
     * @param v Value.
     */
    void u64_le(std::uint64_t v)
    {
      for (std::size_t i = 0; i < 8; ++i)
        out.push_back(static_cast<std::uint8_t>((v >> (8u * i)) & 0xFFu));
    }

    /**
     * @brief Write an unsigned LEB128-style varint.
     *
     * @param v Value.
     */
    void var_u64(std::uint64_t v)
    {
      while (true)
      {
        std::uint8_t byte = static_cast<std::uint8_t>(v & 0x7F);
        v >>= 7;
        if (v == 0)
        {
          out.push_back(byte);
          return;
        }
        out.push_back(static_cast<std::uint8_t>(byte | 0x80));
      }
    }

    /**
     * @brief Append raw bytes.
     *
     * @param b Input byte span.
     */
    void bytes(std::span<const std::uint8_t> b)
    {
      out.insert(out.end(), b.begin(), b.end());
    }

    /**
     * @brief Write a varint-length-prefixed byte array.
     *
     * Encoded format:
     *   [var_u64 length][length bytes]
     *
     * @param b Input byte span.
     */
    void bytes_var(std::span<const std::uint8_t> b)
    {
      var_u64(static_cast<std::uint64_t>(b.size()));
      bytes(b);
    }

    /**
     * @brief Write a varint-length-prefixed string.
     *
     * Encoded format:
     *   [var_u64 length][length bytes]
     *
     * @param s Input string view.
     */
    void str_var(std::string_view s)
    {
      var_u64(static_cast<std::uint64_t>(s.size()));
      out.insert(out.end(), s.begin(), s.end());
    }
  };

} // namespace vix::p2p::bin

#endif // VIX_BINARY_HPP
