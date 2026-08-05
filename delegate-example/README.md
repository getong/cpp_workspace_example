# delegate-example

一个仅头文件的 C++20 委托示例，API 和使用方式仿照 Unreal Engine Delegate。
项目重点是用可运行代码说明委托如何把“事件发生者”和“响应者”解耦，而不是复刻
Unreal 的反射、`UObject` 或蓝图系统。

## 已实现功能

- 单播委托：`BindStatic`、`BindLambda`、`BindRaw`、`BindSP`、
  `BindWeakLambda`、`Execute`、`ExecuteIfBound` 和 `Unbind`
- 带返回值的单播委托，可用于注入策略或回调查询
- 多播委托：`Add*`、`Broadcast`、`Remove(handle)`、`RemoveAll(object)`、
  `Clear` 和 `Num`
- `DelegateHandle`，对应 Unreal 的 `FDelegateHandle`
- `DECLARE_EVENT*`：外部可以订阅，只有拥有者类型可以广播
- Payload：绑定时保存额外参数，执行时追加在委托参数之后
- `shared_ptr`/`weak_ptr` 生命周期跟踪：目标销毁后自动失效
- 广播期间安全地新增、移除或清空监听者

完整 API 对照、设计说明和使用建议见
[委托使用与设计文档](docs/delegate.md)。

## 快速运行

项目使用 CMake 和 vcpkg，要求支持 C++20 的编译器，并设置 `VCPKG_ROOT`；如果
`~/vcpkg` 存在，脚本会自动使用它。

```sh
./build.sh
./build/dev/delegate-example
```

示例程序通过一个简化的角色受伤场景演示：

- 单播委托替换伤害计算策略
- HUD、日志、成就和镜头系统订阅血量变化多播委托
- `AddSP` 监听者销毁后自动跳过
- 使用句柄或对象批量取消订阅
- 死亡事件只能由 `Character` 广播

## 运行测试

```sh
BUILD_DIR=build/test ./compile_commands.sh
ctest --test-dir build/test --output-on-failure
```

## 主要文件

- `source/delegate.hpp`：委托实现和 `DECLARE_*` 宏
- `source/main.cpp`：可直接运行的完整功能示例
- `test/source/delegate-example_test.cpp`：行为和生命周期测试
- `docs/delegate.md`：API、设计、限制及扩展说明

## 与 Unreal 的边界

本项目覆盖普通 C++ 中最有教学价值的委托语义，但不包含动态委托、`UFUNCTION`、
蓝图、序列化、垃圾回收对象跟踪和线程安全保证。项目中的 `BindSP`/`AddSP` 使用
标准库智能指针表达 Unreal `BindSP` 的弱引用语义；它不是 `BindUObject` 的替代品。

构建环境的更多信息见 [BUILDING.md](BUILDING.md)。
