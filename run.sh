#!/usr/bin/env bash
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

LOG_DIR="$SCRIPT_DIR/logs"
mkdir -p "$LOG_DIR"
rm -f "$LOG_DIR"/*.log

PORT=4242
WIDTH=10
HEIGHT=10
TEAM=pioneer
CLIENTS=6
FREQ=10
AGENTS=20
GUI3D=0
SOLO=0

HANDSHAKE_THRESHOLD=3

while [[ $# -gt 0 ]]; do
    case $1 in
        -p) PORT=$2;    shift 2 ;;
        -x) WIDTH=$2;   shift 2 ;;
        -y) HEIGHT=$2;  shift 2 ;;
        -n) TEAM=$2;    shift 2 ;;
        -c) CLIENTS=$2; shift 2 ;;
        -f) FREQ=$2;    shift 2 ;;
        -a) AGENTS=$2;  shift 2 ;;
        --3d) GUI3D=1;  shift ;;
        -s)   SOLO=1;   shift ;;
        *)  echo "Usage: $0 [-p port] [-x w] [-y h] [-n team] [-c slots] [-f freq] [-a agents] [--3d] [-s]"; exit 1 ;;
    esac
done

PIDS=()
declare -A SPAWN_TIME
declare -A CONFIRMED

cleanup() {
    echo ""
    echo "[run] Arrêt de tous les processus..."
    trap - EXIT INT TERM
    for pid in "${PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
        fi
    done
    wait 2>/dev/null || true
    echo "[run] Terminé."

    # Parse agent logs into a human-readable report
    PARSER="$SCRIPT_DIR/tools/parse_zappy_log.py"
    REPORT="$LOG_DIR/parsed_zappy_activity.txt"
    if command -v python3 >/dev/null 2>&1 && [[ -f "$PARSER" ]]; then
        shopt -s nullglob
        AGENT_LOGS=("$LOG_DIR"/agent_*.log)
        shopt -u nullglob
        if [[ ${#AGENT_LOGS[@]} -gt 0 ]]; then
            echo "[run] Génération du rapport d'activité → $REPORT"
            python3 "$PARSER" "${AGENT_LOGS[@]}" -v low -o "$REPORT"
        fi
    fi

    exit 0
}
trap cleanup EXIT INT TERM

echo "[run] Démarrage du serveur sur le port $PORT (${WIDTH}x${HEIGHT}, team=$TEAM, slots=$CLIENTS, freq=$FREQ)"
echo "[run] Logs : $LOG_DIR"
"$SCRIPT_DIR/zappy_server" -p "$PORT" -x "$WIDTH" -y "$HEIGHT" \
    -n "$TEAM" -c "$CLIENTS" -f "$FREQ" \
    > "$LOG_DIR/server.log" 2>&1 &
SERVER_PID=$!
PIDS+=($SERVER_PID)

for i in $(seq 1 20); do
    sleep 0.2
    if 2>/dev/null bash -c "echo > /dev/tcp/localhost/$PORT"; then
        break
    fi
    if [[ $i -eq 20 ]]; then
        echo "[run] Le serveur n'a pas démarré à temps" >&2
        exit 1
    fi
done
echo "[run] Serveur prêt"

if [[ $GUI3D -eq 1 ]]; then
    echo "[run] Démarrage du GUI 3D"
    "$SCRIPT_DIR/zappy_gui3d" -p "$PORT" -h localhost \
        > "$LOG_DIR/gui.log" 2>&1 &
else
    echo "[run] Démarrage du GUI 2D"
    "$SCRIPT_DIR/zappy_gui" -p "$PORT" -h localhost \
        > "$LOG_DIR/gui.log" 2>&1 &
fi
GUI_PID=$!
PIDS+=($GUI_PID)

sleep 0.5

spawn_agent() {
    mkdir -p "$LOG_DIR"
    local tmplog
    tmplog=$(mktemp "$LOG_DIR/agent_tmp.XXXXXX")
    local solo_flag=""
    [[ $SOLO -eq 1 ]] && solo_flag="-s"
    "$SCRIPT_DIR/zappy_ai" -p "$PORT" -n "$TEAM" -h localhost $solo_flag \
        > "$tmplog" 2>&1 &
    local pid=$!
    mv "$tmplog" "$LOG_DIR/agent_$pid.log"
    PIDS+=($pid)
    SPAWN_TIME[$pid]=$(date +%s)
}

echo "[run] Lancement initial de $CLIENTS agents..."
for i in $(seq 1 "$CLIENTS"); do
    spawn_agent
    sleep 0.05
done

echo "[run] Tout est en place — Ctrl+C pour arrêter"

while true; do
    sleep 1

    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "[run] Erreur : Le zappy_server s'est arrêté !" >&2
        cleanup
    fi
    if ! kill -0 "$GUI_PID" 2>/dev/null; then
        echo "[run] Attention : Le zappy_gui s'est arrêté." >&2
    fi

    NOW=$(date +%s)
    ACTIVE_AGENTS=0
    PENDING_AGENTS=0
    STILL_ALIVE=()
    STILL_ALIVE+=("$SERVER_PID" "$GUI_PID")

    for pid in "${PIDS[@]}"; do
        if [[ "$pid" -eq "$SERVER_PID" || "$pid" -eq "$GUI_PID" ]]; then
            continue
        fi

        spawn_ts=${SPAWN_TIME[$pid]:-$NOW}
        age=$((NOW - spawn_ts))

        if kill -0 "$pid" 2>/dev/null; then
            STILL_ALIVE+=("$pid")
            if [[ $age -ge $HANDSHAKE_THRESHOLD ]]; then
                ((ACTIVE_AGENTS++))
                if [[ -z "${CONFIRMED[$pid]+x}" ]]; then
                    CONFIRMED[$pid]=1
                    echo "[run] Agent $pid connecté ($ACTIVE_AGENTS/$AGENTS)"
                fi
            else
                ((PENDING_AGENTS++))
            fi
        else
            unset "SPAWN_TIME[$pid]"
            if [[ -n "${CONFIRMED[$pid]+x}" ]]; then
                unset "CONFIRMED[$pid]"
                echo "[run] Agent $pid mort (${age}s)"
            else
                rm -f "$LOG_DIR/agent_$pid.log"
            fi
        fi
    done

    PIDS=("${STILL_ALIVE[@]}")

    if [[ $((ACTIVE_AGENTS + PENDING_AGENTS)) -lt $AGENTS ]]; then
        spawn_agent
    fi
done
