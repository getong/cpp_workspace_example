# Raft 稳健示例（基于 eBay/NuRaft）

本示例使用 [eBay/NuRaft](https://github.com/eBay/NuRaft)（v3.0，Apache 2.0）实现
多副本计数器。NuRaft 是目前维护最活跃、生产验证最强的 C++ Raft 库：它是
[ClickHouse Keeper](https://clickhouse.com/docs/guides/sre/keeper/clickhouse-keeper)
的共识核心，经过 Jepsen 测试（崩溃 / 网络分区 / 磁盘损坏 / 网络降速）。

## 代码结构

| 文件 | 职责 |
|---|---|
| `source/counter_state_machine.hxx` | 状态机：`commit()` 应用 ADD/GET 操作；逻辑快照的保存/传输/加载 |
| `source/mem_log_store.hxx` | 内存版 Raft log store（生产系统应换成落盘实现，见下） |
| `source/file_state_mgr.hxx` | **持久化** term/投票状态与集群配置（写临时文件 + rename 原子替换） |
| `source/simple_logger.hxx` | 精简版 `nuraft::logger`，写文件日志 |
| `source/counter_server.cpp` | raft_launcher 启动 + 面向客户端的 TCP 行协议端口 |
| `source/counter_client.cpp` | 客户端：多端点尝试、跟随 REDIRECT、指数退避重试 |

## 稳健性设计

- **auto-forwarding**（`params.auto_forwarding_ = true`）：follower 自动把写请求
  转发给 leader，客户端连任意节点即可读写，无需自己做 leader 发现。
- **线性一致读**：`get` 作为 no-op 走一遍 Raft 日志提交，任何时刻（包括刚切主）
  都不会读到旧值；`local` 命令提供不保证一致性的本地快速读作对比。
- **持久化投票状态**：`file_state_mgr` 在投票/升 term 前先落盘 `srv_state`，
  重启的节点不可能在同一 term 内投两票（这是 NuRaft 官方示例内存版 state_mgr
  存在的真实安全隐患）。集群配置同样落盘，重启后自动按原配置重新入群。
- **快照 + 日志压缩**：每 50 条日志自动建快照（`snapshot_distance_`），日志
  随之压缩；新节点/落后节点通过快照追赶。
- **运行时成员变更**：`addsrv` / `rmsrv` 管理命令（NuRaft 每次只允许变更一个
  成员，脚本里逐个加入）。

**已知简化**：log store 是内存版。节点重启后日志为空，靠 leader 的快照+日志
重新同步（演示集群没问题）。生产环境请实现落盘 log store（fsync、防半写），
参考 ClickHouse Keeper 的 `KeeperLogStore`。

## 构建与运行

```bash
cmake -B build/raft -S . \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/raft -j --target counter_server counter_client

./scripts/run_cluster.sh build/raft     # 3 节点：raft 21001-3，客户端 22001-3

EPS=127.0.0.1:22001,127.0.0.1:22002,127.0.0.1:22003
./build/raft/counter_client $EPS add 5      # OK 5
./build/raft/counter_client $EPS get        # OK 5（线性一致读）
./build/raft/counter_client $EPS status     # OK id=.. leader=.. term=.. value=..
./build/raft/counter_client $EPS bench 100  # 压测

./scripts/stop_cluster.sh
```

验证容错：`kill` 掉 leader，1 秒内新 leader 当选（选举超时 300-600ms），
客户端写入不中断；重启被 kill 的节点会自动重新入群并追平数据。

## 客户端协议（TCP 行协议，端口 = raft 端口 + 1000）

```
add <delta>              -> OK <新值>
get                      -> OK <值>            （线性一致）
local                    -> OK <值> leader=<id> term=<t>（本地读，可能旧）
status                   -> OK id=.. leader=.. term=.. commit=.. value=..
addsrv <id> <host:port>  -> OK                 （仅 leader；否则 REDIRECT）
rmsrv <id>               -> OK
```

## 参考

- NuRaft: https://github.com/eBay/NuRaft （`docs/` 下有详细设计文档）
- eBay 博客: https://innovation.ebayinc.com/stories/nuraft-a-lightweight-c-raft-core/
- ClickHouse Keeper（生产级用法范本）: https://github.com/ClickHouse/ClickHouse/tree/master/src/Coordination
