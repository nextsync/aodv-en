#!/usr/bin/env python3
"""
live_monitor.py - real-time AODV-EN mesh dashboard.

Le multiplas portas seriais em paralelo, parseia logs do ESP-IDF monitor,
mantem estado da malha em memoria e empurra eventos via WebSocket para um
dashboard HTML/JS animado em http://localhost:8765/.

Uso:
  python3 firmware/tools/live_monitor.py --demo
  python3 firmware/tools/live_monitor.py \
      --port /dev/ttyUSB0:NODE_A \
      --port /dev/ttyUSB1:NODE_B \
      --port /dev/ttyUSB2:NODE_C

Dependencias:
  pip install aiohttp pyserial
"""

import argparse
import asyncio
import json
import queue
import re
import shutil
import subprocess
import sys
import threading
import time
from pathlib import Path

try:
    import serial as pyserial  # pyserial
except ImportError:
    pyserial = None

from aiohttp import WSMsgType, web


# ============================================================================
# esptool helper (pre-leitura de MAC para resolver alias -> mac no startup)
# ============================================================================

ESPTOOL_CANDIDATES = [
    "/Users/huaksonlima/.espressif/python_env/idf6.0_py3.14_env/bin/esptool",
    "esptool",
    "esptool.py",
]


def find_esptool():
    """Retorna o caminho de um binario esptool valido, ou None."""
    for cand in ESPTOOL_CANDIDATES:
        if Path(cand).exists():
            return cand
    for cand in ("esptool", "esptool.py"):
        which = shutil.which(cand)
        if which:
            return which
    return None


_MAC_PATTERN = re.compile(r"^MAC:\s+([0-9a-fA-F:]{17})\s*$")


def read_mac_via_esptool(esptool_path, dev_path, baud=115200, timeout=20):
    """
    Le o MAC de uma ESP32 sem flashar.
    Retorna string AA:BB:CC:DD:EE:FF (uppercase) ou None em caso de falha.
    """
    if esptool_path is None or pyserial is None:
        return None
    try:
        result = subprocess.run(
            [
                esptool_path,
                "--port", dev_path,
                "--baud", str(baud),
                "--before", "default_reset",
                "--after", "hard_reset",
                "read_mac",
            ],
            capture_output=True,
            text=True,
            timeout=timeout,
        )
    except (FileNotFoundError, subprocess.TimeoutExpired, OSError) as e:
        print(f"[live_monitor] esptool falhou em {dev_path}: {e}", file=sys.stderr)
        return None
    if result.returncode != 0:
        print(
            f"[live_monitor] esptool retornou {result.returncode} em {dev_path}",
            file=sys.stderr,
        )
        return None
    for line in result.stdout.splitlines():
        m = _MAC_PATTERN.match(line.strip())
        if m:
            return m.group(1).upper()
    return None


# ============================================================================
# Regex patterns (mirror firmware/tools/extract_monitor_metrics.py)
# ============================================================================

ANSI_RE = re.compile(r"\x1B\[[0-?]*[ -/]*[@-~]")
LOG_RE = re.compile(r"^\s*([IWE]) \((\d+)\) ([^:]+): (.*)$")
NODE_RE = re.compile(
    r"node=([A-Z0-9_]+)\s+self_mac=([0-9A-F:]{17})\s+channel=(\d+)\s+network_id=0x([0-9A-Fa-f]+)"
)
SNAPSHOT_RE = re.compile(
    r"routes=(\d+)\s+neighbors=(\d+)\s+tx=(\d+)\s+rx=(\d+)\s+delivered=(\d+)"
)
ROUTE_RE = re.compile(
    r"route\[(\d+)\]\s+dest=([0-9A-F:]{17})\s+via=([0-9A-F:]{17})\s+hops=(\d+)\s+metric=(\d+)\s+state=(\d+)\s+expires=(\d+)"
)
ACK_RE = re.compile(r"ACK received from ([0-9A-F:]{17}) for seq=(\d+)")
DELIVER_RE = re.compile(r"DATA deliver from ([0-9A-F:]{17}):")
SEND_FAIL_RE = re.compile(r"ESP-NOW send fail to ([0-9A-F:]{17})")
DATA_STATUS_RE = re.compile(r"DATA send status=(-?\d+)")
INVALIDATED_RE = re.compile(r"invalidated (\d+) route\(s\) via ([0-9A-F:]{17})")
HELLO_STATUS_RE = re.compile(r"HELLO send status=(-?\d+)")


