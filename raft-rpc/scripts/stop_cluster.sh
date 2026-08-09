#!/usr/bin/env bash
# Stop the local counter cluster started by run_cluster.sh.
set -uo pipefail

RUNTIME_DIR=${1:-/tmp/counter_cluster}
for pidfile in "$RUNTIME_DIR"/node*/server.pid; do
    [[ -f "$pidfile" ]] || continue
    pid=$(cat "$pidfile")
    if kill "$pid" 2>/dev/null; then
        echo "stopped pid $pid ($(dirname "$pidfile"))"
    fi
    rm -f "$pidfile"
done
