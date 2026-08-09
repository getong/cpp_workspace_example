# 跨 NAT 组建 Raft 集群：P2P Overlay 方案

本项目的 NuRaft 集群默认走普通 TCP（asio 传输层），节点间必须互相可达。
要让分布在不同家庭/公司网络（NAT 之后）的机器组成一个集群，本方案在
**操作系统层**引入 P2P overlay 网络（ZeroTier 或 Tailscale）：每台机器获得
一个稳定的虚拟 IP，P2P 层负责打洞、加密和中继回退，**应用代码零改动**——
`counter_server` 监听 `0.0.0.0`，天然覆盖虚拟网卡，只需把 endpoint 填成
overlay IP。

## 为什么不用 libzt（嵌入式 ZeroTier SDK）？

libzt 是用户态网络栈，应用必须改用它的 `zts_socket()` API 才能走 overlay；
而 NuRaft 的 asio 传输层用的是内核 socket，流量不会经过 libzt。所以
"零改动"只有 OS 层 overlay（TUN 虚拟网卡）才成立。若将来想把 P2P 栈嵌进
进程内（免装系统服务），正确路线是替换 NuRaft 的
`rpc_client_factory`/`rpc_listener` 传输层（参考 NuRaft 源码
`tests/unit/fake_network.hxx`），那是另一个工程量级。

## 一、搭建 overlay 网络（每台机器执行一次）

### 选项 1：ZeroTier

```bash
# macOS
brew install --cask zerotier-one
# Linux
curl -s https://install.zerotier.com | sudo bash

# 在 https://my.zerotier.com 创建一个 Network，拿到 16 位网络 ID
sudo zerotier-cli join <network_id>
# 到 ZeroTier 控制台把每台机器勾选 "Auth"（私有网络需授权）
sudo zerotier-cli listnetworks   # 最后一列即分配到的虚拟 IP
```

### 选项 2：Tailscale（基于 WireGuard，配置更少）

```bash
# macOS
brew install --cask tailscale
# Linux
curl -fsSL https://tailscale.com/install.sh | sh

tailscale up        # 浏览器登录，同一账号下的机器自动互通
tailscale ip -4     # 查看本机虚拟 IP（100.x.x.x）
```

两者的底层（underlay）只需要能发出 UDP 出站流量（ZeroTier 走 9993，
Tailscale 走 41641），一般家庭/办公网络无需任何端口转发；打洞失败时
自动回退到官方中继（ZeroTier root / Tailscale DERP），只是延迟略高。

## 二、启动集群

约定与 `run_cluster.sh` 一致：Raft 端口 `21000+id`，客户端端口再 `+1000`。
`run_overlay_node.sh` 会自动探测本机 overlay IP（先 Tailscale 后
ZeroTier，可用 `OVERLAY_IP=x.x.x.x` 覆盖）。

```bash
# 机器 A（node 1，引导节点，先单节点成簇）
./scripts/run_overlay_node.sh <build_dir> 1

# 机器 B（node 2，通过 A 的客户端端口加入）
./scripts/run_overlay_node.sh <build_dir> 2 --join <A的overlay_IP>:22001

# 机器 C（node 3）
./scripts/run_overlay_node.sh <build_dir> 3 --join <A的overlay_IP>:22001
```

之后客户端在任意一台机器（或任何加入了同一 overlay 网络的机器）上使用，
写请求由 auto-forwarding 自动转给 leader：

```bash
./build/counter_client <A_ip>:22001,<B_ip>:22002,<C_ip>:22003 add 5
./build/counter_client <A_ip>:22001,<B_ip>:22002,<C_ip>:22003 get
```

停止本机节点：`./scripts/stop_cluster.sh`（pid 文件布局与本地脚本兼容）。

## 三、注意事项

* **安全**：server 监听 `0.0.0.0`，Raft 与客户端端口在物理网卡上同样
  暴露，而 NuRaft 默认流量是明文。请用 OS 防火墙把 21xxx/22xxx 端口
  限制到 overlay 网段（Tailscale 为 `100.64.0.0/10`），或用 ZeroTier
  的 flow rules / Tailscale ACL 收紧。overlay 网络本身端到端加密，
  网内流量无需额外 TLS。
* **延迟与选举超时**：直连打洞成功时延迟接近公网 RTT；走中继时可能到
  100–200ms。当前参数（心跳 100ms、选举超时 300–600ms，见
  `source/counter_server.cpp` 的 `raft_params`）对跨地域中继链路偏紧，
  若观察到无故换主，把心跳调到 500ms、选举超时调到 2000–4000ms。
* **IP 稳定性**：ZeroTier/Tailscale 的虚拟 IP 默认长期稳定，Raft 集群
  配置里记录的 endpoint 不会因机器换物理网络而失效——这正是选 overlay
  的核心收益。若某台机器 overlay IP 确需变更，用 `rmsrv` + `addsrv`
  以新 endpoint 重新加入。
* **成员管理**：overlay 只解决连通性；集群成员变更仍必须通过 leader 的
  `addsrv`/`rmsrv` 共识流程，一次只加/减一个节点。
