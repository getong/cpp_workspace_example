#include <memory>
#include <string>
#include <vector>

#include "delegate.hpp"

#include <catch2/catch_test_macros.hpp>

#include "lib.hpp"

TEST_CASE("Name is delegate-example", "[library]")
{
  auto const lib = library {};
  REQUIRE(lib.name == "delegate-example");
}

// ---------------------------------------------------------------------------
// 单播委托
// ---------------------------------------------------------------------------

DECLARE_DELEGATE_OneParam(FOnValue, int);
DECLARE_DELEGATE_RetVal_TwoParams(int, FBinaryOp, int, int);
DECLARE_DELEGATE_RetVal_OneParam(int, FReadCounter, int);

TEST_CASE("单播委托：绑定、执行与解绑", "[delegate]")
{
  FOnValue del;
  int received = 0;

  SECTION("未绑定时 IsBound 为 false，ExecuteIfBound 返回 false")
  {
    REQUIRE_FALSE(del.IsBound());
    REQUIRE_FALSE(del.ExecuteIfBound(1));
  }

  SECTION("BindLambda 后 Execute 调用目标")
  {
    del.BindLambda([&received](int value) { received = value; });
    REQUIRE(del.IsBound());
    del.Execute(42);
    REQUIRE(received == 42);
  }

  SECTION("重新绑定会覆盖旧目标")
  {
    del.BindLambda([&received](int value) { received = value; });
    del.BindLambda([&received](int value) { received = value * 2; });
    del.Execute(10);
    REQUIRE(received == 20);
  }

  SECTION("Unbind 之后不再执行")
  {
    del.BindLambda([&received](int value) { received = value; });
    del.Unbind();
    REQUIRE_FALSE(del.IsBound());
    REQUIRE_FALSE(del.ExecuteIfBound(42));
    REQUIRE(received == 0);
  }
}

TEST_CASE("单播委托：带返回值", "[delegate]")
{
  FBinaryOp formula;
  formula.BindLambda([](int lhs, int rhs) { return lhs + rhs; });
  REQUIRE(formula.Execute(2, 3) == 5);

  formula.BindLambda([](int lhs, int rhs) { return lhs * rhs; });
  REQUIRE(formula.Execute(2, 3) == 6);
}

namespace
{

int g_static_sink = 0;

void StaticSink(int value)
{
  g_static_sink = value;
}

void StaticSinkWithPayload(int value, const std::string& tag, int bonus)
{
  g_static_sink = value + bonus;
  (void)tag;
}

}  // namespace

TEST_CASE("单播委托：BindStatic 与 Payload 附加参数", "[delegate]")
{
  FOnValue del;

  SECTION("无 payload")
  {
    del.BindStatic(&StaticSink);
    del.Execute(7);
    REQUIRE(g_static_sink == 7);
  }

  SECTION("payload 追加在签名参数之后")
  {
    del.BindStatic(&StaticSinkWithPayload, std::string {"tag"}, 100);
    del.Execute(7);
    REQUIRE(g_static_sink == 107);
  }

  SECTION("lambda 也支持 payload")
  {
    int received = 0;
    del.BindLambda(
        [&received](int value, int bonus) { received = value + bonus; }, 1000);
    del.Execute(7);
    REQUIRE(received == 1007);
  }
}

namespace
{

class Counter
{
public:
  void Add(int value) { total += value; }

  void AddScaled(int value, int scale) { total += value * scale; }

  [[nodiscard]] auto Get(int bias) const -> int { return total + bias; }

  void Observe(int value) const { last_observed = value; }

  int total = 0;
  mutable int last_observed = 0;
};

}  // namespace

TEST_CASE("单播委托：BindRaw 成员函数", "[delegate]")
{
  Counter counter;
  FOnValue del;

  del.BindRaw(&counter, &Counter::Add);
  del.Execute(5);
  REQUIRE(counter.total == 5);
  REQUIRE(del.IsBoundToObject(&counter));

  del.BindRaw(&counter, &Counter::AddScaled, 10);  // payload: scale = 10
  del.Execute(5);
  REQUIRE(counter.total == 55);
}

