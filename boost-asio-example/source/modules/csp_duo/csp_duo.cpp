#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "csp_duo.hpp"

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/error_code.hpp>

// 本模块回答一个问题：**两个 co_spawn 出来的协程，通过 channel 互相
// 通信，到底能得到什么？** 四个最小示例，每个都恰好是两个顶层协程：
//
//   1. 交替打印  —— 通信就是同步：channel 替代锁和条件变量做顺序协调
//   2. 全双工对话 —— 状态封装：一方私有状态，另一方只能通过消息访问
//   3. 所有权移交 —— 数据免竞争：对象经 channel 移动，永远只有一方持有
//   4. 反向流控  —— 控制流可以逆着数据流走：消费方决定生产节奏
//
// 共同的底层机制：co_await send/receive 挂起的是协程不是线程；
// 两个协程在同一个单线程 io_context 上交错推进，
// 每一次 channel 会合都是一次确定的控制权交接。

namespace modules::csp_duo
{

namespace
{

using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::use_awaitable;

using int_channel =
    boost::asio::experimental::channel<void(boost::system::error_code, int)>;
using int_channel_ptr = std::shared_ptr<int_channel>;

// ---- 示例 1：交替打印（channel 当锁/条件变量用） ------------------
//
// 经典面试题"两个线程交替打印奇偶数"要用 mutex + condition_variable +
// 共享 flag。两个协程用一根"接力棒"channel 即可：数字本身就是棒，
// 谁收到谁打印，打完把下一个数递回去。不存在共享变量，也就不存在
// 竞争和虚假唤醒。

constexpr int alternate_limit = 8;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto odd_printer(int_channel_ptr to_even, int_channel_ptr to_odd)
    -> awaitable<void>
{
  int value = 1;
  for (;;) {
    std::cout << "  [odd ] prints " << value << '\n';
    // 把下一个数（偶数）递给对方——这一步同时完成了"解锁+通知"。
    co_await to_even->async_send({}, value + 1, use_awaitable);
    // 等对方递回来；channel 被关即散场信号。
    auto [err, next] = co_await to_odd->async_receive(as_tuple(use_awaitable));
    if (err) {
      break;
    }
    value = next;
  }
  std::cout << "  [odd ] done\n";
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto even_printer(int_channel_ptr to_even, int_channel_ptr to_odd)
    -> awaitable<void>
{
  for (;;) {
    const int value = co_await to_even->async_receive(use_awaitable);
    std::cout << "  [even] prints " << value << '\n';
    if (value + 1 <= alternate_limit) {
      co_await to_odd->async_send({}, value + 1, use_awaitable);
    } else {
      to_odd->close();  // 数字用完，通知对方散场
      break;
    }
  }
  std::cout << "  [even] done\n";
}

void alternate_demo()
{
  boost::asio::io_context ioc;
  auto to_even = std::make_shared<int_channel>(ioc, 0);
  auto to_odd = std::make_shared<int_channel>(ioc, 0);

  // 两个 co_spawn：奇数方先手（第一个数在它手里）。
  boost::asio::co_spawn(
      ioc, odd_printer(to_even, to_odd), boost::asio::detached);
  boost::asio::co_spawn(
      ioc, even_printer(to_even, to_odd), boost::asio::detached);
  ioc.run();
}

// ---- 示例 2：全双工对话（状态封装在对端） --------------------------
//
// requests + responses 两根单向 channel 组成一条全双工链路，
// 像进程内的一对微服务。累加器 total 是 calculator 协程的局部变量：
// 没有任何其他代码能看到它，user 只能"发消息问、收消息知"。
// 这正是"通过通信来共享内存"——内存本身从不共享。

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto calculator(int_channel_ptr requests, int_channel_ptr responses)
    -> awaitable<void>
{
  int total = 0;  // 私有状态：只存在于本协程帧里
  for (;;) {
    auto [err, delta] = co_await requests->async_receive(as_tuple(use_awaitable));
    if (err) {
      break;
    }
    total += delta;
    co_await responses->async_send({}, total, use_awaitable);
  }
  std::cout << "  [calc] session closed, final total " << total << '\n';
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto calc_user(int_channel_ptr requests, int_channel_ptr responses)
    -> awaitable<void>
{
  for (const int delta : {5, 7, 30}) {
    co_await requests->async_send({}, delta, use_awaitable);
    const int total = co_await responses->async_receive(use_awaitable);
    std::cout << "  [user] add " << delta << " -> total " << total << '\n';
  }
  requests->close();  // 挂断：对端的 receive 随即得到 channel_closed
}

void duplex_demo()
{
  boost::asio::io_context ioc;
  auto requests = std::make_shared<int_channel>(ioc, 0);
  auto responses = std::make_shared<int_channel>(ioc, 0);

  boost::asio::co_spawn(
      ioc, calculator(requests, responses), boost::asio::detached);
  boost::asio::co_spawn(
      ioc, calc_user(requests, responses), boost::asio::detached);
  ioc.run();
}

// ---- 示例 3：所有权移交（move-only 数据过 channel） ----------------
//
// channel 传的不只是数字：unique_ptr 这样的 move-only 对象会被移动
// 着送过去。发送完成后生产方手里是空指针，数据自始至终只有一个
// 持有者——数据竞争在类型系统层面就不可能发生，且全程零拷贝
// （对比：跨线程共享大 buffer 要么加锁要么深拷贝）。

using payload = std::unique_ptr<std::vector<int>>;
using payload_channel =
    boost::asio::experimental::channel<void(boost::system::error_code,
                                            payload)>;
using payload_channel_ptr = std::shared_ptr<payload_channel>;

constexpr int payload_size = 1'000'000;

auto payload_builder(payload_channel_ptr chan) -> awaitable<void>
{
  auto data = std::make_unique<std::vector<int>>(payload_size, 7);
  std::cout << "  [builder ] built 1M ints at address " << data->data()
            << '\n';
  co_await chan->async_send({}, std::move(data), use_awaitable);
  std::cout << "  [builder ] after send, my pointer is "
            << (data == nullptr ? "null (ownership gone)" : "still set?!")
            << '\n';
}

auto payload_consumer(payload_channel_ptr chan) -> awaitable<void>
{
  const payload data = co_await chan->async_receive(use_awaitable);
  std::cout << "  [consumer] received the same buffer at " << data->data()
            << " (zero copy), size " << data->size() << '\n';
}

void handoff_demo()
{
  boost::asio::io_context ioc;
  auto chan = std::make_shared<payload_channel>(ioc, 0);

  boost::asio::co_spawn(ioc, payload_builder(chan), boost::asio::detached);
  boost::asio::co_spawn(ioc, payload_consumer(chan), boost::asio::detached);
  ioc.run();
}

// ---- 示例 4：反向流控（拉模式：消费方驱动生产方） ------------------
//
// 数据从 producer 流向 consumer，但"要多少"的指令逆着走：consumer
// 通过 demand channel 授予配额（credit），producer 只在有配额时生产。
// 两根方向相反的 channel 组成闭环，生产节奏完全由消费方决定——
// 这就是 Reactive Streams 的 request(n) 背压协议的最小实现。

constexpr int batch_count = 3;
constexpr int batch_size = 2;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto credit_producer(int_channel_ptr demand, int_channel_ptr data)
    -> awaitable<void>
{
  int next_value = 1;
  for (;;) {
    auto [err, credit] = co_await demand->async_receive(as_tuple(use_awaitable));
    if (err) {
      break;
    }
    std::cout << "  [producer] got credit " << credit << ", producing\n";
    for (int i = 0; i < credit; ++i) {
      co_await data->async_send({}, next_value++, use_awaitable);
    }
  }
  std::cout << "  [producer] demand closed, produced "
            << next_value - 1 << " items in total\n";
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto credit_consumer(int_channel_ptr demand, int_channel_ptr data)
    -> awaitable<void>
{
  for (int batch = 1; batch <= batch_count; ++batch) {
    std::cout << "  [consumer] request(" << batch_size << ")\n";
    co_await demand->async_send({}, batch_size, use_awaitable);
    for (int i = 0; i < batch_size; ++i) {
      const int value = co_await data->async_receive(use_awaitable);
      std::cout << "  [consumer] got " << value << '\n';
    }
  }
  demand->close();  // 不再要了，生产方随即下班
}

void credit_demo()
{
  boost::asio::io_context ioc;
  auto demand = std::make_shared<int_channel>(ioc, 0);
  auto data = std::make_shared<int_channel>(ioc, 0);

  boost::asio::co_spawn(
      ioc, credit_producer(demand, data), boost::asio::detached);
  boost::asio::co_spawn(
      ioc, credit_consumer(demand, data), boost::asio::detached);
  ioc.run();
}

}  // namespace

void run()
{
  std::cout << "-- 1. alternation: channel replaces lock + condvar --\n";
  alternate_demo();
  std::cout << "-- 2. full-duplex dialogue: state stays private --\n";
  duplex_demo();
  std::cout << "-- 3. ownership handoff: move-only payload, zero copy --\n";
  handoff_demo();
  std::cout << "-- 4. demand-driven flow: consumer sets the pace --\n";
  credit_demo();
}

}  // namespace modules::csp_duo
