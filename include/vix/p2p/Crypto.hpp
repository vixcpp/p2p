/**
 *
 *  @file Crypto.hpp
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
  /**
   * @brief Public and private key pair.
   *
   * The exact key format depends on the active Crypto implementation.
   */
  struct KeyPair
  {
    /// Public key bytes
    std::vector<std::uint8_t> public_key;

    /// Private key bytes
    std::vector<std::uint8_t> private_key;
  };

  /**
   * @brief Abstract cryptographic provider for P2P.
   *
   * This interface centralizes identity, signatures, encryption,
   * and authenticated encryption (AEAD) for the P2P stack.
   *
   * Concrete implementations may use libraries such as OpenSSL,
   * libsodium, BoringSSL, or platform-specific crypto backends.
   */
  class Crypto
  {
  public:
    /// Virtual destructor
    virtual ~Crypto() = default;

    /**
     * @brief Generate a new key pair.
     *
     * @return Generated public/private key pair.
     */
    virtual KeyPair generate_keypair() = 0;

    /**
     * @brief Create a signature over a message.
     *
     * @param data Message bytes to sign.
     * @param private_key Private key bytes.
     * @return Signature bytes.
     */
    virtual std::vector<std::uint8_t> sign(
        std::span<const std::uint8_t> data,
        std::span<const std::uint8_t> private_key) = 0;

    /**
     * @brief Verify a signature for a message.
     *
     * @param data Message bytes.
     * @param signature Signature bytes.
     * @param public_key Public key bytes.
     * @return true if signature is valid.
     */
    virtual bool verify(
        std::span<const std::uint8_t> data,
        std::span<const std::uint8_t> signature,
        std::span<const std::uint8_t> public_key) = 0;

    /**
     * @brief Encrypt a plaintext payload.
     *
     * Implementations may use an internal session key or an internal
     * policy appropriate for the current handshake/transport.
     *
     * @param plaintext Plaintext bytes.
     * @return Ciphertext bytes.
     */
    virtual std::vector<std::uint8_t> encrypt(
        std::span<const std::uint8_t> plaintext) = 0;

    /**
     * @brief Decrypt a ciphertext payload.
     *
     * @param ciphertext Ciphertext bytes.
     * @return Plaintext bytes, or empty vector on failure.
     */
    virtual std::vector<std::uint8_t> decrypt(
        std::span<const std::uint8_t> ciphertext) = 0;

    /**
     * @brief Derive a 32-byte key from a transcript.
     *
     * This helper is typically used during handshake and session
     * establishment.
     *
     * @param transcript Transcript bytes.
     * @return Derived key bytes (32 bytes).
     */
    virtual std::vector<std::uint8_t>
    kdf_32(std::span<const std::uint8_t> transcript) = 0;

    /**
     * @brief Encrypt using AEAD and produce an authentication tag.
     *
     * @param key32 32-byte key.
     * @param nonce12 12-byte nonce.
     * @param aad Additional authenticated data.
     * @param plaintext Plaintext bytes.
     * @param out_tag Output authentication tag (16 bytes).
     * @return Ciphertext bytes.
     */
    virtual std::vector<std::uint8_t> aead_encrypt(
        std::span<const std::uint8_t> key32,
        std::span<const std::uint8_t> nonce12,
        std::span<const std::uint8_t> aad,
        std::span<const std::uint8_t> plaintext,
        std::array<std::uint8_t, 16> &out_tag) = 0;

    /**
     * @brief Decrypt using AEAD after verifying the authentication tag.
     *
     * @param key32 32-byte key.
     * @param nonce12 12-byte nonce.
     * @param aad Additional authenticated data.
     * @param ciphertext Ciphertext bytes.
     * @param tag16 Authentication tag (16 bytes).
     * @return Plaintext bytes, or empty vector on authentication failure.
     */
    virtual std::vector<std::uint8_t> aead_decrypt(
        std::span<const std::uint8_t> key32,
        std::span<const std::uint8_t> nonce12,
        std::span<const std::uint8_t> aad,
        std::span<const std::uint8_t> ciphertext,
        std::span<const std::uint8_t> tag16) = 0;
  };

  /**
   * @brief Development-only Crypto implementation.
   *
   * NullCrypto provides placeholder cryptography intended only for
   * local testing, wiring, and early development.
   *
   * It is not secure and must never be used for production workloads.
   */
  class NullCrypto final : public Crypto
  {
  public:
    /**
     * @brief Generate a pseudo-random key pair.
     *
     * Produces 32-byte public and private key buffers using a
     * local PRNG. This is intended only for development.
     *
     * @return Generated key pair.
     */
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

    /**
     * @brief Create a development signature over (data || private_key).
     *
     * This is a placeholder algorithm and is not cryptographically secure.
     *
     * @param data Message bytes.
     * @param private_key Private key bytes.
     * @return Signature bytes.
     */
    std::vector<std::uint8_t> sign(std::span<const std::uint8_t> data,
                                   std::span<const std::uint8_t> private_key) override
    {
      std::vector<std::uint8_t> t;
      t.reserve(data.size() + private_key.size());
      t.insert(t.end(), data.begin(), data.end());
      t.insert(t.end(), private_key.begin(), private_key.end());
      return kdf_32(t);
    }

    /**
     * @brief Development signature verification.
     *
     * Currently only checks that the signature is not empty.
     * This is not secure and is intended only for wiring tests.
     *
     * @param data Message bytes.
     * @param signature Signature bytes.
     * @param public_key Public key bytes.
     * @return true if signature is accepted.
     */
    bool verify(std::span<const std::uint8_t> data,
                std::span<const std::uint8_t> signature,
                std::span<const std::uint8_t> public_key) override
    {
      (void)data;
      (void)public_key;
      return !signature.empty();
    }

    /**
     * @brief Development encryption passthrough.
     *
     * Returns the plaintext unchanged.
     *
     * @param plaintext Plaintext bytes.
     * @return Ciphertext bytes.
     */
    std::vector<std::uint8_t> encrypt(std::span<const std::uint8_t> plaintext) override
    {
      return {plaintext.begin(), plaintext.end()};
    }

    /**
     * @brief Development decryption passthrough.
     *
     * Returns the ciphertext unchanged.
     *
     * @param ciphertext Ciphertext bytes.
     * @return Plaintext bytes.
     */
    std::vector<std::uint8_t> decrypt(std::span<const std::uint8_t> ciphertext) override
    {
      return {ciphertext.begin(), ciphertext.end()};
    }

    /**
     * @brief Development 32-byte KDF.
     *
     * Produces a deterministic 32-byte output from the transcript.
     * This is not a cryptographically secure KDF.
     *
     * @param transcript Transcript bytes.
     * @return Derived bytes (32 bytes).
     */
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

    /**
     * @brief Development AEAD encrypt.
     *
     * Applies a reversible xor-stream transformation and returns a fake
     * authentication tag derived from (key || nonce || aad || ciphertext).
     *
     * This is not secure and is intended only for development.
     *
     * @param key32 32-byte key.
     * @param nonce12 12-byte nonce.
     * @param aad Additional authenticated data.
     * @param plaintext Plaintext bytes.
     * @param out_tag Output authentication tag (16 bytes).
     * @return Ciphertext bytes.
     */
    std::vector<std::uint8_t> aead_encrypt(
        std::span<const std::uint8_t> key32,
        std::span<const std::uint8_t> nonce12,
        std::span<const std::uint8_t> aad,
        std::span<const std::uint8_t> plaintext,
        std::array<std::uint8_t, 16> &out_tag) override
    {
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

    /**
     * @brief Development AEAD decrypt.
     *
     * Verifies the fake tag derived from (key || nonce || aad || ciphertext).
     * If the tag matches, applies the reversible xor-stream transformation
     * and returns plaintext.
     *
     * This is not secure and is intended only for development.
     *
     * @param key32 32-byte key.
     * @param nonce12 12-byte nonce.
     * @param aad Additional authenticated data.
     * @param ciphertext Ciphertext bytes.
     * @param tag16 Authentication tag (16 bytes).
     * @return Plaintext bytes, or empty vector if authentication fails.
     */
    std::vector<std::uint8_t> aead_decrypt(
        std::span<const std::uint8_t> key32,
        std::span<const std::uint8_t> nonce12,
        std::span<const std::uint8_t> aad,
        std::span<const std::uint8_t> ciphertext,
        std::span<const std::uint8_t> tag16) override
    {
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
          return {};

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

#endif // VIX_CRYPTO_HPP
