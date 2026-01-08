#pragma once
#include <cstdint>
#include <span>
#include <string>

namespace vix::p2p
{
    enum class TransportKind : std::uint8_t
    {
        Tcp = 1,
        Quic = 2
    };

    struct TransportStats
    {
        std::uint64_t bytes_sent{0};
        std::uint64_t bytes_received{0};
        std::uint64_t frames_sent{0};
        std::uint64_t frames_received{0};
    };

    class Transport
    {
    public:
        virtual ~Transport() = default;

        virtual TransportKind kind() const = 0;

        virtual bool send(std::span<const std::uint8_t> frame) = 0;
        virtual void close() = 0;
        virtual TransportStats stats() const = 0;
        virtual std::string endpoint_string() const = 0;
    };

} // namespace vix::p2p
