# C++20 Unreal 风格委托：使用与设计 {#delegate-guide}

## 1. 目标

委托把“调用什么函数”封装成一个对象。广播方只依赖委托签名，不需要知道接收方
的具体类型，因此适合事件通知、策略注入和模块解耦。

本项目模仿 Unreal Engine Delegate 的核心使用方式，提供一个可以独立编译、运行和
测试的标准 C++20 示例。实现位于 `source/delegate.hpp`，不依赖 Unreal Engine。

## 2. 功能对照

| Unreal 概念 | 本项目 | 作用 |
| --- | --- | --- |
| `TDelegate<Ret(Args...)>` | `delegates::Delegate<Ret(Args...)>` | 一个委托只绑定一个目标 |
| `TMulticastDelegate<void(Args...)>` | `delegates::MulticastDelegate<void(Args...)>` | 一次广播通知多个目标 |
| `FDelegateHandle` | `delegates::DelegateHandle` | 精确移除一次多播绑定 |
| `DECLARE_DELEGATE*` | 同名宏 | 声明具名的单播委托类型 |
| `DECLARE_MULTICAST_DELEGATE*` | 同名宏 | 声明具名的多播委托类型 |
| `DECLARE_EVENT*` | 同名宏 | 只有拥有者类型可以广播的多播事件 |
| `BindRaw` / `AddRaw` | 同名方法 | 绑定裸指针对象，不检查生命周期 |
| `BindSP` / `AddSP` | 同名方法 | 用 `weak_ptr` 跟踪 `shared_ptr` 对象 |
| `BindLambda` / `AddLambda` | 同名方法 | 绑定 lambda 或其他可调用对象 |
| `BindWeakLambda` / `AddWeakLambda` | 同名方法 | lambda 与一个 `shared_ptr` 对象同寿命 |
| `BindStatic` / `AddStatic` | 同名方法 | 绑定普通函数或静态成员函数 |

## 3. 单播委托

用声明宏给函数签名命名，然后绑定并执行：

```cpp
DECLARE_DELEGATE_OneParam(FOnScoreChanged, int);

FOnScoreChanged on_score_changed;
on_score_changed.BindLambda(
    [](int score) { std::cout << "score = " << score << '\n'; });

if (on_score_changed.IsBound()) {
  on_score_changed.Execute(100);
}

on_score_changed.Unbind();
```

无返回值委托可以使用 `ExecuteIfBound`。未绑定或弱绑定目标已经销毁时，它不调用
目标并返回 `false`。`Execute` 要求委托有效，调用前应检查 `IsBound()`。

带返回值的单播委托适合可替换策略：

```cpp
DECLARE_DELEGATE_RetVal_TwoParams(int, FDamageFormula, int, float);

FDamageFormula formula;
formula.BindLambda([](int base, float multiplier) {
  return static_cast<int>(static_cast<float>(base) * multiplier);
});

int damage = formula.Execute(20, 1.5F);
```

`source/main.cpp` 中的 `Character::DamageFormula` 展示了默认策略和外部注入策略如何
切换，而 `Character` 不需要依赖具体的伤害计算类。

## 4. 多播委托与句柄

多播委托只能返回 `void`。每个 `Add*` 返回唯一句柄，`Broadcast` 按加入顺序调用
当前仍有效的监听者：

```cpp
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, float);

FOnHealthChanged on_health_changed;
auto handle = on_health_changed.AddLambda(
    [](float old_health, float new_health) {
      std::cout << old_health << " -> " << new_health << '\n';
    });

on_health_changed.Broadcast(100.0F, 75.0F);
on_health_changed.Remove(handle);
```

也可以用 `RemoveAll(object)` 移除绑定到某个对象的全部 `AddRaw`、`AddSP` 或
`AddWeakLambda` 监听者，或者用 `Clear()` 清空全部监听者。

广播采用调用节点快照，具有以下确定行为：

- 广播中新增的监听者从下一轮广播开始执行。
- 在轮到某个监听者前将其移除，本轮不再执行它。
- 广播中 `Clear()` 后，其余监听者本轮不再执行。
- mutable lambda 的内部状态保存在原调用节点中，可跨多次广播延续。

这些规则解决同一线程中的重入和容器失效问题；它们不表示委托是线程安全的。

## 5. 绑定方式和生命周期

### `BindRaw` / `AddRaw`

委托只保存对象地址。它开销直接，但不会知道对象何时销毁。必须保证解绑或最后一次
调用发生在对象销毁前，否则执行会访问悬空指针。

