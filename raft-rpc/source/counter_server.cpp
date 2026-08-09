// Replicated counter server built on eBay/NuRaft (Raft core + ASIO transport).
//
// Robustness features demonstrated:
//   * auto-forwarding: followers transparently forward client writes to the
//     leader, so clients may talk to ANY node;
//   * linearizable reads: `get` goes through the Raft log (a committed no-op),
//     so it never returns stale data, even right after a leader change;
//   * durable vote/term state (file_state_mgr): a restarted node can never
//     vote twice in one term;
//   * periodic logical snapshots + log compaction (snapshot_distance);
//   * runtime membership changes (addsrv / rmsrv admin commands).
//
// Cluster formation: every node boots as a single-node cluster; nodes 2..N
// are then joined via `addsrv` sent to node 1 (see scripts/run_cluster.sh).
//
// Client protocol (line-based TCP on raft_port + 1000):
//   add <delta>      -> "OK <new_value>"
//   get              -> "OK <value>"          (linearizable, through the log)
//   local            -> "OK <value> leader=<id> term=<term>"  (may be stale)
//   status           -> "OK id=.. leader=.. term=.. commit=.. value=.."
//   addsrv <id> <raft_endpoint>  -> "OK" (leader only)
//   rmsrv <id>                   -> "OK" (leader only)
//
// Usage: counter_server <server_id> <host:raft_port> [data_dir]

#include <atomic>
#include <csignal>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <libnuraft/nuraft.hxx>

#include "counter_state_machine.hxx"
#include "file_state_mgr.hxx"
#include "simple_logger.hxx"

namespace
{

constexpr int kClientPortOffset = 1000;

std::atomic<bool> g_stop {false};
int g_listen_fd = -1;

void handle_signal(int /*sig*/)
{
  g_stop.store(true);
  if (g_listen_fd >= 0) {
    ::close(g_listen_fd);
  }
}

struct server_ctx
{
  int id = 1;
  std::string host;
  int raft_port = 0;
  std::string endpoint;
  std::string data_dir;

