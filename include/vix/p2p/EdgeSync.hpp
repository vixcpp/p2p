#ifndef EDGE_SYNC_HPP
#define EDGE_SYNC_HPP

#include <cstdint>
#include <span>
#include <vector>
#include <string>
#include <optional>

namespace vix::p2p
{

    using NodeId = std::string;

    struct WalBatch
    {
        std::uint64_t seq_begin{0};
        std::uint64_t seq_end{0};
        std::vector<std::uint8_t> bytes;
    };

    struct WalAck
    {
        std::uint64_t last_applied_seq{0};
    };

    struct OutboxPullRequest
    {
        NodeId target;
        std::uint32_t max_items{128};
    };

    class EdgeSync
    {
    public:
        virtual ~EdgeSync() = default;

        virtual void on_wal_batch(const NodeId &from, const WalBatch &batch) = 0;
        virtual void on_wal_ack(const NodeId &from, const WalAck &ack) = 0;
        virtual void on_outbox_pull(const NodeId &from, const OutboxPullRequest &req) = 0;

        virtual std::optional<WalBatch> next_wal_batch_for(const NodeId &to) = 0;
    };

    class NullEdgeSync final : public EdgeSync
    {
    public:
        void on_wal_batch(const NodeId &, const WalBatch &) override {}
        void on_wal_ack(const NodeId &, const WalAck &) override {}
        void on_outbox_pull(const NodeId &, const OutboxPullRequest &) override {}
        std::optional<WalBatch> next_wal_batch_for(const NodeId &) override { return std::nullopt; }
    };

} // namespace vix::p2p

#endif