#pragma once
#include <cstdint>
#include <string>
#include <chrono>
#include <optional>
#include <unordered_map>

namespace vix::p2p
{

    using PeerId = std::string;

    enum class PeerState : std::uint8_t
    {
        Disconnected = 0,
        Connecting,
        Handshaking,
        Connected,
        Stale,
        Closed
    };

    struct PeerEndpoint
    {
        std::string host; // ip/hostname
        std::uint16_t port{0};
        std::string scheme; // "tcp" => "quic"
    };

    struct PeerMetadata
    {
        // Capabilities = map simple pour figer le contrat
        std::unordered_map<std::string, std::string> capabilities;
        std::chrono::steady_clock::time_point last_seen{};
    };

    struct Peer
    {
        PeerId id;
        PeerState state{PeerState::Disconnected};
        std::optional<PeerEndpoint> endpoint;
        PeerMetadata meta;

        bool is_connected() const { return state == PeerState::Connected; }
    };

} // namespace vix::p2p
