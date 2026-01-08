#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace vix::p2p
{

    struct DiscoveryAnnouncement
    {
        std::string node_id;
        std::string host;
        std::uint16_t port{0};
        std::string transport; // "tcp"
    };

    class Discovery
    {
    public:
        virtual ~Discovery() = default;

        virtual void start() = 0;
        virtual void stop() = 0;

        virtual std::vector<DiscoveryAnnouncement> snapshot() const = 0;
    };

    class NullDiscovery final : public Discovery
    {
    public:
        void start() override {}
        void stop() override {}
        std::vector<DiscoveryAnnouncement> snapshot() const override { return {}; }
    };

} // namespace vix::p2p
