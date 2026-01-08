#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <string>

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
        KeyPair generate_keypair() override { return {}; }

        std::vector<std::uint8_t> sign(std::span<const std::uint8_t>, std::span<const std::uint8_t>) override
        {
            return {};
        }

        bool verify(std::span<const std::uint8_t>, std::span<const std::uint8_t>, std::span<const std::uint8_t>) override
        {
            return true;
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
