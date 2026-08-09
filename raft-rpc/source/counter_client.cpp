// Client for the NuRaft counter cluster.
//
// Talks the line-based TCP protocol to any node (auto-forwarding makes every
// node accept writes). Robustness: tries each endpoint in turn, follows
// REDIRECT hints, and retries with capped exponential backoff.
//
// Usage:
//   counter_client <ep1,ep2,...> add <delta>
//   counter_client <ep1,ep2,...> get | local | status
//   counter_client <ep1,ep2,...> addsrv <id> <host:raft_port>
//   counter_client <ep1,ep2,...> rmsrv <id>
//   counter_client <ep1,ep2,...> bench <n>       # n sequential adds
// Endpoints are CLIENT endpoints (raft_port + 1000).

#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{

constexpr int kMaxRounds = 8;
constexpr int kBackoffCapMs = 3000;

std::vector<std::string> split(const std::string& s, char sep)
{
  std::vector<std::string> out;
  std::istringstream iss(s);
  std::string item;
  while (std::getline(iss, item, sep)) {
    if (!item.empty()) {
      out.push_back(item);
    }
  }
  return out;
}

int connect_to(const std::string& endpoint)
{
  const size_t pos = endpoint.rfind(':');
  if (pos == std::string::npos) {
    return -1;
  }
  const std::string host = endpoint.substr(0, pos);
  const std::string port = endpoint.substr(pos + 1);

  addrinfo hints {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* res = nullptr;
  if (::getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0) {
    return -1;
  }
  int fd = -1;
  for (addrinfo* p = res; p; p = p->ai_next) {
    fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (fd < 0) {
      continue;
    }
    timeval tv {3, 0};
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
      break;
    }
    ::close(fd);
    fd = -1;
  }
  ::freeaddrinfo(res);
  return fd;
}

// One request/response round trip; empty result means transport failure.
std::string request_once(const std::string& endpoint, const std::string& line)
{
  const int fd = connect_to(endpoint);
  if (fd < 0) {
    return "";
  }
  const std::string out = line + "\n";
  if (::send(fd, out.data(), out.size(), 0) < 0) {
    ::close(fd);
    return "";
  }
  std::string reply;
  char buf[512];
  while (reply.find('\n') == std::string::npos) {
    const ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
    if (n <= 0) {
      ::close(fd);
      return "";
    }
    reply.append(buf, static_cast<size_t>(n));
  }
  ::close(fd);
  return reply.substr(0, reply.find('\n'));
}

// Try every endpoint, follow redirects, back off between rounds.
std::string request(std::vector<std::string> endpoints, const std::string& line)
{
  int backoff_ms = 100;
  for (int round = 0; round < kMaxRounds; ++round) {
    for (size_t i = 0; i < endpoints.size(); ++i) {
      const std::string reply = request_once(endpoints[i], line);
      if (reply.empty()) {
        continue;  // transport failure: try next node
      }
      if (reply.rfind("REDIRECT ", 0) == 0) {
        // Put the hinted leader first and retry immediately.
        const std::string leader_ep = reply.substr(9);
        std::vector<std::string> reordered {leader_ep};
        for (const auto& ep : endpoints) {
          if (ep != leader_ep) {
            reordered.push_back(ep);
          }
        }
        endpoints = std::move(reordered);
        break;
      }
      return reply;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
    backoff_ms = std::min(backoff_ms * 2, kBackoffCapMs);
  }
  return "ERR no reachable node after " + std::to_string(kMaxRounds)
      + " rounds";
}

}  // namespace

int main(int argc, char** argv)
{
  if (argc < 3) {
    std::cerr
        << "usage: " << argv[0]
        << " <ep1,ep2,...> <add|get|local|status|addsrv|rmsrv|bench> [args]\n";
    return 1;
  }
  const std::vector<std::string> endpoints = split(argv[1], ',');
  const std::string cmd = argv[2];

  std::ostringstream line;
  line << cmd;
  for (int i = 3; i < argc; ++i) {
    line << ' ' << argv[i];
  }

  if (cmd == "bench") {
    const long n = argc > 3 ? std::atol(argv[3]) : 100;
    const auto t0 = std::chrono::steady_clock::now();
    long ok = 0;
    for (long i = 0; i < n; ++i) {
      if (request(endpoints, "add 1").rfind("OK", 0) == 0) {
        ++ok;
      }
    }
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - t0)
                        .count();
    std::cout << ok << "/" << n << " ops in " << ms << " ms ("
              << (ms > 0 ? ok * 1000 / ms : 0) << " ops/s)\n"
              << request(endpoints, "get") << std::endl;
    return ok == n ? 0 : 1;
  }

  const std::string reply = request(endpoints, line.str());
  std::cout << reply << std::endl;
  return reply.rfind("OK", 0) == 0 ? 0 : 1;
}