def strip_ansi(line: str) -> str:
    return ANSI_RE.sub("", line).replace("\r", "").rstrip("\n")


# ============================================================================
# Mesh state
# ============================================================================


class MeshState:
    """Estado agregado da malha. Plain dicts para serializar facil em JSON."""

    EVENT_RING_SIZE = 200

    def __init__(self):
        self.nodes = {}  # mac -> dict
        self.alias_to_mac = {}  # alias -> mac
        self.events = []  # ring buffer de eventos recentes

    def apply(self, event):
        t = event.get("type")
        if t == "node_seen":
            mac = event["mac"]
            n = self._ensure_node(mac)
            if event.get("alias"):
                n["alias"] = event["alias"]
                self.alias_to_mac[event["alias"]] = mac
            if event.get("channel") is not None:
                n["channel"] = event["channel"]
            if event.get("network_id"):
                n["network_id"] = event["network_id"]
            n["last_seen"] = time.time()
        elif t == "stats":
            mac = event["mac"]
            n = self._ensure_node(mac)
            n["stats"].update(event["stats"])
            n["last_seen"] = time.time()
        elif t == "routes":
            mac = event["mac"]
            n = self._ensure_node(mac)
            n["routes"] = event["routes"]
            n["last_seen"] = time.time()
            # tambem garante que destinos referenciados existam como nos "ghost"
            for r in event["routes"]:
                if r["dest"] not in self.nodes:
                    self._ensure_node(r["dest"])
                if r["via"] not in self.nodes:
                    self._ensure_node(r["via"])
        elif t in (
            "ack",
            "deliver",
            "queued_discovery",
            "queued_route",
            "send_fail",
            "data_status",
            "invalidated",
        ):
            self.events.append(event)
            if len(self.events) > self.EVENT_RING_SIZE:
                self.events = self.events[-self.EVENT_RING_SIZE:]

    def snapshot(self):
        now = time.time()
        for n in self.nodes.values():
            n["online"] = (now - n.get("last_seen", 0.0)) < 30.0
        return {
            "type": "snapshot",
            "nodes": list(self.nodes.values()),
            "events": list(self.events),
            "ts": now,
        }

    def _ensure_node(self, mac):
        if mac not in self.nodes:
            self.nodes[mac] = {
                "mac": mac,
                "alias": None,
                "channel": None,
                "network_id": None,
                "routes": [],
                "stats": {
                    "routes_count": 0,
                    "neighbors_count": 0,
                    "tx": 0,
                    "rx": 0,
                    "delivered": 0,
                },
                "last_seen": 0.0,
                "online": False,
            }
        return self.nodes[mac]


# ============================================================================
# Hub: state + WebSocket subscribers
# ============================================================================


class Hub:
    def __init__(self, loop):
        self.loop = loop
        self.subscribers = set()
        self.mesh = MeshState()

    def subscribe(self, q):
        self.subscribers.add(q)

    def unsubscribe(self, q):
        self.subscribers.discard(q)

    def broadcast(self, event):
        """Aplica no estado e empurra para todos os subscribers."""
        self.mesh.apply(event)
        dead = []
        for q in self.subscribers:
            try:
                q.put_nowait(event)
            except asyncio.QueueFull:
                dead.append(q)
        for q in dead:
            self.subscribers.discard(q)

    def broadcast_threadsafe(self, event):
        """Pode ser chamado de qualquer thread."""
        self.loop.call_soon_threadsafe(self.broadcast, event)


