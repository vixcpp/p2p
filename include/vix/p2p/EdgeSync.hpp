/**
 *
 *  @file EdgeSync.hpp
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
#ifndef VIX_EDGE_SYNC_HPP
#define VIX_EDGE_SYNC_HPP

#include <cstdint>
#include <span>
#include <vector>
#include <string>
#include <optional>

namespace vix::p2p
{
  /**
   * @brief Unique identifier for a node participating in sync.
   */
  using NodeId = std::string;

  /**
   * @brief Serialized WAL batch exchanged between nodes.
   *
   * Represents a contiguous sequence range with a binary payload.
   */
  struct WalBatch
  {
    /// First sequence number included in this batch
    std::uint64_t seq_begin{0};

    /// Last sequence number included in this batch
    std::uint64_t seq_end{0};

    /// Serialized WAL records for [seq_begin, seq_end]
    std::vector<std::uint8_t> bytes;
  };

  /**
   * @brief Acknowledgment of WAL application progress.
   */
  struct WalAck
  {
    /// Highest sequence number applied by the receiver
    std::uint64_t last_applied_seq{0};
  };

  /**
   * @brief Request to pull pending outbox items for replication.
   */
  struct OutboxPullRequest
  {
    /// Target node for which items are requested
    NodeId target;

    /// Maximum number of items to return
    std::uint32_t max_items{128};
  };

  /**
   * @brief Edge sync interface for store-and-forward replication.
   *
   * This interface models the minimal message flow required to
   * relay WAL records and outbox items through edge nodes.
   */
  class EdgeSync
  {
  public:
    /// Virtual destructor
    virtual ~EdgeSync() = default;

    /**
     * @brief Handle an incoming WAL batch.
     *
     * @param from Sender node id.
     * @param batch WAL batch payload.
     */
    virtual void on_wal_batch(const NodeId &from, const WalBatch &batch) = 0;

    /**
     * @brief Handle an incoming WAL acknowledgment.
     *
     * @param from Sender node id.
     * @param ack Acknowledgment payload.
     */
    virtual void on_wal_ack(const NodeId &from, const WalAck &ack) = 0;

    /**
     * @brief Handle an incoming outbox pull request.
     *
     * @param from Sender node id.
     * @param req Pull request payload.
     */
    virtual void on_outbox_pull(const NodeId &from, const OutboxPullRequest &req) = 0;

    /**
     * @brief Get the next WAL batch to send to a given node.
     *
     * Implementations may return std::nullopt when no batch is available.
     *
     * @param to Destination node id.
     * @return Optional WAL batch.
     */
    virtual std::optional<WalBatch> next_wal_batch_for(const NodeId &to) = 0;
  };

  /**
   * @brief No-op EdgeSync implementation.
   *
   * Used when edge replication is disabled.
   */
  class NullEdgeSync final : public EdgeSync
  {
  public:
    /// Ignore WAL batches
    void on_wal_batch(const NodeId &, const WalBatch &) override {}

    /// Ignore acknowledgments
    void on_wal_ack(const NodeId &, const WalAck &) override {}

    /// Ignore outbox pull requests
    void on_outbox_pull(const NodeId &, const OutboxPullRequest &) override {}

    /// Always returns no batch
    std::optional<WalBatch> next_wal_batch_for(const NodeId &) override { return std::nullopt; }
  };

} // namespace vix::p2p

#endif // VIX_EDGE_SYNC_HPP