TEST_CASE("单播委托：BindSP 生命周期安全", "[delegate][lifetime]")
{
  FOnValue del;
  auto counter = std::make_shared<Counter>();
  del.BindSP(counter, &Counter::Add);

  REQUIRE(del.IsBound());
  del.Execute(5);
  REQUIRE(counter->total == 5);

  // 委托内部只持有 weak_ptr，不延长对象寿命
  std::weak_ptr<Counter> watcher = counter;
  counter.reset();
  REQUIRE(watcher.expired());

  // 对象销毁后委托自动失效，ExecuteIfBound 安全返回 false
  REQUIRE_FALSE(del.IsBound());
  REQUIRE_FALSE(del.ExecuteIfBound(5));
}

TEST_CASE("单播委托：const 成员函数工厂", "[delegate]")
{
  Counter raw_counter;
  raw_counter.total = 10;
  auto raw = FReadCounter::CreateRaw(&raw_counter, &Counter::Get);
  REQUIRE(raw.Execute(2) == 12);

  auto shared_counter = std::make_shared<Counter>();
  shared_counter->total = 20;
  auto shared = FReadCounter::CreateSP(shared_counter, &Counter::Get);
  REQUIRE(shared.Execute(3) == 23);
}

TEST_CASE("单播委托：BindWeakLambda 生命周期安全", "[delegate][lifetime]")
{
  FOnValue del;
  auto owner = std::make_shared<Counter>();
  int received = 0;
  del.BindWeakLambda(
      owner,
      [&received](int value, int bonus) { received = value + bonus; },
      10);

  del.Execute(3);
  REQUIRE(received == 13);

  owner.reset();
  REQUIRE_FALSE(del.IsBound());
  REQUIRE_FALSE(del.ExecuteIfBound(9));
  REQUIRE(received == 13);
}

// ---------------------------------------------------------------------------
// 多播委托
// ---------------------------------------------------------------------------

DECLARE_MULTICAST_DELEGATE_OneParam(FOnValueMulti, int);

TEST_CASE("多播委托：Broadcast 按加入顺序通知所有监听者", "[multicast]")
{
  FOnValueMulti multi;
  std::vector<int> order;

  multi.AddLambda([&order](int value) { order.push_back(value * 1); });
  multi.AddLambda([&order](int value) { order.push_back(value * 2); });
  multi.AddLambda([&order](int value) { order.push_back(value * 3); });

  REQUIRE(multi.Num() == 3);
  multi.Broadcast(10);
  REQUIRE(order == std::vector<int> {10, 20, 30});
}

TEST_CASE("多播委托：有状态 lambda 的状态跨广播保留", "[multicast]")
{
  FOnValueMulti multi;
  std::vector<int> states;
  multi.AddLambda([state = 0, &states](int) mutable
                  { states.push_back(++state); });

  multi.Broadcast(0);
  multi.Broadcast(0);
  REQUIRE(states == std::vector<int> {1, 2});
}

TEST_CASE("多播委托：Remove(handle) 与 Clear", "[multicast]")
{
  FOnValueMulti multi;
  int calls = 0;

  auto handle = multi.AddLambda([&calls](int) { ++calls; });
  multi.AddLambda([&calls](int) { ++calls; });

  REQUIRE(multi.Remove(handle));
  REQUIRE_FALSE(multi.Remove(handle));  // 重复移除返回 false
  multi.Broadcast(0);
  REQUIRE(calls == 1);

  multi.Clear();
  REQUIRE_FALSE(multi.IsBound());
  multi.Broadcast(0);
  REQUIRE(calls == 1);
}

TEST_CASE("多播委托：RemoveAll 按对象批量移除", "[multicast]")
{
  FOnValueMulti multi;
  Counter counter_a;
  Counter counter_b;

  multi.AddRaw(&counter_a, &Counter::Add);
  multi.AddRaw(&counter_a, &Counter::AddScaled, 2);
  multi.AddRaw(&counter_b, &Counter::Add);

  REQUIRE(multi.RemoveAll(&counter_a) == 2);
  multi.Broadcast(5);
  REQUIRE(counter_a.total == 0);
  REQUIRE(counter_b.total == 5);
}

