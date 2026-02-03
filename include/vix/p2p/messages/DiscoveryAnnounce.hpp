/**
 *
 *  @file DiscoveryAnnounce.hpp
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
#ifndef VIX_DISCOVERY_ANNOUNCE_HP
#define VIX_DISCOVERY_ANNOUNCE_HP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <optional>

namespace vix::p2p::msg
{
  /**
   * @brief Discovery announcement message payload.
   *
   * This message is broadcast or multicast on the local network
   * to advertise the presence of a node and provide minimal
   * connection and capability information.
   *
   * The serialized representation is expected to fit within
   * a strict size limit to avoid UDP fragmentation.
   */
  struct DiscoveryAnnounce
  {
    /// Unique identifier of the announcing node
    std::string node_id;

    /// TCP listening port of the node
    std::uint16_t tcp_port{0};

    /// Announcement timestamp (milliseconds since epoch)
    std::uint64_t ts_ms{0};

    /// Random nonce to prevent replay and cache collisions
    std::uint64_t nonce{0};

    /// Advertised protocol version, features, and capabilities
    std::unordered_map<std::string, std::string> capabilities;

    /// Maximum allowed serialized size in bytes
    static constexpr std::size_t kMaxBytes = 512;

    /**
     * @brief Serialize the announcement to JSON.
     *
     * @return JSON-encoded string representation.
     */
    std::string to_json() const;

    /**
     * @brief Parse a discovery announcement from JSON.
     *
     * The function returns std::nullopt if parsing fails or
     * if the payload exceeds the maximum allowed size.
     *
     * @param s JSON string input.
     * @return Parsed DiscoveryAnnounce on success.
     */
    static std::optional<DiscoveryAnnounce> from_json(const std::string &s);
  };

} // namespace vix::p2p::msg

#endif // VIX_DISCOVERY_ANNOUNCE_HP
