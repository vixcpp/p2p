/**
 *
 *  @file P2P.hpp
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
#ifndef VIX_P2P_HPP
#define VIX_P2P_HPP

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <cstdint>
#include <algorithm>

#include <vix/p2p/Node.hpp>

namespace vix::p2p
{
  /**
   * @brief Minimal P2P runtime interface.
   *
   * Provides lifecycle control and manual peer connection.
   */
  class P2P
  {
  public:
    /// Virtual destructor
    virtual ~P2P() = default;

    /**
     * @brief Start the runtime.
     */
    virtual void start() = 0;

    /**
     * @brief Stop the runtime.
     */
    virtual void stop() = 0;

    /**
     * @brief Block until the runtime has stopped.
     */
    virtual void wait() = 0;

    /**
     * @brief Manually connect to a peer endpoint.
     *
     * @param ep Peer endpoint.
     * @return true if the connection attempt was initiated.
     */
    virtual bool connect(const PeerEndpoint &ep) = 0;

    /**
     * @brief Get node-level statistics.
     *
     * @return NodeStats snapshot.
     */
    virtual NodeStats stats() const = 0;
  };

  /**
   * @brief Connection guard statistics at runtime level.
   *
   * These counters mirror CLI diagnostics and reflect deduping,
   * failures, and backoff behavior across connection attempts.
   */
  struct ConnectStats
  {
    /// Total number of connection attempts
    std::uint64_t connect_attempts{0};

    /// Attempts skipped due to dedupe protection
    std::uint64_t connect_deduped{0};

    /// Number of failed connection attempts
    std::uint64_t connect_failures{0};

    /// Attempts skipped because of backoff
    std::uint64_t backoff_skips{0};

    /// Number of endpoints currently tracked in the guard table
    std::uint64_t tracked_endpoints{0};
  };

  /**
   * @brief Runtime statistics including node stats and connect guard stats.
   */
  struct RuntimeStats : NodeStats
  {
    /// Connect guard statistics
    ConnectStats connect{};
  };

  /**
   * @brief Concrete P2P runtime implementation backed by a Node.
   *
   * P2PRuntime wraps a Node and provides additional connection guard
   * logic including deduping and backoff tracking.
   */
  class P2PRuntime final : public P2P
  {
  public:
    /**
     * @brief Construct a runtime from an existing node.
     *
     * @param node Node instance.
     */
    explicit P2PRuntime(std::shared_ptr<Node> node)
        : node_(std::move(node))
    {
    }

    /**
     * @brief Start the underlying node.
     */
    void start() override
    {
      if (node_)
        node_->start();
    }

    /**
     * @brief Stop the underlying node.
     */
    void stop() override
    {
      if (node_)
        node_->stop();
    }

    /**
     * @brief Wait for the underlying node to stop.
     */
    void wait() override
    {
      if (node_)
        node_->wait();
    }

    /**
     * @brief Manual connect entry point.
     *
     * Equivalent to CLI-style explicit connection requests.
     *
     * @param ep Peer endpoint.
     * @return true if the connection attempt was initiated.
     */
    bool connect(const PeerEndpoint &ep) override
    {
      return connect(ep, true);
    }

    /**
     * @brief Auto-connect entry point.
     *
     * Intended to be used by discovery/bootstrap subsystems.
     *
     * @param ep Peer endpoint.
     * @return true if the connection attempt was initiated.
     */
    bool connect_auto(const PeerEndpoint &ep)
    {
      return connect(ep, false);
    }

    /**
     * @brief Return node-level statistics.
     *
     * @return NodeStats snapshot.
     */
    NodeStats stats() const override
    {
      return node_ ? node_->stats() : NodeStats{};
    }

    /**
     * @brief Return runtime-level statistics.
     *
     * Includes node stats and connection guard stats.
     *
     * @return RuntimeStats snapshot.
     */
    RuntimeStats runtime_stats() const
    {
      RuntimeStats out{};
      if (node_)
        static_cast<NodeStats &>(out) = node_->stats();

      std::lock_guard<std::mutex> lock(mu_);
      out.connect = connect_stats_;
      out.connect.tracked_endpoints = table_.size();
      return out;
    }

    /**
     * @brief Access the underlying node.
     *
     * @return Shared pointer to Node.
     */
    std::shared_ptr<Node> node() const { return node_; }

  private:
    /**
     * @brief Per-endpoint connection guard entry.
     */
    struct ConnectEntry
    {
      /// Consecutive failure count
      std::uint32_t failures{0};

      /// Time point until which attempts should be skipped
      std::chrono::steady_clock::time_point backoff_until{};

      /// Last attempt time point (dedupe protection)
      std::chrono::steady_clock::time_point last_attempt{};
    };

    /**
     * @brief Convert a string to lowercase.
     *
     * @param s Input string.
     * @return Lowercase copy.
     */
    static std::string to_lower_copy(std::string s)
    {
      for (auto &c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      return s;
    }

    /**
     * @brief Build a stable table key for an endpoint.
     *
     * @param ep Peer endpoint.
     * @return Key formatted as scheme://host:port.
     */
    static std::string key_for(const PeerEndpoint &ep)
    {
      std::string scheme = ep.scheme.empty() ? "tcp" : to_lower_copy(ep.scheme);
      return scheme + "://" + ep.host + ":" + std::to_string(ep.port);
    }

    /**
     * @brief Check whether a connection attempt is allowed.
     *
     * Enforces backoff and dedupe depending on the attempt mode.
     *
     * @param ep Peer endpoint.
     * @param manual true for manual connect, false for auto connect.
     * @return true if attempt is allowed.
     */
    bool allow_attempt_unlocked_(const PeerEndpoint &ep, bool manual)
    {
      const auto now = std::chrono::steady_clock::now();
      const std::string key = key_for(ep);
      auto &e = table_[key];

      if (e.backoff_until.time_since_epoch().count() != 0 && now < e.backoff_until)
      {
        ++connect_stats_.backoff_skips;
        return false;
      }

      constexpr auto kMinAttemptGap = std::chrono::milliseconds(900);
      if (!manual && e.last_attempt.time_since_epoch().count() != 0 && (now - e.last_attempt) < kMinAttemptGap)
      {
        ++connect_stats_.connect_deduped;
        return false;
      }

      e.last_attempt = now;
      ++connect_stats_.connect_attempts;
      return true;
    }

    /**
     * @brief Record a failed connection attempt and apply backoff.
     *
     * @param ep Peer endpoint.
     * @param manual true for manual connect, false for auto connect.
     */
    void mark_failure_unlocked_(const PeerEndpoint &ep, bool manual)
    {
      const auto now = std::chrono::steady_clock::now();
      const std::string key = key_for(ep);
      auto &e = table_[key];

      ++connect_stats_.connect_failures;

      if (manual)
      {
        e.failures = std::min<std::uint32_t>(e.failures + 1, 8u);
        const std::uint64_t backoff_ms =
            std::min<std::uint64_t>(2500ULL * (1ULL << std::min<std::uint32_t>(e.failures - 1, 5u)), 12000ULL);
        e.backoff_until = now + std::chrono::milliseconds(backoff_ms);
      }
      else
      {
        e.failures = std::min<std::uint32_t>(e.failures + 1, 10u);
        const std::uint64_t backoff_ms =
            std::min<std::uint64_t>(2000ULL * (1ULL << std::min<std::uint32_t>(e.failures - 1, 6u)), 15000ULL);
        e.backoff_until = now + std::chrono::milliseconds(backoff_ms);
      }
    }

    /**
     * @brief Record a successful connection attempt and clear backoff state.
     *
     * @param ep Peer endpoint.
     */
    void mark_success_unlocked_(const PeerEndpoint &ep)
    {
      const std::string key = key_for(ep);
      auto it = table_.find(key);
      if (it == table_.end())
        return;
      it->second.failures = 0;
      it->second.backoff_until = {};
    }

    /**
     * @brief Internal connect implementation with guard logic.
     *
     * Ensures the node is started, enforces backoff and dedupe,
     * and updates runtime counters.
     *
     * @param ep Peer endpoint.
     * @param manual true for manual connect, false for auto connect.
     * @return true if the connection attempt was initiated.
     */
    bool connect(const PeerEndpoint &ep, bool manual)
    {
      if (!node_)
        return false;

      node_->start();

      {
        std::lock_guard<std::mutex> lock(mu_);
        if (!allow_attempt_unlocked_(ep, manual))
          return false;
      }

      try
      {
        const bool ok = node_->connect(ep);
        if (ok)
        {
          std::lock_guard<std::mutex> lock(mu_);
          mark_success_unlocked_(ep);
        }
        else
        {
          std::lock_guard<std::mutex> lock(mu_);
          mark_failure_unlocked_(ep, manual);
        }
        return ok;
      }
      catch (...)
      {
        std::lock_guard<std::mutex> lock(mu_);
        mark_failure_unlocked_(ep, manual);
        return false;
      }
    }

  private:
    /// Underlying node implementation
    std::shared_ptr<Node> node_;

    /// Guard table mutex
    mutable std::mutex mu_;

    /// Endpoint guard table
    std::unordered_map<std::string, ConnectEntry> table_;

    /// Connect guard statistics
    ConnectStats connect_stats_{};
  };

} // namespace vix::p2p

#endif // VIX_P2P_HPP
