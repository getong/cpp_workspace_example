#pragma once

/**
 * @file delegate.hpp
 * @brief 仿 Unreal Engine 委托系统的轻量级 C++20 实现（仅头文件）。
 *
 * 提供三类委托，API 命名与 Unreal 保持一致：
 *  - Delegate<Ret(Args...)>          单播委托（对应 TDelegate /
 * DECLARE_DELEGATE*）
 *  - MulticastDelegate<void(Args...)> 多播委托（对应 TMulticastDelegate /
 *    DECLARE_MULTICAST_DELEGATE*）
 *  - DECLARE_EVENT*                   事件：只有拥有者类才能 Broadcast
 *
 * 支持的绑定方式（与 Unreal 对应）：
 *  - BindStatic / AddStatic          绑定普通函数
 *  - BindLambda / AddLambda          绑定 lambda / 任意可调用对象
 *  - BindRaw / AddRaw 绑定裸指针对象的成员函数（不做生命周期检查）
 *  - BindSP / AddSP                  绑定 shared_ptr 管理的对象成员函数，
 *                                    内部持有 weak_ptr，对象销毁后自动失效
 *  - BindWeakLambda / AddWeakLambda  lambda 与某对象生命周期绑定
 *
 * 所有绑定方式都支持 Payload（绑定时附加的额外参数），调用时 Payload
 * 会追加在委托签名参数之后传给目标函数，与 Unreal 的行为一致。
 *
 * 与 Unreal 的主要差异见 docs/delegate.md。
 */

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace delegates
{

/**
 * @brief 委托句柄，对应 Unreal 的 FDelegateHandle。
 *
 * 向多播委托 Add* 时返回，之后可用它把这次绑定 Remove 掉。
 * 默认构造的句柄是无效句柄。
 */
class DelegateHandle
{
public:
  DelegateHandle() = default;

  /// 生成一个新的全局唯一句柄
  static auto Create() -> DelegateHandle
  {
    static std::atomic<std::uint64_t> counter {1};
    return DelegateHandle {counter.fetch_add(1)};
  }

  /// 句柄是否有效（是否代表一次真实的绑定）
  [[nodiscard]] auto IsValid() const -> bool { return id_ != 0; }

  /// 将句柄重置为无效
  void Reset() { id_ = 0; }

  friend auto operator==(DelegateHandle, DelegateHandle) -> bool = default;

private:
  explicit DelegateHandle(std::uint64_t id)
      : id_ {id}
  {
  }

  std::uint64_t id_ = 0;
};

template<typename Signature>
class Delegate;

/**
 * @brief 单播委托，对应 Unreal 的 TDelegate<Ret(Args...)>。
 *
 * 同一时刻最多绑定一个目标；重新 Bind 会覆盖旧的绑定。
 * 支持返回值（对应 DECLARE_DELEGATE_RetVal*）。
 */
template<typename Ret, typename... Args>
class Delegate<Ret(Args...)>
{
public:
  Delegate() = default;

  // ---- 静态工厂，对应 FDelegate::CreateStatic / CreateLambda / ... ----

  template<typename... FuncPayload, typename... Payload>
  static auto CreateStatic(Ret (*func)(Args..., FuncPayload...),
                           Payload&&... payload) -> Delegate
  {
    Delegate result;
    result.BindStatic(func, std::forward<Payload>(payload)...);
    return result;
  }

  template<typename Functor, typename... Payload>
  static auto CreateLambda(Functor&& functor, Payload&&... payload) -> Delegate
  {
    Delegate result;
    result.BindLambda(std::forward<Functor>(functor),
                      std::forward<Payload>(payload)...);
    return result;
  }

  template<typename Class, typename... FuncPayload, typename... Payload>
  static auto CreateRaw(Class* object,
                        Ret (Class::*method)(Args..., FuncPayload...),
                        Payload&&... payload) -> Delegate
  {
    Delegate result;
    result.BindRaw(object, method, std::forward<Payload>(payload)...);
    return result;
  }

  template<typename Class, typename... FuncPayload, typename... Payload>
  static auto CreateRaw(const Class* object,
                        Ret (Class::*method)(Args..., FuncPayload...) const,
                        Payload&&... payload) -> Delegate
  {
    Delegate result;
    result.BindRaw(object, method, std::forward<Payload>(payload)...);
    return result;
  }

  template<typename Class, typename... FuncPayload, typename... Payload>
  static auto CreateSP(const std::shared_ptr<Class>& object,
                       Ret (Class::*method)(Args..., FuncPayload...),
                       Payload&&... payload) -> Delegate
  {
    Delegate result;
    result.BindSP(object, method, std::forward<Payload>(payload)...);
    return result;
  }

  template<typename Class, typename... FuncPayload, typename... Payload>
  static auto CreateSP(const std::shared_ptr<Class>& object,
                       Ret (Class::*method)(Args..., FuncPayload...) const,
                       Payload&&... payload) -> Delegate
  {
    Delegate result;
    result.BindSP(object, method, std::forward<Payload>(payload)...);
    return result;
  }

  template<typename Class, typename Functor, typename... Payload>
  static auto CreateWeakLambda(const std::shared_ptr<Class>& object,
                               Functor&& functor,
                               Payload&&... payload) -> Delegate
  {
    Delegate result;
    result.BindWeakLambda(object,
                          std::forward<Functor>(functor),
                          std::forward<Payload>(payload)...);
    return result;
  }

  // ---- 绑定接口 ----

  /**
   * @brief 绑定普通（静态）函数，可附带 Payload。
   *
   * Payload 在调用时追加到签名参数之后：
   * @code
   * void OnScore(int score, const char* tag);          // tag 是 payload
   * DECLARE_DELEGATE_OneParam(FOnScore, int);
   * FOnScore d;
   * d.BindStatic(&OnScore, "combo");
   * d.Execute(100);  // 实际调用 OnScore(100, "combo")
   * @endcode
   */
  template<typename... FuncPayload, typename... Payload>
  void BindStatic(Ret (*func)(Args..., FuncPayload...), Payload&&... payload)
  {
    BindInternal(func, std::forward<Payload>(payload)...);
  }

  /// 绑定 lambda 或任意可调用对象（按值保存一份拷贝）
  template<typename Functor, typename... Payload>
  void BindLambda(Functor&& functor, Payload&&... payload)
  {
    BindInternal(std::forward<Functor>(functor),
                 std::forward<Payload>(payload)...);
  }

  /**
   * @brief 绑定裸指针对象的成员函数。
   *
   * 不做任何生命周期检查——对象销毁后再 Execute 是未定义行为，
   * 与 Unreal 的 BindRaw 语义一致。对象生命周期不确定时请用 BindSP。
   */
  template<typename Class, typename... FuncPayload, typename... Payload>
  void BindRaw(Class* object,
               Ret (Class::*method)(Args..., FuncPayload...),
               Payload&&... payload)
  {
    BindInternal(
        [object, method](Args... args, const auto&... extras) -> Ret
        { return (object->*method)(std::forward<Args>(args)..., extras...); },
        std::forward<Payload>(payload)...);
    owner_ = object;
  }

  /// BindRaw 的 const 成员函数重载
  template<typename Class, typename... FuncPayload, typename... Payload>
  void BindRaw(const Class* object,
               Ret (Class::*method)(Args..., FuncPayload...) const,
               Payload&&... payload)
  {
    BindInternal(
        [object, method](Args... args, const auto&... extras) -> Ret
        { return (object->*method)(std::forward<Args>(args)..., extras...); },
        std::forward<Payload>(payload)...);
    owner_ = object;
  }

  /**
   * @brief 绑定 shared_ptr 管理的对象的成员函数（生命周期安全）。
   *
   * 委托内部只持有 weak_ptr，不会延长对象寿命；对象销毁后
   * IsBound() 返回 false，ExecuteIfBound() 安全跳过。
   * 对应 Unreal 的 BindSP / BindUObject 的安全语义。
   */
  template<typename Class, typename... FuncPayload, typename... Payload>
  void BindSP(const std::shared_ptr<Class>& object,
              Ret (Class::*method)(Args..., FuncPayload...),
              Payload&&... payload)
  {
    std::weak_ptr<Class> weak = object;
    BindInternal(
        [weak, method](Args... args, const auto&... extras) -> Ret
        {
          auto locked = weak.lock();
          assert(locked && "Execute called on an expired BindSP delegate");
          return (locked.get()->*method)(std::forward<Args>(args)...,
                                         extras...);
        },
        std::forward<Payload>(payload)...);
    weak_ = object;
    uses_weak_ = true;
    owner_ = object.get();
  }

  /// BindSP 的 const 成员函数重载
  template<typename Class, typename... FuncPayload, typename... Payload>
  void BindSP(const std::shared_ptr<Class>& object,
              Ret (Class::*method)(Args..., FuncPayload...) const,
              Payload&&... payload)
  {
    std::weak_ptr<const Class> weak = object;
    BindInternal(
        [weak, method](Args... args, const auto&... extras) -> Ret
        {
          auto locked = weak.lock();
          assert(locked && "Execute called on an expired BindSP delegate");
          return (locked.get()->*method)(std::forward<Args>(args)...,
                                         extras...);
        },
        std::forward<Payload>(payload)...);
    weak_ = object;
    uses_weak_ = true;
    owner_ = object.get();
  }

  /**
   * @brief 绑定一个与 @p object 生命周期挂钩的 lambda。
   *
   * 对象销毁后委托自动失效，对应 Unreal 的 BindWeakLambda。
   */
  template<typename Class, typename Functor, typename... Payload>
  void BindWeakLambda(const std::shared_ptr<Class>& object,
                      Functor&& functor,
                      Payload&&... payload)
  {
    std::weak_ptr<Class> weak = object;
    BindInternal(
        [weak, functor = std::forward<Functor>(functor)](
            Args... args, const auto&... extras) mutable -> Ret
        {
          auto locked = weak.lock();
          assert(locked && "Execute called on an expired weak-lambda delegate");
          return std::invoke(functor, std::forward<Args>(args)..., extras...);
        },
        std::forward<Payload>(payload)...);
    weak_ = object;
    uses_weak_ = true;
    owner_ = object.get();
  }

  // ---- 查询与调用 ----

  /// 当前是否绑定了可调用目标（弱绑定的对象已销毁则视为未绑定）
  [[nodiscard]] auto IsBound() const -> bool
  {
    return static_cast<bool>(invocation_) && (!uses_weak_ || !weak_.expired());
  }

  /// 是否绑定到指定对象（BindRaw / BindSP / BindWeakLambda 时记录）
  [[nodiscard]] auto IsBoundToObject(const void* object) const -> bool
  {
    return object != nullptr && owner_ == object && IsBound();
  }

  explicit operator bool() const { return IsBound(); }

  /// 解除绑定，对应 Unreal 的 Unbind()
  void Unbind()
  {
    invocation_ = nullptr;
    weak_.reset();
    uses_weak_ = false;
    owner_ = nullptr;
  }

  /**
   * @brief 执行委托并返回结果。
   *
   * 调用前必须保证 IsBound() 为 true（与 Unreal 一致，
   * 未绑定时 Execute 是错误用法，Debug 下触发断言）。
   */
  auto Execute(Args... args) const -> Ret
  {
    assert(IsBound() && "Execute called on an unbound delegate");
    return invocation_(std::forward<Args>(args)...);
  }

  /**
   * @brief 已绑定则执行并返回 true，否则什么也不做返回 false。
   *
   * 与 Unreal 一致，仅对无返回值的委托提供（有返回值时
   * “没执行”无法给出合理的返回结果，请先 IsBound 再 Execute）。
   */
  auto ExecuteIfBound(Args... args) const -> bool
  {
    static_assert(std::is_void_v<Ret>,
                  "ExecuteIfBound is only available for void-returning "
                  "delegates; check IsBound() and call Execute() instead");
    if (!IsBound()) {
      return false;
    }
    invocation_(std::forward<Args>(args)...);
    return true;
  }

private:
  // 统一的绑定入口：把目标可调用对象与 Payload 打包成 std::function。
  // 调用时实参顺序为：签名参数在前，Payload 在后（与 Unreal 一致）。
  template<typename Callable, typename... Payload>
  void BindInternal(Callable callable, Payload&&... payload)
  {
    weak_.reset();
    uses_weak_ = false;
    owner_ = nullptr;
    if constexpr (sizeof...(Payload) == 0) {
      invocation_ = [callable =
                         std::move(callable)](Args... args) mutable -> Ret
      { return std::invoke(callable, std::forward<Args>(args)...); };
    } else {
      invocation_ = [callable = std::move(callable),
                     payload = std::make_tuple(std::forward<Payload>(
                         payload)...)](Args... args) mutable -> Ret
      {
        return std::apply(
            [&](const auto&... extras) -> Ret
            {
              return std::invoke(
                  callable, std::forward<Args>(args)..., extras...);
            },
            payload);
      };
    }
  }

  std::function<Ret(Args...)> invocation_;
  std::weak_ptr<const void> weak_;  // 弱绑定时跟踪目标对象的生命周期
  bool uses_weak_ = false;
  const void* owner_ = nullptr;  // 供 IsBoundToObject / RemoveAll 使用
};

template<typename Signature>
class MulticastDelegate;

/**
 * @brief 多播委托，对应 Unreal 的 TMulticastDelegate<void(Args...)>。
 *
 * 可以绑定任意多个目标，Broadcast 时按加入顺序依次调用。
 * 多播委托没有返回值（与 Unreal 一致）。
 *
 * 生命周期安全：AddSP / AddWeakLambda 加入的目标在对象销毁后
 * 会被 Broadcast 自动跳过并清理。
 *
 * 允许在 Broadcast 过程中 Add / Remove：本轮广播基于快照进行，
 * 广播中新加入的目标下一轮才会被调用，被移除的目标不再被调用。
 */
template<typename... Args>
class MulticastDelegate<void(Args...)>
{
public:
  using DelegateType = Delegate<void(Args...)>;

  /// 加入一个已构造好的单播委托，对应 Unreal 的 Add(FDelegate)
  auto Add(DelegateType delegate) -> DelegateHandle
  {
    auto handle = DelegateHandle::Create();
    slots_.push_back(
        Slot {handle, std::make_shared<DelegateType>(std::move(delegate))});
    return handle;
  }

  template<typename... FuncPayload, typename... Payload>
  auto AddStatic(void (*func)(Args..., FuncPayload...), Payload&&... payload)
      -> DelegateHandle
  {
    return Add(
        DelegateType::CreateStatic(func, std::forward<Payload>(payload)...));
  }

  template<typename Functor, typename... Payload>
  auto AddLambda(Functor&& functor, Payload&&... payload) -> DelegateHandle
  {
    return Add(DelegateType::CreateLambda(std::forward<Functor>(functor),
                                          std::forward<Payload>(payload)...));
  }

  template<typename Class, typename... FuncPayload, typename... Payload>
  auto AddRaw(Class* object,
              void (Class::*method)(Args..., FuncPayload...),
              Payload&&... payload) -> DelegateHandle
  {
    return Add(DelegateType::CreateRaw(
        object, method, std::forward<Payload>(payload)...));
  }

  template<typename Class, typename... FuncPayload, typename... Payload>
  auto AddRaw(const Class* object,
              void (Class::*method)(Args..., FuncPayload...) const,
              Payload&&... payload) -> DelegateHandle
  {
    return Add(DelegateType::CreateRaw(
        object, method, std::forward<Payload>(payload)...));
  }

  template<typename Class, typename... FuncPayload, typename... Payload>
  auto AddSP(const std::shared_ptr<Class>& object,
             void (Class::*method)(Args..., FuncPayload...),
             Payload&&... payload) -> DelegateHandle
  {
    return Add(DelegateType::CreateSP(
        object, method, std::forward<Payload>(payload)...));
  }

  template<typename Class, typename... FuncPayload, typename... Payload>
  auto AddSP(const std::shared_ptr<Class>& object,
             void (Class::*method)(Args..., FuncPayload...) const,
             Payload&&... payload) -> DelegateHandle
  {
    return Add(DelegateType::CreateSP(
        object, method, std::forward<Payload>(payload)...));
  }

  template<typename Class, typename Functor, typename... Payload>
  auto AddWeakLambda(const std::shared_ptr<Class>& object,
                     Functor&& functor,
                     Payload&&... payload) -> DelegateHandle
  {
    DelegateType delegate;
    delegate.BindWeakLambda(object,
                            std::forward<Functor>(functor),
                            std::forward<Payload>(payload)...);
    return Add(std::move(delegate));
  }

  /**
   * @brief 按句柄移除一次绑定，对应 Unreal 的 Remove(FDelegateHandle)。
   * @return 是否找到并移除了对应的绑定
   */
  auto Remove(DelegateHandle handle) -> bool
  {
    auto iter = std::find_if(slots_.begin(),
                             slots_.end(),
                             [handle](const Slot& slot)
                             { return slot.handle == handle; });
    if (iter == slots_.end()) {
      return false;
    }
    slots_.erase(iter);
    return true;
  }

  /**
   * @brief 移除绑定到指定对象上的所有绑定，对应 Unreal 的 RemoveAll(this)。
   * @return 移除的绑定数量
   */
  auto RemoveAll(const void* object) -> std::size_t
  {
    auto const before = slots_.size();
    std::erase_if(slots_,
                  [object](const Slot& slot)
                  { return slot.delegate->IsBoundToObject(object); });
    return before - slots_.size();
  }

  /// 移除全部绑定，对应 Unreal 的 Clear()
  void Clear() { slots_.clear(); }

  /// 是否还有至少一个有效绑定
  [[nodiscard]] auto IsBound() const -> bool
  {
    return std::any_of(slots_.begin(),
                       slots_.end(),
                       [](const Slot& slot)
                       { return slot.delegate->IsBound(); });
  }

  /// 当前有效绑定的数量（已失效的弱绑定不计入）
  [[nodiscard]] auto Num() const -> std::size_t
  {
    return static_cast<std::size_t>(std::count_if(
        slots_.begin(),
        slots_.end(),
        [](const Slot& slot) { return slot.delegate->IsBound(); }));
  }

  /**
   * @brief 按加入顺序调用所有有效绑定，对应 Unreal 的 Broadcast()。
   *
   * 弱绑定目标已销毁的槽位被跳过，并在广播结束后清理。
   */
  void Broadcast(Args... args)
  {
    // 基于快照广播，容忍监听者在回调里 Add / Remove
    auto snapshot = slots_;
    for (const auto& slot : snapshot) {
      if (Contains(slot.handle)) {
        slot.delegate->ExecuteIfBound(args...);
      }
    }
    // 清理已失效的弱绑定槽位
    std::erase_if(slots_,
                  [](const Slot& slot) { return !slot.delegate->IsBound(); });
  }

private:
  struct Slot
  {
    DelegateHandle handle;
    // 广播快照共享同一个调用节点，既避免容器变更导致悬空引用，也保证
    // mutable lambda 等有状态可调用对象的状态能跨多次 Broadcast 保留。
    std::shared_ptr<DelegateType> delegate;
  };

  [[nodiscard]] auto Contains(DelegateHandle handle) const -> bool
  {
    return std::any_of(slots_.begin(),
                       slots_.end(),
                       [handle](const Slot& slot)
                       { return slot.handle == handle; });
  }

  std::vector<Slot> slots_;
};

}  // namespace delegates

