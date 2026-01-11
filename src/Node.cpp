#include <asio.hpp>
#include <thread>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <iostream>
#include <chrono>
#include <vector>
#include <cstdint>
#include <random>
#include <deque>
#include <span>

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
#include "vix/p2p/Bootstrap.hpp"

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

            if (self_keys_.public_key.empty())
                self_keys_ = crypto_->generate_keypair();

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

            if (bootstrap_)
                bootstrap_->start();
            schedule_bootstrap_tick_();

            schedule_heartbeat_();

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

            std::vector<std::shared_ptr<Transport>> to_close;

            {
                std::scoped_lock lk(mu_);
                for (auto &[_, t] : transports_)
                    if (t)
                        to_close.push_back(t);

                transports_.clear();
                peers_.clear();
                bootstrap_last_connect_.clear();
            }

            for (auto &t : to_close)
                t->close();

            std::error_code ec;
            heartbeat_.cancel(ec);

            if (bootstrap_)
                bootstrap_->stop();

            std::error_code ec2;
            bootstrap_timer_.cancel(ec2);

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
            const PeerId transient_id = ep.host + ":" + std::to_string(ep.port);

            {
                std::scoped_lock lk(mu_);
                if (has_peer_by_endpoint_unlocked_(ep))
                    return;

                Peer p;
                p.id = transient_id;
                p.state = PeerState::Connecting;
                p.endpoint = ep;
                peers_[transient_id] = std::move(p);
            }

            EnvelopeHandler on_env = [this](const PeerId &peer_id, const Envelope &env)
            {
                on_envelope(peer_id, env);
            };

            TcpFailHandler on_fail = [this, transient_id](std::error_code)
            {
                std::scoped_lock lk(mu_);
                auto it = peers_.find(transient_id);
                if (it != peers_.end() && it->second.state == PeerState::Connecting)
                    it->second.state = PeerState::Closed;
            };

            TcpReadyHandler on_ready =
                [this, transient_id](PeerId peer_id, PeerEndpoint endpoint, std::shared_ptr<Transport> transport)
            {
                PeerId use_id = peer_id;

                {
                    std::scoped_lock lk(mu_);

                    if (peer_id != transient_id)
                    {
                        auto pit = peers_.find(transient_id);
                        if (pit != peers_.end())
                        {
                            Peer p = pit->second;
                            peers_.erase(pit);

                            p.id = peer_id;
                            p.endpoint = endpoint;
                            peers_[peer_id] = std::move(p);
                        }
                        else
                        {
                            Peer p;
                            p.id = peer_id;
                            p.state = PeerState::Connecting;
                            p.endpoint = endpoint;
                            peers_[peer_id] = std::move(p);
                        }
                    }
                    else
                    {
                        auto it = peers_.find(transient_id);
                        if (it != peers_.end() && it->second.endpoint)
                            it->second.endpoint = endpoint;
                    }

                    transports_[use_id] = std::move(transport);
                    stats_.handshakes_started++;
                }

                schedule_handshake_timeout_(use_id);
                send_hello(use_id);
            };

            tcp_connect_async(ioc_, ep, std::move(on_env), std::move(on_ready), std::move(on_fail)); });

            return true;
        }

        void disconnect(const PeerId &peer_id) override
        {
            asio::post(
                ioc_,
                [this, peer_id]()
                {
                    std::shared_ptr<Transport> t;
                    {
                        std::scoped_lock lk(mu_);
                        auto it = transports_.find(peer_id);
                        if (it == transports_.end())
                            return;
                        t = it->second;
                    }

                    if (t)
                        t->close();

                    {
                        std::scoped_lock lk(mu_);
                        transports_.erase(peer_id);
                        auto pit = peers_.find(peer_id);
                        if (pit != peers_.end())
                            pit->second.state = PeerState::Closed;
                    }
                });
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

        void set_bootstrap(std::shared_ptr<Bootstrap> b) override { bootstrap_ = std::move(b); }

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

            schedule_handshake_timeout_(pid);
            send_hello(pid);
        }

        bool has_peer_by_endpoint_unlocked_(const PeerEndpoint &ep) const
        {
            const PeerId transient = ep.host + ":" + std::to_string(ep.port);

            // déjà connu via id transient
            if (peers_.find(transient) != peers_.end())
                return true;

            // déjà connu via endpoint (cas rekey stable_id)
            for (const auto &[id, p] : peers_)
            {
                if (!p.endpoint)
                    continue;
                if (p.endpoint->host == ep.host && p.endpoint->port == ep.port)
                    return true;
            }

            return false;
        }

        void schedule_heartbeat_()
        {
            heartbeat_.expires_after(kPingEvery);
            heartbeat_.async_wait([this](const std::error_code &ec)
                                  {
            if (ec) return;
            if (!running_) return;

            const auto now = std::chrono::steady_clock::now();

            std::vector<PeerId> to_ping;
            std::vector<PeerId> to_drop;

            {
                std::scoped_lock lk(mu_);
                for (auto &[id, p] : peers_)
                {
                    // stale detect (only if we ever saw activity)
                    if (p.meta.last_seen.time_since_epoch().count() != 0 &&
                        (now - p.meta.last_seen) > kStaleAfter)
                    {
                        to_drop.push_back(id);
                        continue;
                    }

                    if (p.state == PeerState::Connected)
                        to_ping.push_back(id);
                }
            }

            // ping outside lock
            const std::uint64_t nonce =
                static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                            now.time_since_epoch())
                                            .count());

            for (auto &id : to_ping)
                send_ping(id, nonce);

            for (auto &id : to_drop)
                disconnect(id);

            schedule_heartbeat_(); });
        }

        void schedule_handshake_timeout_(PeerId id)
        {
            auto t = std::make_shared<asio::steady_timer>(ioc_);
            t->expires_after(kHsTimeout);

            t->async_wait([this, id, t](const std::error_code &ec)
                          {
            if (ec) return;
            if (!running_) return;

            bool drop = false;
            {
                std::scoped_lock lk(mu_);
                auto it = peers_.find(id);
                if (it == peers_.end()) return;

                // still not connected => drop
                if (it->second.state != PeerState::Connected)
                    drop = true;
            }

            if (drop)
                disconnect(id); });
        }

        void on_hello(const PeerId &peer_id, const msg::Hello &h)
        {
            std::scoped_lock lk(mu_);

            auto &peer = peers_[peer_id];

            // anti-replay basique
            auto now_ms = now_ms_();
            if (std::llabs((long long)now_ms - (long long)h.ts_ms) > 60'000)
                throw std::runtime_error("hello replay");

            peer.handshake.emplace();
            peer.handshake->stage = HandshakeState::Stage::HelloReceived;
            peer.handshake->nonce_a = h.nonce_a;
            peer.handshake->ts_ms = h.ts_ms;
            peer.handshake->started_at = std::chrono::steady_clock::now();

            // stocker pubkey pour la suite
            peer.meta.public_key = h.public_key;

            constexpr std::size_t kMinPubKey = 16;
            constexpr std::size_t kMaxPubKey = 2048;

            if (peer.meta.public_key.size() < kMinPubKey || peer.meta.public_key.size() > kMaxPubKey)
                throw std::runtime_error("bad public_key size");

            // répondre avec HelloAck
            msg::HelloAck ack;
            ack.nonce_a = h.nonce_a;
            ack.nonce_b = rand_u64_();

            peer.handshake->nonce_b = ack.nonce_b;
            peer.handshake->stage = HandshakeState::Stage::AckSent;

            send_envelope(peer_id, pack::make_envelope(MessageType::HelloAck, ack));
        }

        void on_hello_ack(const PeerId &peer_id, const msg::HelloAck &a)
        {
            std::scoped_lock lk(mu_);

            auto &peer = peers_[peer_id];

            if (!peer.handshake)
                throw std::runtime_error("missing handshake state");

            auto &hs = *peer.handshake;

            if (hs.ts_ms == 0)
                throw std::runtime_error("hs missing ts_ms");

            if (hs.stage != HandshakeState::Stage::HelloSent)
                throw std::runtime_error("unexpected HelloAck");

            if (a.nonce_a != hs.nonce_a)
                throw std::runtime_error("nonce mismatch");

            hs.nonce_b = a.nonce_b;
            hs.stage = HandshakeState::Stage::AckReceived;

            // signer (nonce_a, nonce_b)
            auto data = make_handshake_bytes_(
                hs.nonce_a,
                hs.nonce_b,
                hs.ts_ms,
                kProto_.major,
                kProto_.minor,
                kTransport_());

            msg::HelloFinish fin;
            fin.nonce_a = hs.nonce_a;
            fin.nonce_b = hs.nonce_b;
            fin.signature = crypto_->sign(data, self_keys_.private_key);

            send_envelope(peer_id, pack::make_envelope(MessageType::HelloFinish, fin));
        }

        void on_hello_finish(const PeerId &peer_id, const msg::HelloFinish &f)
        {
            std::scoped_lock lk(mu_);

            auto itPeer = peers_.find(peer_id);
            if (itPeer == peers_.end())
                throw std::runtime_error("unknown peer");

            auto &peer = itPeer->second;

            if (!peer.handshake)
                throw std::runtime_error("missing handshake state");

            auto &hs = *peer.handshake;

            if (hs.stage != HandshakeState::Stage::AckSent)
                throw std::runtime_error("unexpected HelloFinish");

            if (f.nonce_a != hs.nonce_a || f.nonce_b != hs.nonce_b)
                throw std::runtime_error("nonce mismatch");

            if (hs.ts_ms == 0)
                throw std::runtime_error("hs missing ts_ms");

            if (peer.meta.public_key.empty())
                throw std::runtime_error("missing peer public_key");

            auto data = make_handshake_bytes_(
                hs.nonce_a,
                hs.nonce_b,
                hs.ts_ms,
                kProto_.major,
                kProto_.minor,
                kTransport_());

            if (!crypto_->verify(data, f.signature, peer.meta.public_key))
                throw std::runtime_error("bad signature");

            const std::string replay_key =
                to_hex_(peer.meta.public_key) + "|" +
                std::to_string(f.nonce_a) + "|" +
                std::to_string(f.nonce_b);

            if (seen_hs_check_and_put_unlocked_(replay_key))
                throw std::runtime_error("replay handshake");

            PeerId stable_id = to_hex_(peer.meta.public_key);

            // derive before rekey
            auto sk = derive_session_key_unlocked_(hs, peer.meta.public_key);

            rekey_peer_unlocked_(peer_id, stable_id);

            // apply after rekey
            auto &sp = peers_[stable_id];
            sp.meta.session_key_32 = std::move(sk);
            sp.meta.secure = true;
            sp.meta.send_nonce_counter = 1;

            sp.state = PeerState::Connected;
            sp.handshake.reset();

            stats_.handshakes_completed++;
        }

        void on_envelope(const PeerId &peer_id, const Envelope &env)
        {
            try
            {
                if (!running_)
                    return;

                std::vector<std::uint8_t> plaintext;

                if (has_flag(env.flags, EnvelopeFlag::Encrypted))
                {
                    std::vector<std::uint8_t> key32;
                    bool secure = false;
                    std::uint64_t ctr = 0;

                    {
                        std::scoped_lock lk(mu_);
                        auto itP = peers_.find(peer_id);
                        if (itP == peers_.end())
                            throw std::runtime_error("unknown peer");

                        secure = itP->second.meta.secure;
                        key32 = itP->second.meta.session_key_32;

                        if (!secure || key32.size() != 32)
                            throw std::runtime_error("missing session key");

                        ctr = nonce_counter_from_12_(env.nonce);

                        if (!accept_recv_nonce_unlocked_(itP->second.meta, ctr))
                            throw std::runtime_error("replay (nonce counter)");
                    }

                    auto aad = pack::make_aad(env);

                    plaintext = crypto_->aead_decrypt(
                        key32,
                        std::span<const std::uint8_t>(env.nonce.data(), env.nonce.size()),
                        aad,
                        env.payload,
                        std::span<const std::uint8_t>(env.tag.data(), env.tag.size()));

                    if (plaintext.empty())
                        throw std::runtime_error("aead auth failed");
                }
                else
                {
                    plaintext = env.payload;
                }

                auto any = msg::decode_payload_or_throw(env.type, plaintext);

                if (std::holds_alternative<msg::Hello>(any))
                {
                    on_hello(peer_id, std::get<msg::Hello>(any));
                    return;
                }
                if (std::holds_alternative<msg::HelloAck>(any))
                {
                    on_hello_ack(peer_id, std::get<msg::HelloAck>(any));
                    return;
                }
                if (std::holds_alternative<msg::HelloFinish>(any))
                {
                    on_hello_finish(peer_id, std::get<msg::HelloFinish>(any));
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
            h.nonce_a = rand_u64_();
            h.ts_ms = now_ms_();
            h.node_id = cfg_.node_id;
            h.public_key = self_keys_.public_key;

            peers_[peer_id].handshake.emplace();
            peers_[peer_id].handshake->nonce_a = h.nonce_a;
            peers_[peer_id].handshake->stage = HandshakeState::Stage::HelloSent;
            peers_[peer_id].handshake->started_at = std::chrono::steady_clock::now();

            send_envelope(peer_id, pack::make_envelope(MessageType::Hello, h));
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

        void send_message(const PeerId &peer_id, MessageType type, std::span<const std::uint8_t> plaintext)
        {
            Envelope out;

            std::shared_ptr<Transport> t;
            bool secure = false;
            std::array<std::uint8_t, 32> sk{};
            bool has_sk = false;
            std::uint64_t ctr = 0;

            auto can_encrypt_type = [](MessageType t)
            {
                switch (t)
                {
                case MessageType::Hello:
                case MessageType::HelloAck:
                case MessageType::HelloFinish:
                case MessageType::Ping:
                case MessageType::Pong:
                    return false;
                default:
                    return true;
                }
            };

            {
                std::scoped_lock lk(mu_);

                auto itT = transports_.find(peer_id);
                if (itT == transports_.end())
                    return;
                t = itT->second;

                auto itP = peers_.find(peer_id);
                if (itP != peers_.end())
                {
                    secure = itP->second.meta.secure;
                    ctr = itP->second.meta.send_nonce_counter++;

                    if (itP->second.meta.session_key_32.size() == 32)
                    {
                        std::copy_n(itP->second.meta.session_key_32.begin(), 32, sk.begin());
                        has_sk = true;
                    }
                }
            }

            out.version = ProtocolVersion{1, 0};
            out.type = type;
            out.msg_id = pack::next_message_id();

            const bool do_encrypt = secure && has_sk && can_encrypt_type(type);

            if (do_encrypt)
            {
                out.flags = (std::uint32_t)EnvelopeFlag::Encrypted;

                // nonce12 = counter(LE u64) + 0(u32)
                for (int i = 0; i < 8; ++i)
                    out.nonce[i] = (std::uint8_t)((ctr >> (8 * i)) & 0xFF);
                out.nonce[8] = out.nonce[9] = out.nonce[10] = out.nonce[11] = 0;

                auto aad = pack::make_aad(out);

                out.payload = crypto_->aead_encrypt(
                    std::span<const std::uint8_t>(sk.data(), sk.size()),
                    std::span<const std::uint8_t>(out.nonce.data(), out.nonce.size()),
                    aad,
                    plaintext,
                    out.tag);
            }
            else
            {
                out.flags = 0;
                out.payload.assign(plaintext.begin(), plaintext.end());
            }

            t->send(out.encode());
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
        asio::steady_timer heartbeat_{ioc_};

        static constexpr auto kPingEvery = std::chrono::seconds(5);
        static constexpr auto kStaleAfter = std::chrono::seconds(15);
        static constexpr auto kHsTimeout = std::chrono::seconds(5);

        std::optional<tcp::acceptor> acceptor_;
        std::thread io_thread_;

        std::unordered_map<PeerId, Peer> peers_;
        std::unordered_map<PeerId, std::shared_ptr<Transport>> transports_;
        std::shared_ptr<Bootstrap> bootstrap_{std::make_shared<NullBootstrap>()};
        asio::steady_timer bootstrap_timer_{ioc_};
        static constexpr auto kBootstrapEvery = std::chrono::seconds(5);
        static constexpr auto kBootstrapConnectCooldown = std::chrono::seconds(12);
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> bootstrap_last_connect_;

        KeyPair self_keys_{};

        struct SeenHandshake
        {
            std::chrono::steady_clock::time_point at;
        };

        std::unordered_map<std::string, SeenHandshake> seen_hs_;
        std::deque<std::string> seen_hs_order_;
        static constexpr std::size_t kSeenMax = 4096;
        static constexpr auto kSeenTtl = std::chrono::minutes(2);

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

        void schedule_bootstrap_tick_()
        {
            bootstrap_timer_.expires_after(kBootstrapEvery);
            bootstrap_timer_.async_wait([this](const std::error_code &ec)
                                        {
            if (ec) return;
            if (!running_) return;

            if (bootstrap_)
            {
                const auto now = std::chrono::steady_clock::now();
                auto seeds = bootstrap_->snapshot();

                std::size_t attempts = 0;
                for (const auto& s : seeds)
                {
                    if (s.tcp_port == 0)
                        continue;

                    if (!s.transport.empty() && s.transport != "tcp")
                        continue;

                    PeerEndpoint ep;
                    ep.host = s.host;
                    ep.port = s.tcp_port;
                    ep.scheme = "tcp";

                    if (cfg_.listen_port != 0 &&
                        ep.port == cfg_.listen_port &&
                        (ep.host == "127.0.0.1" || ep.host == "localhost"))
                        continue;

                    {
                        std::scoped_lock lk(mu_);

                        if (has_peer_by_endpoint_unlocked_(ep))
                            continue;

                        if (!bootstrap_cooldown_ok_unlocked_(ep, now))
                            continue;

                        mark_bootstrap_attempt_unlocked_(ep, now);
                    }

                    connect_from_bootstrap_(ep);
                    attempts++;

                    if (attempts >= 8)
                        break;
                }
            }

            schedule_bootstrap_tick_(); });
        }

        void connect_from_bootstrap_(const PeerEndpoint &ep)
        {
            const PeerId transient_id = ep.host + ":" + std::to_string(ep.port);

            {
                std::scoped_lock lk(mu_);
                if (has_peer_by_endpoint_unlocked_(ep))
                    return;

                Peer p;
                p.id = transient_id;
                p.state = PeerState::Connecting;
                p.endpoint = ep;
                peers_[transient_id] = std::move(p);
            }

            EnvelopeHandler on_env = [this](const PeerId &peer_id, const Envelope &env)
            {
                on_envelope(peer_id, env);
            };

            TcpFailHandler on_fail = [this, transient_id](std::error_code)
            {
                std::scoped_lock lk(mu_);
                auto it = peers_.find(transient_id);
                if (it != peers_.end() && it->second.state == PeerState::Connecting)
                    it->second.state = PeerState::Closed;
            };

            TcpReadyHandler on_ready =
                [this, transient_id](PeerId peer_id, PeerEndpoint endpoint, std::shared_ptr<Transport> transport)
            {
                PeerId use_id = peer_id;

                {
                    std::scoped_lock lk(mu_);

                    if (peer_id != transient_id)
                    {
                        auto pit = peers_.find(transient_id);
                        if (pit != peers_.end())
                        {
                            Peer p = pit->second;
                            peers_.erase(pit);

                            p.id = peer_id;
                            p.endpoint = endpoint;
                            peers_[peer_id] = std::move(p);
                        }
                        else
                        {
                            Peer p;
                            p.id = peer_id;
                            p.state = PeerState::Connecting;
                            p.endpoint = endpoint;
                            peers_[peer_id] = std::move(p);
                        }
                    }
                    else
                    {
                        auto it = peers_.find(transient_id);
                        if (it != peers_.end())
                            it->second.endpoint = endpoint;
                    }

                    transports_[use_id] = std::move(transport);
                    stats_.handshakes_started++;
                }

                schedule_handshake_timeout_(use_id);
                send_hello(use_id);
            };

            tcp_connect_async(ioc_, ep, std::move(on_env), std::move(on_ready), std::move(on_fail));
        }

        static std::string ep_key_(const PeerEndpoint &ep)
        {
            return ep.host + ":" + std::to_string(ep.port);
        }

        bool bootstrap_cooldown_ok_unlocked_(const PeerEndpoint &ep, std::chrono::steady_clock::time_point now)
        {
            const auto key = ep_key_(ep);
            auto it = bootstrap_last_connect_.find(key);
            if (it == bootstrap_last_connect_.end())
                return true;

            return (now - it->second) >= kBootstrapConnectCooldown;
        }

        void mark_bootstrap_attempt_unlocked_(const PeerEndpoint &ep, std::chrono::steady_clock::time_point now)
        {
            bootstrap_last_connect_[ep_key_(ep)] = now;
        }

        static std::uint64_t rand_u64_()
        {
            static thread_local std::mt19937_64 rng{std::random_device{}()};
            return rng();
        }

        static void append_u64_le_(std::vector<std::uint8_t> &out, std::uint64_t v)
        {
            for (int i = 0; i < 8; ++i)
                out.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
        }

        static std::uint64_t now_ms_()
        {
            using namespace std::chrono;
            return static_cast<std::uint64_t>(
                duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
        }

        static std::string to_hex_(std::span<const std::uint8_t> bytes)
        {
            static const char *hex = "0123456789abcdef";
            std::string out;
            out.reserve(bytes.size() * 2);
            for (auto b : bytes)
            {
                out.push_back(hex[(b >> 4) & 0xF]);
                out.push_back(hex[b & 0xF]);
            }
            return out;
        }

        static void append_u16_le_(std::vector<std::uint8_t> &out, std::uint16_t v)
        {
            out.push_back(static_cast<std::uint8_t>(v & 0xFF));
            out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        }

        static void append_str_(std::vector<std::uint8_t> &out, const std::string &s)
        {
            append_u16_le_(out, static_cast<std::uint16_t>(s.size()));
            out.insert(out.end(), s.begin(), s.end());
        }

        static std::vector<std::uint8_t> make_handshake_bytes_(
            std::uint64_t nonce_a,
            std::uint64_t nonce_b,
            std::uint64_t ts_ms,
            std::uint16_t proto_major,
            std::uint16_t proto_minor,
            const std::string &transport)
        {
            std::vector<std::uint8_t> out;
            out.reserve(8 + 8 + 8 + 2 + 2 + 2 + transport.size());
            append_u64_le_(out, nonce_a);
            append_u64_le_(out, nonce_b);
            append_u64_le_(out, ts_ms);
            append_u16_le_(out, proto_major);
            append_u16_le_(out, proto_minor);
            append_str_(out, transport);
            return out;
        }

        bool seen_hs_check_and_put_unlocked_(const std::string &key)
        {
            const auto now = std::chrono::steady_clock::now();

            // purge TTL
            while (!seen_hs_order_.empty())
            {
                const auto &k = seen_hs_order_.front();
                auto it = seen_hs_.find(k);
                if (it == seen_hs_.end())
                {
                    seen_hs_order_.pop_front();
                    continue;
                }
                if ((now - it->second.at) <= kSeenTtl)
                    break;

                seen_hs_.erase(it);
                seen_hs_order_.pop_front();
            }

            // replay ?
            if (seen_hs_.find(key) != seen_hs_.end())
                return true;

            // insert
            seen_hs_[key] = SeenHandshake{now};
            seen_hs_order_.push_back(key);

            // cap size
            while (seen_hs_order_.size() > kSeenMax)
            {
                auto drop = seen_hs_order_.front();
                seen_hs_order_.pop_front();
                seen_hs_.erase(drop);
            }

            return false;
        }

        static constexpr ProtocolVersion kProto_{1, 0};

        static constexpr const char *kTransport_()
        {
            return "tcp";
        }

        std::vector<std::uint8_t> derive_session_key_unlocked_(
            const HandshakeState &hs,
            const std::vector<std::uint8_t> &peer_public_key) const
        {
            // transcript = (nonce_a, nonce_b, ts_ms, proto, transport) + peer_pubkey + self_pubkey
            auto transcript = make_handshake_bytes_(
                hs.nonce_a,
                hs.nonce_b,
                hs.ts_ms,
                kProto_.major,
                kProto_.minor,
                kTransport_());

            transcript.insert(transcript.end(), peer_public_key.begin(), peer_public_key.end());
            transcript.insert(transcript.end(), self_keys_.public_key.begin(), self_keys_.public_key.end());

            return crypto_->kdf_32(transcript);
        }

        std::vector<std::uint8_t> decrypt_payload_unlocked_(const PeerId &peer_id, const Envelope &env)
        {
            auto it = peers_.find(peer_id);
            if (it == peers_.end())
                throw std::runtime_error("decrypt: unknown peer");

            auto &pm = it->second.meta;

            if (!pm.secure || pm.session_key_32.size() != 32)
                throw std::runtime_error("decrypt: peer not secure");

            // AAD = header (version/type/msg_id/flags) => doit matcher Pack.hpp
            auto aad = pack::make_aad(env);

            auto pt = crypto_->aead_decrypt(
                std::span<const std::uint8_t>(pm.session_key_32.data(), pm.session_key_32.size()),
                std::span<const std::uint8_t>(env.nonce.data(), env.nonce.size()),
                aad,
                std::span<const std::uint8_t>(env.payload.data(), env.payload.size()),
                std::span<const std::uint8_t>(env.tag.data(), env.tag.size()));

            if (pt.empty())
                throw std::runtime_error("decrypt: auth failed");

            return pt;
        }

        static std::uint64_t nonce_counter_from_12_(const std::array<std::uint8_t, 12> &nonce)
        {
            // nonce12 = [u64 counter LE] + [u32 zeros]
            std::uint64_t ctr = 0;
            for (int i = 0; i < 8; ++i)
                ctr |= (std::uint64_t)nonce[i] << (8 * i);
            return ctr;
        }

        // Sliding window of 64 counters
        static bool accept_recv_nonce_unlocked_(PeerMetadata &m, std::uint64_t ctr)
        {
            constexpr std::uint64_t kWindow = 64;

            // First packet initializes max
            if (m.recv_nonce_max == 0)
            {
                m.recv_nonce_max = ctr;
                m.recv_nonce_window = 1ULL; // mark "max" seen
                return true;
            }

            if (ctr > m.recv_nonce_max)
            {
                const std::uint64_t shift = ctr - m.recv_nonce_max;

                if (shift >= kWindow)
                {
                    // too far ahead => reset window
                    m.recv_nonce_window = 1ULL;
                }
                else
                {
                    m.recv_nonce_window <<= shift;
                    m.recv_nonce_window |= 1ULL; // mark newest
                }

                m.recv_nonce_max = ctr;
                return true;
            }

            // ctr <= max : check within window
            const std::uint64_t diff = m.recv_nonce_max - ctr;
            if (diff >= kWindow)
                return false; // too old

            const std::uint64_t bit = 1ULL << diff;
            if (m.recv_nonce_window & bit)
                return false; // replay

            m.recv_nonce_window |= bit;
            return true;
        }
    };

    std::shared_ptr<Node> make_tcp_node(NodeConfig cfg)
    {
        return std::make_shared<TcpNode>(std::move(cfg));
    }

} // namespace vix::p2p
