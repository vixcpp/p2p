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
#include <vix/p2p/Node.hpp>

namespace vix::p2p
{

  class P2P
  {
  public:
    virtual ~P2P() = default;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual bool connect(const PeerEndpoint &ep) = 0;

    virtual NodeStats stats() const = 0;
  };

  class P2PRuntime final : public P2P
  {
  public:
    explicit P2PRuntime(std::shared_ptr<Node> node) : node_(std::move(node)) {}

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

    bool connect(const PeerEndpoint &ep) override
    {
      return node_ ? node_->connect(ep) : false;
    }

    NodeStats stats() const override
    {
      return node_ ? node_->stats() : NodeStats{};
    }

    std::shared_ptr<Node> node() const { return node_; }

  private:
    std::shared_ptr<Node> node_;
  };

} // namespace vix::p2p

#endif
