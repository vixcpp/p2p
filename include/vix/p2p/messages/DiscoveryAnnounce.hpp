/**
 *
 *  @file DiscoveryAnnounce.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
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
  struct DiscoveryAnnounce
  {
    std::string node_id;
    std::uint16_t tcp_port{0};
    std::uint64_t ts_ms{0};
    std::uint64_t nonce{0};
    // proto/ver, features…
    std::unordered_map<std::string, std::string> capabilities;
    static constexpr std::size_t kMaxBytes = 512;
    std::string to_json() const;
    static std::optional<DiscoveryAnnounce> from_json(const std::string &s);
  };

} // namespace vix::p2p::msg

#endif
