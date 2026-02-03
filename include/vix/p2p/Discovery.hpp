/**
 *
 *  @file Discovery.hpp
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
#ifndef VIX_DISCOVERY_HPP
#define VIX_DISCOVERY_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>

#include <vix/p2p/Peer.hpp>

namespace vix::p2p
{
  /**
   * @brief UDP discovery announcement payload.
   *
   * This structure contains the minimal information required to
   * connect to a peer discovered on the local network.
   */
  struct DiscoveryAnnouncement
  {
    /// Unique identifier of the announcing node
    std::string node_id;

    /// Host address (IP)
    std::string host;

    /// TCP listening port of the peer
    std::uint16_t port{0};

    /// Transport protocol name (e.g. "tcp")
    std::string transport;
  };

  /**
   * @brief Discovery transport mode.
   */
  enum class DiscoveryMode : std::uint8_t
  {
    Broadcast = 0,
    Multicast = 1
  };

  /**
   * @brief Configuration parameters for UDP peer discovery.
   *
   * Controls announce frequency, packet limits, TTL for seen peers,
   * and optional multicast group settings.
   */
  struct DiscoveryConfig
  {
    /// Local node identifier
    std::string self_node_id;

    /// Local TCP listening port advertised to other peers
    std::uint16_t self_tcp_port{0};

    /// UDP port used for discovery announcements
    std::uint16_t discovery_port{37020};

    /// Broadcast or multicast discovery mode
    DiscoveryMode mode{DiscoveryMode::Broadcast};

    /// Announcement interval (milliseconds)
    std::uint32_t announce_interval_ms{2000};

    /// Time to keep a seen peer entry before expiring it (milliseconds)
    std::uint32_t seen_ttl_ms{15000};

    /// Cooldown before attempting to reconnect to the same peer (milliseconds)
    std::uint32_t connect_cooldown_ms{8000};

    /// Maximum allowed UDP packet size (bytes)
    std::size_t max_packet_bytes{1024};

    /// Multicast group address (multicast mode only)
    std::string multicast_group{"239.255.0.1"};
  };

  /**
   * @brief Callback invoked when a peer announcement is received.
   */
  using DiscoveryCallback = std::function<void(const DiscoveryAnnouncement &)>;

  /**
   * @brief Abstract peer discovery interface.
   *
   * Implementations continuously announce and listen for peers
   * on the local network and expose a snapshot of recently seen nodes.
   */
  class Discovery
  {
  public:
    /// Virtual destructor
    virtual ~Discovery() = default;

    /**
     * @brief Start discovery (announce and listen).
     */
    virtual void start() = 0;

    /**
     * @brief Stop discovery.
     */
    virtual void stop() = 0;

    /**
     * @brief Return a snapshot of currently known announcements.
     *
     * @return Vector of discovery announcements.
     */
    virtual std::vector<DiscoveryAnnouncement> snapshot() const = 0;
  };

  /**
   * @brief No-op discovery implementation.
   *
   * Used when local network discovery is disabled.
   */
  class NullDiscovery final : public Discovery
  {
  public:
    /// Start does nothing
    void start() override {}

    /// Stop does nothing
    void stop() override {}

    /// Always returns an empty snapshot
    std::vector<DiscoveryAnnouncement> snapshot() const override { return {}; }
  };

  /**
   * @brief Create a UDP-based discovery implementation.
   *
   * @param cfg Discovery configuration.
   * @param on_peer Callback invoked for each received announcement.
   * @return Shared pointer to a Discovery instance.
   */
  std::shared_ptr<Discovery>
  make_udp_discovery(DiscoveryConfig cfg, DiscoveryCallback on_peer);

} // namespace vix::p2p

#endif // VIX_DISCOVERY_HPP
