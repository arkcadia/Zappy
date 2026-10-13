#!/usr/bin/env python3
"""
Zappy log parser — translates raw agent logs into a human-readable chronology.

Works with both release and debug builds by extracting events from:
  - [Connection] traces (>> commands sent, << server responses)
  - [Broadcast] encode/parse lines
  - [Agent] role init, movement, turns

Usage:
    python3 tools/parse_zappy_log.py logs/agent_<pid>.log [logs/agent_<pid>.log ...]
    python3 tools/parse_zappy_log.py logs/agent_*.log -o parsed_zappy_activity.txt
"""

import re
import sys
import argparse
from dataclasses import dataclass, field
from typing import Optional


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

_ROLE_NAMES = {
    "0": "Worker", "1": "Bodyguard", "2": "Ritualist", "3": "Chaos",
}

_BCAST_CODES = {
    0: "annonce", 1: "heartbeat", 5: "query/arrivée",
    6: "balise", 7: "rôle", 8: "go", 10: "packed",
}


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

@dataclass
class Event:
    line_no: int
    agent_id: str
    category: str  # CONNECTION | ROLE | PHASE | MOVE | INCANTATION |
                   # FORK | PICKUP | DROP | BROADCAST | DEATH | COMMAND
    message: str
    raw: str = ""


@dataclass
class AgentState:
    pid: str
    role: str = "?"
    level: int = 1
    phase2: bool = False
    last_pos: Optional[tuple] = None
    move_count: int = 0
    fork_count: int = 0
    take_counts: dict = field(default_factory=dict)
    set_counts: dict = field(default_factory=dict)
    bcast_seen: set = field(default_factory=set)


# ---------------------------------------------------------------------------
# Regex patterns — work in release mode (Connection/Broadcast traces)
# ---------------------------------------------------------------------------

# [Connection] connecting to HOST:PORT
_RE_CONNECT = re.compile(r'^\[Connection\] connecting to (.+)$')
# [Agent X] role -> Y ...
_RE_ROLE_INIT = re.compile(r'^\[Agent (\d+)\] role -> (\w+)')
# [Agent X] starting run loop
_RE_RUN_LOOP = re.compile(r'^\[Agent (\d+)\] starting run loop$')
# [Agent X] role transition: A -> B
_RE_ROLE_TRANSITION = re.compile(r'^\[Agent (\d+)\] role transition: (\w+) -> (\w+)$')
# [Agent X] Queue -> Pioneer / Queue -> role N
_RE_QUEUE_TRANS = re.compile(r'^\[Agent (\d+)\] Queue -> (\w+)(?: (\d+))?$')
# [Agent X] moved forward -> pos=(X,Y)
_RE_MOVED = re.compile(r'^\[Agent (\d+)\] moved forward -> pos=\((\d+),(\d+)\)$')
# [Connection fd=N] << Current level: N
_RE_LEVEL_UP = re.compile(r'^\[Connection fd=\d+\] << Current level: (\d+)$')
# [Connection fd=N] << dead
_RE_DEAD = re.compile(r'^\[Connection fd=\d+\] << dead$')
# [Connection fd=N] >> COMMAND
_RE_CMD_SENT = re.compile(r'^\[Connection fd=\d+\] >> (.+)$')
# [Connection fd=N] << RESPONSE
_RE_SERVER_RESP = re.compile(r'^\[Connection fd=\d+\] << (.+)$')
# [Broadcast PID] encode: code=N seq=S -> 'TEXT'
_RE_BCAST_ENCODE = re.compile(r'^\[Broadcast (\d+)\] encode: code=(\d+) seq=(\d+) -> \'(.+)\'$')
# [Broadcast] parsed: code=N sender=PID seq=S payload='TEXT'
_RE_BCAST_PARSED = re.compile(r'^\[Broadcast\] parsed: code=(\d+) sender=(\d+) seq=(\d+) payload=\'(.*)\'$')

