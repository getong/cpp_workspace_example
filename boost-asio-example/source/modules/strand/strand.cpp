#include <atomic>
#include <iostream>
#include <thread>
#include <unordered_set>

#include "strand.hpp"

#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/thread_pool.hpp>

namespace modules::strand
{

void run()
{
  constexpr int task_count = 10'000;

  // thread_pool：多个线程并发地执行提交的处理器。
  boost::asio::thread_pool pool {4};

  // strand 包装底层执行器，保证经它提交的处理器串行执行
  // （任意时刻最多一个在运行），即使底层有多个线程。
  auto serialized = boost::asio::make_strand(pool);

  // counter 被上万个处理器读写却不需要任何锁——strand 就是同步机制。
  int counter = 0;
  std::unordered_set<std::thread::id> worker_threads;

  for (int i = 0; i < task_count; ++i) {
    boost::asio::post(serialized,
                      [&]
                      {
                        ++counter;
                        worker_threads.insert(std::this_thread::get_id());
                      });
  }

  // 对照组：直接 post 到线程池的处理器并发运行，
  // 共享的普通 int 会产生数据竞争，这里只能用 atomic。
  std::atomic<int> atomic_counter {0};
  for (int i = 0; i < task_count; ++i) {
    boost::asio::post(pool, [&] { ++atomic_counter; });
  }

  pool.join();

  std::cout << "strand-serialized counter: " << counter << " / " << task_count
            << " (no lock, no atomic)\n";
  std::cout << "strand handlers ran on " << worker_threads.size()
            << " pool thread(s), but never concurrently\n";
  std::cout << "plain pool atomic counter: " << atomic_counter.load() << " / "
            << task_count << '\n';
}

}  // namespace modules::strand