// ---------------------------------------------------------------------------
// 声明宏：与 Unreal 的 DECLARE_* 系列一一对应。
// 参数部分使用 __VA_ARGS__，因此带逗号的模板类型也可以直接使用。
// ---------------------------------------------------------------------------

// 单播委托
#define DECLARE_DELEGATE(DelegateName) \
  using DelegateName = ::delegates::Delegate<void()>
#define DECLARE_DELEGATE_OneParam(DelegateName, ...) \
  using DelegateName = ::delegates::Delegate<void(__VA_ARGS__)>
#define DECLARE_DELEGATE_TwoParams(DelegateName, ...) \
  using DelegateName = ::delegates::Delegate<void(__VA_ARGS__)>
#define DECLARE_DELEGATE_ThreeParams(DelegateName, ...) \
  using DelegateName = ::delegates::Delegate<void(__VA_ARGS__)>
#define DECLARE_DELEGATE_FourParams(DelegateName, ...) \
  using DelegateName = ::delegates::Delegate<void(__VA_ARGS__)>

// 带返回值的单播委托
#define DECLARE_DELEGATE_RetVal(ReturnType, DelegateName) \
  using DelegateName = ::delegates::Delegate<ReturnType()>
#define DECLARE_DELEGATE_RetVal_OneParam(ReturnType, DelegateName, ...) \
  using DelegateName = ::delegates::Delegate<ReturnType(__VA_ARGS__)>