  nuraft::ptr<counter::counter_state_machine> sm;
  nuraft::ptr<counter::file_state_mgr> smgr;
  nuraft::ptr<counter::simple_logger> raft_logger;
  nuraft::raft_launcher launcher;
  nuraft::ptr<nuraft::raft_server> raft;
};

// The redirect hint sent to clients: leader's client port by convention.
std::string client_endpoint_of(server_ctx& ctx, int srv_id)
{
  auto conf = ctx.raft->get_srv_config(srv_id);
  if (!conf) {
    return "";
  }
  const std::string ep = conf->get_endpoint();
  const size_t pos = ep.rfind(':');
  if (pos == std::string::npos) {
    return "";
  }
  const int raft_port = std::stoi(ep.substr(pos + 1));
  return ep.substr(0, pos) + ":"
      + std::to_string(raft_port + kClientPortOffset);
}

std::string redirect_or_err(server_ctx& ctx, const std::string& what)
{
  const int leader = ctx.raft->get_leader();
  if (leader > 0 && leader != ctx.id) {
    const std::string ep = client_endpoint_of(ctx, leader);
    if (!ep.empty()) {
      return "REDIRECT " + ep;
    }
  }
  return "ERR " + what;
}

// Append one op through the Raft log and wait for consensus (blocking mode).
std::string replicate(server_ctx& ctx,
                      counter::counter_state_machine::op_type op,
                      int64_t delta)
{
  auto log = counter::counter_state_machine::enc_log(op, delta);
  auto ret = ctx.raft->append_entries({log});
  if (!ret->get_accepted()) {
    return redirect_or_err(
        ctx, "append rejected: " + std::to_string(ret->get_result_code()));
  }
  if (ret->get_result_code() != nuraft::cmd_result_code::OK) {
    return redirect_or_err(
        ctx, "commit failed: " + std::to_string(ret->get_result_code()));
  }
  auto buf = ret->get();
  if (!buf) {
    return "ERR empty result";
  }
  nuraft::buffer_serializer bs(*buf);
  return "OK " + std::to_string(bs.get_i64());
}

std::string handle_command(server_ctx& ctx, const std::string& line)
{
  std::istringstream iss(line);
  std::string cmd;
  iss >> cmd;

  if (cmd == "add") {
    int64_t delta = 0;
    if (!(iss >> delta)) {
      return "ERR usage: add <delta>";
    }
    return replicate(ctx, counter::counter_state_machine::OP_ADD, delta);
  }
  if (cmd == "get") {
    // Linearizable read: committed through the log like any write.
    return replicate(ctx, counter::counter_state_machine::OP_GET, 0);
  }
  if (cmd == "local") {
    return "OK " + std::to_string(ctx.sm->current_value())
        + " leader=" + std::to_string(ctx.raft->get_leader())
        + " term=" + std::to_string(ctx.raft->get_term());
  }
  if (cmd == "status") {
    std::ostringstream oss;
    oss << "OK id=" << ctx.id << " leader=" << ctx.raft->get_leader()
        << " term=" << ctx.raft->get_term()
        << " commit=" << ctx.raft->get_committed_log_idx()
        << " value=" << ctx.sm->current_value();
    return oss.str();
  }
  if (cmd == "addsrv") {
    int id = 0;
    std::string ep;
    if (!(iss >> id >> ep)) {
      return "ERR usage: addsrv <id> <host:raft_port>";
    }
    if (!ctx.raft->is_leader()) {
      return redirect_or_err(ctx, "not leader");
    }
    nuraft::srv_config conf(id, ep);
    auto ret = ctx.raft->add_srv(conf);
    return ret->get_accepted()
        ? "OK"
        : "ERR add_srv: " + std::to_string(ret->get_result_code());
  }
  if (cmd == "rmsrv") {
    int id = 0;
    if (!(iss >> id)) {
      return "ERR usage: rmsrv <id>";
    }
    if (!ctx.raft->is_leader()) {
      return redirect_or_err(ctx, "not leader");
    }
    auto ret = ctx.raft->remove_srv(id);
    return ret->get_accepted()
        ? "OK"
        : "ERR remove_srv: " + std::to_string(ret->get_result_code());
  }
  return "ERR unknown command";
}

void serve_connection(server_ctx& ctx, int fd)
{
  std::string pending;
  char buf[512];
  while (!g_stop.load()) {
    const ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
    if (n <= 0) {
      break;
    }
    pending.append(buf, static_cast<size_t>(n));
    size_t pos = 0;
    while ((pos = pending.find('\n')) != std::string::npos) {
      std::string line = pending.substr(0, pos);
      pending.erase(0, pos + 1);
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      if (line == "quit") {
        ::close(fd);
        return;
      }
      const std::string reply = handle_command(ctx, line) + "\n";
      if (::send(fd, reply.data(), reply.size(), 0) < 0) {
        ::close(fd);
        return;
      }
    }
  }
  ::close(fd);
}

int run_client_listener(server_ctx& ctx, int port)
{
  g_listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (g_listen_fd < 0) {
    return -1;
  }
  int one = 1;
  ::setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  sockaddr_in addr {};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(static_cast<uint16_t>(port));
  if (::bind(g_listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0
      || ::listen(g_listen_fd, 64) < 0)
  {
    ::close(g_listen_fd);
    return -1;
  }
  std::vector<std::thread> workers;
  while (!g_stop.load()) {
    const int fd = ::accept(g_listen_fd, nullptr, nullptr);
    if (fd < 0) {
      break;  // listener closed by signal handler
    }
    workers.emplace_back([&ctx, fd] { serve_connection(ctx, fd); });
  }
  for (auto& w : workers) {
    w.join();
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv)
{
  if (argc < 3) {
    std::cerr << "usage: " << argv[0]
              << " <server_id> <host:raft_port> [data_dir]\n";
    return 1;
  }
  server_ctx ctx;
  ctx.id = std::atoi(argv[1]);
  ctx.endpoint = argv[2];
  const size_t pos = ctx.endpoint.rfind(':');
  if (ctx.id < 1 || pos == std::string::npos) {
    std::cerr << "bad server id or endpoint\n";
    return 1;
  }
  ctx.host = ctx.endpoint.substr(0, pos);
  ctx.raft_port = std::stoi(ctx.endpoint.substr(pos + 1));
  ctx.data_dir =
      argc > 3 ? argv[3] : "./counter_data_" + std::to_string(ctx.id);

  ctx.sm = nuraft::cs_new<counter::counter_state_machine>();
  ctx.smgr = nuraft::cs_new<counter::file_state_mgr>(
      ctx.id, ctx.endpoint, ctx.data_dir);
  ctx.raft_logger =
      nuraft::cs_new<counter::simple_logger>(ctx.data_dir + "/raft.log", 4);

  nuraft::asio_service::options asio_opt;
  asio_opt.thread_pool_size_ = 4;

  nuraft::raft_params params;
  params.heart_beat_interval_ = 100;
  params.election_timeout_lower_bound_ = 300;
  params.election_timeout_upper_bound_ = 600;
  params.snapshot_distance_ = 50;  // snapshot + compact every 50 log entries
  params.reserved_log_items_ = 10;
  params.client_req_timeout_ = 3000;
  params.return_method_ = nuraft::raft_params::blocking;
  // Followers forward writes to the leader, so clients need no leader
  // discovery. REDIRECT replies remain as a fallback (e.g. no quorum).
  params.auto_forwarding_ = true;
  params.auto_forwarding_max_connections_ = 8;

  ctx.raft = ctx.launcher.init(
      ctx.sm, ctx.smgr, ctx.raft_logger, ctx.raft_port, asio_opt, params);
  if (!ctx.raft) {
    std::cerr << "failed to initialize raft server\n";
    return 1;
  }
  for (int i = 0; i < 100 && !ctx.raft->is_initialized(); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  if (!ctx.raft->is_initialized()) {
    std::cerr << "raft server failed to initialize in time\n";
    return 1;
  }

  std::signal(SIGINT, handle_signal);
  std::signal(SIGTERM, handle_signal);

  const int client_port = ctx.raft_port + kClientPortOffset;
  std::cout << "counter server id=" << ctx.id << " raft=" << ctx.endpoint
            << " client=" << ctx.host << ":" << client_port
            << " data=" << ctx.data_dir << std::endl;

  run_client_listener(ctx, client_port);

  std::cout << "shutting down" << std::endl;
  ctx.launcher.shutdown(5);
  return 0;
}