# ============================================================================
# Parser
# ============================================================================


class LineParser:
    """Mantem o estado de coleta de rotas por alias e despacha eventos."""

    # ANSI colors para debug print
    _C_RESET = "\033[0m"
    _C_GRAY = "\033[90m"
    _C_CYAN = "\033[36m"
    _C_GREEN = "\033[32m"
    _C_YELLOW = "\033[33m"
    _C_RED = "\033[31m"
    _C_MAGENTA = "\033[35m"

    _STATE_NAME = {0: "INVALID", 1: "REVERSE", 2: "VALID"}

    def __init__(self, hub, alias_hint, verbose=0):
        self.hub = hub
        self.alias_hint = alias_hint  # passado via --port DEV:ALIAS
        self.pending_routes = []
        self.expected_routes = 0
        self.collecting = False
        self.verbose = verbose

    def _emit(self, event):
        """Broadcasta o evento e, se verbose, imprime resumo no terminal."""
        if self.verbose >= 1:
            self._debug_print(event)
        self.hub.broadcast(event)

    def _short(self, mac):
        if not mac:
            return "?"
        return mac[-8:].upper() if len(mac) >= 8 else mac

    def _debug_print(self, event):
        now_struct = time.localtime()
        ms = int((time.time() % 1) * 1000)
        timestamp = time.strftime("%H:%M:%S", now_struct)
        alias = (self.alias_hint or "?")[:8]
        prefix = f"{self._C_GRAY}[{timestamp}.{ms:03d}] {alias:<8s}{self._C_RESET}"

        t = event.get("type", "?")
        if t == "node_seen":
            color = self._C_MAGENTA
            body = (
                f"alias={event.get('alias')} "
                f"mac={self._short(event.get('mac'))} "
                f"channel={event.get('channel')} "
                f"network_id={event.get('network_id')}"
            )
        elif t == "stats":
            color = self._C_GRAY
            s = event.get("stats", {})
            body = (
                f"routes={s.get('routes_count')} "
                f"neighbors={s.get('neighbors_count')} "
                f"tx={s.get('tx')} rx={s.get('rx')} "
                f"delivered={s.get('delivered')}"
            )
        elif t == "routes":
            color = self._C_GRAY
            routes = event.get("routes", [])
            body = f"{len(routes)} rotas"
            print(f"{prefix} {color}{t:<11s}{self._C_RESET} {body}", flush=True)
            for r in routes:
                state_name = self._STATE_NAME.get(r.get("state"), str(r.get("state")))
                print(
                    f"{prefix}             "
                    f"dest={self._short(r.get('dest'))} "
                    f"via={self._short(r.get('via'))} "
                    f"hops={r.get('hops')} "
                    f"metric={r.get('metric')} "
                    f"state={state_name} "
                    f"expires={r.get('expires')}",
                    flush=True,
                )
            return
        elif t == "ack":
            color = self._C_GREEN
            body = f"seq={event.get('seq')} from={self._short(event.get('from_mac'))}"
        elif t == "deliver":
            color = self._C_CYAN
            body = f"from={self._short(event.get('from_mac'))}"
        elif t == "send_fail":
            color = self._C_RED
            body = f"to={self._short(event.get('to_mac'))}"
        elif t == "data_status":
            color = self._C_RED
            body = f"status={event.get('status')}"
        elif t == "invalidated":
            color = self._C_RED
            body = f"via={self._short(event.get('via'))} count={event.get('count')}"
        elif t == "queued_route":
            color = self._C_YELLOW
            body = "to_route"
        elif t == "queued_discovery":
            color = self._C_YELLOW
            body = "while_route_discovery"
        else:
            color = self._C_GRAY
            body = json.dumps(event, default=str)[:120]

        print(f"{prefix} {color}{t:<11s}{self._C_RESET} {body}", flush=True)

    def feed(self, raw_line):
        line = strip_ansi(raw_line)
        if not line:
            return
        m = LOG_RE.match(line)
        if not m:
            return
        _level, ts_s, _tag, message = m.groups()
        try:
            ts = int(ts_s)
        except ValueError:
            ts = 0

        # node identity
        node_m = NODE_RE.search(message)
        if node_m:
            alias_log, self_mac, chan_s, network_hex = node_m.groups()
            self._flush_pending_routes()
            self._emit(
                {
                    "type": "node_seen",
                    "mac": self_mac,
                    "alias": alias_log,
                    "channel": int(chan_s),
                    "network_id": f"0x{network_hex.upper()}",
                    "ts": ts,
                }
            )
            return

        # snapshot: routes=X neighbors=Y tx=... rx=... delivered=...
        snap_m = SNAPSHOT_RE.search(message)
        if snap_m:
            routes_n, neighbors_n, tx, rx, delivered = map(int, snap_m.groups())
            self._flush_pending_routes()
            mac = self._mac_for_alias()
            if mac is None:
                return
            self._emit(
                {
                    "type": "stats",
                    "mac": mac,
                    "stats": {
                        "routes_count": routes_n,
                        "neighbors_count": neighbors_n,
                        "tx": tx,
                        "rx": rx,
                        "delivered": delivered,
                    },
                    "ts": ts,
                }
            )
            self.expected_routes = routes_n
            self.pending_routes = []
            self.collecting = True
            if routes_n == 0:
                self._flush_pending_routes()
            return

        # individual route entry
        route_m = ROUTE_RE.search(message)
        if route_m and self.collecting:
            _idx, dest, via, hops, metric, state, expires = route_m.groups()
            self.pending_routes.append(
                {
                    "dest": dest,
                    "via": via,
                    "hops": int(hops),
                    "metric": int(metric),
                    "state": int(state),
                    "expires": int(expires),
                }
            )
            if len(self.pending_routes) >= self.expected_routes:
                self._flush_pending_routes()
            return

        # at this point any non-route line ends a routes batch
        if self.collecting and self.pending_routes:
            self._flush_pending_routes()

        # ack received
        ack_m = ACK_RE.search(message)
        if ack_m:
            from_mac, seq = ack_m.groups()
            mac = self._mac_for_alias()
            if mac:
                self._emit(
                    {
                        "type": "ack",
                        "mac": mac,
                        "from_mac": from_mac,
                        "seq": int(seq),
                        "ts": ts,
                    }
                )
            return

        # data deliver
        deliver_m = DELIVER_RE.search(message)
        if deliver_m:
            from_mac = deliver_m.group(1)
            mac = self._mac_for_alias()
            if mac:
                self._emit(
                    {
                        "type": "deliver",
                        "mac": mac,
                        "from_mac": from_mac,
                        "ts": ts,
                    }
                )
            return

        # send fail
        fail_m = SEND_FAIL_RE.search(message)
        if fail_m:
            to_mac = fail_m.group(1)
            mac = self._mac_for_alias()
            if mac:
                self._emit(
                    {
                        "type": "send_fail",
                        "mac": mac,
                        "to_mac": to_mac,
                        "ts": ts,
                    }
                )
            return

        # data status (e.g. -2 = queue full)
        status_m = DATA_STATUS_RE.search(message)
        if status_m:
            status = int(status_m.group(1))
            mac = self._mac_for_alias()
            if mac:
                self._emit(
                    {
                        "type": "data_status",
                        "mac": mac,
                        "status": status,
                        "ts": ts,
                    }
                )
            return

        # invalidated routes after link fail
        inv_m = INVALIDATED_RE.search(message)
        if inv_m:
            count, via_mac = inv_m.groups()
            mac = self._mac_for_alias()
            if mac:
                self._emit(
                    {
                        "type": "invalidated",
                        "mac": mac,
                        "via": via_mac,
                        "count": int(count),
                        "ts": ts,
                    }
                )
            return

        if "DATA queued while route discovery is in progress" in message:
            mac = self._mac_for_alias()
            if mac:
                self._emit(
                    {"type": "queued_discovery", "mac": mac, "ts": ts}
                )
            return

        if "DATA queued to route" in message:
            mac = self._mac_for_alias()
            if mac:
                self._emit(
                    {"type": "queued_route", "mac": mac, "ts": ts}
                )
            return

    def _mac_for_alias(self):
        if self.alias_hint is None:
            return None
        return self.hub.mesh.alias_to_mac.get(self.alias_hint)

    def _flush_pending_routes(self):
        if not self.collecting:
            return
        mac = self._mac_for_alias()
        self.collecting = False
        routes = self.pending_routes
        self.pending_routes = []
        self.expected_routes = 0
        if mac is None:
            return
        self._emit({"type": "routes", "mac": mac, "routes": routes})


