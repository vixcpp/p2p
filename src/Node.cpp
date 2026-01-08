#include <asio.hpp>
#include <thread>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <iostream>

#include "vix/p2p/Node.hpp"
#include "vix/p2p/Peer.hpp"
#include "vix/p2p/Discovery.hpp"
#include "vix/p2p/Router.hpp"
#include "vix/p2p/EdgeSync.hpp"
#include "vix/p2p/Crypto.hpp"
#include "vix/p2p/Protocol.hpp"

#include "vix/p2p/messages/Envelope.hpp"
#include "vix/p2p/messages/Dispatch.hpp"
#include "vix/p2p/messages/Pack.hpp"
#include "vix/p2p/messages/Hello.hpp"
#include "vix/p2p/messages/Ping.hpp"
#include "vix/p2p/messages/Pong.hpp"

#include "vix/p2p/transport/Tcp.hpp"

namespace vix::p2p
{

    using asio::ip::tcp;

    class TcpNode final : public Node
    {
    public:
        explicit TcpNode(NodeConfig cfg)
            : cfg_(std::move(cfg)),
              discovery_(std::make_shared<NullDiscovery>()),
              router_(std::make_shared<MemoryRouter>()),
              edge_sync_(std::make_shared<NullEdgeSync>()),
              crypto_(std::make_shared<NullCrypto>()),
              ioc_(1) {}

        ~TcpNode() override { stop(); }

        const NodeConfig &config() const override { return cfg_; }

        void start() override
        {
            if (running_)
                return;
            running_ = true;

            if (cfg_.listen_port != 0)
            {
                tcp::endpoint ep(tcp::v4(), cfg_.listen_port);
                acceptor_.emplace(ioc_);
                acceptor_->open(ep.protocol());
                acceptor_->set_option(tcp::acceptor::reuse_address(true));
                acceptor_->bind(ep);
                acceptor_->listen();
                do_accept();
            }

            if (discovery_)
                discovery_->start();

            io_thread_ = std::thread([this]()
                                     { ioc_.run(); });
        }

        void stop() override
        {
            if (!running_)
                return;
            running_ = false;

            if (discovery_)
                discovery_->stop();

            if (acceptor_)
            {
                std::error_code ec;
                acceptor_->close(ec);
            }

            {
                std::scoped_lock lk(mu_);
                for (auto &[_, t] : transports_)
                {
                    if (t)
                        t->close();
                }
                transports_.clear();
                peers_.clear();
            }

            ioc_.stop();
            if (io_thread_.joinable())
                io_thread_.join();
        }

        bool running() const override { return running_; }

        bool connect(const PeerEndpoint &ep) override
        {
            if (!running_)
                start();

            asio::post(ioc_, [this, ep]()
                       {
      EnvelopeHandler on_env = [this](const PeerId& peer_id, const Envelope& env) {
        on_envelope(peer_id, env);
      };

      TcpReadyHandler on_ready = [this](PeerId peer_id, PeerEndpoint endpoint, std::shared_ptr<Transport> transport) {
        {
          std::scoped_lock lk(mu_);
          Peer p;
          p.id = peer_id;
          p.state = PeerState::Connecting;
          p.endpoint = endpoint;
          peers_[peer_id] = p;

          transports_[peer_id] = std::move(transport);
          stats_.handshakes_started++;
        }

        send_hello(peer_id);
      };

      tcp_connect_async(ioc_, ep, std::move(on_env), std::move(on_ready)); });

            return true;
        }

        void disconnect(const PeerId &peer_id) override
        {
            asio::post(ioc_, [this, peer_id]()
                       {
      std::shared_ptr<Transport> t;
      {
        std::scoped_lock lk(mu_);
        auto it = transports_.find(peer_id);
        if (it == transports_.end()) return;
        t = it->second;
      }

      if (t) t->close();

      {
        std::scoped_lock lk(mu_);
        transports_.erase(peer_id);
        auto pit = peers_.find(peer_id);
        if (pit != peers_.end()) pit->second.state = PeerState::Closed;
      } });
        }

        std::optional<Peer> get_peer(const PeerId &peer_id) const override
        {
            std::scoped_lock lk(mu_);
            auto it = peers_.find(peer_id);
            if (it == peers_.end())
                return std::nullopt;
            return it->second;
        }

        std::unordered_map<PeerId, Peer> peers_snapshot() const override
        {
            std::scoped_lock lk(mu_);
            return peers_;
        }

        void set_discovery(std::shared_ptr<Discovery> d) override { discovery_ = std::move(d); }
        void set_router(std::shared_ptr<Router> r) override { router_ = std::move(r); }
        void set_edge_sync(std::shared_ptr<EdgeSync> s) override { edge_sync_ = std::move(s); }
        void set_crypto(std::shared_ptr<Crypto> c) override { crypto_ = std::move(c); }

        NodeStats stats() const override
        {
            std::scoped_lock lk(mu_);
            NodeStats out = stats_;
            out.peers_total = peers_.size();

            std::uint64_t connected = 0;
            for (const auto &[_, p] : peers_)
            {
                if (p.state == PeerState::Connected)
                    connected++;
            }
            out.peers_connected = connected;
            return out;
        }

