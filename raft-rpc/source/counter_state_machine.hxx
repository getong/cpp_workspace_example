// Replicated counter state machine for NuRaft.
//
// Two operations go through the Raft log:
//   ADD <delta> — mutates the counter, returns the new value;
//   GET         — no-op read; because it is committed through the log it is a
//                 linearizable read on ANY node (with auto-forwarding).
// Snapshots use NuRaft's logical-snapshot path (single object holding the
// counter value), and the last few snapshots are retained in memory.

#pragma once

#include <atomic>
#include <map>
#include <mutex>

#include <libnuraft/nuraft.hxx>

namespace counter
{

class counter_state_machine : public nuraft::state_machine
{
public:
  enum op_type : uint8_t
  {
    OP_ADD = 0,
    OP_GET = 1,
  };

  counter_state_machine()
      : value_(0)
      , last_committed_idx_(0)
  {
  }

  static nuraft::ptr<nuraft::buffer> enc_log(op_type op, int64_t delta)
  {
    auto buf = nuraft::buffer::alloc(sizeof(uint8_t) + sizeof(int64_t));
    nuraft::buffer_serializer bs(buf);
    bs.put_u8(op);
    bs.put_i64(delta);
    return buf;
  }

  static void dec_log(nuraft::buffer& data, op_type& op_out, int64_t& delta_out)
  {
    nuraft::buffer_serializer bs(data);
    op_out = static_cast<op_type>(bs.get_u8());
    delta_out = bs.get_i64();
  }

  nuraft::ptr<nuraft::buffer> pre_commit(const nuraft::ulong /*log_idx*/,
                                         nuraft::buffer& /*data*/) override
  {
    return nullptr;
  }

  nuraft::ptr<nuraft::buffer> commit(const nuraft::ulong log_idx,
                                     nuraft::buffer& data) override
  {
    op_type op = OP_GET;
    int64_t delta = 0;
    dec_log(data, op, delta);

    int64_t result = 0;
    if (op == OP_ADD) {
      result = value_.fetch_add(delta, std::memory_order_relaxed) + delta;
    } else {
      result = value_.load(std::memory_order_relaxed);
    }
    last_committed_idx_.store(log_idx, std::memory_order_release);

    auto ret = nuraft::buffer::alloc(sizeof(int64_t));
    nuraft::buffer_serializer bs(ret);
    bs.put_i64(result);
    return ret;
  }

  void commit_config(const nuraft::ulong log_idx,
                     nuraft::ptr<nuraft::cluster_config>& /*new_conf*/) override
  {
    last_committed_idx_.store(log_idx, std::memory_order_release);
  }

  void rollback(const nuraft::ulong /*log_idx*/,
                nuraft::buffer& /*data*/) override
  {
    // pre_commit does nothing, so there is nothing to roll back.
  }

  // ---- Logical snapshot: one object carrying the counter value. ----

  int read_logical_snp_obj(nuraft::snapshot& s,
                           void*& /*user_snp_ctx*/,
                           nuraft::ulong /*obj_id*/,
                           nuraft::ptr<nuraft::buffer>& data_out,
                           bool& is_last_obj) override
  {
    int64_t snapshot_value = 0;
    {
      std::lock_guard<std::mutex> l(snapshots_lock_);
      auto it = snapshots_.find(s.get_last_log_idx());
      if (it == snapshots_.end()) {
        data_out = nullptr;
        is_last_obj = true;
        return -1;
      }
      snapshot_value = it->second.value;
    }
    data_out = nuraft::buffer::alloc(sizeof(int64_t));
    nuraft::buffer_serializer bs(data_out);
    bs.put_i64(snapshot_value);
    is_last_obj = true;
    return 0;
  }

  void save_logical_snp_obj(nuraft::snapshot& s,
                            nuraft::ulong& obj_id,
                            nuraft::buffer& data,
                            bool /*is_first_obj*/,
                            bool /*is_last_obj*/) override
  {
    nuraft::buffer_serializer bs(data);
    const int64_t snapshot_value = bs.get_i64();

    auto snp_buf = s.serialize();
    auto ss = nuraft::snapshot::deserialize(*snp_buf);
    {
      std::lock_guard<std::mutex> l(snapshots_lock_);
      snapshots_[s.get_last_log_idx()] = {ss, snapshot_value};
      gc_snapshots_locked();
    }
    obj_id++;
  }

  bool apply_snapshot(nuraft::snapshot& s) override
  {
    std::lock_guard<std::mutex> l(snapshots_lock_);
    auto it = snapshots_.find(s.get_last_log_idx());
    if (it == snapshots_.end()) {
      return false;
    }
    value_.store(it->second.value, std::memory_order_relaxed);
    last_committed_idx_.store(s.get_last_log_idx(), std::memory_order_release);
    return true;
  }

  void free_user_snp_ctx(void*& /*user_snp_ctx*/) override {}

  nuraft::ptr<nuraft::snapshot> last_snapshot() override
  {
    std::lock_guard<std::mutex> l(snapshots_lock_);
    auto it = snapshots_.rbegin();
    return it == snapshots_.rend() ? nullptr : it->second.meta;
  }

  nuraft::ulong last_commit_index() override
  {
    return last_committed_idx_.load(std::memory_order_acquire);
  }

  void create_snapshot(
      nuraft::snapshot& s,
      nuraft::async_result<bool>::handler_type& when_done) override
  {
    // Capturing one int64 is cheap; do it synchronously. A bigger state
    // machine should copy-on-write here and flush on another thread.
    auto snp_buf = s.serialize();
    auto ss = nuraft::snapshot::deserialize(*snp_buf);
    {
      std::lock_guard<std::mutex> l(snapshots_lock_);
      snapshots_[s.get_last_log_idx()] = {
          ss, value_.load(std::memory_order_relaxed)};
      gc_snapshots_locked();
    }
    bool ok = true;
    nuraft::ptr<std::exception> err(nullptr);
    when_done(ok, err);
  }

  int64_t current_value() const
  {
    return value_.load(std::memory_order_relaxed);
  }

private:
  struct snapshot_ctx
  {
    nuraft::ptr<nuraft::snapshot> meta;
    int64_t value = 0;
  };

  void gc_snapshots_locked()
  {
    constexpr size_t kMaxSnapshots = 3;
    while (snapshots_.size() > kMaxSnapshots) {
      snapshots_.erase(snapshots_.begin());
    }
  }

  std::atomic<int64_t> value_;
  std::atomic<nuraft::ulong> last_committed_idx_;
  std::map<nuraft::ulong, snapshot_ctx> snapshots_;
  std::mutex snapshots_lock_;
};

}  // namespace counter
