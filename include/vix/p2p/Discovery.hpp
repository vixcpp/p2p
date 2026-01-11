#ifndef DISCOVERY_HPP
#define DISCOVERY_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <functional>

#include "Peer.hpp"

namespace vix::p2p
{
    struct DiscoveryAnnouncement
    {
        std::string node_id;
        std::string host;      // ip
        std::uint16_t port{0}; // tcp listen port du peer
        std::string transport; // "tcp"
    };

    enum class DiscoveryMode : std::uint8_t
    {
        Broadcast = 0,
        Multicast = 1
    };

    struct DiscoveryConfig
    {
        std::string self_node_id;
        std::uint16_t self_tcp_port{0};

        std::uint16_t discovery_port{37020};
        DiscoveryMode mode{DiscoveryMode::Broadcast};

        std::uint32_t announce_interval_ms{2000};

        // hardening / rate limit
        std::uint32_t seen_ttl_ms{15000};        // 10-30s ok
        std::uint32_t connect_cooldown_ms{8000}; // 5-15s ok
        std::size_t max_packet_bytes{1024};

        // multicast only
        std::string multicast_group{"239.255.0.1"};
    };

    using DiscoveryCallback = std::function<void(const DiscoveryAnnouncement &)>;

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

    // factory
    std::shared_ptr<Discovery> make_udp_discovery(DiscoveryConfig cfg, DiscoveryCallback on_peer);

} // namespace vix::p2p

#endif