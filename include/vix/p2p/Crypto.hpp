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
    };

} // namespace vix::p2p
