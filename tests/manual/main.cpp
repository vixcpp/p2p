#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <optional>

#include "vix/p2p/P2P.hpp"
#include "vix/p2p/Node.hpp"
#include "vix/p2p/Peer.hpp"

static std::atomic<bool> g_running{true};

static void on_sigint(int)
{
    g_running = false;
}

static void print_usage()
{
    std::cout
        << "Usage:\n"
        << "  p2p_demo --id <node_id> --listen <port> [--connect <host:port>]\n"
        << "\n"
        << "Examples:\n"
        << "  # Terminal A\n"
        << "  p2p_demo --id A --listen 9001\n"
        << "\n"
        << "  # Terminal B\n"
        << "  p2p_demo --id B --listen 9002 --connect 127.0.0.1:9001\n";
}

static std::optional<std::string> arg_value(int argc, char **argv, const std::string &key)
{
    for (int i = 1; i < argc - 1; ++i)
    {
        if (argv[i] == key)
            return std::string(argv[i + 1]);
    }
    return std::nullopt;
}

static bool has_flag(int argc, char **argv, const std::string &key)
{
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == key)
            return true;
    }
    return false;
}

static std::optional<vix::p2p::PeerEndpoint> parse_endpoint(const std::string &s)
{
    auto pos = s.find(':');
    if (pos == std::string::npos)
        return std::nullopt;

    vix::p2p::PeerEndpoint ep;
    ep.host = s.substr(0, pos);
    try
    {
        ep.port = static_cast<std::uint16_t>(std::stoul(s.substr(pos + 1)));
    }
    catch (...)
    {
        return std::nullopt;
    }
    ep.scheme = "tcp";
    return ep;
}

int main(int argc, char **argv)
{
    std::signal(SIGINT, on_sigint);

    if (argc == 1 || has_flag(argc, argv, "--help") || has_flag(argc, argv, "-h"))
    {
        print_usage();
        return 0;
    }

    auto id_opt = arg_value(argc, argv, "--id");
    auto listen_opt = arg_value(argc, argv, "--listen");
    auto connect_opt = arg_value(argc, argv, "--connect");

    if (!id_opt || !listen_opt)
    {
        std::cerr << "Missing --id or --listen\n\n";
        print_usage();
        return 1;
    }

    std::uint16_t listen_port = 0;
    try
    {
        listen_port = static_cast<std::uint16_t>(std::stoul(*listen_opt));
    }
    catch (...)
    {
        std::cerr << "Invalid --listen port\n";
        return 1;
    }

    vix::p2p::NodeConfig cfg;
    cfg.node_id = *id_opt;
    cfg.listen_port = listen_port;

    auto node = vix::p2p::make_tcp_node(cfg);

    vix::p2p::P2PRuntime p2p(node);

    std::cout << "[p2p_demo] starting node_id=" << cfg.node_id
              << " listen=" << cfg.listen_port << "\n";

    p2p.start();

    if (connect_opt)
    {
        auto ep = parse_endpoint(*connect_opt);
        if (!ep)
        {
            std::cerr << "Invalid --connect format, expected host:port\n";
            return 1;
        }
        std::cout << "[p2p_demo] connect -> " << ep->host << ":" << ep->port << "\n";
        p2p.connect(*ep);
    }

    while (g_running)
    {
        auto st = p2p.stats();
        std::cout << "[p2p_demo] peers_total=" << st.peers_total
                  << " peers_connected=" << st.peers_connected
                  << " handshakes_started=" << st.handshakes_started
                  << " handshakes_completed=" << st.handshakes_completed
                  << "\n";

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "[p2p_demo] stopping...\n";
    p2p.stop();
    std::cout << "[p2p_demo] bye\n";
    return 0;
}