# Debug-mode only patterns (bonus info when available)
# [Agent X] Queue: ...
_RE_QUEUE_DBG = re.compile(r'^\[Agent (\d+)\] Queue: (.+)$')
# [Agent X] Pioneer: ...
_RE_PIONEER_DBG = re.compile(r'^\[Agent (\d+)\] Pioneer: (.+)$')
# [Agent X] Ritualist: ...
_RE_RITUALIST_DBG = re.compile(r'^\[Agent (\d+)\] Ritualist: (.+)$')
# [Agent X] Worker: ...
_RE_WORKER_DBG = re.compile(r'^\[Agent (\d+)\] Worker: (.+)$')
# [Agent X] Ritualist leader: ...
_RE_RITUALIST_LEADER = re.compile(r'^\[Agent (\d+)\] Ritualist leader: (.+)$')
# [Agent X] phase 2 rally: N agents, M lvl2+
_RE_RALLY = re.compile(r'^\[Agent (\d+)\] phase 2 rally: (\d+) agents, (\d+) lvl2\+$')
# [Agent X] Pioneer settle ...
_RE_SETTLE_DBG = re.compile(r'^\[Agent (\d+)\] \w+ settle.*: (.+)$')
# [Agent X] Leader_ID = PID
_RE_LEADER_ID = re.compile(r'^\[Agent (\d+)\] Leader_ID = (\d+)$')
# [Agent X] survival: ...
_RE_SURVIVAL = re.compile(r'^\[Agent (\d+)\] survival: (.+)$')
# [Agent X] calibrate: ...
_RE_CALIBRATE = re.compile(r'^\[Agent (\d+)\] calibrate: (.+)$')
# [Agent X] navigateTo: ...
_RE_NAVIGATE = re.compile(r'^\[Agent (\d+)\] navigateTo: (.+)$')


def _code_label(code: int) -> str:
    return _BCAST_CODES.get(code, f"code-{code}")


def _decode_packed_roles(text: str) -> list[tuple[str, str]]:
    """Parse packed broadcast payload for role assignments (code-7 segments)."""
    results = []
    for segment in text.split('|'):
        segment = segment.strip()
        if not segment.startswith('7 '):
            continue
        parts = segment.split()
        if len(parts) < 2:
            continue
        n = int(parts[1])
        idx = 2
        for _ in range(n):
            if idx + 1 >= len(parts):
                break
            pid = parts[idx]
            role = _ROLE_NAMES.get(parts[idx + 1], f"Role#{parts[idx + 1]}")
            results.append((pid, role))
            idx += 2
    return results


# ---------------------------------------------------------------------------
# Main parser
# ---------------------------------------------------------------------------

