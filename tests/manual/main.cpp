/**
 *
 *  @file main.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <vix/p2p/P2P.hpp>
#include <vix/p2p/Node.hpp>
#include <vix/p2p/Peer.hpp>
#include <vix/p2p/Bootstrap.hpp>

static std::atomic<bool> g_running{true};

static void on_sigint(int)
{
  g_running = false;
}

static void print_usage()
{
  std::cout
      << "Usage:\n"
      << "  p2p_demo --id <node_id> --listen <port> [options]\n"
      << "\n"
      << "Options:\n"
      << "  --connect <host:port>       Connect to a peer after start\n"
      << "  --connect-delay <ms>        Delay before calling connect()\n"
      << "  --run <seconds>             Auto-stop after N seconds\n"
      << "  --stats-every <ms>          Stats print interval (default: 1000)\n"
      << "  --quiet                     Print only final stats\n"
      << "  --help, -h                  Show this help\n"
      << "\n"
      << "Examples:\n"
      << "  # Terminal A\n"
      << "  p2p_demo --id A --listen 9001\n"
      << "\n"
      << "  # Terminal B\n"
      << "  p2p_demo --id B --listen 9002 --connect 127.0.0.1:9001\n"
      << "\n"
      << "  # Handshake-timeout scenario (connect late)\n"
      << "  p2p_demo --id B --listen 9002 --connect 127.0.0.1:9001 --connect-delay 8000\n"
      << "\n"
      << "  # Run 20s and exit\n"
      << "  p2p_demo --id A --listen 9001 --run 20\n";
}

static std::optional<std::string> arg_value(int argc, char **argv, const std::string &key)
{
  for (int i = 1; i < argc - 1; ++i)
  {
    if (std::string(argv[i]) == key)
      return std::string(argv[i + 1]);
  }
  return std::nullopt;
}

static bool has_flag(int argc, char **argv, const std::string &key)
{
  for (int i = 1; i < argc; ++i)
  {
    if (std::string(argv[i]) == key)
      return true;
  }
  return false;
}

static bool has_any_flag(int argc, char **argv, const std::vector<std::string> &keys)
{
  for (const auto &k : keys)
    if (has_flag(argc, argv, k))
      return true;
  return false;
}

static std::optional<std::uint64_t> parse_u64(const std::string &s)
{
  try
  {
    std::size_t idx = 0;
    unsigned long long v = std::stoull(s, &idx, 10);
    if (idx != s.size())
      return std::nullopt;
    return static_cast<std::uint64_t>(v);
  }
  catch (...)
  {
    return std::nullopt;
  }
}

static std::optional<std::uint16_t> parse_u16(const std::string &s)
{
  auto v = parse_u64(s);
  if (!v || *v > 65535ULL)
    return std::nullopt;
  return static_cast<std::uint16_t>(*v);
}

static std::optional<vix::p2p::PeerEndpoint> parse_endpoint(const std::string &s)
{
  // Support:
  //   host:port
  //   tcp://host:port
  std::string x = s;
  constexpr const char *kPrefix = "tcp://";
  if (x.rfind(kPrefix, 0) == 0)
    x = x.substr(std::string(kPrefix).size());

  auto pos = x.rfind(':');
  if (pos == std::string::npos || pos == 0 || pos + 1 >= x.size())
    return std::nullopt;

  vix::p2p::PeerEndpoint ep;
  ep.host = x.substr(0, pos);

  auto port_opt = parse_u16(x.substr(pos + 1));
  if (!port_opt)
    return std::nullopt;

  ep.port = *port_opt;
  ep.scheme = "tcp";
  return ep;
}

static void print_stats_line(const vix::p2p::NodeStats &st)
{
  std::cout
      << "[p2p_demo] peers_total=" << st.peers_total
      << " peers_connected=" << st.peers_connected
      << " handshakes_started=" << st.handshakes_started
      << " handshakes_completed=" << st.handshakes_completed
      << "\n";
}

int main(int argc, char **argv)
{
  std::signal(SIGINT, on_sigint);

  if (argc == 1 || has_any_flag(argc, argv, {"--help", "-h"}))
  {
    print_usage();
    return 0;
  }

  const auto id_opt = arg_value(argc, argv, "--id");
  const auto listen_opt = arg_value(argc, argv, "--listen");
  const auto connect_opt = arg_value(argc, argv, "--connect");

  const bool quiet = has_flag(argc, argv, "--quiet");

  std::uint64_t connect_delay_ms = 0;
  if (auto s = arg_value(argc, argv, "--connect-delay"))
  {
    auto v = parse_u64(*s);
    if (!v)
    {
      std::cerr << "Invalid --connect-delay (ms)\n";
      return 1;
    }
    connect_delay_ms = *v;
  }

  std::uint64_t run_seconds = 0;
  if (auto s = arg_value(argc, argv, "--run"))
  {
    auto v = parse_u64(*s);
    if (!v)
    {
      std::cerr << "Invalid --run (seconds)\n";
      return 1;
    }
    run_seconds = *v;
  }

  std::uint64_t stats_every_ms = 1000;
  if (auto s = arg_value(argc, argv, "--stats-every"))
  {
    auto v = parse_u64(*s);
    if (!v || *v == 0)
    {
      std::cerr << "Invalid --stats-every (ms)\n";
      return 1;
    }
    stats_every_ms = *v;
  }

  if (!id_opt || !listen_opt)
  {
    std::cerr << "Missing --id or --listen\n\n";
    print_usage();
    return 1;
  }

  const auto listen_port_opt = parse_u16(*listen_opt);
  if (!listen_port_opt)
  {
    std::cerr << "Invalid --listen port\n";
    return 1;
  }

  vix::p2p::NodeConfig cfg;
  cfg.node_id = *id_opt;
  cfg.listen_port = *listen_port_opt;

  auto node = vix::p2p::make_tcp_node(cfg);
  vix::p2p::P2PRuntime p2p(node);

  if (!quiet)
  {
    std::cout << "[p2p_demo] starting node_id=" << cfg.node_id
              << " listen=" << cfg.listen_port << "\n";
  }

  bool discovery_on = true;
  if (auto s = arg_value(argc, argv, "--discovery"))
  {
    if (*s == "on")
      discovery_on = true;
    else if (*s == "off")
      discovery_on = false;
    else
    {
      std::cerr << "Invalid --discovery (on|off)\n";
      return 1;
    }
  }

  std::uint16_t disc_port = 37020;
  if (auto s = arg_value(argc, argv, "--disc-port"))
  {
    auto v = parse_u16(*s);
    if (!v)
    {
      std::cerr << "Invalid --disc-port\n";
      return 1;
    }
    disc_port = *v;
  }

  vix::p2p::DiscoveryMode disc_mode = vix::p2p::DiscoveryMode::Broadcast;
  if (auto s = arg_value(argc, argv, "--disc-mode"))
  {
    if (*s == "broadcast")
      disc_mode = vix::p2p::DiscoveryMode::Broadcast;
    else if (*s == "multicast")
      disc_mode = vix::p2p::DiscoveryMode::Multicast;
    else
    {
      std::cerr << "Invalid --disc-mode (broadcast|multicast)\n";
      return 1;
    }
  }

  std::uint32_t disc_interval_ms = 2000;
  if (auto s = arg_value(argc, argv, "--disc-interval"))
  {
    auto v = parse_u64(*s);
    if (!v || *v == 0)
    {
      std::cerr << "Invalid --disc-interval\n";
      return 1;
    }
    disc_interval_ms = (std::uint32_t)(*v * 1000ULL);
  }

  bool bootstrap_on = false;
  if (auto s = arg_value(argc, argv, "--bootstrap"))
  {
    if (*s == "on")
      bootstrap_on = true;
    else if (*s == "off")
      bootstrap_on = false;
    else
    {
      std::cerr << "Invalid --bootstrap (on|off)\n";
      return 1;
    }
  }

  std::string registry = "http://127.0.0.1:8080/p2p/v1";
  if (auto s = arg_value(argc, argv, "--registry"))
    registry = *s;

  std::uint64_t boot_interval_sec = 15;
  if (auto s = arg_value(argc, argv, "--boot-interval"))
  {
    auto v = parse_u64(*s);
    if (!v || *v == 0)
    {
      std::cerr << "Invalid --boot-interval (seconds)\n";
      return 1;
    }
    boot_interval_sec = *v;
  }

  bool announce_on = true;
  if (auto s = arg_value(argc, argv, "--announce"))
  {
    if (*s == "on")
      announce_on = true;
    else if (*s == "off")
      announce_on = false;
    else
    {
      std::cerr << "Invalid --announce (on|off)\n";
      return 1;
    }
  }

  const bool no_connect = has_flag(argc, argv, "--no-connect");

  if (bootstrap_on)
  {
    vix::p2p::BootstrapConfig bc;
    bc.self_node_id = cfg.node_id;
    bc.self_tcp_port = cfg.listen_port;
    bc.registry_url = registry;
    bc.poll_interval_ms = (std::uint32_t)(boot_interval_sec * 1000ULL);
    bc.mode = announce_on ? vix::p2p::BootstrapMode::PullAndAnnounce
                          : vix::p2p::BootstrapMode::PullOnly;

    auto boot = vix::p2p::make_http_bootstrap(bc, [node, no_connect](const vix::p2p::BootstrapPeer &p)
                                              {
        if (no_connect) return;

        vix::p2p::PeerEndpoint ep;
        ep.host = p.host;
        ep.port = p.tcp_port;
        ep.scheme = "tcp";

        node->connect(ep); });

    node->set_bootstrap(boot);
  }

  // inject discovery
  if (discovery_on)
  {
    vix::p2p::DiscoveryConfig dc;
    dc.self_node_id = cfg.node_id;
    dc.self_tcp_port = cfg.listen_port;
    dc.discovery_port = disc_port;
    dc.mode = disc_mode;
    dc.announce_interval_ms = disc_interval_ms;

    auto disc = vix::p2p::make_udp_discovery(dc, [node, no_connect](const vix::p2p::DiscoveryAnnouncement &a)
                                             {
        if (no_connect) return;

        vix::p2p::PeerEndpoint ep;
        ep.host = a.host;
        ep.port = a.port;
        ep.scheme = "tcp";

        node->connect(ep); });

    node->set_discovery(disc);
  }

  p2p.start();

  // Optional outbound connect
  if (connect_opt)
  {
    auto ep = parse_endpoint(*connect_opt);
    if (!ep)
    {
      std::cerr << "Invalid --connect format, expected host:port or tcp://host:port\n";
      return 1;
    }

    if (!quiet)
    {
      std::cout << "[p2p_demo] connect -> " << ep->host << ":" << ep->port
                << " (delay=" << connect_delay_ms << "ms)\n";
    }

    if (connect_delay_ms > 0)
      std::this_thread::sleep_for(std::chrono::milliseconds(connect_delay_ms));

    p2p.connect(*ep);
  }

  // Auto-stop timer (optional)
  std::thread stopper;
  if (run_seconds > 0)
  {
    stopper = std::thread([run_seconds]()
                          {
            std::this_thread::sleep_for(std::chrono::seconds(run_seconds));
            g_running = false; });
  }

  // Stats loop
  const auto tick = std::chrono::milliseconds(stats_every_ms);
  while (g_running)
  {
    if (!quiet)
    {
      auto st = p2p.stats();
      print_stats_line(st);
    }
    std::this_thread::sleep_for(tick);
  }

  if (stopper.joinable())
    stopper.join();

  if (!quiet)
    std::cout << "[p2p_demo] stopping...\n";

  p2p.stop();

  auto final_st = p2p.stats();
  std::cout << "[p2p_demo] final: ";
  print_stats_line(final_st);

  if (!quiet)
    std::cout << "[p2p_demo] bye\n";

  return 0;
}
