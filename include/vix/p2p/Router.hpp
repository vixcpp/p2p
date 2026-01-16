/**
 *
 *  @file Router.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
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

  using NodeId = std::string;

  struct Route
  {
    std::string next_hop;
    bool via_relay{false};
    std::uint8_t ttl{8};
  };

  class Router
  {
  public:
    virtual ~Router() = default;

    virtual void upsert_route(const NodeId &target, const Route &route) = 0;
    virtual void remove_route(const NodeId &target) = 0;
    virtual std::optional<Route> resolve(const NodeId &target) const = 0;
  };

  class MemoryRouter final : public Router
  {
  public:
    void upsert_route(const NodeId &target, const Route &route) override
    {
      table_[target] = route;
    }

    void remove_route(const NodeId &target) override
    {
      table_.erase(target);
    }

    std::optional<Route> resolve(const NodeId &target) const override
    {
      auto it = table_.find(target);
      if (it == table_.end())
        return std::nullopt;
      return it->second;
    }

  private:
    std::unordered_map<NodeId, Route> table_;
  };

} // namespace vix::p2p

#endif
