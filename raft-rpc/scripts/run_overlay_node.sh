#!/usr/bin/env bash
# Start ONE counter node on a P2P overlay network (ZeroTier / Tailscale).
#   ./scripts/run_overlay_node.sh <build_dir> <server_id> [--join <bootstrap_client_ep>] [runtime_dir]
#
# The node binds its Raft endpoint to this machine's overlay IP, which is
# auto-detected (Tailscale first, then ZeroTier). Override with OVERLAY_IP=x.x.x.x.
# Raft port = 21000 + id, client port = raft port + 1000 (same convention as
# run_cluster.sh). Stop with: ./scripts/stop_cluster.sh [runtime_dir]
#
# Typical 3-machine flow (see docs/overlay-cluster.md):
#   machine A:  ./scripts/run_overlay_node.sh build 1
#   machine B:  ./scripts/run_overlay_node.sh build 2 --join <A_overlay_ip>:22001
#   machine C:  ./scripts/run_overlay_node.sh build 3 --join <A_overlay_ip>:22001
set -euo pipefail

BUILD_DIR=${1:?"usage: $0 <build_dir> <server_id> [--join <bootstrap_client_ep>] [runtime_dir]"}
SERVER_ID=${2:?"usage: $0 <build_dir> <server_id> [--join <bootstrap_client_ep>] [runtime_dir]"}
shift 2

BOOTSTRAP_EP=""
if [[ "${1:-}" == "--join" ]]; then
    BOOTSTRAP_EP=${2:?"--join requires <bootstrap_client_ep>"}
    shift 2
fi
RUNTIME_DIR=${1:-/tmp/counter_cluster}

# Absolute paths: the server is started from inside $NODE_DIR.
BUILD_DIR=$(cd "$BUILD_DIR" && pwd)
SERVER="$BUILD_DIR/counter_server"
CLIENT="$BUILD_DIR/counter_client"
[[ -x "$SERVER" ]] || { echo "counter_server not found at $SERVER" >&2; exit 1; }

detect_overlay_ip() {
    if [[ -n "${OVERLAY_IP:-}" ]]; then
        echo "$OVERLAY_IP"
        return 0
    fi
    if command -v tailscale > /dev/null 2>&1; then
        local ip
        ip=$(tailscale ip -4 2> /dev/null | head -1 || true)
        if [[ -n "$ip" ]]; then
            echo "$ip"
            return 0
        fi
    fi
    if command -v zerotier-cli > /dev/null 2>&1; then
        # "200 listnetworks <nwid> <name> <mac> OK <type> <dev> <ip/cidr,...>"
        local ip
        ip=$(zerotier-cli listnetworks 2> /dev/null | awk '$1 == "200" && $2 == "listnetworks"' \
            | tr ',' '\n' | grep -Eo '([0-9]{1,3}\.){3}[0-9]{1,3}/[0-9]+' \
            | head -1 | cut -d/ -f1 || true)
        if [[ -n "$ip" ]]; then
            echo "$ip"
            return 0
        fi
    fi
    return 1
}

HOST=$(detect_overlay_ip) || {
    echo "no overlay IP found: install/join Tailscale or ZeroTier," >&2
    echo "or set OVERLAY_IP=<ip> explicitly" >&2
    exit 1
}

RAFT_PORT=$((21000 + SERVER_ID))
CLIENT_PORT=$((RAFT_PORT + 1000))
RAFT_EP="$HOST:$RAFT_PORT"

NODE_DIR="$RUNTIME_DIR/node$SERVER_ID"
mkdir -p "$NODE_DIR"
(
    cd "$NODE_DIR"
    "$SERVER" "$SERVER_ID" "$RAFT_EP" ./data > server.log 2>&1 &
    echo $! > server.pid
)
echo "node$SERVER_ID raft=$RAFT_EP client=$HOST:$CLIENT_PORT log=$NODE_DIR/server.log"

if [[ -n "$BOOTSTRAP_EP" ]]; then
    [[ -x "$CLIENT" ]] || { echo "counter_client not found at $CLIENT" >&2; exit 1; }
    sleep 1
    echo "joining cluster via $BOOTSTRAP_EP ..."
    "$CLIENT" "$BOOTSTRAP_EP" addsrv "$SERVER_ID" "$RAFT_EP"
    sleep 1
    "$CLIENT" "$BOOTSTRAP_EP" status
fi
