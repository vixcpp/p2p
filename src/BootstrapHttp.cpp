#include "vix/p2p/Bootstrap.hpp"

#include <asio.hpp>
#include <chrono>
#include <mutex>
#include <random>
#include <sstream>
#include <unordered_map>

namespace vix::p2p
{
    using asio::ip::tcp;

    struct ParsedUrl
    {
        std::string host;
        std::string port;      // string for resolver
        std::string base_path; // no trailing slash
    };

    static bool parse_http_url(const std::string &url, ParsedUrl &out)
    {
        // expected: http://host:port/path
        const std::string prefix = "http://";
        if (url.rfind(prefix, 0) != 0)
            return false;

        std::string rest = url.substr(prefix.size()); // host:port/path...
        auto slash = rest.find('/');
        std::string hostport = (slash == std::string::npos) ? rest : rest.substr(0, slash);
        std::string path = (slash == std::string::npos) ? "" : rest.substr(slash);

        std::string host;
        std::string port = "80";

        auto colon = hostport.rfind(':');
        if (colon != std::string::npos)
        {
            host = hostport.substr(0, colon);
            port = hostport.substr(colon + 1);
        }
        else
        {
            host = hostport;
        }

        if (host.empty())
            return false;

        // normalize base_path
        if (path.empty())
            path = "";
        if (!path.empty() && path.back() == '/')
            path.pop_back();

        out.host = host;
        out.port = port;
        out.base_path = path;
        return true;
    }

    // ---- tiny JSON extractors (same spirit as Phase 4)
    static std::optional<std::string> find_json_string(const std::string &s, const char *key, std::size_t start_pos)
    {
        std::string pat = std::string("\"") + key + "\"";
        auto p = s.find(pat, start_pos);
        if (p == std::string::npos)
            return std::nullopt;
        p = s.find(':', p);
        if (p == std::string::npos)
            return std::nullopt;
        p++;
        while (p < s.size() && std::isspace((unsigned char)s[p]))
            p++;
        if (p >= s.size() || s[p] != '"')
            return std::nullopt;
        p++;
        std::string out;
        while (p < s.size())
        {
            char c = s[p++];
            if (c == '"')
                break;
            if (c == '\\' && p < s.size())
            {
                out += s[p++];
                continue;
            }
            out += c;
        }
        return out;
    }

    static std::optional<std::uint64_t> find_json_u64(const std::string &s, const char *key, std::size_t start_pos)
    {
        std::string pat = std::string("\"") + key + "\"";
        auto p = s.find(pat, start_pos);
        if (p == std::string::npos)
            return std::nullopt;
        p = s.find(':', p);
        if (p == std::string::npos)
            return std::nullopt;
        p++;
        while (p < s.size() && std::isspace((unsigned char)s[p]))
            p++;
        std::uint64_t v = 0;
        bool any = false;
        while (p < s.size() && std::isdigit((unsigned char)s[p]))
        {
            any = true;
            v = v * 10 + (std::uint64_t)(s[p] - '0');
            p++;
        }
        if (!any)
            return std::nullopt;
        return v;
    }

    static std::vector<BootstrapPeer> parse_peers_json(const std::string &body, std::uint32_t limit)
    {
        // expected: { "peers":[ { "host":"x", "tcp_port":9001, "node_id":"A" }, ... ] }
        std::vector<BootstrapPeer> out;
        if (body.empty())
            return out;

        auto peers_pos = body.find("\"peers\"");
        if (peers_pos == std::string::npos)
            return out;

        auto arr_pos = body.find('[', peers_pos);
        if (arr_pos == std::string::npos)
            return out;

        std::size_t p = arr_pos + 1;
        while (p < body.size() && out.size() < limit)
        {
            auto obj = body.find('{', p);
            if (obj == std::string::npos)
                break;

            auto end = body.find('}', obj);
            if (end == std::string::npos)
                break;

            BootstrapPeer peer;

            if (auto host = find_json_string(body, "host", obj))
                peer.host = *host;

            if (auto port = find_json_u64(body, "tcp_port", obj))
            {
                if (*port > 0 && *port <= 65535ULL)
                    peer.tcp_port = (std::uint16_t)(*port);
            }

            if (auto nid = find_json_string(body, "node_id", obj))
                peer.node_id = *nid;

            if (!peer.host.empty() && peer.tcp_port != 0)
                out.push_back(std::move(peer));

            p = end + 1;
        }

        return out;
    }

    static std::string make_get_request(const std::string &host, const std::string &target)
    {
        std::ostringstream os;
        os << "GET " << target << " HTTP/1.1\r\n";
        os << "Host: " << host << "\r\n";
        os << "Connection: close\r\n";
        os << "Accept: application/json\r\n";
        os << "\r\n";
        return os.str();
    }

