/**
 *
 *  @file p2p/p2p.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Internal aggregation header for the Vix P2P module.
 *
 *  This file includes the core peer-to-peer components of Vix,
 *  including bootstrap, discovery, protocol, routing, transport,
 *  framing, and message types.
 *
 *  For most use cases, prefer:
 *    #include <vix/p2p.hpp>
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
#ifndef VIX_P2P_P2P_HPP
#define VIX_P2P_P2P_HPP

// Core
#include <vix/p2p/Bootstrap.hpp>
#include <vix/p2p/Crypto.hpp>
#include <vix/p2p/Discovery.hpp>
#include <vix/p2p/EdgeSync.hpp>
#include <vix/p2p/Framing.hpp>
#include <vix/p2p/Node.hpp>
#include <vix/p2p/P2P.hpp>
#include <vix/p2p/Peer.hpp>
#include <vix/p2p/Protocol.hpp>
#include <vix/p2p/Router.hpp>
#include <vix/p2p/Transport.hpp>

// framing
#include <vix/p2p/framing/LengthPrefixVarint.hpp>

// messages
#include <vix/p2p/messages/Binary.hpp>
#include <vix/p2p/messages/Dispatch.hpp>
#include <vix/p2p/messages/DiscoveryAnnounce.hpp>
#include <vix/p2p/messages/Envelope.hpp>
#include <vix/p2p/messages/Hello.hpp>
#include <vix/p2p/messages/HelloAck.hpp>
#include <vix/p2p/messages/HelloFinish.hpp>
#include <vix/p2p/messages/OutboxPull.hpp>
#include <vix/p2p/messages/Pack.hpp>
#include <vix/p2p/messages/Ping.hpp>
#include <vix/p2p/messages/Pong.hpp>
#include <vix/p2p/messages/WalAck.hpp>
#include <vix/p2p/messages/WalPush.hpp>

#endif // VIX_P2P_P2P_HPP
