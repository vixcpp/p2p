/**
 *
 *  @file DiscoveryAnnounce.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#include <vix/p2p/messages/DiscoveryAnnounce.hpp>
#include <sstream>
#include <cctype>

namespace vix::p2p::msg
{
  static std::string escape_json(const std::string &s)
  {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s)
    {
      switch (c)
      {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out += c;
        break;
      }
    }
    return out;
  }

  std::string DiscoveryAnnounce::to_json() const
  {
    std::ostringstream os;
    os << "{";
    os << "\"node_id\":\"" << escape_json(node_id) << "\"";
    os << ",\"tcp_port\":" << tcp_port;
    os << ",\"ts_ms\":" << ts_ms;
    os << ",\"nonce\":" << nonce;

    if (!capabilities.empty())
    {
      os << ",\"cap\":{";
      bool first = true;
      for (const auto &[k, v] : capabilities)
      {
        if (!first)
          os << ",";
        first = false;
        os << "\"" << escape_json(k) << "\":\"" << escape_json(v) << "\"";
      }
      os << "}";
    }

    os << "}";
    auto s = os.str();
    if (s.size() > kMaxBytes)
      s.resize(kMaxBytes); // hard cap
    return s;
  }

  // find "key":VALUE
  static std::optional<std::string> find_json_string(const std::string &s, const char *key)
  {
    std::string pat = std::string("\"") + key + "\"";
    auto p = s.find(pat);
    if (p == std::string::npos)
      return std::nullopt;
    p = s.find(':', p);
    if (p == std::string::npos)
      return std::nullopt;
    p++;
    while (p < s.size() && std::isspace((unsigned char)s[p]))
      p++;
    if (p >= s.size() || s[p] != '"')
      return std::nullopt;
    p++;
    std::string out;
    while (p < s.size())
    {
      char c = s[p++];
      if (c == '"')
        break;
      if (c == '\\' && p < s.size())
      {
        out += s[p++];
        continue;
      }
      out += c;
    }
    if (out.empty())
      return std::nullopt;
    return out;
  }

  static std::optional<std::uint64_t> find_json_u64(const std::string &s, const char *key)
  {
    std::string pat = std::string("\"") + key + "\"";
    auto p = s.find(pat);
    if (p == std::string::npos)
      return std::nullopt;
    p = s.find(':', p);
    if (p == std::string::npos)
      return std::nullopt;
    p++;
    while (p < s.size() && std::isspace((unsigned char)s[p]))
      p++;
    std::uint64_t v = 0;
    bool any = false;
    while (p < s.size() && std::isdigit((unsigned char)s[p]))
    {
      any = true;
      v = v * 10 + (std::uint64_t)(s[p] - '0');
      p++;
    }
    if (!any)
      return std::nullopt;
    return v;
  }

  std::optional<DiscoveryAnnounce> DiscoveryAnnounce::from_json(const std::string &s)
  {
    if (s.empty() || s.size() > kMaxBytes)
      return std::nullopt;
    if (s.front() != '{' || s.back() != '}')
      return std::nullopt;

    DiscoveryAnnounce a;

    auto id = find_json_string(s, "node_id");
    auto port = find_json_u64(s, "tcp_port");
    auto ts = find_json_u64(s, "ts_ms");
    auto nonce = find_json_u64(s, "nonce");

    if (!id || !port || !ts || !nonce)
      return std::nullopt;
    if (*port == 0 || *port > 65535ULL)
      return std::nullopt;

    a.node_id = *id;
    a.tcp_port = (std::uint16_t)(*port);
    a.ts_ms = *ts;
    a.nonce = *nonce;

    return a;
  }

} // namespace vix::p2p::msg