# ============================================================================
# Serial pump: thread-per-port, blocking readline, push to queue
# ============================================================================


class SerialPump:
    def __init__(self, dev_path, baud=115200, verbose=0, alias=None):
        self.dev_path = dev_path
        self.baud = baud
        self.verbose = verbose
        self.alias = alias or dev_path[-4:]
        self.q = queue.Queue()
        self._stop = threading.Event()
        self.thread = None

    def start(self):
        self.thread = threading.Thread(target=self._run, daemon=True)
        self.thread.start()

    def stop(self):
        self._stop.set()

    def _run(self):
        if pyserial is None:
            print(
                f"[live_monitor] pyserial nao instalado; ignorando {self.dev_path}",
                file=sys.stderr,
            )
            return
        was_unavailable = False
        backoff_s = 2.0
        backoff_max_s = 30.0
        while not self._stop.is_set():
            try:
                ser = pyserial.Serial(self.dev_path, self.baud, timeout=1)
            except Exception as e:
                if not was_unavailable:
                    # log only on transition: presente -> indisponivel
                    print(
                        f"[live_monitor] {self.dev_path} indisponivel ({e}); "
                        f"vou tentar de novo silenciosamente",
                        file=sys.stderr,
                    )
                    was_unavailable = True
                time.sleep(backoff_s)
                # backoff exponencial ate o teto, evita poluir saida
                backoff_s = min(backoff_s * 1.5, backoff_max_s)
                continue
            if was_unavailable:
                print(f"[live_monitor] {self.dev_path} voltou a estar disponivel")
                was_unavailable = False
            backoff_s = 2.0
            print(f"[live_monitor] aberto {self.dev_path} @ {self.baud}")
            try:
                while not self._stop.is_set():
                    line = ser.readline()
                    if not line:
                        continue
                    decoded = line.decode("utf-8", errors="replace")
                    self.q.put(decoded)
                    if self.verbose >= 2:
                        clean = decoded.rstrip("\r\n")
                        if clean:
                            print(
                                f"\033[90m[serial {self.alias:<8s}]\033[0m {clean}",
                                flush=True,
                            )
            except Exception as e:
                print(
                    f"[live_monitor] erro lendo {self.dev_path}: {e}; reabrindo",
                    file=sys.stderr,
                )
            finally:
                try:
                    ser.close()
                except Exception:
                    pass

    def drain(self, max_n=200):
        out = []
        for _ in range(max_n):
            try:
                out.append(self.q.get_nowait())
            except queue.Empty:
                break
        return out