TEST_CASE("多播委托：AddSP 对象销毁后自动清理", "[multicast][lifetime]")
{
  FOnValueMulti multi;
  auto counter = std::make_shared<Counter>();
  int lambda_calls = 0;

  multi.AddSP(counter, &Counter::Add);
  multi.AddLambda([&lambda_calls](int) { ++lambda_calls; });
  REQUIRE(multi.Num() == 2);

  counter.reset();
  REQUIRE(multi.Num() == 1);  // 失效的弱绑定不计入

  multi.Broadcast(1);  // 不崩溃，只有 lambda 被调用，失效槽位被清理
  REQUIRE(lambda_calls == 1);
  REQUIRE(multi.Num() == 1);
}

TEST_CASE("多播委托：const 成员函数与弱 lambda payload", "[multicast]")
{
  FOnValueMulti multi;
  Counter raw_counter;
  auto shared_counter = std::make_shared<Counter>();
  int weak_result = 0;

  multi.AddRaw(&raw_counter, &Counter::Observe);
  multi.AddSP(shared_counter, &Counter::Observe);
  multi.AddWeakLambda(
      shared_counter,
      [&weak_result](int value, int bonus) { weak_result = value + bonus; },
      5);

  multi.Broadcast(7);
  REQUIRE(raw_counter.last_observed == 7);
  REQUIRE(shared_counter->last_observed == 7);
  REQUIRE(weak_result == 12);
}

TEST_CASE("多播委托：广播过程中移除监听者是安全的", "[multicast][reentrancy]")
{
  FOnValueMulti multi;
  int second_calls = 0;

  delegates::DelegateHandle second_handle;
  multi.AddLambda([&multi, &second_handle](int)
                  { multi.Remove(second_handle); });
  second_handle = multi.AddLambda([&second_calls](int) { ++second_calls; });

  // 第一个监听者在回调里移除了第二个，第二个不应再被调用
  multi.Broadcast(0);
  REQUIRE(second_calls == 0);
}

TEST_CASE("多播委托：广播中新监听者从下一轮开始执行", "[multicast][reentrancy]")
{
  FOnValueMulti multi;
  int late_calls = 0;
  bool added = false;
  multi.AddLambda(
      [&multi, &late_calls, &added](int)
      {
        if (!added) {
          added = true;
          multi.AddLambda([&late_calls](int) { ++late_calls; });
        }
      });

  multi.Broadcast(0);
  REQUIRE(late_calls == 0);
  multi.Broadcast(0);
  REQUIRE(late_calls == 1);
}

TEST_CASE("多播委托：广播中清空后不再执行剩余监听者", "[multicast][reentrancy]")
{
  FOnValueMulti multi;
  int later_calls = 0;
  multi.AddLambda([&multi](int) { multi.Clear(); });
  multi.AddLambda([&later_calls](int) { ++later_calls; });

  multi.Broadcast(0);
  REQUIRE(later_calls == 0);
  REQUIRE_FALSE(multi.IsBound());
}

// ---------------------------------------------------------------------------
// 事件
// ---------------------------------------------------------------------------

namespace
{

class Door
{
public:
  DECLARE_EVENT_OneParam(Door, FOpenedEvent, const std::string&);

  auto OnOpened() -> FOpenedEvent& { return opened_; }

  void Open() { opened_.Broadcast("front-door"); }  // 只有 Door 能广播

private:
  FOpenedEvent opened_;
};

}  // namespace

TEST_CASE("事件：拥有者广播，外部订阅", "[event]")
{
  Door door;
  std::string opened_name;
  door.OnOpened().AddLambda([&opened_name](const std::string& name)
                            { opened_name = name; });

  door.Open();
  REQUIRE(opened_name == "front-door");
  // door.OnOpened().Broadcast("fake");  // 编译不过：Broadcast 对外部私有
}
