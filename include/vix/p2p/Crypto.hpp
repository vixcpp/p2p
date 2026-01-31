/**
 *
 *  @file Bootstrap.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#ifndef VIX_CRYPTO_HPP
#define VIX_CRYPTO_HPP

#include <cstdint>
#include <span>
#include <vector>
#include <string>
#include <array>
#include <random>

namespace vix::p2p
{
  struct KeyPair
  {
    std::vector<std::uint8_t> public_key;
    std::vector<std::uint8_t> private_key;
  };

  class Crypto
  {
  public:
    virtual ~Crypto() = default;

    virtual KeyPair generate_keypair() = 0;

    virtual std::vector<std::uint8_t> sign(
        std::span<const std::uint8_t> data,
        std::span<const std::uint8_t> private_key) = 0;

    virtual bool verify(
        std::span<const std::uint8_t> data,
        std::span<const std::uint8_t> signature,
        std::span<const std::uint8_t> public_key) = 0;

    virtual std::vector<std::uint8_t> encrypt(
        std::span<const std::uint8_t> plaintext) = 0;

    virtual std::vector<std::uint8_t> decrypt(
        std::span<const std::uint8_t> ciphertext) = 0;

    virtual std::vector<std::uint8_t> kdf_32(std::span<const std::uint8_t> transcript) = 0;

    virtual std::vector<std::uint8_t> aead_encrypt(
        std::span<const std::uint8_t> key32,
        std::span<const std::uint8_t> nonce12,
        std::span<const std::uint8_t> aad,
        std::span<const std::uint8_t> plaintext,
        std::array<std::uint8_t, 16> &out_tag) = 0;

    virtual std::vector<std::uint8_t> aead_decrypt(
        std::span<const std::uint8_t> key32,
        std::span<const std::uint8_t> nonce12,
        std::span<const std::uint8_t> aad,
        std::span<const std::uint8_t> ciphertext,
        std::span<const std::uint8_t> tag16) = 0;
  };

  class NullCrypto final : public Crypto
  {
  public:
    KeyPair generate_keypair() override
    {
      KeyPair kp;
      kp.public_key.resize(32);
      kp.private_key.resize(32);

      static thread_local std::mt19937_64 rng{std::random_device{}()};

      auto fill = [&](std::vector<std::uint8_t> &v)
      {
        for (size_t i = 0; i < v.size(); ++i)
          v[i] = static_cast<std::uint8_t>(rng() & 0xFF);
      };

      fill(kp.public_key);
      fill(kp.private_key);

      bool all0 = true;
      for (auto b : kp.public_key)
      {
        if (b != 0)
        {
          all0 = false;
          break;
        }
      }
      if (all0)
        kp.public_key[0] = 1;

      return kp;
    }

    std::vector<std::uint8_t> sign(std::span<const std::uint8_t> data,
                                   std::span<const std::uint8_t> private_key) override
    {
      // DEV: signature = kdf_32(data || priv) (non secure)
      std::vector<std::uint8_t> t;
      t.reserve(data.size() + private_key.size());
      t.insert(t.end(), data.begin(), data.end());
      t.insert(t.end(), private_key.begin(), private_key.end());
      return kdf_32(t);
    }

    bool verify(std::span<const std::uint8_t> data,
                std::span<const std::uint8_t> signature,
                std::span<const std::uint8_t> public_key) override
    {
      (void)data;
      (void)public_key;
      return !signature.empty();
    }

    std::vector<std::uint8_t> encrypt(std::span<const std::uint8_t> plaintext) override
    {
      return {plaintext.begin(), plaintext.end()};
    }

    std::vector<std::uint8_t> decrypt(std::span<const std::uint8_t> ciphertext) override
    {
      return {ciphertext.begin(), ciphertext.end()};
    }

    std::vector<std::uint8_t> kdf_32(std::span<const std::uint8_t> transcript) override
    {
      std::vector<std::uint8_t> out(32, 0);

      std::uint64_t x = 1469598103934665603ULL;
      for (auto b : transcript)
      {
        x ^= static_cast<std::uint8_t>(b);
        x *= 1099511628211ULL;
      }

      for (size_t i = 0; i < out.size(); ++i)
      {
        x ^= (x >> 33);
        x *= 0xff51afd7ed558ccdULL;
        out[i] = static_cast<std::uint8_t>((x >> ((i % 8) * 8)) & 0xFF);
      }

      return out;
    }

    std::vector<std::uint8_t> aead_encrypt(
        std::span<const std::uint8_t> key32,
        std::span<const std::uint8_t> nonce12,
        std::span<const std::uint8_t> aad,
        std::span<const std::uint8_t> plaintext,
        std::array<std::uint8_t, 16> &out_tag) override
    {
      // DEV: "xor-stream" fake + tag fake (non secure)
      std::vector<std::uint8_t> ct(plaintext.begin(), plaintext.end());

      std::uint64_t x = 1469598103934665603ULL;
      for (auto b : key32)
      {
        x ^= b;
        x *= 1099511628211ULL;
      }
      for (auto b : nonce12)
      {
        x ^= b;
        x *= 1099511628211ULL;
      }
      for (auto b : aad)
      {
        x ^= b;
        x *= 1099511628211ULL;
      }

      for (size_t i = 0; i < ct.size(); ++i)
      {
        x ^= (x >> 33);
        x *= 0xff51afd7ed558ccdULL;
        ct[i] ^= static_cast<std::uint8_t>(x & 0xFF);
      }

      // fake tag = kdf_32(key||nonce||aad||ct) first 16 bytes
      std::vector<std::uint8_t> t;
      t.insert(t.end(), key32.begin(), key32.end());
      t.insert(t.end(), nonce12.begin(), nonce12.end());
      t.insert(t.end(), aad.begin(), aad.end());
      t.insert(t.end(), ct.begin(), ct.end());
      auto h = kdf_32(t);
      for (size_t i = 0; i < 16; ++i)
        out_tag[i] = h[i];

      return ct;
    }

    std::vector<std::uint8_t> aead_decrypt(
        std::span<const std::uint8_t> key32,
        std::span<const std::uint8_t> nonce12,
        std::span<const std::uint8_t> aad,
        std::span<const std::uint8_t> ciphertext,
        std::span<const std::uint8_t> tag16) override
    {
      // verify fake tag
      std::vector<std::uint8_t> t;
      t.insert(t.end(), key32.begin(), key32.end());
      t.insert(t.end(), nonce12.begin(), nonce12.end());
      t.insert(t.end(), aad.begin(), aad.end());
      t.insert(t.end(), ciphertext.begin(), ciphertext.end());
      auto h = kdf_32(t);

      if (tag16.size() != 16)
        return {};
      for (size_t i = 0; i < 16; ++i)
        if (tag16[i] != h[i])
          return {}; // auth failed

      // decrypt = same xor
      std::vector<std::uint8_t> pt(ciphertext.begin(), ciphertext.end());

      std::uint64_t x = 1469598103934665603ULL;
      for (auto b : key32)
      {
        x ^= b;
        x *= 1099511628211ULL;
      }
      for (auto b : nonce12)
      {
        x ^= b;
        x *= 1099511628211ULL;
      }
      for (auto b : aad)
      {
        x ^= b;
        x *= 1099511628211ULL;
      }

      for (size_t i = 0; i < pt.size(); ++i)
      {
        x ^= (x >> 33);
        x *= 0xff51afd7ed558ccdULL;
        pt[i] ^= static_cast<std::uint8_t>(x & 0xFF);
      }

      return pt;
    }
  };

} // namespace vix::p2p

#endif
