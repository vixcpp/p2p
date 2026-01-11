#ifndef FRAMING_HPP
#define FRAMING_HPP

#include <cstdint>
#include <span>
#include <vector>
#include <optional>

namespace vix::p2p
{

    // (length-prefix / varint / checksum etc).
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