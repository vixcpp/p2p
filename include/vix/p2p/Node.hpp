/**
 *
 *  @file Node.hpp
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
#ifndef VIX_NODE_HPP
#define VIX_NODE_HPP

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <cstdint>
#include <functional>
#include <string_view>

#include <vix/p2p/Peer.hpp>
#include <vix/p2p/Discovery.hpp>
#include <vix/p2p/Bootstrap.hpp>
#include <vix/p2p/EdgeSync.hpp>
#include <vix/p2p/Crypto.hpp>

namespace vix::p2p
{
  /**
   * @brief Configuration parameters for a P2P node.
   *
   * Defines identity, networking limits, timeouts, and logging hooks.
   */
  struct NodeConfig
  {
    /// Unique identifier of the local node
    std::string node_id;

    /// TCP port to listen on for incoming connections
    std::uint16_t listen_port{0};

    /// Maximum number of peers managed by the node
    std::uint32_t max_peers{64};

    /// Handshake timeout in milliseconds
    std::uint32_t handshake_timeout_ms{3000};

    /// Optional log callback
    std::function<void(std::string_view)> on_log = nullptr;
  };

  /**
   * @brief Runtime statistics for a node.
   *
   * Used for monitoring and diagnostics.
   */
  struct NodeStats
  {
    /// Total number of peers known to the node
    std::uint64_t peers_total{0};

    /// Number of peers currently connected
    std::uint64_t peers_connected{0};

    /// Number of handshakes initiated
    std::uint64_t handshakes_started{0};

    /// Number of handshakes completed successfully
    std::uint64_t handshakes_completed{0};
  };

  class Router; // forward declaration

  /**
   * @brief High-level P2P node interface.
   *
   * A Node coordinates peer connections, discovery, bootstrap,
   * routing, cryptography, and edge synchronization.
   */
  class Node
  {
  public:
    /// Virtual destructor
    virtual ~Node() = default;

    /**
     * @brief Access the node configuration.
     *
     * @return Constant reference to NodeConfig.
     */
    virtual const NodeConfig &config() const = 0;

    /**
     * @brief Start the node.
     *
     * Initializes networking, discovery, and background workers.
     */
    virtual void start() = 0;

    /**
     * @brief Stop the node.
     *
     * Gracefully shuts down networking and background tasks.
     */
    virtual void stop() = 0;

    /**
     * @brief Check whether the node is currently running.
     *
     * @return true if the node is running.
     */
    virtual bool running() const = 0;

    /**
     * @brief Block until the node has stopped.
     */
    virtual void wait() = 0;

    /**
     * @brief Initiate a connection to a remote peer.
     *
     * @param ep Peer endpoint information.
     * @return true if the connection attempt was initiated.
     */
    virtual bool connect(const PeerEndpoint &ep) = 0;

    /**
     * @brief Disconnect from a peer by identifier.
     *
     * @param peer_id Identifier of the peer to disconnect.
     */
    virtual void disconnect(const PeerId &peer_id) = 0;

    /**
     * @brief Retrieve a peer by identifier.
     *
     * @param peer_id Peer identifier.
     * @return Optional peer snapshot.
     */
    virtual std::optional<Peer> get_peer(const PeerId &peer_id) const = 0;

    /**
     * @brief Get a snapshot of all known peers.
     *
     * @return Map of peer identifiers to peer snapshots.
     */
    virtual std::unordered_map<PeerId, Peer> peers_snapshot() const = 0;

    /**
     * @brief Attach a discovery mechanism to the node.
     *
     * @param d Discovery implementation.
     */
    virtual void set_discovery(std::shared_ptr<Discovery> d) = 0;

    /**
     * @brief Attach a bootstrap mechanism to the node.
     *
     * @param b Bootstrap implementation.
     */
    virtual void set_bootstrap(std::shared_ptr<Bootstrap> b) = 0;

    /**
     * @brief Attach a router for application-level messaging.
     *
     * @param r Router implementation.
     */
    virtual void set_router(std::shared_ptr<Router> r) = 0;

    /**
     * @brief Attach an edge synchronization engine.
     *
     * @param s EdgeSync implementation.
     */
    virtual void set_edge_sync(std::shared_ptr<EdgeSync> s) = 0;

    /**
     * @brief Attach a cryptographic provider.
     *
     * @param c Crypto implementation.
     */
    virtual void set_crypto(std::shared_ptr<Crypto> c) = 0;

    /**
     * @brief Retrieve current node statistics.
     *
     * @return NodeStats snapshot.
     */
    virtual NodeStats stats() const = 0;
  };

  /**
   * @brief Create a TCP-based P2P node.
   *
   * @param cfg Node configuration.
   * @return Shared pointer to a Node instance.
   */
  std::shared_ptr<Node> make_tcp_node(NodeConfig cfg);

  /**
   * @brief Set a global log sink for all nodes.
   *
   * @param sink Logging callback.
   */
  void set_global_log_sink(std::function<void(std::string_view)> sink);

  /**
   * @brief Clear the global log sink.
   */
  void clear_global_log_sink();

} // namespace vix::p2p

#endif // VIX_NODE_HPP
