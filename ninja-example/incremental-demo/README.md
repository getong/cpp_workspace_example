# CMake + Ninja 增量编译示例

这是一个自包含的最小示例，用来说明 **增量编译（incremental compilation）**：
修改代码后重新构建时，Ninja 只重新编译"真正受影响"的编译单元，而不是从头编译整个项目。

## 项目结构

```
incremental-demo/
├── CMakeLists.txt
├── demo.sh                     # 一键演示脚本
└── src/
    ├── main.cpp                # 包含 version.hpp、greeting.hpp、add.hpp、mul.hpp
    ├── version.hpp             # 被 main.cpp 和 greeting.cpp 共同包含
    ├── greeting/
    │   ├── greeting.hpp
    │   └── greeting.cpp        # 包含 version.hpp
    └── mathlib/
        ├── add.hpp / add.cpp   # 不包含 version.hpp
        └── mul.hpp / mul.cpp   # 不包含 version.hpp
```

每个 `.cpp` 是一个独立的编译单元。文件之间的头文件包含关系是故意设计的，
用来观察"改一个文件会连带重编哪些文件"。

## 环境准备

需要 `cmake`（≥ 3.20）、`ninja` 和一个 C++20 编译器。macOS 上：

```sh
brew install cmake ninja
# 编译器用 Xcode 自带的 AppleClang 即可（xcode-select --install）

# 验证
cmake --version
ninja --version
```

## CLI 使用步骤与流程

CMake + Ninja 的日常工作流只有两条命令：**配置一次，之后反复构建**。

```
┌─────────────────────┐         ┌──────────────────────┐         ┌────────────┐
│ 1. 配置（只需一次）   │  ────▶  │ 2. 构建（每次改代码后）│  ────▶  │ 3. 运行     │
│ cmake -S . -B build │         │ ninja -C build       │         │ ./build/demo│
│       -G Ninja      │         │   （增量，只编译动过的）│         └────────────┘
└─────────────────────┘         └──────────┬───────────┘
                                           │ 改代码后回到第 2 步即可，
                                           ▼ 无需重新执行 cmake
                                    （新增/删除源文件并改了
                                     CMakeLists.txt 时，Ninja
                                     会自动触发 CMake 重新生成）
```

### 第 1 步：配置（Configure）

在 `incremental-demo/` 目录下执行：

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

各参数含义：

| 参数 | 作用 |
|---|---|
| `-S .` | 源码目录（含顶层 `CMakeLists.txt`） |
| `-B build` | 构建目录（所有产物放这里，不污染源码树） |
| `-G Ninja` | 使用 Ninja 生成器，产出 `build/build.ninja` |
| `-DCMAKE_BUILD_TYPE=Debug` | 构建类型，可换成 `Release`、`RelWithDebInfo` |

配置只需执行一次。之后即使修改了 `CMakeLists.txt`（比如新增源文件），
直接跑 `ninja` 也会自动检测到并重新运行 CMake，不需要手动重新配置。
只有想换生成器、换构建类型、换编译器时才需要重新配置（建议直接删掉
`build/` 重来，或另开一个构建目录如 `build-release/`）。

### 第 2 步：构建（Build）

```sh
ninja -C build          # -C：先 cd 进 build 目录再构建
```

等价写法（不依赖生成器，任何 CMake 项目通用）：

```sh
cmake --build build
```

常用构建选项：

```sh
ninja -C build -j 8         # 限制并行任务数（默认用满 CPU 核心数）
ninja -C build -v           # 显示完整编译命令行
ninja -C build -n           # 干跑：只打印将要执行的步骤，不真正执行
ninja -C build demo         # 只构建指定目标（及其依赖）
ninja -C build -t targets   # 列出所有可构建的目标
ninja -C build -t clean     # 清理所有产物（等价 cmake --build build --target clean）
```

### 第 3 步：运行

```sh
./build/demo
```

预期输出：

```
Hello from incremental-demo v1.0.0
add(2, 3) = 5
mul(4, 5) = 20
version   = 1.0.0
```

### 一键演示

不想手敲的话，直接运行演示脚本，它会按顺序执行配置、全量构建和下面的全部增量场景：

```sh
./demo.sh
```

## 增量编译的原理

1. **CMake 负责生成构建图**：`cmake -G Ninja` 把每个编译单元、静态库、
   可执行文件之间的依赖关系写进 `build.ninja`。
2. **编译器负责报告头文件依赖**：CMake 生成的编译命令带有 `-MD -MF xxx.o.d`，
   编译器在编译每个 `.cpp` 时会输出一个 depfile，列出它实际包含的所有头文件。
   Ninja 把这些信息存进 `.ninja_deps` 数据库（可用 `ninja -t deps` 查看）。
3. **Ninja 负责判断"脏"节点**：重新构建时，Ninja 对比每个输出与其全部输入
   （源文件 + depfile 记录的头文件）的时间戳，只重建过期的节点，
   并沿依赖图向下传播（重编 → 重新归档静态库 → 重新链接）。

## 动手实验：观察增量编译

```sh
# 场景 0：全量构建 —— 7 步（4 个编译 + 2 个静态库 + 1 个链接）
ninja -C build

# 场景 1：什么都不改 —— 输出 "ninja: no work to do."
ninja -C build

# 场景 2：只改一个 .cpp —— 只重编 add.cpp.o，重新归档 libmathlib.a，重链 demo（3 步）
#         greeting.cpp、mul.cpp、main.cpp 完全不动
touch src/mathlib/add.cpp
ninja -C build

# 场景 3：改一个被多处包含的头文件 —— 重编 greeting.cpp.o 和 main.cpp.o（4 步）
#         add.cpp / mul.cpp 因为没有包含 version.hpp，不受影响
touch src/version.hpp
ninja -C build
```

## 观察与调试工具

```sh
# 让 Ninja 解释"为什么要重建某个目标"
ninja -C build -d explain

# 查看某个目标文件记录在依赖数据库里的全部头文件依赖
ninja -C build -t deps CMakeFiles/greeting.dir/src/greeting/greeting.cpp.o

# 以图形方式查看构建依赖图（需要 graphviz）
ninja -C build -t graph demo | dot -Tpng -o graph.png

# 查看上次构建每一步的耗时记录
cat build/.ninja_log

# 生成 compile_commands.json 供 clangd 等工具使用（配置时加上）
cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

## 关键结论

- 增量编译的粒度是**编译单元（.cpp）**：改动 `.cpp` 只影响它自己 + 下游的归档/链接。
- 改动**头文件**的代价取决于它被多少个编译单元包含 —— 这就是为什么要尽量减少
  头文件的传递包含（用前置声明、Pimpl 等手段），公共头文件改一行可能触发全量重编。
- Ninja 本身不解析 C++，它完全信任编译器 depfile + 时间戳，所以判断"要不要重编"
  非常快（大项目上 no-op 构建通常在几十毫秒内完成），这是它比 Make 更适合做
  CMake 后端的主要原因之一。
