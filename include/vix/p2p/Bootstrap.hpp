/**
 *
 *  @file Bootstrap.hpp
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
#ifndef VIX_BOOTSTRAP_HPP
#define VIX_BOOTSTRAP_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <vix/p2p/Peer.hpp>

namespace vix::p2p
{
  /**
   * @brief Description of a peer obtained via bootstrap.
   *
   * Represents minimal connection information required to
   * initiate a P2P connection to a remote node.
   */
  struct BootstrapPeer
  {
    /// Unique identifier of the remote node
    std::string node_id;

    /// Hostname or IP address
    std::string host;

    /// TCP listening port
    std::uint16_t tcp_port{0};

    /// Transport protocol name (e.g. "tcp", "quic")
    std::string transport{"tcp"};
  };

  /**
   * @brief Bootstrap operating mode.
   */
  enum class BootstrapMode : std::uint8_t
  {
    PullOnly = 0,
    PullAndAnnounce = 1
  };

  /**
   * @brief Configuration parameters for the bootstrap process.
   *
   * Controls polling behavior, timeouts, backoff strategy,
   * and peer discovery limits.
   */
  struct BootstrapConfig
  {
    /// Local node identifier
    std::string self_node_id;

    /// Local TCP listening port
    std::uint16_t self_tcp_port{0};

    /// Bootstrap registry endpoint URL
    std::string registry_url;

    /// Bootstrap behavior mode
    BootstrapMode mode{BootstrapMode::PullOnly};

    /// Poll interval for registry queries (milliseconds)
    std::uint32_t poll_interval_ms{15000};

    /// Cooldown before retrying a failed connection (milliseconds)
    std::uint32_t connect_cooldown_ms{12000};

    /// Maximum HTTP response size (bytes)
    std::size_t max_http_bytes{64 * 1024};

    /// Connection establishment timeout (milliseconds)
    std::uint32_t connect_timeout_ms{3000};

    /// HTTP request timeout (milliseconds)
    std::uint32_t request_timeout_ms{5000};

    /// Maximum exponential backoff delay (milliseconds)
    std::uint32_t backoff_max_ms{60000};

    /// Maximum number of peers returned per poll
    std::uint32_t max_peers_per_poll{20};
  };

  /**
   * @brief Callback invoked when a bootstrap peer is discovered.
   */
  using BootstrapCallback = std::function<void(const BootstrapPeer &)>;

  /**
   * @brief Abstract bootstrap interface.
   *
   * Implementations are responsible for discovering peers
   * and providing periodic snapshots of known bootstrap nodes.
   */
  class Bootstrap
  {
  public:
    /// Virtual destructor
    virtual ~Bootstrap() = default;

    /**
     * @brief Start the bootstrap process.
     */
    virtual void start() = 0;

    /**
     * @brief Stop the bootstrap process.
     */
    virtual void stop() = 0;

    /**
     * @brief Return a snapshot of currently known bootstrap peers.
     *
     * @return Vector of bootstrap peers.
     */
    virtual std::vector<BootstrapPeer> snapshot() const = 0;
  };

  /**
   * @brief No-op bootstrap implementation.
   *
   * Used when peer discovery is disabled or handled externally.
   */
  class NullBootstrap final : public Bootstrap
  {
  public:
    /// Start does nothing
    void start() override {}

    /// Stop does nothing
    void stop() override {}

    /// Always returns an empty snapshot
    std::vector<BootstrapPeer> snapshot() const override { return {}; }
  };

  /**
   * @brief Create an HTTP-based bootstrap implementation.
   *
   * @param cfg Bootstrap configuration.
   * @param on_peer Callback invoked for each discovered peer.
   * @return Shared pointer to a Bootstrap instance.
   */
  std::shared_ptr<Bootstrap>
  make_http_bootstrap(BootstrapConfig cfg, BootstrapCallback on_peer);

} // namespace vix::p2p

#endif // VIX_BOOTSTRAP_HPP
