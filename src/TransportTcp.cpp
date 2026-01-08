#include <asio.hpp>
#include <deque>
#include <memory>
#include <vector>
#include <string>

#include "vix/p2p/Transport.hpp"
#include "vix/p2p/Peer.hpp"

#include "vix/p2p/messages/Envelope.hpp"
#include "vix/p2p/messages/Dispatch.hpp"
#include "vix/p2p/framing/LengthPrefixVarint.hpp"

#include "vix/p2p/transport/Tcp.hpp"

namespace vix::p2p
{

    using asio::ip::tcp;

    struct TcpSession : std::enable_shared_from_this<TcpSession>
    {
        tcp::socket socket;
        PeerId peer_id;
        PeerEndpoint endpoint;
        TransportStats stats{};
        framing::LengthPrefixVarint framer;

        std::vector<std::uint8_t> read_chunk;
        std::vector<std::uint8_t> pending;
        std::deque<std::vector<std::uint8_t>> write_q;

        static constexpr std::size_t kMaxQueuedFrames = 256;
        static constexpr std::size_t kMaxQueuedBytes = 4 * 1024 * 1024; // 4MB
        std::size_t queued_bytes{0};
        std::atomic<bool> closed{false};

        EnvelopeHandler on_envelope;

        explicit TcpSession(tcp::socket s)
            : socket(std::move(s)), read_chunk(64 * 1024) {}

        void start() { do_read(); }

        void close()
        {
            if (closed.exchange(true))
                return; // idempotent
            asio::error_code ec;
            socket.shutdown(tcp::socket::shutdown_both, ec);
            socket.close(ec);
        }

        void send_frame(std::span<const std::uint8_t> payload)
        {
            if (closed.load())
                return;

            auto frame = framer.encode(payload);

            // backpressure
            if (write_q.size() >= kMaxQueuedFrames || (queued_bytes + frame.bytes.size()) > kMaxQueuedBytes)
            {
                close();
                return;
            }

            stats.frames_sent++;
            stats.bytes_sent += frame.bytes.size();

            queued_bytes += frame.bytes.size();
            bool in_progress = !write_q.empty();
            write_q.push_back(std::move(frame.bytes));
            if (!in_progress)
                do_write();
        }

        std::string endpoint_string() const
        {
            return "tcp://" + endpoint.host + ":" + std::to_string(endpoint.port);
        }

    private:
        void do_write()
        {
            auto self = shared_from_this();

            const std::size_t sent_size = write_q.front().size();

            asio::async_write(
                socket,
                asio::buffer(write_q.front()),
                [self, sent_size](std::error_code ec, std::size_t /*n*/)
                {
                    if (ec)
                    {
                        self->close();
                        return;
                    }

                    self->write_q.pop_front();

                    if (self->queued_bytes >= sent_size)
                        self->queued_bytes -= sent_size;
                    else
                        self->queued_bytes = 0;

                    if (!self->write_q.empty())
                        self->do_write();
                });
        }

        void do_read()
        {
            if (closed.load())
                return;
            auto self = shared_from_this();
            socket.async_read_some(
                asio::buffer(read_chunk),
                [self](std::error_code ec, std::size_t n)
                {
                    if (ec)
                    {
                        self->close();
                        return;
                    }
                    if (self->closed.load())
                        return;

                    self->stats.bytes_received += n;
                    self->pending.insert(self->pending.end(),
                                         self->read_chunk.begin(),
                                         self->read_chunk.begin() + (std::ptrdiff_t)n);

                    auto decoded = self->framer.decode(self->pending);
                    self->pending = std::move(decoded.remaining);

                    for (auto &f : decoded.frames)
                    {
                        self->stats.frames_received++;
                        try
                        {
                            Envelope env = Envelope::decode_or_throw(f.bytes);
                            if (self->on_envelope)
                                self->on_envelope(self->peer_id, env);
                        }
                        catch (...)
                        {
                            self->close();
                            return;
                        }
                    }

                    self->do_read();
                });
        }
    };

    class TransportTcp final : public Transport
    {
    public:
        explicit TransportTcp(std::shared_ptr<TcpSession> s) : s_(std::move(s)) {}

        TransportKind kind() const override { return TransportKind::Tcp; }

        bool send(std::span<const std::uint8_t> bytes) override
        {
            if (!s_)
                return false;
            s_->send_frame(bytes);
            return true;
        }

        void close() override
        {
            if (s_)
                s_->close();
        }

        TransportStats stats() const override
        {
            return s_ ? s_->stats : TransportStats{};
        }

        std::string endpoint_string() const override
        {
            return s_ ? s_->endpoint_string() : "tcp://(closed)";
        }

    private:
        std::shared_ptr<TcpSession> s_;
    };

    static PeerId make_peer_id(const std::string &host, std::uint16_t port)
    {
        return host + ":" + std::to_string(port);
    }

    std::shared_ptr<Transport> tcp_accept(
        tcp::socket sock,
        EnvelopeHandler on_envelope,
        PeerId &out_peer_id,
        PeerEndpoint &out_endpoint)
    {
        auto remote = sock.remote_endpoint();
        const std::string host = remote.address().to_string();
        const std::uint16_t port = remote.port();

        out_peer_id = make_peer_id(host, port);
        out_endpoint = PeerEndpoint{host, port, "tcp"};

        auto session = std::make_shared<TcpSession>(std::move(sock));
        session->peer_id = out_peer_id;
        session->endpoint = out_endpoint;
        session->on_envelope = std::move(on_envelope);
        session->start();

        return std::make_shared<TransportTcp>(std::move(session));
    }

    void tcp_connect_async(
        asio::io_context &ioc,
        const PeerEndpoint &ep,
        EnvelopeHandler on_envelope,
        TcpReadyHandler on_ready,
        TcpFailHandler on_fail)
    {
        tcp::resolver resolver(ioc);
        auto results = resolver.resolve(ep.host, std::to_string(ep.port));

        auto sock = std::make_shared<tcp::socket>(ioc);

        asio::async_connect(
            *sock, results,
            [sock, on_envelope = std::move(on_envelope),
             on_ready = std::move(on_ready),
             on_fail = std::move(on_fail)](std::error_code ec, const tcp::endpoint &remote) mutable
            {
                if (ec)
                {
                    if (on_fail)
                        on_fail(ec);
                    return;
                }

                const std::string host = remote.address().to_string();
                const std::uint16_t port = remote.port();

                PeerId pid = make_peer_id(host, port);
                PeerEndpoint endpoint{host, port, "tcp"};

                auto session = std::make_shared<TcpSession>(std::move(*sock));
                session->peer_id = pid;
                session->endpoint = endpoint;
                session->on_envelope = std::move(on_envelope);
                session->start();

                auto transport = std::make_shared<TransportTcp>(session);
                if (on_ready)
                    on_ready(std::move(pid), std::move(endpoint), std::move(transport));
            });
    }

} // namespace vix::p2p