    static std::string make_post_request(const std::string &host, const std::string &target, const std::string &body)
    {
        std::ostringstream os;
        os << "POST " << target << " HTTP/1.1\r\n";
        os << "Host: " << host << "\r\n";
        os << "Connection: close\r\n";
        os << "Content-Type: application/json\r\n";
        os << "Content-Length: " << body.size() << "\r\n";
        os << "\r\n";
        os << body;
        return os.str();
    }

    static std::string json_escape(const std::string &s)
    {
        std::string out;
        out.reserve(s.size() + 8);
        for (char c : s)
        {
            switch (c)
            {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += c;
                break;
            }
        }
        return out;
    }

    class HttpBootstrap final : public Bootstrap, public std::enable_shared_from_this<HttpBootstrap>
    {
    public:
        HttpBootstrap(BootstrapConfig cfg, BootstrapCallback cb)
            : cfg_(std::move(cfg)),
              on_peer_(std::move(cb)),
              ioc_(1),
              timer_(ioc_) {}

        ~HttpBootstrap() override { stop(); }

        void start() override
        {
            if (running_.exchange(true))
                return;

            if (!parse_http_url(cfg_.registry_url, url_))
            {
                running_ = false;
                return;
            }

            thr_ = std::thread([this]()
                               { ioc_.run(); });

            asio::post(ioc_, [self = shared_from_this()]()
                       { self->schedule_tick_(0); });
        }

        void stop() override
        {
            if (!running_.exchange(false))
                return;

            asio::post(ioc_, [self = shared_from_this()]()
                       {
                std::error_code ec;
                self->timer_.cancel(ec); });

            ioc_.stop();
            if (thr_.joinable())
                thr_.join();
        }

        std::vector<BootstrapPeer> snapshot() const override
        {
            std::scoped_lock lk(mu_);
            return snapshot_;
        }

    private:
        void schedule_tick_(std::uint32_t delay_ms)
        {
            if (!running_.load())
                return;

            timer_.expires_after(std::chrono::milliseconds(delay_ms));
            timer_.async_wait([self = shared_from_this()](const std::error_code &ec)
                              {
                if (ec) return;
                if (!self->running_.load()) return;

                self->tick_(); });
        }

        void tick_()
        {
            // optional: announce then pull
            if (cfg_.mode == BootstrapMode::PullAndAnnounce)
            {
                do_announce_([self = shared_from_this()]()
                             { self->do_pull_peers_(); });
            }
            else
            {
                do_pull_peers_();
            }
        }

        void do_pull_peers_()
        {
            const std::string target = url_.base_path + "/peers?limit=" + std::to_string(cfg_.max_peers_per_poll);
            const std::string req = make_get_request(url_.host, target);

            do_http_(req, [self = shared_from_this()](bool ok, const std::string &body)
                     {
                if (!ok)
                {
                    self->backoff_();
                    return;
                }

                auto peers = parse_peers_json(body, self->cfg_.max_peers_per_poll);

                {
                    std::scoped_lock lk(self->mu_);
                    self->snapshot_ = peers;
                }

                // emit with cooldown
                const auto now = std::chrono::steady_clock::now();

                for (const auto& p : peers)
                {
                    const std::string key = p.host + ":" + std::to_string(p.tcp_port);
                    auto it = self->last_connect_attempt_.find(key);
                    if (it != self->last_connect_attempt_.end())
                    {
                        if ((now - it->second) < std::chrono::milliseconds(self->cfg_.connect_cooldown_ms))
                            continue;
                    }
                    self->last_connect_attempt_[key] = now;

                    if (self->on_peer_) self->on_peer_(p);
                }

                // reset backoff & schedule next poll
                self->backoff_ms_ = self->cfg_.poll_interval_ms;
                self->schedule_tick_(self->cfg_.poll_interval_ms); });
        }

        void do_announce_(std::function<void()> next)
        {
            if (cfg_.self_tcp_port == 0)
            {
                next();
                return;
            }

            std::ostringstream os;
            os << "{";
            os << "\"node_id\":\"" << json_escape(cfg_.self_node_id) << "\"";
            os << ",\"tcp_port\":" << cfg_.self_tcp_port;
            os << ",\"cap\":{\"proto\":\"1.0\",\"transport\":\"tcp\"}";
            os << "}";

            const std::string body = os.str();
            const std::string target = url_.base_path + "/announce";
            const std::string req = make_post_request(url_.host, target, body);

            do_http_(req, [self = shared_from_this(), next = std::move(next)](bool /*ok*/, const std::string & /*body*/)
                     { next(); });
        }

        void backoff_()
        {
            // exponential backoff until backoff_max_ms
            if (backoff_ms_ == 0)
                backoff_ms_ = cfg_.poll_interval_ms;
            backoff_ms_ = std::min<std::uint32_t>(cfg_.backoff_max_ms, backoff_ms_ * 2);
            schedule_tick_(backoff_ms_);
        }