async def serial_loop(pump, parser):
    while True:
        for line in pump.drain():
            parser.feed(line)
        await asyncio.sleep(0.05)


# ============================================================================
# Demo mode: emite eventos sinteticos de uma malha A-B-C-D
# ============================================================================


async def demo_loop(hub):
    nodes = [
        ("28:05:A5:33:D6:1C", "NODE_A"),
        ("28:05:A5:33:EB:80", "NODE_B"),
        ("28:05:A5:34:99:34", "NODE_C"),
        ("28:05:A5:34:AA:AA", "NODE_D"),
    ]
    A, B, C, D = (m for m, _ in nodes)

    # introduce nodes one by one
    for mac, alias in nodes:
        hub.broadcast(
            {
                "type": "node_seen",
                "mac": mac,
                "alias": alias,
                "channel": 6,
                "network_id": "0xA0DE0001",
                "ts": int(time.time() * 1000) & 0xFFFFFFFF,
            }
        )
        await asyncio.sleep(0.6)

    # initial empty stats
    for mac, _ in nodes:
        hub.broadcast(
            {
                "type": "stats",
                "mac": mac,
                "stats": {
                    "routes_count": 0,
                    "neighbors_count": 0,
                    "tx": 0,
                    "rx": 0,
                    "delivered": 0,
                },
            }
        )
    await asyncio.sleep(1)

    seq = 0
    cycle = 0
    while True:
        cycle += 1
        # full-mesh routes for chain A-B-C-D
        # NODE_A: route to D via B (3 hops), to C via B (2 hops), to B direct
        hub.broadcast(
            {
                "type": "routes",
                "mac": A,
                "routes": [
                    {"dest": B, "via": B, "hops": 1, "metric": 1, "state": 2, "expires": 30000 + cycle * 1000},
                    {"dest": C, "via": B, "hops": 2, "metric": 2, "state": 2, "expires": 30000 + cycle * 1000},
                    {"dest": D, "via": B, "hops": 3, "metric": 3, "state": 2, "expires": 30000 + cycle * 1000},
                ],
            }
        )
        # NODE_B: routes to A direct, C direct, D via C
        hub.broadcast(
            {
                "type": "routes",
                "mac": B,
                "routes": [
                    {"dest": A, "via": A, "hops": 1, "metric": 1, "state": 2, "expires": 30000 + cycle * 1000},
                    {"dest": C, "via": C, "hops": 1, "metric": 1, "state": 2, "expires": 30000 + cycle * 1000},
                    {"dest": D, "via": C, "hops": 2, "metric": 2, "state": 2, "expires": 30000 + cycle * 1000},
                ],
            }
        )
        # NODE_C: routes to A via B, B direct, D direct
        hub.broadcast(
            {
                "type": "routes",
                "mac": C,
                "routes": [
                    {"dest": A, "via": B, "hops": 2, "metric": 2, "state": 2, "expires": 30000 + cycle * 1000},
                    {"dest": B, "via": B, "hops": 1, "metric": 1, "state": 2, "expires": 30000 + cycle * 1000},
                    {"dest": D, "via": D, "hops": 1, "metric": 1, "state": 2, "expires": 30000 + cycle * 1000},
                ],
            }
        )
        # NODE_D: routes to A via C, B via C, C direct
        hub.broadcast(
            {
                "type": "routes",
                "mac": D,
                "routes": [
                    {"dest": A, "via": C, "hops": 3, "metric": 3, "state": 2, "expires": 30000 + cycle * 1000},
                    {"dest": B, "via": C, "hops": 2, "metric": 2, "state": 2, "expires": 30000 + cycle * 1000},
                    {"dest": C, "via": C, "hops": 1, "metric": 1, "state": 2, "expires": 30000 + cycle * 1000},
                ],
            }
        )
        # stats with growing tx/rx
        hub.broadcast(
            {"type": "stats", "mac": A, "stats": {"routes_count": 3, "neighbors_count": 1, "tx": 90 + cycle * 4, "rx": 12 + cycle * 2, "delivered": 0}}
        )
        hub.broadcast(
            {"type": "stats", "mac": B, "stats": {"routes_count": 3, "neighbors_count": 2, "tx": 120 + cycle * 6, "rx": 95 + cycle * 5, "delivered": 0}}
        )
        hub.broadcast(
            {"type": "stats", "mac": C, "stats": {"routes_count": 3, "neighbors_count": 2, "tx": 110 + cycle * 6, "rx": 100 + cycle * 5, "delivered": 0}}
        )
        hub.broadcast(
            {"type": "stats", "mac": D, "stats": {"routes_count": 3, "neighbors_count": 1, "tx": 40 + cycle * 2, "rx": 30 + cycle * 2, "delivered": cycle}}
        )

        await asyncio.sleep(2.0)

        # emit a DATA flow A -> D
        seq += 1
        hub.broadcast({"type": "queued_route", "mac": A, "ts": cycle * 1000})
        await asyncio.sleep(0.6)
        hub.broadcast({"type": "deliver", "mac": D, "from_mac": A, "ts": cycle * 1000 + 50})
        await asyncio.sleep(0.4)
        hub.broadcast({"type": "ack", "mac": A, "from_mac": D, "seq": seq, "ts": cycle * 1000 + 100})

        await asyncio.sleep(1.5)

        # every 4 cycles, simulate a link failure B<->C and recovery
        if cycle % 4 == 0:
            hub.broadcast({"type": "send_fail", "mac": B, "to_mac": C, "ts": cycle * 1000 + 200})
            await asyncio.sleep(0.3)
            hub.broadcast(
                {"type": "invalidated", "mac": B, "via": C, "count": 2, "ts": cycle * 1000 + 250}
            )
            # B drops route to D
            hub.broadcast(
                {
                    "type": "routes",
                    "mac": B,
                    "routes": [
                        {"dest": A, "via": A, "hops": 1, "metric": 1, "state": 2, "expires": 30000},
                    ],
                }
            )
            hub.broadcast({"type": "data_status", "mac": A, "status": -2, "ts": cycle * 1000 + 300})
            await asyncio.sleep(2.0)
            # recover
            hub.broadcast(
                {
                    "type": "routes",
                    "mac": B,
                    "routes": [
                        {"dest": A, "via": A, "hops": 1, "metric": 1, "state": 2, "expires": 30000},
                        {"dest": C, "via": C, "hops": 1, "metric": 1, "state": 2, "expires": 30000},
                        {"dest": D, "via": C, "hops": 2, "metric": 2, "state": 2, "expires": 30000},
                    ],
                }
            )
            await asyncio.sleep(0.6)