def parse_log(lines: list[str], agent_id: str) -> list[Event]:
    events: list[Event] = []
    state = AgentState(pid=agent_id)
    pending_cmd: Optional[str] = None
    incant_pending = False

    def emit(cat: str, msg: str, lineno: int, raw: str = ""):
        events.append(Event(lineno, agent_id, cat, msg, raw))

    for i, raw_line in enumerate(lines):
        lineno = i + 1
        line = raw_line.strip()
        if not line:
            continue

        # --- Connection ---
        m = _RE_CONNECT.match(line)
        if m:
            emit("CONNECTION", f"Connexion à {m.group(1)}", lineno, line)
            continue

        # --- Role init ---
        m = _RE_ROLE_INIT.match(line)
        if m:
            state.role = m.group(2)
            emit("ROLE", f"Rôle initial : {m.group(2)}", lineno, line)
            continue

        m = _RE_RUN_LOOP.match(line)
        if m:
            emit("CONNECTION", "Boucle principale démarrée", lineno, line)
            continue

        # --- Role transition (debug mode) ---
        m = _RE_ROLE_TRANSITION.match(line)
        if m:
            old, new = m.group(2), m.group(3)
            state.role = new
            state.phase2 = True
            emit("ROLE", f"{old} → {new}", lineno, line)
            continue

        m = _RE_QUEUE_TRANS.match(line)
        if m:
            dest = m.group(2)
            role_num = m.group(3)
            if dest == "Pioneer":
                state.role = "Pioneer"
                emit("ROLE", "Queue → Pioneer (Phase 1)", lineno, line)
            elif dest == "role" and role_num:
                rname = _ROLE_NAMES.get(role_num, f"Role#{role_num}")
                state.role = rname
                state.phase2 = True
                emit("ROLE", f"Queue → {rname} (Phase 2)", lineno, line)
            continue

        # --- Level up ---
        m = _RE_LEVEL_UP.match(line)
        if m:
            lvl = int(m.group(1))
            state.level = lvl
            emit("INCANTATION", f"Niveau {lvl}", lineno, line)
            incant_pending = False
            continue

        # --- Death ---
        m = _RE_DEAD.match(line)
        if m:
            emit("DEATH", "Mort", lineno, line)
            continue

        # --- Movement ---
        m = _RE_MOVED.match(line)
        if m:
            x, y = int(m.group(2)), int(m.group(3))
            state.last_pos = (x, y)
            state.move_count += 1
            emit("MOVE", f"→ ({x},{y})", lineno, line)
            continue

        # --- Commands sent ---
        m = _RE_CMD_SENT.match(line)
        if m:
            cmd = m.group(1)
            pending_cmd = cmd

            if cmd == "Fork":
                state.fork_count += 1
                emit("FORK", f"Fork #{state.fork_count}", lineno, line)
            elif cmd == "Incantation":
                incant_pending = True
                emit("INCANTATION", "Incantation lancée", lineno, line)
            elif cmd.startswith("Take "):
                res = cmd[5:]
                state.take_counts[res] = state.take_counts.get(res, 0) + 1
                # only emit every Nth to reduce noise, or always for non-food
                if res != "food" or state.take_counts[res] <= 3 or state.take_counts[res] % 5 == 0:
                    emit("PICKUP", f"Take {res} (total: {state.take_counts[res]})", lineno, line)
            elif cmd.startswith("Set "):
                res = cmd[4:]
                state.set_counts[res] = state.set_counts.get(res, 0) + 1
                if res != "food" or state.set_counts[res] <= 3 or state.set_counts[res] % 5 == 0:
                    emit("DROP", f"Set {res} (total: {state.set_counts[res]})", lineno, line)
            elif cmd.startswith("Broadcast "):
                pass  # handled by encode line
            # Ignore Inventory, Look, Forward, Left, Right (noise)
            continue

        # --- Server responses (ok/ko after important commands) ---
        m = _RE_SERVER_RESP.match(line)
        if m:
            resp = m.group(1)
            if resp == "ko" and incant_pending:
                emit("INCANTATION", "Incantation échouée (ko)", lineno, line)
                incant_pending = False
            elif resp == "Elevation underway":
                emit("INCANTATION", "Élévation en cours...", lineno, line)
            continue

        # --- Broadcast encode (outgoing) — also infer role ---
        m = _RE_BCAST_ENCODE.match(line)
        if m:
            sender, code, seq, text = m.group(1), int(m.group(2)), m.group(3), m.group(4)
            label = _code_label(code)

            # Infer role from what we broadcast
            if code == 0 and state.role == "Queue":
                state.role = "Pioneer"
            if code == 7 and state.role in ("Queue", "Pioneer"):
                state.role = "Leader"
            if code == 6 and state.fork_count > 0 and state.role in ("Queue", "Pioneer"):
                state.role = "Ritualist"

            if code == 10:
                # Packed broadcast — decode role assignments
                roles = _decode_packed_roles(text)
                segments = [s.strip() for s in text.split('|') if s.strip()]
                code_list = []
                for seg in segments:
                    try:
                        c = int(seg.split()[0])
                        code_list.append(str(c))
                    except (ValueError, IndexError):
                        pass
                detail = f"codes [{','.join(code_list)}]"
                if roles:
                    assigns = ", ".join(f"{p}→{r}" for p, r in roles)
                    detail += f" assigns: {assigns}"
                emit("BROADCAST", f">>> packed {detail}", lineno, line)
            elif code == 5:
                emit("BROADCAST", f">>> code-5 [{label}] (phase query)", lineno, line)
            elif code == 6:
                # Extract agent count from payload if present
                parts = text.split()
                payload = parts[3] if len(parts) > 3 else ""
                if payload:
                    emit("BROADCAST", f">>> code-6 [{label}] (total agents: {payload})", lineno, line)
                else:
                    emit("BROADCAST", f">>> code-6 [{label}]", lineno, line)
            elif code == 7:
                roles = []
                parts = text.split()
                if len(parts) > 3:
                    payload_parts = parts[3:]
                    try:
                        n = int(payload_parts[0])
                        idx = 1
                        for _ in range(n):
                            if idx + 1 < len(payload_parts):
                                pid = payload_parts[idx]
                                role = _ROLE_NAMES.get(payload_parts[idx + 1], f"R#{payload_parts[idx + 1]}")
                                roles.append(f"{pid}→{role}")
                                idx += 2
                    except (ValueError, IndexError):
                        pass
                if roles:
                    emit("BROADCAST", f">>> code-7 [{label}] assign: {', '.join(roles)}", lineno, line)
                else:
                    emit("BROADCAST", f">>> code-7 [{label}]", lineno, line)
            else:
                emit("BROADCAST", f">>> code-{code} [{label}]", lineno, line)
            continue

        # --- Broadcast parsed (incoming) ---
        m = _RE_BCAST_PARSED.match(line)
        if m:
            code, sender, seq, payload = int(m.group(1)), m.group(2), m.group(3), m.group(4)
            label = _code_label(code)

            # Deduplicate repetitive heartbeats/beacons: show first from each sender
            if code in (0, 1, 6, 9):
                key = (code, sender)
                if key in state.bcast_seen:
                    continue
                state.bcast_seen.add(key)

            if code == 7:
                # Role assignment — always show; also infer own role if targeted
                roles = []
                parts = payload.split()
                try:
                    n = int(parts[0]) if parts else 0
                    idx = 1
                    for _ in range(n):
                        if idx + 1 < len(parts):
                            pid = parts[idx]
                            rname = _ROLE_NAMES.get(parts[idx + 1], f"R#{parts[idx + 1]}")
                            roles.append(f"{pid}→{rname}")
                            if pid == agent_id:
                                state.role = rname
                                state.phase2 = True
                            idx += 2
                except (ValueError, IndexError):
                    pass
                detail = f" assign: {', '.join(roles)}" if roles else f" payload='{payload}'"
                emit("BROADCAST", f"<<< code-7 [{label}] from {sender}{detail}", lineno, line)
            elif code == 10:
                roles = _decode_packed_roles(payload)
                # Infer own role if we're targeted
                for pid, rname in roles:
                    if pid == agent_id:
                        state.role = rname
                        state.phase2 = True
                if roles:
                    assigns = ", ".join(f"{p}→{r}" for p, r in roles)
                    emit("BROADCAST", f"<<< packed from {sender} assigns: {assigns}", lineno, line)
                else:
                    # Check for code-8 (go signal)
                    has_go = any(s.strip().startswith('8') for s in payload.split('|'))
                    if has_go:
                        emit("BROADCAST", f"<<< packed from {sender}: code-8 go signal", lineno, line)
                    else:
                        emit("BROADCAST", f"<<< packed from {sender}: {payload[:60]}", lineno, line)
            elif code == 5:
                emit("BROADCAST", f"<<< code-5 [{label}] from {sender}", lineno, line)
            elif code == 6 and payload:
                emit("BROADCAST", f"<<< code-6 [{label}] from {sender} (total: {payload})", lineno, line)
            elif code == 8:
                emit("BROADCAST", f"<<< code-8 [{label}] from {sender}", lineno, line)
            else:
                emit("BROADCAST", f"<<< code-{code} [{label}] from {sender}", lineno, line)
            continue

        # --- Debug-mode bonus patterns ---
        m = _RE_RALLY.match(line)
        if m:
            pid, total, lvl2 = m.group(1), m.group(2), m.group(3)
            state.phase2 = True
            emit("PHASE", f"Phase 2 rally: {total} agents, {lvl2} lvl2+", lineno, line)
            continue

        m = _RE_LEADER_ID.match(line)
        if m:
            emit("PHASE", f"Leader confirmé: {m.group(2)}", lineno, line)
            continue

        m = _RE_QUEUE_DBG.match(line)
        if m:
            emit("PHASE", f"Queue: {m.group(2)}", lineno, line)
            continue

        m = _RE_SETTLE_DBG.match(line)
        if m:
            emit("PHASE", f"Settle: {m.group(2)}", lineno, line)
            continue

        m = _RE_RITUALIST_LEADER.match(line)
        if m:
            emit("PHASE", f"Ritualist leader: {m.group(2)}", lineno, line)
            continue

        m = _RE_RITUALIST_DBG.match(line)
        if m:
            msg = m.group(2)
            if "forking" in msg:
                emit("FORK", f"Ritualist: {msg}", lineno, line)
            elif "picked up food" in msg:
                emit("PICKUP", f"Ritualist: {msg}", lineno, line)
            else:
                emit("PHASE", f"Ritualist: {msg}", lineno, line)
            continue

        m = _RE_WORKER_DBG.match(line)
        if m:
            msg = m.group(2)
            if "delivered" in msg:
                emit("DROP", f"Worker: {msg}", lineno, line)
            elif "DELIVER" in msg:
                emit("PHASE", f"Worker: {msg}", lineno, line)
            else:
                emit("PHASE", f"Worker: {msg}", lineno, line)
            continue

        m = _RE_PIONEER_DBG.match(line)
        if m:
            msg = m.group(2)
            if "recalled" in msg:
                emit("ROLE", f"Pioneer: {msg}", lineno, line)
            elif "ANNOUNCE" in msg or "COLLECT" in msg or "SETTLE" in msg or "FORK" in msg:
                emit("PHASE", f"Pioneer: {msg}", lineno, line)
            elif "forking" in msg:
                emit("FORK", f"Pioneer: {msg}", lineno, line)
            else:
                emit("PHASE", f"Pioneer: {msg}", lineno, line)
            continue

        m = _RE_CALIBRATE.match(line)
        if m:
            emit("PHASE", f"Calibrate: {m.group(2)}", lineno, line)
            continue

        m = _RE_NAVIGATE.match(line)
        if m:
            emit("MOVE", f"NavigateTo: {m.group(2)}", lineno, line)
            continue

        m = _RE_SURVIVAL.match(line)
        if m:
            emit("PHASE", f"Survie: {m.group(2)}", lineno, line)
            continue

    # Append summary for this agent
    summary_parts = [f"rôle={state.role}", f"lvl={state.level}"]
    if state.last_pos:
        summary_parts.append(f"pos={state.last_pos}")
    summary_parts.append(f"moves={state.move_count}")
    if state.fork_count:
        summary_parts.append(f"forks={state.fork_count}")
    if state.take_counts:
        takes = ", ".join(f"{k}:{v}" for k, v in sorted(state.take_counts.items()))
        summary_parts.append(f"takes=[{takes}]")
    if state.set_counts:
        drops = ", ".join(f"{k}:{v}" for k, v in sorted(state.set_counts.items()))
        summary_parts.append(f"drops=[{drops}]")
    emit("SUMMARY", " | ".join(summary_parts), len(lines), "")

    return events


