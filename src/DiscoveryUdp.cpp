#include <vix/p2p/Discovery.hpp>
#include <vix/p2p/messages/DiscoveryAnnounce.hpp>

#include <asio.hpp>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <random>

namespace vix::p2p
{
    using asio::ip::udp;

    static std::uint64_t now_epoch_ms()
    {
        using namespace std::chrono;
        return (std::uint64_t)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    static std::chrono::steady_clock::time_point now_steady()
    {
        return std::chrono::steady_clock::now();
    }

    class DiscoveryUdp final : public Discovery, public std::enable_shared_from_this<DiscoveryUdp>
    {
    public:
        DiscoveryUdp(DiscoveryConfig cfg, DiscoveryCallback cb)
            : cfg_(std::move(cfg)),
              on_peer_(std::move(cb)),
              ioc_(1),
              sock_(ioc_),
              timer_(ioc_) {}

        ~DiscoveryUdp() override { stop(); }

        void start() override
        {
            if (running_.exchange(true))
                return;

            asio::post(ioc_, [self = shared_from_this()]()
                       {
                self->open_socket_();
                self->do_receive_();
                self->schedule_announce_(); });

            thr_ = std::thread([this]()
                               { ioc_.run(); });
        }

        void stop() override
        {
            if (!running_.exchange(false))
                return;

            asio::post(ioc_, [self = shared_from_this()]()
                       {
                std::error_code ec;
                self->timer_.cancel(ec);
                self->sock_.close(ec); });

            ioc_.stop();
            if (thr_.joinable())
                thr_.join();
        }

        std::vector<DiscoveryAnnouncement> snapshot() const override
        {
            std::scoped_lock lk(mu_);
            return snapshot_;
        }

    private:
        void open_socket_()
        {
            std::error_code ec;

            udp::endpoint bind_ep(udp::v4(), cfg_.discovery_port);

            sock_.open(bind_ep.protocol(), ec);
            if (ec)
                return;

            sock_.set_option(udp::socket::reuse_address(true), ec);
            sock_.bind(bind_ep, ec);
            if (ec)
                return;

            if (cfg_.mode == DiscoveryMode::Broadcast)
            {
                sock_.set_option(asio::socket_base::broadcast(true), ec);
            }
            else
            {
                // join multicast group
                auto group = asio::ip::make_address(cfg_.multicast_group, ec);
                if (!ec)
                    sock_.set_option(asio::ip::multicast::join_group(group), ec);
            }
        }

        udp::endpoint announce_target_() const
        {
            if (cfg_.mode == DiscoveryMode::Broadcast)
            {
                return udp::endpoint(asio::ip::address_v4::broadcast(), cfg_.discovery_port);
            }
            std::error_code ec;
            auto group = asio::ip::make_address(cfg_.multicast_group, ec);
            if (ec)
                return udp::endpoint(asio::ip::address_v4::broadcast(), cfg_.discovery_port);
            return udp::endpoint(group, cfg_.discovery_port);
        }

        void schedule_announce_()
        {
            if (!running_.load())
                return;

            timer_.expires_after(std::chrono::milliseconds(cfg_.announce_interval_ms));
            timer_.async_wait([self = shared_from_this()](const std::error_code &ec)
                              {
                if (ec) return;
                if (!self->running_.load()) return;

                self->send_announce_();
                self->schedule_announce_(); });
        }

        void send_announce_()
        {
            if (cfg_.self_tcp_port == 0)
                return;

            msg::DiscoveryAnnounce a;
            a.node_id = cfg_.self_node_id;
            a.tcp_port = cfg_.self_tcp_port;
            a.ts_ms = now_epoch_ms();
            a.nonce = rng64_();

            a.capabilities = {{"proto", "1.0"}, {"transport", "tcp"}};

            auto payload = a.to_json();
            if (payload.size() > cfg_.max_packet_bytes)
                return;

            auto target = announce_target_();

            sock_.async_send_to(
                asio::buffer(payload),
                target,
                [](std::error_code /*ec*/, std::size_t /*n*/) {});
        }

        void do_receive_()
        {
            if (!running_.load())
                return;

            recv_buf_.resize(cfg_.max_packet_bytes);

            sock_.async_receive_from(
                asio::buffer(recv_buf_),
                remote_ep_,
                [self = shared_from_this()](std::error_code ec, std::size_t n)
                {
                    if (ec)
                        return;
                    if (!self->running_.load())
                        return;

                    if (n == 0 || n > self->cfg_.max_packet_bytes)
                    {
                        self->do_receive_();
                        return;
                    }

                    self->handle_packet_(n);
                    self->do_receive_();
                });
        }

        void handle_packet_(std::size_t n)
        {
            const auto ip = remote_ep_.address().to_string();
            {
                const auto now = now_steady();
                auto it = ip_last_packet_.find(ip);
                if (it != ip_last_packet_.end())
                {
                    if ((now - it->second) < std::chrono::milliseconds(150)) // anti-spam
                        return;
                }
                ip_last_packet_[ip] = now;
            }

            std::string s((const char *)recv_buf_.data(), n);

            auto ann = msg::DiscoveryAnnounce::from_json(s);
            if (!ann)
                return;

            if (ann->node_id == cfg_.self_node_id)
                return;

            if (ann->tcp_port == 0)
                return;

            if (ann->tcp_port == cfg_.self_tcp_port && (ip == "127.0.0.1" || ip == "0.0.0.0"))
                return;

            const auto now = now_steady();

            {
                auto it = last_seen_.find(ann->node_id);
                if (it != last_seen_.end())
                {
                    if ((now - it->second) < std::chrono::milliseconds(cfg_.seen_ttl_ms))
                    {
                        it->second = now;
                    }
                    else
                    {
                        it->second = now;
                    }
                }
                else
                {
                    last_seen_[ann->node_id] = now;
                }
            }

            DiscoveryAnnouncement out;
            out.node_id = ann->node_id;
            out.host = ip; // source ip = vérité
            out.port = ann->tcp_port;
            out.transport = "tcp";

            // snapshot (debug)
            {
                std::scoped_lock lk(mu_);
                bool replaced = false;
                for (auto &x : snapshot_)
                {
                    if (x.node_id == out.node_id)
                    {
                        x = out;
                        replaced = true;
                        break;
                    }
                }
                if (!replaced)
                    snapshot_.push_back(out);
            }

            // connect cooldown par endpoint
            const std::string ep_key = out.host + ":" + std::to_string(out.port);
            {
                auto it = last_connect_attempt_.find(ep_key);
                if (it != last_connect_attempt_.end())
                {
                    if ((now - it->second) < std::chrono::milliseconds(cfg_.connect_cooldown_ms))
                        return;
                }
                last_connect_attempt_[ep_key] = now;
            }

            if (on_peer_)
                on_peer_(out);
        }

        std::uint64_t rng64_()
        {
            return (std::uint64_t)rd_() ^ ((std::uint64_t)rd_() << 32);
        }

    private:
        DiscoveryConfig cfg_;
        DiscoveryCallback on_peer_;

        asio::io_context ioc_;
        udp::socket sock_;
        asio::steady_timer timer_;
        std::thread thr_;
        std::atomic<bool> running_{false};

        udp::endpoint remote_ep_;
        std::vector<std::uint8_t> recv_buf_;

        mutable std::mutex mu_;
        std::vector<DiscoveryAnnouncement> snapshot_;

        std::unordered_map<std::string, std::chrono::steady_clock::time_point> ip_last_packet_;
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_seen_;            // by node_id
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_connect_attempt_; // by ep string

        std::random_device rd_;
    };

    std::shared_ptr<Discovery> make_udp_discovery(DiscoveryConfig cfg, DiscoveryCallback on_peer)
    {
        return std::make_shared<DiscoveryUdp>(std::move(cfg), std::move(on_peer));
    }

} // namespace vix::p2p
