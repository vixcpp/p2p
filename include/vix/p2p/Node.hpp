/**
 *
 *  @file Node.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#ifndef VIX_NODE_HPP
#define VIX_NODE_HPP

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <cstdint>

#include <vix/p2p/Peer.hpp>
#include <vix/p2p/Discovery.hpp>
#include <vix/p2p/Bootstrap.hpp>
#include <vix/p2p/EdgeSync.hpp>
#include <vix/p2p/Crypto.hpp>

namespace vix::p2p
{
  struct NodeConfig
  {
    std::string node_id;
    std::uint16_t listen_port{0};
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

  class Router; // forward declaration

  class Node
  {
  public:
    virtual ~Node() = default;

    virtual const NodeConfig &config() const = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool running() const = 0;

    virtual bool connect(const PeerEndpoint &ep) = 0;
    virtual void disconnect(const PeerId &peer_id) = 0;
    virtual std::optional<Peer> get_peer(const PeerId &peer_id) const = 0;
    virtual std::unordered_map<PeerId, Peer> peers_snapshot() const = 0;

    virtual void set_discovery(std::shared_ptr<Discovery> d) = 0;
    virtual void set_bootstrap(std::shared_ptr<Bootstrap> b) = 0;
    virtual void set_router(std::shared_ptr<Router> r) = 0;
    virtual void set_edge_sync(std::shared_ptr<EdgeSync> s) = 0;
    virtual void set_crypto(std::shared_ptr<Crypto> c) = 0;

    virtual NodeStats stats() const = 0;
  };

  std::shared_ptr<Node> make_tcp_node(NodeConfig cfg);
}

#endif
