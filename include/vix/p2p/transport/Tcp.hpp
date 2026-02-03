/**
 *
 *  @file Tcp.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#ifndef VIX_TCP_HPP
#define VIX_TCP_HPP

#include <asio.hpp>
#include <memory>
#include <functional>

#include <vix/p2p/Transport.hpp>
#include <vix/p2p/Peer.hpp>
#include <vix/p2p/messages/Envelope.hpp>

namespace vix::p2p
{
  /**
   * @brief Callback invoked when a full envelope is received from a peer.
   *
   * @param peer_id Logical peer identifier associated with the connection.
   * @param env Decoded envelope received from the wire.
   */
  using EnvelopeHandler = std::function<void(const PeerId &peer_id, const Envelope &env)>;

  /**
   * @brief Accept an inbound TCP connection and bind it to a Transport.
   *
   * The implementation is expected to:
   * - Wrap the accepted socket into a Transport implementation (TCP).
   * - Start read loops and decode incoming frames into Envelope objects.
   * - Invoke @p on_envelope on each successfully decoded envelope.
   * - Populate @p out_peer_id and @p out_endpoint with the discovered
   *   peer identity/endpoint once available.
   *
   * @param sock Accepted TCP socket (moved into the transport).
   * @param on_envelope Envelope receive callback.
   * @param out_peer_id Output peer id for the accepted connection.
   * @param out_endpoint Output endpoint for the accepted connection.
   * @return Transport instance bound to the accepted connection.
   */
  std::shared_ptr<Transport> tcp_accept(
      asio::ip::tcp::socket sock,
      EnvelopeHandler on_envelope,
      PeerId &out_peer_id,
      PeerEndpoint &out_endpoint);

  /**
   * @brief Callback invoked when an outbound TCP connection is ready.
   *
   * @param peer_id Logical peer identifier (may be provisional depending on handshake).
   * @param endpoint Remote endpoint used for the connection.
   * @param transport Transport instance bound to the connection.
   */
  using TcpReadyHandler = std::function<void(PeerId peer_id, PeerEndpoint endpoint, std::shared_ptr<Transport> transport)>;

  /**
   * @brief Start an asynchronous TCP connect to a remote peer.
   *
   * On success, @p on_ready is invoked with a transport bound to the new
   * connection. Incoming envelopes should be decoded and delivered via
   * @p on_envelope.
   *
   * @param ioc Asio I/O context driving the async operations.
   * @param ep Remote endpoint to connect to.
   * @param on_envelope Envelope receive callback.
   * @param on_ready Called when the connection is established.
   */
  void tcp_connect_async(
      asio::io_context &ioc,
      const PeerEndpoint &ep,
      EnvelopeHandler on_envelope,
      TcpReadyHandler on_ready);

  /**
   * @brief Callback invoked when an outbound TCP connection fails.
   *
   * @param ec Error code describing the failure.
   */
  using TcpFailHandler = std::function<void(std::error_code ec)>;

  /**
   * @brief Start an asynchronous TCP connect with explicit failure handling.
   *
   * This overload provides an additional @p on_fail callback invoked when the
   * connect operation cannot be completed.
   *
   * @param ioc Asio I/O context driving the async operations.
   * @param ep Remote endpoint to connect to.
   * @param on_envelope Envelope receive callback.
   * @param on_ready Called when the connection is established.
   * @param on_fail Called when the connection attempt fails.
   */
  void tcp_connect_async(
      asio::io_context &ioc,
      const PeerEndpoint &ep,
      EnvelopeHandler on_envelope,
      TcpReadyHandler on_ready,
      TcpFailHandler on_fail);

} // namespace vix::p2p

#endif // VIX_TCP_HPP
