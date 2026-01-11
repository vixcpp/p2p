#ifndef BOOTSTRAP_HPP
#define BOOTSTRAP_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <vix/p2p/Peer.hpp>

namespace vix::p2p
{
    struct BootstrapPeer
    {
        std::string node_id; // optional
        std::string host;    // ip/hostname
        std::uint16_t tcp_port{0};
        std::string transport{"tcp"};
    };

    enum class BootstrapMode : std::uint8_t
    {
        PullOnly = 0,       // GET /peers
        PullAndAnnounce = 1 // + POST /announce
    };

    struct BootstrapConfig
    {
        std::string self_node_id;
        std::uint16_t self_tcp_port{0};

        std::string registry_url; // ex: http://127.0.0.1:8080/p2p/v1
        BootstrapMode mode{BootstrapMode::PullOnly};

        std::uint32_t poll_interval_ms{15000};
        std::uint32_t connect_cooldown_ms{12000};

        std::size_t max_http_bytes{64 * 1024}; // 64KB
        std::uint32_t connect_timeout_ms{3000};
        std::uint32_t request_timeout_ms{5000};
        std::uint32_t backoff_max_ms{60000};

        std::uint32_t max_peers_per_poll{20};
    };

    using BootstrapCallback = std::function<void(const BootstrapPeer &)>;

    class Bootstrap
    {
    public:
        virtual ~Bootstrap() = default;
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual std::vector<BootstrapPeer> snapshot() const = 0;
    };

    class NullBootstrap final : public Bootstrap
    {
    public:
        void start() override {}
        void stop() override {}
        std::vector<BootstrapPeer> snapshot() const override { return {}; }
    };

    std::shared_ptr<Bootstrap> make_http_bootstrap(BootstrapConfig cfg, BootstrapCallback on_peer);

} // namespace vix::p2p

#endif