#define DECLARE_DELEGATE_RetVal_TwoParams(ReturnType, DelegateName, ...) \
  using DelegateName = ::delegates::Delegate<ReturnType(__VA_ARGS__)>
#define DECLARE_DELEGATE_RetVal_ThreeParams(ReturnType, DelegateName, ...) \
  using DelegateName = ::delegates::Delegate<ReturnType(__VA_ARGS__)>

// 多播委托
#define DECLARE_MULTICAST_DELEGATE(DelegateName) \
  using DelegateName = ::delegates::MulticastDelegate<void()>
#define DECLARE_MULTICAST_DELEGATE_OneParam(DelegateName, ...) \
  using DelegateName = ::delegates::MulticastDelegate<void(__VA_ARGS__)>
#define DECLARE_MULTICAST_DELEGATE_TwoParams(DelegateName, ...) \
  using DelegateName = ::delegates::MulticastDelegate<void(__VA_ARGS__)>
#define DECLARE_MULTICAST_DELEGATE_ThreeParams(DelegateName, ...) \
  using DelegateName = ::delegates::MulticastDelegate<void(__VA_ARGS__)>

// 事件：本质是多播委托，但 Broadcast/Clear 只有 OwningType 能调用，
// 外部只能 Add* / Remove。对应 Unreal 的 DECLARE_EVENT*。
#define DECLARE_EVENT(OwningType, EventName) \
  class EventName : public ::delegates::MulticastDelegate<void()> \
  { \
    friend OwningType; \
    using ::delegates::MulticastDelegate<void()>::Broadcast; \
    using ::delegates::MulticastDelegate<void()>::Clear; \
  }
#define DECLARE_EVENT_OneParam(OwningType, EventName, ...) \
  class EventName : public ::delegates::MulticastDelegate<void(__VA_ARGS__)> \
  { \
    friend OwningType; \
    using ::delegates::MulticastDelegate<void(__VA_ARGS__)>::Broadcast; \
    using ::delegates::MulticastDelegate<void(__VA_ARGS__)>::Clear; \
  }
#define DECLARE_EVENT_TwoParams(OwningType, EventName, ...) \
  class EventName : public ::delegates::MulticastDelegate<void(__VA_ARGS__)> \
  { \
    friend OwningType; \
    using ::delegates::MulticastDelegate<void(__VA_ARGS__)>::Broadcast; \
    using ::delegates::MulticastDelegate<void(__VA_ARGS__)>::Clear; \
  }
