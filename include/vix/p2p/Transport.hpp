/**
 *
 *  @file Transport.hpp
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
#ifndef VIX_TRANSPORT_HPP
#define VIX_TRANSPORT_HPP

#include <cstdint>
#include <span>
#include <string>

namespace vix::p2p
{
  /**
   * @brief Supported transport kinds.
   */
  enum class TransportKind : std::uint8_t
  {
    Tcp = 1,
    Quic = 2
  };

  /**
   * @brief Transport-level statistics.
   *
   * Provides byte and frame counters for diagnostics.
   */
  struct TransportStats
  {
    /// Total number of bytes sent
    std::uint64_t bytes_sent{0};

    /// Total number of bytes received
    std::uint64_t bytes_received{0};

    /// Number of frames sent
    std::uint64_t frames_sent{0};

    /// Number of frames received
    std::uint64_t frames_received{0};
  };

  /**
   * @brief Abstract transport interface.
   *
   * Encapsulates low-level data transmission between peers.
   */
  class Transport
  {
  public:
    /// Virtual destructor
    virtual ~Transport() = default;

    /**
     * @brief Return the transport kind.
     *
     * @return Transport kind.
     */
    virtual TransportKind kind() const = 0;

    /**
     * @brief Send a framed payload.
     *
     * @param frame Encoded frame bytes.
     * @return true if the send was accepted.
     */
    virtual bool send(std::span<const std::uint8_t> frame) = 0;

    /**
     * @brief Close the transport connection.
     */
    virtual void close() = 0;

    /**
     * @brief Retrieve transport statistics.
     *
     * @return TransportStats snapshot.
     */
    virtual TransportStats stats() const = 0;

    /**
     * @brief Return a human-readable endpoint string.
     *
     * Typically formatted as scheme://host:port.
     *
     * @return Endpoint string.
     */
    virtual std::string endpoint_string() const = 0;

    /**
     * @brief Associate a peer identifier with the transport.
     *
     * @param peer_id Peer identifier.
     */
    virtual void set_peer_id(std::string peer_id) = 0;
  };

} // namespace vix::p2p

#endif // VIX_TRANSPORT_HPP