# ---------------------------------------------------------------------------
# Renderer
# ---------------------------------------------------------------------------

_CATEGORY_LABELS = {
    "CONNECTION":  "CONNEXION  ",
    "ROLE":        "RÔLE       ",
    "PHASE":       "PHASE      ",
    "MOVE":        "DÉPLACEMENT",
    "INCANTATION": "INCANTATION",
    "FORK":        "FORK       ",
    "PICKUP":      "RAMASSAGE  ",
    "DROP":        "DÉPÔT      ",
    "BROADCAST":   "BROADCAST  ",
    "DEATH":       "MORT       ",
    "COMMAND":     "COMMANDE   ",
    "SUMMARY":     "RÉSUMÉ     ",
}

_CATEGORY_FILTER = {
    "low":    {"CONNECTION", "ROLE", "INCANTATION", "PHASE", "FORK", "DEATH", "SUMMARY"},
    "medium": {"CONNECTION", "ROLE", "INCANTATION", "PHASE", "FORK", "DEATH", "SUMMARY",
               "BROADCAST", "PICKUP", "DROP"},
    "high":   None,  # all
}


def render(events, agent_ids, verbosity="medium", output_file=None):
    allowed = _CATEGORY_FILTER.get(verbosity)
    lines_out = []

    lines_out.append("=" * 72)
    lines_out.append("   CHRONOLOGIE DE LA SIMULATION ZAPPY")
    lines_out.append(f"   Agents analysés : {', '.join(agent_ids)}")
    lines_out.append(f"   Verbosité : {verbosity}")
    lines_out.append("=" * 72)
    lines_out.append("")

    current_agent = None
    event_count = 0

    for ev in events:
        if allowed and ev.category not in allowed:
            continue
        event_count += 1

        if ev.agent_id != current_agent:
            current_agent = ev.agent_id
            lines_out.append(f"\n{'─' * 72}")
            lines_out.append(f"  AGENT {ev.agent_id}")
            lines_out.append(f"{'─' * 72}")

        cat_label = _CATEGORY_LABELS.get(ev.category, ev.category.ljust(11))
        lines_out.append(f"  [Ligne {ev.line_no:>5}] {cat_label} | {ev.message}")

    lines_out.append("")
    lines_out.append("=" * 72)
    lines_out.append(f"   FIN — {event_count} événements affichés")
    lines_out.append("=" * 72)

    output = "\n".join(lines_out)

    if output_file:
        with open(output_file, "w", encoding="utf-8") as f:
            f.write(output)
        print(f"Rapport écrit dans : {output_file}")
    else:
        print(output)