        void do_http_(const std::string &request, std::function<void(bool, std::string)> done)
        {
            auto self = shared_from_this();

            // Objects must outlive async ops
            auto resolver = std::make_shared<tcp::resolver>(ioc_);
            auto socket = std::make_shared<tcp::socket>(ioc_);
            auto response = std::make_shared<asio::streambuf>();

            // connect timeout
            auto t_connect = std::make_shared<asio::steady_timer>(ioc_);
            t_connect->expires_after(std::chrono::milliseconds(cfg_.connect_timeout_ms));
            t_connect->async_wait([socket](const std::error_code &ec)
                                  {
                              if (ec)
                                  return;
                              std::error_code ignore;
                              socket->close(ignore); });

            // Resolve host:port
            resolver->async_resolve(
                url_.host, url_.port,
                [self, resolver, socket, response, t_connect, request, done = std::move(done)](
                    std::error_code ec, tcp::resolver::results_type results) mutable
                {
                    if (ec)
                    {
                        std::error_code ign;
                        t_connect->cancel(ign);
                        done(false, {});
                        return;
                    }

                    // Connect
                    asio::async_connect(
                        *socket, results,
                        [self, socket, response, t_connect, request, done = std::move(done)](
                            std::error_code ec2, const tcp::endpoint &) mutable
                        {
                            std::error_code ign;
                            t_connect->cancel(ign);

                            if (ec2)
                            {
                                done(false, {});
                                return;
                            }

                            // request timeout
                            auto t_req = std::make_shared<asio::steady_timer>(self->ioc_);
                            t_req->expires_after(std::chrono::milliseconds(self->cfg_.request_timeout_ms));
                            t_req->async_wait([socket](const std::error_code &ec3)
                                              {
                                          if (ec3)
                                              return;
                                          std::error_code ignore;
                                          socket->close(ignore); });

                            // Write request
                            asio::async_write(
                                *socket, asio::buffer(request),
                                [self, socket, response, t_req, done = std::move(done)](
                                    std::error_code ecw, std::size_t /*n*/) mutable
                                {
                                    if (ecw)
                                    {
                                        std::error_code ign;
                                        t_req->cancel(ign);
                                        done(false, {});
                                        return;
                                    }

                                    // Read until EOF (Connection: close).
                                    // We read in chunks and enforce max_http_bytes.
                                    auto tmp = std::make_shared<std::vector<char>>(4096);

                                    auto read_more = std::make_shared<std::function<void()>>();
                                    *read_more = [self, socket, response, tmp, t_req, done, read_more]()
                                    {
                                        socket->async_read_some(
                                            asio::buffer(*tmp),
                                            [self, socket, response, tmp, t_req, done, read_more](
                                                std::error_code ecr, std::size_t n) mutable
                                            {
                                                if (n > 0)
                                                {
                                                    // enforce max bytes
                                                    if (response->size() + n > self->cfg_.max_http_bytes)
                                                    {
                                                        std::error_code ign;
                                                        t_req->cancel(ign);
                                                        std::error_code ign2;
                                                        socket->close(ign2);
                                                        done(false, {});
                                                        return;
                                                    }

                                                    std::ostream os(response.get());
                                                    os.write(tmp->data(), static_cast<std::streamsize>(n));
                                                }

                                                if (ecr)
                                                {
                                                    std::error_code ign;
                                                    t_req->cancel(ign);

                                                    // EOF = success (server closed)
                                                    if (ecr == asio::error::eof)
                                                    {
                                                        std::istream is(response.get());
                                                        std::string full(
                                                            (std::istreambuf_iterator<char>(is)),
                                                            std::istreambuf_iterator<char>());

                                                        auto pos = full.find("\r\n\r\n");
                                                        if (pos == std::string::npos)
                                                        {
                                                            done(false, {});
                                                            return;
                                                        }

                                                        std::string body = full.substr(pos + 4);
                                                        done(true, std::move(body));
                                                        return;
                                                    }

                                                    done(false, {});
                                                    return;
                                                }

                                                // continue reading
                                                (*read_more)();
                                            });
                                    };

                                    (*read_more)();
                                });
                        });
                });
        }

    private:
        BootstrapConfig cfg_;
        BootstrapCallback on_peer_;
        ParsedUrl url_{};

        asio::io_context ioc_;
        asio::steady_timer timer_;
        std::thread thr_;
        std::atomic<bool> running_{false};

        mutable std::mutex mu_;
        std::vector<BootstrapPeer> snapshot_;

        std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_connect_attempt_;
        std::uint32_t backoff_ms_{0};
    };

    std::shared_ptr<Bootstrap> make_http_bootstrap(BootstrapConfig cfg, BootstrapCallback on_peer)
    {
        return std::make_shared<HttpBootstrap>(std::move(cfg), std::move(on_peer));
    }

} // namespace vix::p2p