# ============================================================================
# WebSocket + HTTP server
# ============================================================================


WEB_DIR = Path(__file__).parent / "live_monitor_web"


async def index_handler(request):
    return web.FileResponse(WEB_DIR / "index.html")


async def websocket_handler(request):
    ws = web.WebSocketResponse(heartbeat=20)
    await ws.prepare(request)
    hub = request.app["hub"]
    q = asyncio.Queue(maxsize=400)
    hub.subscribe(q)

    # snapshot inicial
    try:
        await ws.send_str(json.dumps(hub.mesh.snapshot()))
    except Exception:
        hub.unsubscribe(q)
        return ws

    async def reader():
        async for msg in ws:
            if msg.type == WSMsgType.CLOSE:
                break

    async def sender():
        while not ws.closed:
            try:
                event = await asyncio.wait_for(q.get(), timeout=10.0)
            except asyncio.TimeoutError:
                continue
            try:
                await ws.send_str(json.dumps(event))
            except Exception:
                break

    try:
        await asyncio.gather(reader(), sender())
    finally:
        hub.unsubscribe(q)
        if not ws.closed:
            await ws.close()
    return ws


def make_app(hub):
    app = web.Application()
    app["hub"] = hub
    app.router.add_get("/", index_handler)
    app.router.add_get("/ws", websocket_handler)
    app.router.add_static("/static/", WEB_DIR, show_index=False)
    return app


