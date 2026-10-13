#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="$SCRIPT_DIR/logs/launch_all"

PORT=4242
WIDTH=10
HEIGHT=10
CLIENTS=5
FREQ=100
HOST=localhost
GUI_BIN=zappy_gui
AI_COUNT=1
TEAM_NAMES=(team1 team2)
AI_TEAM=team1

usage() {
    cat <<EOF
Usage: $0 [options]

Launch zappy_server, zappy_gui, and zappy_ai together.

Options:
  -p, --port PORT        Server port (default: $PORT)
  -x, --width WIDTH      Map width (default: $WIDTH)
  -y, --height HEIGHT    Map height (default: $HEIGHT)
  -c, --clients N        Slots per team (default: $CLIENTS)
  -f, --freq FREQ        Server frequency (default: $FREQ)
  -a, --agents N         Number of AI clients to launch (default: $AI_COUNT)
  -n, --team NAME        AI team name and only server team
      --teams "A B"      Server team names, space-separated
      --ai-team NAME     Team used by launched AI clients
      --host HOST        Host passed to GUI/AI clients (default: $HOST)
      --gui BIN          GUI binary to launch: zappy_gui or zappy_gui3d (default: $GUI_BIN)
  -h, --help             Show this help
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--port) PORT="$2"; shift 2 ;;
        -x|--width) WIDTH="$2"; shift 2 ;;
        -y|--height) HEIGHT="$2"; shift 2 ;;
        -c|--clients) CLIENTS="$2"; shift 2 ;;
        -f|--freq) FREQ="$2"; shift 2 ;;
        -a|--agents) AI_COUNT="$2"; shift 2 ;;
        -n|--team)
            AI_TEAM="$2"
            TEAM_NAMES=("$2")
            shift 2
            ;;
        --teams)
            read -r -a TEAM_NAMES <<< "$2"
            if [[ ${#TEAM_NAMES[@]} -eq 0 ]]; then
                echo "[launch] --teams needs at least one team name" >&2
                exit 1
            fi
            AI_TEAM="${TEAM_NAMES[0]}"
            shift 2
            ;;
        --ai-team) AI_TEAM="$2"; shift 2 ;;
        --host) HOST="$2"; shift 2 ;;
        --gui) GUI_BIN="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *)
            echo "[launch] Unknown option: $1" >&2
            usage
            exit 1
            ;;
    esac
done

if [[ "$GUI_BIN" != "zappy_gui" && "$GUI_BIN" != "zappy_gui3d" ]]; then
    echo "[launch] --gui must be zappy_gui or zappy_gui3d" >&2
    exit 1
fi

for bin in zappy_server "$GUI_BIN" zappy_ai; do
    if [[ ! -x "$SCRIPT_DIR/$bin" ]]; then
        echo "[launch] Missing executable: $SCRIPT_DIR/$bin" >&2
        echo "[launch] Build first with: make" >&2
        exit 1
    fi
done

mkdir -p "$LOG_DIR"
rm -f "$LOG_DIR"/*.log

PIDS=()
declare -A PROC_NAME

register_pid() {
    local pid="$1"
    local name="$2"

    PIDS+=("$pid")
    PROC_NAME[$pid]="$name"
}

cleanup() {
    trap - EXIT INT TERM
    if [[ ${#PIDS[@]} -eq 0 ]]; then
        return
    fi
    echo
    echo "[launch] Stopping processes..."
    for pid in "${PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
        fi
    done
    wait 2>/dev/null || true
    echo "[launch] Done."
}
trap cleanup EXIT INT TERM

wait_for_server() {
    for _ in $(seq 1 50); do
        if ! kill -0 "$SERVER_PID" 2>/dev/null; then
            return 1
        fi
        if 2>/dev/null bash -c "echo > /dev/tcp/$HOST/$PORT"; then
            return 0
        fi
        sleep 0.1
    done
    return 1
}

echo "[launch] Logs: $LOG_DIR"
if 2>/dev/null bash -c "echo > /dev/tcp/$HOST/$PORT"; then
    echo "[launch] Port $PORT is already accepting connections on $HOST." >&2
    echo "[launch] Stop the existing server or choose another port with -p." >&2
    exit 1
fi

echo "[launch] Starting server on port $PORT (${WIDTH}x${HEIGHT}, teams: ${TEAM_NAMES[*]})"
"$SCRIPT_DIR/zappy_server" -p "$PORT" -x "$WIDTH" -y "$HEIGHT" \
    -n "${TEAM_NAMES[@]}" -c "$CLIENTS" -f "$FREQ" \
    > "$LOG_DIR/server.log" 2>&1 &
SERVER_PID=$!
register_pid "$SERVER_PID" "zappy_server"

if ! wait_for_server; then
    echo "[launch] Server did not become ready in time" >&2
    echo "[launch] Last server log lines:" >&2
    tail -20 "$LOG_DIR/server.log" >&2 || true
    exit 1
fi

echo "[launch] Starting GUI: $GUI_BIN"
"$SCRIPT_DIR/$GUI_BIN" -p "$PORT" -h "$HOST" \
    > "$LOG_DIR/gui.log" 2>&1 &
GUI_PID=$!
register_pid "$GUI_PID" "$GUI_BIN"

sleep 0.3

echo "[launch] Starting $AI_COUNT AI client(s) for team '$AI_TEAM'"
for i in $(seq 1 "$AI_COUNT"); do
    "$SCRIPT_DIR/zappy_ai" -p "$PORT" -n "$AI_TEAM" -h "$HOST" \
        > "$LOG_DIR/ai_$i.log" 2>&1 &
    register_pid "$!" "zappy_ai#$i"
    sleep 0.05
done

echo "[launch] All binaries launched. Press Ctrl+C to stop."

while true; do
    sleep 1
    for pid in "${PIDS[@]}"; do
        if ! kill -0 "$pid" 2>/dev/null; then
            echo "[launch] ${PROC_NAME[$pid]:-process} (pid $pid) exited; stopping the rest." >&2
            exit 1
        fi
    done
done
