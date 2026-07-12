#pragma once

#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <deque>
#include <exception>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <tbb/task_arena.h>

class TbbScheduler
{
public:
  explicit TbbScheduler(int max_concurrency = tbb::task_arena::automatic)
      : arena_ {max_concurrency, 0}
  {
  }

  TbbScheduler(TbbScheduler const&) = delete;
  TbbScheduler& operator=(TbbScheduler const&) = delete;

  ~TbbScheduler() { wait(); }

  void schedule(std::coroutine_handle<> coroutine) noexcept
  {
    {
      auto const lock = std::lock_guard<std::mutex> {mutex_};
      ++pending_resumes_;
    }

    try {
      arena_.enqueue(
          [this, coroutine]
          {
            coroutine.resume();
            resume_complete();
          });
    } catch (...) {
      std::terminate();
    }
  }

  void wait()
  {
    auto lock = std::unique_lock<std::mutex> {mutex_};
    completed_.wait(
        lock, [this] { return active_tasks_ == 0 && pending_resumes_ == 0; });
  }

private:
  template<typename>
  friend class Task;

  void start(std::coroutine_handle<> coroutine)
  {
    {
      auto const lock = std::lock_guard<std::mutex> {mutex_};
      ++active_tasks_;
    }
    schedule(coroutine);
  }

  void complete() noexcept
  {
    {
      auto const lock = std::lock_guard<std::mutex> {mutex_};
      --active_tasks_;
    }
    completed_.notify_all();
  }

  void resume_complete() noexcept
  {
    {
      auto const lock = std::lock_guard<std::mutex> {mutex_};
      --pending_resumes_;
    }
    completed_.notify_all();
  }

  tbb::task_arena arena_;
  std::mutex mutex_;
  std::condition_variable completed_;
  std::size_t active_tasks_ {0};
  std::size_t pending_resumes_ {0};
};

template<typename T>
class [[nodiscard]] Task
{
public:
  struct promise_type
  {
    Task get_return_object()
    {
      return Task {std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_always initial_suspend() const noexcept { return {}; }

    struct FinalAwaiter
    {
      bool await_ready() const noexcept { return false; }

      void await_suspend(
          std::coroutine_handle<promise_type> coroutine) const noexcept
      {
        coroutine.promise().scheduler->complete();
      }

      void await_resume() const noexcept {}
    };

    FinalAwaiter final_suspend() const noexcept { return {}; }

    template<typename Value>
    void return_value(Value&& value)
    {
      result.emplace(std::forward<Value>(value));
    }

    void unhandled_exception() noexcept
    {
      exception = std::current_exception();
    }

    TbbScheduler* scheduler {nullptr};
    std::optional<T> result;
    std::exception_ptr exception;
    bool started {false};
  };

  Task(Task const&) = delete;
  Task& operator=(Task const&) = delete;

  Task(Task&& other) noexcept
      : coroutine_ {std::exchange(other.coroutine_, {})}
  {
  }

  Task& operator=(Task&& other) noexcept
  {
    if (this != &other) {
      destroy();
      coroutine_ = std::exchange(other.coroutine_, {});
    }
    return *this;
  }

  ~Task() { destroy(); }

  void start(TbbScheduler& scheduler)
  {
    if (!coroutine_ || coroutine_.promise().started) {
      throw std::logic_error {"a coroutine task can only be started once"};
    }

    auto& promise = coroutine_.promise();
    promise.started = true;
    promise.scheduler = &scheduler;
    scheduler.start(coroutine_);
  }

  bool done() const noexcept { return coroutine_ && coroutine_.done(); }

  T result()
  {
    ensure_complete();
    auto& promise = coroutine_.promise();
    if (promise.exception) {
      std::rethrow_exception(promise.exception);
    }
    return std::move(*promise.result);
  }

private:
  explicit Task(std::coroutine_handle<promise_type> coroutine)
      : coroutine_ {coroutine}
  {
  }

  void ensure_complete() const
  {
    if (!done()) {
      throw std::logic_error {"coroutine result requested before completion"};
    }
  }

  void destroy() noexcept
  {
    if (!coroutine_) {
      return;
    }
    if (coroutine_.promise().started && !coroutine_.done()) {
      std::terminate();
    }
    coroutine_.destroy();
  }

  std::coroutine_handle<promise_type> coroutine_;
};

template<>
class [[nodiscard]] Task<void>
{
public:
  struct promise_type
  {
    Task get_return_object()
    {
      return Task {std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_always initial_suspend() const noexcept { return {}; }

    struct FinalAwaiter
    {
      bool await_ready() const noexcept { return false; }

      void await_suspend(
          std::coroutine_handle<promise_type> coroutine) const noexcept
      {
        coroutine.promise().scheduler->complete();
      }

      void await_resume() const noexcept {}
    };

    FinalAwaiter final_suspend() const noexcept { return {}; }

    void return_void() const noexcept {}

    void unhandled_exception() noexcept
    {
      exception = std::current_exception();
    }

    TbbScheduler* scheduler {nullptr};
    std::exception_ptr exception;
    bool started {false};
  };

  Task(Task const&) = delete;
  Task& operator=(Task const&) = delete;

  Task(Task&& other) noexcept
      : coroutine_ {std::exchange(other.coroutine_, {})}
  {
  }

  Task& operator=(Task&& other) noexcept
  {
    if (this != &other) {
      destroy();
      coroutine_ = std::exchange(other.coroutine_, {});
    }
    return *this;
  }

  ~Task() { destroy(); }

  void start(TbbScheduler& scheduler)
  {
    if (!coroutine_ || coroutine_.promise().started) {
      throw std::logic_error {"a coroutine task can only be started once"};
    }

    auto& promise = coroutine_.promise();
    promise.started = true;
    promise.scheduler = &scheduler;
    scheduler.start(coroutine_);
  }

  bool done() const noexcept { return coroutine_ && coroutine_.done(); }

  void result()
  {
    if (!done()) {
      throw std::logic_error {"coroutine result requested before completion"};
    }
    if (coroutine_.promise().exception) {
      std::rethrow_exception(coroutine_.promise().exception);
    }
  }

private:
  explicit Task(std::coroutine_handle<promise_type> coroutine)
      : coroutine_ {coroutine}
  {
  }

  void destroy() noexcept
  {
    if (!coroutine_) {
      return;
    }
    if (coroutine_.promise().started && !coroutine_.done()) {
      std::terminate();
    }
    coroutine_.destroy();
  }

  std::coroutine_handle<promise_type> coroutine_;
};

template<typename T>
class Channel
{
public:
  class SendOperation;
  class ReceiveOperation;

