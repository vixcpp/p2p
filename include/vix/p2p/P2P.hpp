/**
 *
 *  @file P2P.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
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
  class P2P
  {
  public:
    virtual ~P2P() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void wait() = 0;
    virtual bool connect(const PeerEndpoint &ep) = 0;
    virtual NodeStats stats() const = 0;
  };

  // Runtime-level stats (CLI parity)
  struct ConnectStats
  {
    std::uint64_t connect_attempts{0};
    std::uint64_t connect_deduped{0};
    std::uint64_t connect_failures{0};
    std::uint64_t backoff_skips{0};
    std::uint64_t tracked_endpoints{0};
  };

  struct RuntimeStats : NodeStats
  {
    ConnectStats connect{};
  };

  class P2PRuntime final : public P2P
  {
  public:
    explicit P2PRuntime(std::shared_ptr<Node> node)
        : node_(std::move(node))
    {
    }

    void start() override
    {
      if (node_)
        node_->start();
    }

    void stop() override
    {
      if (node_)
        node_->stop();
    }

    void wait() override
    {
      if (node_)
        node_->wait();
    }

    // Default: manual connect (same spirit as CLI --connect)
    bool connect(const PeerEndpoint &ep) override
    {
      return connect(ep, /*manual=*/true);
    }

    // Used by discovery/bootstrap (auto connects)
    bool connect_auto(const PeerEndpoint &ep)
    {
      return connect(ep, /*manual=*/false);
    }

    NodeStats stats() const override
    {
      return node_ ? node_->stats() : NodeStats{};
    }

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

    std::shared_ptr<Node> node() const { return node_; }

  private:
    struct ConnectEntry
    {
      std::uint32_t failures{0};
      std::chrono::steady_clock::time_point backoff_until{};
      std::chrono::steady_clock::time_point last_attempt{};
    };

    static std::string to_lower_copy(std::string s)
    {
      for (auto &c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      return s;
    }

    static std::string key_for(const PeerEndpoint &ep)
    {
      std::string scheme = ep.scheme.empty() ? "tcp" : to_lower_copy(ep.scheme);
      return scheme + "://" + ep.host + ":" + std::to_string(ep.port);
    }

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

    void mark_success_unlocked_(const PeerEndpoint &ep)
    {
      const std::string key = key_for(ep);
      auto it = table_.find(key);
      if (it == table_.end())
        return;
      it->second.failures = 0;
      it->second.backoff_until = {};
    }

    bool connect(const PeerEndpoint &ep, bool manual)
    {
      if (!node_)
        return false;

      // ensure started
      node_->start();

      // guard + backoff/dedupe
      {
        std::lock_guard<std::mutex> lock(mu_);
        if (!allow_attempt_unlocked_(ep, manual))
          return false;
      }

      // attempt connect
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
    std::shared_ptr<Node> node_;

    // ConnectGuard state (runtime truth)
    mutable std::mutex mu_;
    std::unordered_map<std::string, ConnectEntry> table_;
    ConnectStats connect_stats_{};
  };

} // namespace vix::p2p

#endif
