/**
 *
 *  @file Router.hpp
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
#ifndef VIX_ROUTER_HPP
#define VIX_ROUTER_HPP

#include <string>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace vix::p2p
{
  /**
   * @brief Unique identifier of a node.
   */
  using NodeId = std::string;

  /**
   * @brief Routing information for reaching a target node.
   */
  struct Route
  {
    /// Identifier of the next hop node
    std::string next_hop;

    /// Indicates whether the route goes through a relay
    bool via_relay{false};

    /// Time-to-live for forwarded messages
    std::uint8_t ttl{8};
  };

  /**
   * @brief Abstract routing table interface.
   *
   * Responsible for resolving next-hop information for
   * destination nodes in the P2P network.
   */
  class Router
  {
  public:
    /// Virtual destructor
    virtual ~Router() = default;

    /**
     * @brief Insert or update a route entry.
     *
     * @param target Destination node identifier.
     * @param route Route information.
     */
    virtual void upsert_route(const NodeId &target, const Route &route) = 0;

    /**
     * @brief Remove a route entry.
     *
     * @param target Destination node identifier.
     */
    virtual void remove_route(const NodeId &target) = 0;

    /**
     * @brief Resolve routing information for a target node.
     *
     * @param target Destination node identifier.
     * @return Optional route information.
     */
    virtual std::optional<Route> resolve(const NodeId &target) const = 0;
  };

  /**
   * @brief In-memory router implementation.
   *
   * Stores routing information in a local hash table.
   */
  class MemoryRouter final : public Router
  {
  public:
    /**
     * @brief Insert or update a route entry.
     *
     * @param target Destination node identifier.
     * @param route Route information.
     */
    void upsert_route(const NodeId &target, const Route &route) override
    {
      table_[target] = route;
    }

    /**
     * @brief Remove a route entry.
     *
     * @param target Destination node identifier.
     */
    void remove_route(const NodeId &target) override
    {
      table_.erase(target);
    }

    /**
     * @brief Resolve routing information for a target node.
     *
     * @param target Destination node identifier.
     * @return Optional route information.
     */
    std::optional<Route> resolve(const NodeId &target) const override
    {
      auto it = table_.find(target);
      if (it == table_.end())
        return std::nullopt;
      return it->second;
    }

  private:
    /// Internal routing table
    std::unordered_map<NodeId, Route> table_;
  };

} // namespace vix::p2p

#endif // VIX_ROUTER_HPP