```cpp
Hud hud;
auto handle = character.OnHealthChanged.AddRaw(
    &hud, &Hud::HandleHealthChanged);

// hud 销毁之前：
character.OnHealthChanged.Remove(handle);
```

### `BindSP` / `AddSP`

传入 `std::shared_ptr<T>`，委托内部只保存 `std::weak_ptr<T>`，不会延长对象寿命。
对象销毁后，`IsBound()` 为 `false`，单播的 `ExecuteIfBound` 会跳过调用，多播会在
广播时跳过并清理失效槽位。

```cpp
auto achievements = std::make_shared<AchievementSystem>();
character.OnHealthChanged.AddSP(
    achievements, &AchievementSystem::HandleHealthChanged);

achievements.reset();  // 后续广播不会访问已销毁对象
```

普通成员函数和 const 成员函数都支持 `BindRaw`、`CreateRaw`、`AddRaw`、`BindSP`、
`CreateSP` 和 `AddSP`。

### `BindWeakLambda` / `AddWeakLambda`

lambda 本身不必调用拥有者；拥有者参数只控制绑定是否有效。这适合 lambda 捕获其他
状态，但仍希望某个对象销毁后自动停止回调的场景。

## 6. Payload

所有绑定方法都可附带 Payload。Payload 在绑定时按值保存，在执行时追加到委托签名
参数后面。这与 Unreal 的 Payload 概念一致。

```cpp
void LogHealth(float old_health,
               float new_health,
               const std::string& category);

FOnHealthChanged changed;
changed.AddStatic(&LogHealth, std::string{"combat"});
changed.Broadcast(100.0F, 80.0F);

// 实际调用：LogHealth(100.0F, 80.0F, "combat")
```

Payload 的保存副本以 const 引用传给目标函数，因此目标参数应使用值或 const 引用。

## 7. Event：限制广播权限

普通多播委托的 `Broadcast` 是公开方法。事件进一步限制权限：外部只能订阅，只有
声明事件时指定的拥有者类型可以广播或清空。

```cpp
class Character
{
public:
  DECLARE_EVENT(Character, FDeathEvent);
  FDeathEvent& OnDeath() { return death_event_; }

  void Die() { death_event_.Broadcast(); }

private:
  FDeathEvent death_event_;
};
```

`character.OnDeath().AddLambda(...)` 可以编译；外部调用
`character.OnDeath().Broadcast()` 会因访问私有成员而编译失败。这只是一种 C++
访问控制，不涉及运行时权限检查。

## 8. 完整示例

`source/main.cpp` 用一个小型游戏场景串联全部核心功能：

1. 游戏开始回调说明单播绑定、检查、执行和解绑。
2. 玩家改名回调说明签名参数和 Payload。
3. 伤害公式说明带返回值单播委托和策略注入。
4. 血量变化说明静态函数、裸指针、智能指针和 lambda 的多播订阅。
5. 成就系统销毁说明弱生命周期绑定。
6. 句柄及对象移除说明订阅管理。
7. 死亡事件说明只有拥有者可以广播。

构建并运行：

```sh
./build.sh
./build/dev/delegate-example
```

构建并运行测试：

```sh
BUILD_DIR=build/test ./compile_commands.sh
ctest --test-dir build/test --output-on-failure
```

## 9. 与 Unreal Engine 的差异

本项目是教学实现，不是 Unreal Delegate 的 ABI 或源码兼容版本：

- 没有动态委托、动态多播委托、`UFUNCTION`、蓝图暴露和序列化。
- 没有 `UObject`/垃圾回收感知；标准 C++ 对象使用 `shared_ptr`/`weak_ptr`。
- 没有 `BindUObject`、`BindUFunction`、`BindThreadSafeSP` 等引擎专用绑定方式。
- 调用存储基于 `std::function`，性能、内存布局和内联策略与 Unreal 不同。
- 可调用对象必须能存入 `std::function`；仅可移动而不可复制的 lambda 不受支持。
- 未提供并发读写保证。跨线程 Add、Remove 或 Broadcast 需要调用方加锁。
- 声明宏提供示例所需的常见参数数量，不追求覆盖 Unreal 的全部宏集合。

如果项目已经运行在 Unreal 中，应直接使用引擎委托。这个实现适合学习委托语义、
演示解耦方式，或在普通 C++20 小型项目中使用相似的 API。
