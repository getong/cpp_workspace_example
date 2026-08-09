// In-memory Raft log store for the NuRaft counter example.
//
// Modeled after NuRaft's examples/in_memory_log_store (Apache 2.0) but
// self-contained: it only uses installed public headers.
//
// NOTE: a production log store must be durable (fsync'ed writes, torn-write
// protection). See ClickHouse Keeper's `KeeperLogStore` or eBay's usage for
// real implementations. Here, a restarted node recovers by fetching the
// leader's snapshot + log tail, which is fine for a demo cluster.

#pragma once

#include <map>
#include <mutex>

#include <libnuraft/nuraft.hxx>

namespace counter
{

class mem_log_store : public nuraft::log_store
{
public:
  mem_log_store()
      : start_idx_(1)
  {
    // Index 0 is a dummy entry required by the interface contract.
    nuraft::ptr<nuraft::buffer> buf = nuraft::buffer::alloc(sizeof(uint64_t));
    logs_[0] = nuraft::cs_new<nuraft::log_entry>(0, buf);
  }

  nuraft::ulong next_slot() const override
  {
    std::lock_guard<std::mutex> l(lock_);
    return start_idx_ + logs_.size() - 1;
  }

  nuraft::ulong start_index() const override
  {
    std::lock_guard<std::mutex> l(lock_);
    return start_idx_;
  }

  nuraft::ptr<nuraft::log_entry> last_entry() const override
  {
    std::lock_guard<std::mutex> l(lock_);
    auto it = logs_.find(start_idx_ + logs_.size() - 2);
    if (it == logs_.end()) {
      it = logs_.find(0);
    }
    return clone(it->second);
  }

  nuraft::ulong append(nuraft::ptr<nuraft::log_entry>& entry) override
  {
    std::lock_guard<std::mutex> l(lock_);
    nuraft::ulong idx = start_idx_ + logs_.size() - 1;
    logs_[idx] = clone(entry);
    return idx;
  }

  void write_at(nuraft::ulong index,
                nuraft::ptr<nuraft::log_entry>& entry) override
  {
    std::lock_guard<std::mutex> l(lock_);
    // Discard all logs equal to or greater than `index`, then write.
    auto it = logs_.lower_bound(index);
    while (it != logs_.end()) {
      it = logs_.erase(it);
    }
    logs_[index] = clone(entry);
  }

  nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> log_entries(
      nuraft::ulong start, nuraft::ulong end) override
  {
    auto ret = nuraft::cs_new<std::vector<nuraft::ptr<nuraft::log_entry>>>();
    ret->reserve(end - start);
    std::lock_guard<std::mutex> l(lock_);
    for (nuraft::ulong i = start; i < end; ++i) {
      auto it = logs_.find(i);
      if (it == logs_.end()) {
        return nullptr;
      }
      ret->push_back(clone(it->second));
    }
    return ret;
  }

  nuraft::ptr<nuraft::log_entry> entry_at(nuraft::ulong index) override
  {
    std::lock_guard<std::mutex> l(lock_);
    auto it = logs_.find(index);
    if (it == logs_.end()) {
      it = logs_.find(0);
    }
    return clone(it->second);
  }

  nuraft::ulong term_at(nuraft::ulong index) override
  {
    std::lock_guard<std::mutex> l(lock_);
    auto it = logs_.find(index);
    if (it == logs_.end()) {
      it = logs_.find(0);
    }
    return it->second->get_term();
  }

  nuraft::ptr<nuraft::buffer> pack(nuraft::ulong index,
                                   nuraft::int32 cnt) override
  {
    std::vector<nuraft::ptr<nuraft::buffer>> serialized;
    serialized.reserve(cnt);
    size_t total = 0;
    {
      std::lock_guard<std::mutex> l(lock_);
      for (nuraft::ulong i = index; i < index + cnt; ++i) {
        auto it = logs_.find(i);
        if (it == logs_.end()) {
          break;
        }
        auto buf = it->second->serialize();
        total += buf->size();
        serialized.push_back(buf);
      }
    }
    auto out = nuraft::buffer::alloc(sizeof(nuraft::int32)
                                     + serialized.size() * sizeof(nuraft::int32)
                                     + total);
    out->put(static_cast<nuraft::int32>(serialized.size()));
    for (auto& b : serialized) {
      out->put(static_cast<nuraft::int32>(b->size()));
      out->put(*b);
    }
    out->pos(0);
    return out;
  }

  void apply_pack(nuraft::ulong index, nuraft::buffer& pack) override
  {
    pack.pos(0);
    nuraft::int32 num = pack.get_int();
    std::lock_guard<std::mutex> l(lock_);
    for (nuraft::int32 i = 0; i < num; ++i) {
      nuraft::ulong idx = index + i;
      nuraft::int32 size = pack.get_int();
      auto buf = nuraft::buffer::alloc(size);
      pack.get(buf);
      buf->pos(0);
      logs_[idx] = nuraft::log_entry::deserialize(*buf);
    }
    auto it = logs_.upper_bound(0);
    if (it != logs_.end()) {
      start_idx_ = it->first;
    } else {
      start_idx_ = 1;
    }
  }

  bool compact(nuraft::ulong last_log_index) override
  {
    std::lock_guard<std::mutex> l(lock_);
    for (nuraft::ulong i = start_idx_; i <= last_log_index; ++i) {
      logs_.erase(i);
    }
    if (start_idx_ <= last_log_index) {
      start_idx_ = last_log_index + 1;
    }
    return true;
  }

  bool flush() override
  {
    // Nothing to fsync for an in-memory store.
    return true;
  }

private:
  static nuraft::ptr<nuraft::log_entry> clone(
      const nuraft::ptr<nuraft::log_entry>& entry)
  {
    return nuraft::cs_new<nuraft::log_entry>(
        entry->get_term(),
        nuraft::buffer::clone(entry->get_buf()),
        entry->get_val_type(),
        entry->get_timestamp());
  }

  std::map<nuraft::ulong, nuraft::ptr<nuraft::log_entry>> logs_;
  mutable std::mutex lock_;
  nuraft::ulong start_idx_;
};

}  // namespace counter
