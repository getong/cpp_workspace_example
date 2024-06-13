#include "folly/experimental/coro/BlockingWait.h"
#include <coroutine>
#include <mutex>
#include <string>
#include <folly/executors/GlobalExecutor.h>
#include <iostream>
#include <folly/init/Init.h>
#include <folly/experimental/coro/Task.h>
#include <folly/experimental/coro/Collect.h>
#include <sys/select.h>
#include <time.h>
#include <folly/experimental/coro/SharedMutex.h>
#include "folly/executors/CPUThreadPoolExecutor.h"
using namespace folly;
using namespace folly::coro;

int64_t get_us_time() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (int64_t)(t.tv_sec * 1000000 + t.tv_usec);
}

static folly::CPUThreadPoolExecutor& get_executor1() {
    static folly::CPUThreadPoolExecutor executor(1);
    return executor;
}

static folly::CPUThreadPoolExecutor& get_executor2() {
    static folly::CPUThreadPoolExecutor executor(1);
    return executor;
}

SharedMutexFair coro_lock;

std::map<std::string, int> global_value = {
    {"a", 0},
    {"b", 0},
    {"c", 0},
    {"d", 0}
};
int global = 0;

Task<void> a() {
    global_value["a"] = ++global;
    std::cout << "a process time is " << get_us_time() << std::endl;
    co_return;
}

Task<void> b() {
    sleep(2);
    global_value["b"] = ++global;
    std::cout << "b process time is " << get_us_time() << std::endl;
    co_return;
}

Task<void> c() {
    global_value["c"] = ++global;
    std::cout << "c process time is " << get_us_time() << std::endl;
    co_return;
}

Task<void> d() {
    global_value["d"] = ++global;
    std::cout << "d process time is " << get_us_time() << std::endl;
    co_return;
}

Task<void> getA() {
    // Lock lock(mutex);
    auto lock = co_await coro_lock.co_scoped_lock();
    std::cout << "process get A" << std::endl;
    co_return;
}

Task<void> getB() {
    // Lock lock(mutex);
    auto lock = co_await coro_lock.co_scoped_lock();
    co_await getA().scheduleOn(getGlobalCPUExecutor());
    std::cout << "process get B" << std::endl;
    co_return;
}

Task<void> getB_unlock() {
    co_await getA().scheduleOn(getGlobalCPUExecutor());
    std::cout << "process get B unlock" << std::endl;
    co_return;
}

Task<bool> sycn() {
    std::vector<Task<void>> sum;
    std::cout << "Coroutine started on thread: " << std::this_thread::get_id() << '\n';
    sum.push_back(a());
    sum.push_back(b());
    sum.push_back(c());
    sum.push_back(d());
    // co_await folly::coro::collectAll(sum);
    try {
        co_await folly::coro::collectAllRange(std::move(sum));
    } catch (...) {
        std::cout << "catch error" << std::endl;
    }
    std::cout << "Coroutine ended on thread: " << std::this_thread::get_id() << '\n';
    co_return true;
}

Task<bool> sycn_v2() {
    std::vector<Task<void>> sum;
    std::cout << "Coroutine started on thread: " << std::this_thread::get_id() << '\n';
    sum.push_back(a());
    sum.push_back(b());
    sum.push_back(c());
    sum.push_back(d());
    // co_await folly::coro::collectAll(sum);
    co_await folly::coro::collectAllRange(std::move(sum)).scheduleOn(&get_executor1());
    std::cout << "Coroutine ended on thread: " << std::this_thread::get_id() << '\n';
    co_return true;
}

Task<bool> asycn() {
    std::vector<TaskWithExecutor<void>> sum;
    std::cout << "Coroutine started on thread: " << std::this_thread::get_id() << '\n';
    sum.push_back(a().scheduleOn(getGlobalCPUExecutor()));
    sum.push_back(b().scheduleOn(getGlobalCPUExecutor()));
    sum.push_back(c().scheduleOn(getGlobalCPUExecutor()));
    sum.push_back(d().scheduleOn(getGlobalCPUExecutor()));
    co_await folly::coro::collectAllRange(std::move(sum));
    std::cout << "Coroutine ended on thread: " << std::this_thread::get_id() << '\n';
    co_return true;
}

int main(int argc, char *argv[]) {
    // Initialize Folly
    folly::Init init(&argc, &argv);

    LOG(INFO) << "info test";
    
    auto task1 = sycn();
    folly::coro::blockingWait(std::move(task1).scheduleOn(&get_executor2()));
    std::cout << "a is " << global_value["a"] << " b is " << global_value["b"] << " c is " << global_value["c"] << " d is " << global_value["d"] << std::endl;
    std::cout << "global is " << global << std::endl;

    auto task3 = sycn_v2();
    folly::coro::blockingWait(std::move(task3).scheduleOn(&get_executor2()));
    std::cout << "a is " << global_value["a"] << " b is " << global_value["b"] << " c is " << global_value["c"] << " d is " << global_value["d"] << std::endl;
    std::cout << "global is " << global << std::endl;

    auto task2 = asycn();
    folly::coro::blockingWait(std::move(task2).scheduleOn(&get_executor2()));
    std::cout << "a is " << global_value["a"] << " b is " << global_value["b"] << " c is " << global_value["c"] << " d is " << global_value["d"] << std::endl;
    std::cout << "global is " << global << std::endl;

    folly::coro::blockingWait(getB_unlock().scheduleOn(&get_executor2()));

    folly::coro::blockingWait(getB().scheduleOn(&get_executor2()));
    google::ShutdownGoogleLogging();
    return 0;
}

