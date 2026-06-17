#!/usr/bin/env python3
"""rssi_probe.py - mede o alcance/topologia real entre ESP32 a partir dos logs RSSIPROBE.

Cada no roda o firmware AODV (que emite HELLO periodico e loga, para cada quadro recebido,
'RSSIPROBE src=<mac> rssi=<dbm>'). Este script le os logs de N portas em paralelo, monta a
matriz quem-ouve-quem com RSSI medio por enlace, e diz se o arranjo forma estrela (todos se
ouvem) ou tem nos isolados (potencial multi-hop).

Uso:
  rssi_probe.py --duration 30 N1:/dev/cu.usbserial-XXX N2:/dev/cu.usbserial-YYY [N3:...]
"""

import argparse
import re
import threading
import time
from collections import defaultdict

import serial

RE_PROBE = re.compile(r"RSSIPROBE src=([0-9A-Fa-f:]{17}) rssi=(-?\d+)")
RE_SELF = re.compile(r"(?:RSSISELF self|self_mac)=([0-9A-Fa-f:]{17})")


def capture(label, port, duration, baud, out):
    self_mac = None
    heard = defaultdict(list)
    try:
        with serial.Serial(port, baud, timeout=1, exclusive=True) as ser:
            deadline = time.monotonic() + duration
            while time.monotonic() < deadline:
                raw = ser.readline()
                if not raw:
                    continue
                line = raw.decode("utf-8", errors="replace")
                ms = RE_SELF.search(line)
                if ms and self_mac is None:
                    self_mac = ms.group(1).upper()
                mp = RE_PROBE.search(line)
                if mp:
                    heard[mp.group(1).upper()].append(int(mp.group(2)))
        out[label] = {"self": self_mac, "heard": dict(heard)}
    except Exception as exc:
        out[label] = {"error": str(exc)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--duration", type=int, default=30)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("ports", nargs="+", help="label:port")
    args = ap.parse_args()

    pairs = [(p.split(":", 1)[0], p.split(":", 1)[1]) for p in args.ports]
    out = {}
    threads = []
    for label, port in pairs:
        t = threading.Thread(target=capture, args=(label, port, args.duration, args.baud, out))
        t.start()
        threads.append(t)
    for t in threads:
        t.join()

    mac2label = {}
    for label, d in out.items():
        if d.get("self"):
            mac2label[d["self"]] = label

    print(f"\n=== RSSI probe ({args.duration}s, {len(pairs)} nos) ===\n")
    labels = [lb for lb, _ in pairs]
    for label, _ in pairs:
        d = out.get(label, {})
        if "error" in d:
            print(f"{label}: ERRO {d['error']}")
            continue
        self_mac = d.get("self") or "?"
        print(f"{label} (self={self_mac}) ouve:")
        if not d.get("heard"):
            print("   (ninguem)")
        for mac, rssis in sorted(d["heard"].items(), key=lambda kv: -sum(kv[1]) / len(kv[1])):
            who = mac2label.get(mac, mac)
            avg = sum(rssis) / len(rssis)
            print(f"   {who:<6} rssi_med={avg:6.1f} dBm  min={min(rssis)} max={max(rssis)} n={len(rssis)}")
        print()

    n = len(pairs)
    edges = 0
    for label, _ in pairs:
        d = out.get(label, {})
        if "heard" in d:
            edges += sum(1 for mac in d["heard"] if mac in mac2label)
    possible = n * (n - 1)
    print(f"=== veredito ===")
    print(f"enlaces ouvidos: {edges}/{possible} pares dirigidos")
    if possible and edges >= possible * 0.9:
        print(">> ESTRELA: quase todos se ouvem. A 5m nao forma multi-hop sem barreira/allowlist.")
    elif possible and edges <= possible * 0.5:
        print(">> ESPARSO: ha nos que nao se ouvem. Potencial p/ cadeia/multi-hop real.")
    else:
        print(">> PARCIAL: topologia intermediaria; verificar par a par antes de fixar o cenario.")


if __name__ == "__main__":
    main()