# ---------------------------------------------------------------------------
# Multi-file merge
# ---------------------------------------------------------------------------

def merge_and_parse(file_paths):
    import os
    all_events = []
    agent_ids = []

    for path in file_paths:
        basename = os.path.basename(path)
        pid_match = re.search(r'(\d+)', basename)
        agent_id = pid_match.group(1) if pid_match else basename
        agent_ids.append(agent_id)

        try:
            with open(path, encoding="utf-8", errors="replace") as f:
                lines = f.readlines()
        except FileNotFoundError:
            print(f"[ERREUR] Fichier non trouvé : {path}", file=sys.stderr)
            continue

        events = parse_log(lines, agent_id)
        all_events.extend(events)

    return all_events, agent_ids


def main():
    parser = argparse.ArgumentParser(
        description="Parse et traduit les logs d'agents Zappy en rapport lisible."
    )
    parser.add_argument("log_files", nargs="+", metavar="LOG",
                        help="Fichiers de log (ex: logs/agent_*.log)")
    parser.add_argument("-o", "--output", metavar="FILE",
                        help="Écrire le rapport dans ce fichier")
    parser.add_argument("-v", "--verbosity", choices=["low", "medium", "high"],
                        default="medium",
                        help="low=rôle/phase/fork ; medium=+broadcasts/pickup ; high=tout")
    args = parser.parse_args()

    events, agent_ids = merge_and_parse(args.log_files)
    render(events, agent_ids, verbosity=args.verbosity, output_file=args.output)


if __name__ == "__main__":
    main()
