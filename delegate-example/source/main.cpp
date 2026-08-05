/**
 * @file main.cpp
 * @brief 仿 Unreal 委托系统的完整功能演示。
 *
 * 用一个简化的游戏场景演示委托的典型用法：
 *  - 角色 (Character) 拥有多播委托 OnHealthChanged 和事件 OnDeath
 *  - HUD / 成就系统等监听者通过 AddRaw / AddSP / AddLambda 订阅
 *  - 伤害计算通过带返回值的单播委托注入，实现可替换的策略
 *
 * 详细文档见 docs/delegate.md。
 */

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

#include "delegate.hpp"

// ---------------------------------------------------------------------------
// 委托类型声明：与 Unreal 一样，用 DECLARE_* 宏声明具名委托类型
// ---------------------------------------------------------------------------

// 单播：无参数
DECLARE_DELEGATE(FOnGameStarted);
// 单播：一个参数
DECLARE_DELEGATE_OneParam(FOnPlayerRenamed, const std::string&);
// 单播：带返回值（伤害计算策略：基础伤害 * 倍率 -> 最终伤害）
DECLARE_DELEGATE_RetVal_TwoParams(int,
                                  FDamageFormula,
                                  int /*base*/,
                                  float /*mult*/);
// 多播：血量变化 (旧值, 新值)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged,
                                     float /*old*/,
                                     float /*now*/);

// ---------------------------------------------------------------------------
// 游戏对象
// ---------------------------------------------------------------------------

/// 角色：委托的“广播方”。它只管广播，不知道也不关心谁在监听——这就是委托解耦的意义。
class Character
{
public:
  /// 血量变化多播委托：任何人都可以订阅，也允许外部 Broadcast
  FOnHealthChanged OnHealthChanged;

  /// 死亡事件：DECLARE_EVENT 保证只有 Character（友元）能 Broadcast，
  /// 外部只能订阅，不能伪造“死亡”通知
  DECLARE_EVENT(Character, FDeathEvent);

  auto OnDeath() -> FDeathEvent& { return death_event_; }

  /// 可替换的伤害计算策略（单播 + 返回值），未绑定时用默认公式
  FDamageFormula DamageFormula;

  void TakeDamage(int base_damage, float multiplier)
  {
    // 有绑定就用外部注入的公式，否则用默认公式——先 IsBound 再 Execute
    int const damage = DamageFormula.IsBound()
        ? DamageFormula.Execute(base_damage, multiplier)
        : static_cast<int>(static_cast<float>(base_damage) * multiplier);

    float const old_health = health_;
    health_ = std::max(0.0F, health_ - static_cast<float>(damage));
    std::cout << "[Character] 受到 " << damage << " 点伤害，血量 " << old_health
              << " -> " << health_ << '\n';

    OnHealthChanged.Broadcast(old_health, health_);

    if (old_health > 0.0F && health_ <= 0.0F) {
      death_event_.Broadcast();  // 只有 Character 自己能触发死亡事件
    }
  }

private:
  float health_ = 100.0F;
  FDeathEvent death_event_;
};

/// HUD：栈上/裸指针对象，用 AddRaw 订阅（需自己保证生命周期）
class Hud
{
public:
  void HandleHealthChanged(float old_health, float new_health)
  {
    std::cout << "  [HUD-AddRaw] 血条刷新: " << old_health << " -> "
              << new_health << '\n';
  }
};

/// 成就系统：shared_ptr 管理，用 AddSP 订阅（对象销毁后自动失效，生命周期安全）
class AchievementSystem
{
public:
  void HandleHealthChanged(float /*old_health*/, float new_health)
  {
    std::cout << "  [成就-AddSP] 检查成就（当前血量 " << new_health << "）\n";
  }
};

/// 普通函数 + Payload：最后一个参数 tag 不来自广播，而是绑定时附带的
void LogHealthChanged(float old_health,
                      float new_health,
                      const std::string& tag)
{
  std::cout << "  [日志-AddStatic+Payload:" << tag << "] " << old_health
            << " -> " << new_health << '\n';
}