# ============================================================================
# Main
# ============================================================================


async def amain(args):
    loop = asyncio.get_running_loop()
    hub = Hub(loop)
    app = make_app(hub)

    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, args.bind, args.http_port)
    await site.start()
    print(
        f"[live_monitor] dashboard em http://{args.bind}:{args.http_port}/  "
        "(Ctrl-C para sair)"
    )

    background = []
    if args.demo:
        print("[live_monitor] modo --demo ativo")
        background.append(asyncio.create_task(demo_loop(hub)))

    # Pre-leitura de MAC via esptool para resolver alias->mac no startup.
    # Sem isso, dependiamos da linha "node=NODE_X self_mac=..." emitida no boot,
    # que pode ser perdida durante a reabertura da porta apos um reset.
    esptool_path = None if args.skip_mac_lookup else find_esptool()
    if esptool_path:
        print(f"[live_monitor] esptool encontrado em {esptool_path}")
    elif not args.skip_mac_lookup:
        print(
            "[live_monitor] WARN: esptool nao encontrado; "
            "sera necessario reset manual de cada ESP para associar alias->mac",
            file=sys.stderr,
        )

    pumps = []
    for spec in args.port:
        if ":" not in spec:
            print(f"[live_monitor] --port invalido: {spec} (use DEV:ALIAS)", file=sys.stderr)
            continue
        dev, alias = spec.split(":", 1)

        if esptool_path:
            print(f"[live_monitor] lendo MAC de {dev} via esptool ...")
            mac = read_mac_via_esptool(esptool_path, dev, args.baud)
            if mac:
                print(f"[live_monitor] {dev} = {mac} ({alias})")
                hub.broadcast({
                    "type": "node_seen",
                    "mac": mac,
                    "alias": alias,
                    "channel": None,
                    "network_id": None,
                })
            else:
                print(
                    f"[live_monitor] WARN: nao consegui ler MAC de {dev}; "
                    f"alias {alias} so sera mapeado quando o ESP emitir node= line",
                    file=sys.stderr,
                )

        pump = SerialPump(dev, args.baud, verbose=args.verbose, alias=alias)
        pump.start()
        pumps.append(pump)
        parser_ = LineParser(hub, alias, verbose=args.verbose)
        background.append(asyncio.create_task(serial_loop(pump, parser_)))
        print(f"[live_monitor] porta {dev} mapeada para alias {alias}")

    try:
        # run forever
        await asyncio.Event().wait()
    finally:
        for p in pumps:
            p.stop()
        for t in background:
            t.cancel()
        await runner.cleanup()


