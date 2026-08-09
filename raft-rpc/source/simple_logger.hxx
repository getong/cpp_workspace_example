// Minimal nuraft::logger writing timestamped lines to a file.
// (NuRaft's bundled SimpleLogger example is ~1700 lines; this keeps the
// example readable. Levels: 1=fatal ... 6=trace, matching NuRaft.)

#pragma once

#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

#include <libnuraft/nuraft.hxx>

namespace counter
{

class simple_logger : public nuraft::logger
{
public:
  explicit simple_logger(const std::string& path, int level = 3)
      : fp_(std::fopen(path.c_str(), "a"))
      , level_(level)
  {
  }

  ~simple_logger() override
  {
    if (fp_) {
      std::fclose(fp_);
    }
  }

  void put_details(int level,
                   const char* source_file,
                   const char* func_name,
                   size_t line_number,
                   const std::string& msg) override
  {
    if (level > level_ || !fp_) {
      return;
    }
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);
    const auto us =
        duration_cast<microseconds>(now.time_since_epoch()).count() % 1000000;
    std::tm tm_buf {};
    localtime_r(&t, &tm_buf);
    char ts[32];
    std::strftime(ts, sizeof(ts), "%F %T", &tm_buf);

    static const char* kLevel[] = {
        "??", "FATL", "ERRO", "WARN", "INFO", "DEBG", "TRAC"};
    std::lock_guard<std::mutex> l(lock_);
    std::fprintf(fp_,
                 "%s.%06lld [%s] %s:%zu %s | %s\n",
                 ts,
                 static_cast<long long>(us),
                 kLevel[level >= 0 && level <= 6 ? level : 0],
                 source_file ? source_file : "",
                 line_number,
                 func_name ? func_name : "",
                 msg.c_str());
    std::fflush(fp_);
  }

  void set_level(int l) override { level_ = l; }

  int get_level() override { return level_; }

private:
  std::FILE* fp_;
  int level_;
  std::mutex lock_;
};

}  // namespace counter
