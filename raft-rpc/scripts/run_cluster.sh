#!/usr/bin/env bash
# Start a local 3-node NuRaft counter cluster.
#   ./scripts/run_cluster.sh <build_dir> [runtime_dir]
# Raft ports 21001-21003, client ports 22001-22003.
# Stop with: ./scripts/stop_cluster.sh [runtime_dir]
set -euo pipefail

BUILD_DIR=${1:?"usage: $0 <build_dir> [runtime_dir]"}
RUNTIME_DIR=${2:-/tmp/counter_cluster}
SERVER="$BUILD_DIR/counter_server"
CLIENT="$BUILD_DIR/counter_client"
CLIENT_EPS="127.0.0.1:22001,127.0.0.1:22002,127.0.0.1:22003"

[[ -x "$SERVER" ]] || { echo "counter_server not found at $SERVER" >&2; exit 1; }

mkdir -p "$RUNTIME_DIR"
for i in 1 2 3; do
    raft_port=$((21000 + i))
    dir="$RUNTIME_DIR/node$i"
    mkdir -p "$dir"
    (
        cd "$dir"
        "$SERVER" "$i" "127.0.0.1:$raft_port" ./data > server.log 2>&1 &
        echo $! > server.pid
    )
    echo "node$i raft=127.0.0.1:$raft_port client=127.0.0.1:$((raft_port + 1000))"
done

sleep 1
# Join node 2 and 3 into node 1's group (one membership change at a time).
"$CLIENT" 127.0.0.1:22001 addsrv 2 127.0.0.1:21002
sleep 1
"$CLIENT" 127.0.0.1:22001 addsrv 3 127.0.0.1:21003
sleep 1
"$CLIENT" 127.0.0.1:22001 status

echo
echo "Try: $CLIENT $CLIENT_EPS add 5 ; $CLIENT $CLIENT_EPS get"