    private:
        void do_accept()
        {
            if (!acceptor_)
                return;

            acceptor_->async_accept([this](std::error_code ec, tcp::socket sock)
                                    {
      if (ec) return;
      on_inbound_socket(std::move(sock));
      if (running_) do_accept(); });
        }

        void on_inbound_socket(tcp::socket sock)
        {
            EnvelopeHandler on_env = [this](const PeerId &peer_id, const Envelope &env)
            {
                on_envelope(peer_id, env);
            };

            PeerId pid;
            PeerEndpoint endpoint;

            auto transport = tcp_accept(std::move(sock), std::move(on_env), pid, endpoint);

            {
                std::scoped_lock lk(mu_);
                Peer p;
                p.id = pid;
                p.state = PeerState::Handshaking;
                p.endpoint = endpoint;
                peers_[pid] = p;

                transports_[pid] = std::move(transport);
                stats_.handshakes_started++;
            }

            send_hello(pid);
        }

        void on_envelope(const PeerId &peer_id, const Envelope &env)
        {
            try
            {
                auto any = msg::decode_payload_or_throw(env.type, env.payload);

                if (std::holds_alternative<msg::Hello>(any))
                {
                    on_hello(peer_id, std::get<msg::Hello>(any));
                    return;
                }
                if (std::holds_alternative<msg::Ping>(any))
                {
                    on_ping(peer_id, std::get<msg::Ping>(any));
                    return;
                }
                if (std::holds_alternative<msg::Pong>(any))
                {
                    on_pong(peer_id, std::get<msg::Pong>(any));
                    return;
                }
            }
            catch (...)
            {
                disconnect(peer_id);
            }
        }

        void on_hello(const PeerId &peer_id, const msg::Hello &h)
        {
            PeerId stable_id = peer_id;

            {
                std::scoped_lock lk(mu_);

                if (!h.node_id.empty())
                {
                    stable_id = h.node_id;
                    rekey_peer_unlocked_(peer_id, stable_id);
                }

                auto it = peers_.find(stable_id);
                if (it != peers_.end())
                {
                    it->second.state = PeerState::Connected;
                    it->second.meta.last_seen = std::chrono::steady_clock::now();
                    it->second.meta.capabilities = h.capabilities;
                }

                stats_.handshakes_completed++;
            }

            send_ping(stable_id, 123);
        }

        void on_ping(const PeerId &peer_id, const msg::Ping &p)
        {
            msg::Pong pong{p.nonce};
            auto env = pack::make_envelope(MessageType::Pong, pong);
            send_envelope(peer_id, env);
        }

        void on_pong(const PeerId &peer_id, const msg::Pong &)
        {
            std::scoped_lock lk(mu_);
            auto it = peers_.find(peer_id);
            if (it != peers_.end())
                it->second.meta.last_seen = std::chrono::steady_clock::now();
        }

        void send_hello(const PeerId &peer_id)
        {
            msg::Hello h;
            h.node_id = cfg_.node_id;
            h.capabilities = {
                {"proto", "1.0"},
                {"transport", "tcp"},
                {"wal", "push"},
            };

            auto env = pack::make_envelope(MessageType::Hello, h);
            send_envelope(peer_id, env);
        }

        void send_ping(const PeerId &peer_id, std::uint64_t nonce)
        {
            msg::Ping p;
            p.nonce = nonce;
            auto env = pack::make_envelope(MessageType::Ping, p);
            send_envelope(peer_id, env);
        }

        void send_envelope(const PeerId &peer_id, const Envelope &env)
        {
            auto bytes = env.encode();

            std::shared_ptr<Transport> t;
            {
                std::scoped_lock lk(mu_);
                auto it = transports_.find(peer_id);
                if (it == transports_.end())
                    return;
                t = it->second;
            }

            t->send(bytes);
        }

    private:
        NodeConfig cfg_;
        std::atomic<bool> running_{false};

        std::shared_ptr<Discovery> discovery_;
        std::shared_ptr<Router> router_;
        std::shared_ptr<EdgeSync> edge_sync_;
        std::shared_ptr<Crypto> crypto_;

        mutable std::mutex mu_;
        NodeStats stats_{};

        asio::io_context ioc_;
        std::optional<tcp::acceptor> acceptor_;
        std::thread io_thread_;

        std::unordered_map<PeerId, Peer> peers_;
        std::unordered_map<PeerId, std::shared_ptr<Transport>> transports_;

        void rekey_peer_unlocked_(const PeerId &old_id, const PeerId &new_id)
        {
            if (old_id == new_id)
                return;

            auto pit = peers_.find(old_id);
            auto tit = transports_.find(old_id);

            if (pit == peers_.end() || tit == transports_.end())
                return;

            Peer p = pit->second;
            auto t = tit->second;

            peers_.erase(pit);
            transports_.erase(tit);

            p.id = new_id;
            peers_.emplace(new_id, std::move(p));
            transports_.emplace(new_id, std::move(t));
        }
    };

    std::shared_ptr<Node> make_tcp_node(NodeConfig cfg)
    {
        return std::make_shared<TcpNode>(std::move(cfg));
    }

} // namespace vix::p2p