def main():
    parser = argparse.ArgumentParser(description="AODV-EN real-time mesh dashboard.")
    parser.add_argument(
        "--port",
        action="append",
        default=[],
        metavar="DEV:ALIAS",
        help="porta serial e alias do no, ex: /dev/ttyUSB0:NODE_A (pode repetir)",
    )
    parser.add_argument("--baud", type=int, default=115200, help="baud rate (default 115200)")
    parser.add_argument("--bind", default="127.0.0.1", help="endereco para o HTTP (default 127.0.0.1)")
    parser.add_argument("--http-port", type=int, default=8765, help="porta HTTP (default 8765)")
    parser.add_argument("--demo", action="store_true", help="emite eventos sinteticos sem hardware")
    parser.add_argument(
        "--skip-mac-lookup",
        action="store_true",
        help="nao chamar esptool no startup; depende do reset manual para associar alias->mac",
    )
    parser.add_argument(
        "-v", "--verbose",
        action="count",
        default=0,
        help="aumenta verbosidade: -v imprime cada evento parseado; -vv tambem imprime cada linha serial bruta",
    )
    args = parser.parse_args()

    if not args.demo and not args.port:
        print("ATENCAO: nenhuma --port informada e --demo nao usado; o dashboard ficara vazio.")

    try:
        asyncio.run(amain(args))
    except KeyboardInterrupt:
        print("\n[live_monitor] encerrado")


if __name__ == "__main__":
    main()
