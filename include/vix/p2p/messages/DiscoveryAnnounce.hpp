#ifndef DISCOVERY_ANNOUNCE_HP
#define DISCOVERY_ANNOUNCE_HP

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