// State manager with durable Raft vote/term state and cluster config.
//
// NuRaft's example state manager keeps everything in memory, which allows a
// restarted node to vote twice in the same term (a real safety violation).
// This one persists `srv_state` (term + voted_for) and the cluster config to
// disk with write-to-temp + rename, so restarts are safe.
//
// The log store itself stays in memory (a restarted node catches up from the
// leader's snapshot); a production system would persist it too.

#pragma once

#include <cstdio>
#include <fstream>
#include <string>

#include <sys/stat.h>

#include <libnuraft/nuraft.hxx>

#include "mem_log_store.hxx"

namespace counter
{

class file_state_mgr : public nuraft::state_mgr
{
public:
  file_state_mgr(int srv_id,
                 const std::string& endpoint,
                 const std::string& data_dir)
      : my_id_(srv_id)
      , my_endpoint_(endpoint)
      , data_dir_(data_dir)
      , log_store_(nuraft::cs_new<mem_log_store>())
  {
    ::mkdir(data_dir_.c_str(), 0755);
    my_srv_config_ = nuraft::cs_new<nuraft::srv_config>(srv_id, endpoint);
  }

  nuraft::ptr<nuraft::cluster_config> load_config() override
  {
    auto buf = read_file(data_dir_ + "/config");
    if (buf) {
      return nuraft::cluster_config::deserialize(*buf);
    }
    // First boot: cluster contains only myself; other members join
    // through add_srv (see `addsrv` admin command).
    auto conf = nuraft::cs_new<nuraft::cluster_config>();
    conf->get_servers().push_back(my_srv_config_);
    return conf;
  }

  void save_config(const nuraft::cluster_config& config) override
  {
    write_file(data_dir_ + "/config", *config.serialize());
  }

  void save_state(const nuraft::srv_state& state) override
  {
    // Persisted BEFORE casting a vote / bumping term, so a restarted
    // node can never vote twice in one term.
    write_file(data_dir_ + "/state", *state.serialize());
  }

  nuraft::ptr<nuraft::srv_state> read_state() override
  {
    auto buf = read_file(data_dir_ + "/state");
    if (buf) {
      return nuraft::srv_state::deserialize(*buf);
    }
    return nullptr;
  }

  nuraft::ptr<nuraft::log_store> load_log_store() override
  {
    return log_store_;
  }

  nuraft::int32 server_id() override { return my_id_; }

  void system_exit(const int /*exit_code*/) override {}

  nuraft::ptr<nuraft::srv_config> get_srv_config() const
  {
    return my_srv_config_;
  }

private:
  static nuraft::ptr<nuraft::buffer> read_file(const std::string& path)
  {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) {
      return nullptr;
    }
    const std::streamsize size = in.tellg();
    if (size <= 0) {
      return nullptr;
    }
    in.seekg(0);
    auto buf = nuraft::buffer::alloc(static_cast<size_t>(size));
    in.read(reinterpret_cast<char*>(buf->data_begin()), size);
    buf->pos(0);
    return buf;
  }

  static void write_file(const std::string& path, nuraft::buffer& buf)
  {
    const std::string tmp = path + ".tmp";
    {
      std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
      out.write(reinterpret_cast<const char*>(buf.data_begin()),
                static_cast<std::streamsize>(buf.size()));
      out.flush();
    }
    std::rename(tmp.c_str(), path.c_str());
  }

  int my_id_;
  std::string my_endpoint_;
  std::string data_dir_;
  nuraft::ptr<mem_log_store> log_store_;
  nuraft::ptr<nuraft::srv_config> my_srv_config_;
};

}  // namespace counter