  Channel(TbbScheduler& scheduler, std::size_t capacity)
      : scheduler_ {scheduler}
      , capacity_ {capacity}
  {
    if (capacity == 0) {
      throw std::invalid_argument {
          "channel capacity must be greater than zero"};
    }
  }

  Channel(Channel const&) = delete;
  Channel& operator=(Channel const&) = delete;

  SendOperation send(T value)
  {
    return SendOperation {*this, std::move(value)};
  }

  ReceiveOperation receive() { return ReceiveOperation {*this}; }

  void close()
  {
    auto resumable = std::vector<std::coroutine_handle<>> {};
    {
      auto const lock = std::lock_guard<std::mutex> {mutex_};
      if (closed_) {
        return;
      }

      closed_ = true;
      resumable.reserve(waiting_senders_.size() + waiting_receivers_.size());
      while (!waiting_senders_.empty()) {
        auto* sender = waiting_senders_.front();
        waiting_senders_.pop_front();
        sender->accepted_ = false;
        resumable.push_back(sender->coroutine_);
      }
      while (!waiting_receivers_.empty()) {
        auto* receiver = waiting_receivers_.front();
        waiting_receivers_.pop_front();
        resumable.push_back(receiver->coroutine_);
      }
    }

    for (auto coroutine : resumable) {
      scheduler_.schedule(coroutine);
    }
  }

  class SendOperation
  {
  public:
    SendOperation(SendOperation const&) = delete;
    SendOperation& operator=(SendOperation const&) = delete;

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<> coroutine)
    {
      coroutine_ = coroutine;
      auto receiver_to_resume = std::coroutine_handle<> {};
      {
        auto const lock = std::lock_guard<std::mutex> {channel_.mutex_};
        if (channel_.closed_) {
          accepted_ = false;
          return false;
        }

        if (!channel_.waiting_receivers_.empty()) {
          auto* receiver = channel_.waiting_receivers_.front();
          channel_.waiting_receivers_.pop_front();
          receiver->value_.emplace(std::move(value_));
          receiver_to_resume = receiver->coroutine_;
        } else if (channel_.buffer_.size() < channel_.capacity_) {
          channel_.buffer_.push_back(std::move(value_));
        } else {
          channel_.waiting_senders_.push_back(this);
          return true;
        }
      }

      if (receiver_to_resume) {
        channel_.scheduler_.schedule(receiver_to_resume);
      }
      return false;
    }

    bool await_resume() const noexcept { return accepted_; }

  private:
    friend class Channel;

    SendOperation(Channel& channel, T value)
        : channel_ {channel}
        , value_ {std::move(value)}
    {
    }

    Channel& channel_;
    T value_;
    std::coroutine_handle<> coroutine_;
    bool accepted_ {true};
  };

  class ReceiveOperation
  {
  public:
    ReceiveOperation(ReceiveOperation const&) = delete;
    ReceiveOperation& operator=(ReceiveOperation const&) = delete;

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<> coroutine)
    {
      coroutine_ = coroutine;
      auto sender_to_resume = std::coroutine_handle<> {};
      {
        auto const lock = std::lock_guard<std::mutex> {channel_.mutex_};
        if (!channel_.buffer_.empty()) {
          value_.emplace(std::move(channel_.buffer_.front()));
          channel_.buffer_.pop_front();

          if (!channel_.waiting_senders_.empty()) {
            auto* sender = channel_.waiting_senders_.front();
            channel_.waiting_senders_.pop_front();
            channel_.buffer_.push_back(std::move(sender->value_));
            sender_to_resume = sender->coroutine_;
          }
        } else if (channel_.closed_) {
          return false;
        } else {
          channel_.waiting_receivers_.push_back(this);
          return true;
        }
      }

      if (sender_to_resume) {
        channel_.scheduler_.schedule(sender_to_resume);
      }
      return false;
    }

    std::optional<T> await_resume() { return std::move(value_); }

  private:
    friend class Channel;

    explicit ReceiveOperation(Channel& channel)
        : channel_ {channel}
    {
    }

    Channel& channel_;
    std::optional<T> value_;
    std::coroutine_handle<> coroutine_;
  };

private:
  TbbScheduler& scheduler_;
  std::size_t capacity_;
  std::mutex mutex_;
  std::deque<T> buffer_;
  std::deque<SendOperation*> waiting_senders_;
  std::deque<ReceiveOperation*> waiting_receivers_;
  bool closed_ {false};
};