auto main() -> int
{
  std::cout << "===== 1. 单播委托：Bind / IsBound / Execute / Unbind =====\n";
  {
    FOnGameStarted on_game_started;
    std::cout << "绑定前 IsBound = " << on_game_started.IsBound() << '\n';
    // 未绑定时 ExecuteIfBound 安全返回 false，而 Execute 是错误用法
    std::cout << "未绑定 ExecuteIfBound 返回 "
              << on_game_started.ExecuteIfBound() << '\n';

    on_game_started.BindLambda([] { std::cout << "  [Lambda] 游戏开始！\n"; });
    std::cout << "绑定后 IsBound = " << on_game_started.IsBound() << '\n';
    on_game_started.Execute();

    on_game_started.Unbind();
    std::cout << "Unbind 后 ExecuteIfBound 返回 "
              << on_game_started.ExecuteIfBound() << '\n';
  }

  std::cout << "\n===== 2. 单播委托：带参数 + Payload 附加参数 =====\n";
  {
    FOnPlayerRenamed on_renamed;
    // BindStatic 附带 payload："改名系统" 在调用时追加到签名参数之后
    on_renamed.BindStatic(
        +[](const std::string& name, const std::string& source)
        { std::cout << "  [" << source << "] 玩家改名为: " << name << '\n'; },
        std::string {"改名系统"});
    on_renamed.Execute("Gerald");
  }

  std::cout << "\n===== 3. 带返回值的单播委托：可替换的伤害公式 =====\n";
  Character character;
  {
    character.TakeDamage(10, 1.5F);  // 未绑定公式，用默认计算：15 点

    // 注入“暴击”公式：策略被替换，Character 的代码无需任何改动
    character.DamageFormula.BindLambda(
        [](int base, float mult)
        { return static_cast<int>(static_cast<float>(base) * mult * 2.0F); });
    character.TakeDamage(10, 1.5F);  // 暴击公式：30 点
    character.DamageFormula.Unbind();
  }

  std::cout
      << "\n===== 4. 多播委托：AddRaw / AddSP / AddLambda / AddStatic =====\n";
  Hud hud;
  auto achievements = std::make_shared<AchievementSystem>();
  {
    character.OnHealthChanged.AddRaw(&hud, &Hud::HandleHealthChanged);
    character.OnHealthChanged.AddSP(achievements,
                                    &AchievementSystem::HandleHealthChanged);
    character.OnHealthChanged.AddStatic(&LogHealthChanged,
                                        std::string {"战斗日志"});
    auto camera_handle = character.OnHealthChanged.AddLambda(
        [](float, float) { std::cout << "  [镜头-AddLambda] 播放受击震屏\n"; });

    std::cout << "监听者数量 Num() = " << character.OnHealthChanged.Num()
              << "，Broadcast 依次通知所有监听者：\n";
    character.TakeDamage(5, 1.0F);

    std::cout << "\n-- Remove(handle) 移除镜头监听 --\n";
    character.OnHealthChanged.Remove(camera_handle);
    character.TakeDamage(5, 1.0F);
  }

  std::cout << "\n===== 5. 生命周期安全：AddSP 的对象销毁后自动失效 =====\n";
  {
    achievements.reset();  // 成就系统被销毁，无需手动取消订阅
    std::cout << "成就系统已销毁，广播自动跳过它（不会崩溃）：\n";
    character.TakeDamage(5, 1.0F);
    std::cout << "失效槽位已被清理，Num() = " << character.OnHealthChanged.Num()
              << '\n';
  }

  std::cout << "\n===== 6. RemoveAll(object)：按对象批量取消订阅 =====\n";
  {
    auto removed = character.OnHealthChanged.RemoveAll(&hud);
    std::cout << "RemoveAll(&hud) 移除了 " << removed
              << " 个绑定，剩余 Num() = " << character.OnHealthChanged.Num()
              << '\n';
  }

  std::cout
      << "\n===== 7. 事件 (DECLARE_EVENT)：外部只能订阅，不能广播 =====\n";
  {
    character.OnDeath().AddLambda(
        [] { std::cout << "  [游戏模式] 角色死亡，结算本局\n"; });
    // 下面这行编译不过——Broadcast 对外部是私有的，只有 Character 能触发：
    // character.OnDeath().Broadcast();  // error: 'Broadcast' is private
    std::cout << "连续攻击直到角色死亡：\n";
    character.TakeDamage(30, 1.0F);
    character.TakeDamage(50, 1.0F);  // 首次归零，Character 内部广播一次 OnDeath
  }

  return 0;
}
