#pragma once
#include <asio.hpp>
#include <memory>
#include <functional>

#include "../Transport.hpp"
#include "../Peer.hpp"
#include "../messages/Envelope.hpp"

namespace vix::p2p
{

    using EnvelopeHandler = std::function<void(const PeerId &, const Envelope &)>;

    std::shared_ptr<Transport> tcp_accept(
        asio::ip::tcp::socket sock,
        EnvelopeHandler on_envelope,
        PeerId &out_peer_id,
        PeerEndpoint &out_endpoint);

    using TcpReadyHandler = std::function<void(PeerId, PeerEndpoint, std::shared_ptr<Transport>)>;

    void tcp_connect_async(
        asio::io_context &ioc,
        const PeerEndpoint &ep,
        EnvelopeHandler on_envelope,
        TcpReadyHandler on_ready);

    using TcpFailHandler = std::function<void(std::error_code)>;

    void tcp_connect_async(
        asio::io_context &ioc,
        const PeerEndpoint &ep,
        EnvelopeHandler on_envelope,
        TcpReadyHandler on_ready,
        TcpFailHandler on_fail);

} // namespace vix::p2p
