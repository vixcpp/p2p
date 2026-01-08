#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>

#include "Peer.hpp"
#include "Transport.hpp"
#include "Discovery.hpp"
#include "Bootstrap.hpp"
#include "Router.hpp"
#include "EdgeSync.hpp"
#include "Crypto.hpp"
#include "Protocol.hpp"

namespace vix::p2p
{

    struct NodeConfig
    {
        std::string node_id;
        std::uint16_t listen_port{0};

        // policy
        std::uint32_t max_peers{64};
        std::uint32_t handshake_timeout_ms{3000};
    };

    struct NodeStats
    {
        std::uint64_t peers_total{0};
        std::uint64_t peers_connected{0};
        std::uint64_t handshakes_started{0};
        std::uint64_t handshakes_completed{0};
    };

    class Node
    {
    public:
        virtual ~Node() = default;

        virtual const NodeConfig &config() const = 0;

        // Lifecycle
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual bool running() const = 0;

        // Peers
        virtual bool connect(const PeerEndpoint &ep) = 0;
        virtual void disconnect(const PeerId &peer_id) = 0;

        virtual std::optional<Peer> get_peer(const PeerId &peer_id) const = 0;
        virtual std::unordered_map<PeerId, Peer> peers_snapshot() const = 0;

        // Dependencies
        virtual void set_discovery(std::shared_ptr<Discovery> d) = 0;
        virtual void set_bootstrap(std::shared_ptr<Bootstrap> b) = 0;

        virtual void set_router(std::shared_ptr<Router> r) = 0;
        virtual void set_edge_sync(std::shared_ptr<EdgeSync> s) = 0;
        virtual void set_crypto(std::shared_ptr<Crypto> c) = 0;

        virtual NodeStats stats() const = 0;
    };

    class MemoryNode final : public Node
    {
    public:
        explicit MemoryNode(NodeConfig cfg)
            : cfg_(std::move(cfg)),
              discovery_(std::make_shared<NullDiscovery>()),
              bootstrap_(std::make_shared<NullBootstrap>()),
              router_(std::make_shared<MemoryRouter>()),
              edge_sync_(std::make_shared<NullEdgeSync>()),
              crypto_(std::make_shared<NullCrypto>()) {}

        const NodeConfig &config() const override { return cfg_; }

        void start() override { running_ = true; }
        void stop() override { running_ = false; }

        bool running() const override { return running_; }

        bool connect(const PeerEndpoint &ep) override
        {
            PeerId pid = ep.host + ":" + std::to_string(ep.port);

            Peer p;
            p.id = pid;
            p.state = PeerState::Connecting;
            p.endpoint = ep;

            peers_[pid] = p;
            stats_.peers_total = peers_.size();
            return true;
        }

        void disconnect(const PeerId &peer_id) override
        {
            auto it = peers_.find(peer_id);
            if (it != peers_.end())
                it->second.state = PeerState::Closed;
        }

        std::optional<Peer> get_peer(const PeerId &peer_id) const override
        {
            auto it = peers_.find(peer_id);
            if (it == peers_.end())
                return std::nullopt;
            return it->second;
        }

        std::unordered_map<PeerId, Peer> peers_snapshot() const override
        {
            return peers_;
        }

        void set_discovery(std::shared_ptr<Discovery> d) override { discovery_ = std::move(d); }
        void set_bootstrap(std::shared_ptr<Bootstrap> b) override { bootstrap_ = std::move(b); }

        void set_router(std::shared_ptr<Router> r) override { router_ = std::move(r); }
        void set_edge_sync(std::shared_ptr<EdgeSync> s) override { edge_sync_ = std::move(s); }
        void set_crypto(std::shared_ptr<Crypto> c) override { crypto_ = std::move(c); }

        NodeStats stats() const override
        {
            NodeStats out = stats_;
            std::uint64_t connected = 0;
            for (const auto &[_, p] : peers_)
                if (p.is_connected())
                    connected++;
            out.peers_connected = connected;
            return out;
        }

    private:
        NodeConfig cfg_;
        bool running_{false};

        std::unordered_map<PeerId, Peer> peers_;
        NodeStats stats_{};

        std::shared_ptr<Discovery> discovery_;
        std::shared_ptr<Bootstrap> bootstrap_;
        std::shared_ptr<Router> router_;
        std::shared_ptr<EdgeSync> edge_sync_;
        std::shared_ptr<Crypto> crypto_;
    };

    std::shared_ptr<Node> make_tcp_node(NodeConfig cfg);

} // namespace vix::p2p
