#!/usr/bin/env python3
"""rssi_matrix.py - monta a matriz RSSI N×N (quem ouve quem) da rede inteira.

Duas fontes (combinaveis):
  1. LOG DO COLETOR com linhas NEIGHREP (rede inteira via telemetria in-band multi-hop):
       NEIGHREP node=<self> hears=<mac>:<rssi>,<mac>:<rssi>,...
     Cada linha da a linha da matriz daquele no, mesmo nos nao-vizinhos do coletor.
  2. LOGS DE PROBE por porta (metodo 1-a-1): linhas
       RSSISELF self=<mac>   +   RSSIPROBE src=<mac> rssi=<dbm>
     Usado quando se pluga cada no na serial separadamente.

Uso:
  rssi_matrix.py --collector results/matrix-EB80.log
  rssi_matrix.py --probe results/p1.log --probe results/p2.log ...
  rssi_matrix.py --collector C.log --probe p1.log   (combina)
  [--names EB80=28:05:A5:33:EB:80,N2=28:05:A5:33:D6:1C,...]  (apelidos)
"""

import argparse
import re
from collections import defaultdict

RE_NEIGH = re.compile(r"NEIGHREP node=([0-9A-Fa-f:]{17}) hears=(.*)")
RE_PAIR = re.compile(r"([0-9A-Fa-f:]{17}):(-?\d+)")
RE_SELF = re.compile(r"(?:RSSISELF self|self_mac)=([0-9A-Fa-f:]{17})")
RE_PROBE = re.compile(r"RSSIPROBE src=([0-9A-Fa-f:]{17}) rssi=(-?\d+)")


def add(matrix, a, b, rssi):
    matrix[a.upper()][b.upper()].append(rssi)


def parse_collector(path, matrix):
    for line in open(path, encoding="utf-8", errors="replace"):
        m = RE_NEIGH.search(line)
        if not m:
            continue
        node = m.group(1).upper()
        for nb, rssi in RE_PAIR.findall(m.group(2)):
            add(matrix, node, nb, int(rssi))


def parse_probe(path, matrix):
    self_mac = None
    for line in open(path, encoding="utf-8", errors="replace"):
        ms = RE_SELF.search(line)
        if ms:
            self_mac = ms.group(1).upper()
        mp = RE_PROBE.search(line)
        if mp and self_mac:
            add(matrix, self_mac, mp.group(1), int(mp.group(2)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--collector", action="append", default=[])
    ap.add_argument("--probe", action="append", default=[])
    ap.add_argument("--names", default="", help="N1=mac,N2=mac,...")
    args = ap.parse_args()

    alias = {}
    for pair in args.names.split(",") if args.names else []:
        if "=" in pair:
            name, mac = pair.split("=", 1)
            alias[mac.strip().upper()] = name.strip()

    matrix = defaultdict(lambda: defaultdict(list))
    for c in args.collector:
        parse_collector(c, matrix)
    for p in args.probe:
        parse_probe(p, matrix)

    macs = sorted(set(matrix.keys()) | {b for row in matrix.values() for b in row})
    if not macs:
        print("nenhum dado RSSI encontrado.")
        return

    def lbl(mac):
        return alias.get(mac, mac[-5:])

    w = max(8, max(len(lbl(m)) for m in macs) + 1)
    print(f"\n=== matriz RSSI (dBm; linha=ouvinte, coluna=ouvido) ===\n")
    hdr = " " * w + "".join(f"{lbl(m):>{w}}" for m in macs)
    print(hdr)
    for a in macs:
        cells = []
        for b in macs:
            if a == b:
                cells.append(f"{'—':>{w}}")
            elif matrix[a].get(b):
                vals = matrix[a][b]
                cells.append(f"{sum(vals)/len(vals):>{w}.1f}")
            else:
                cells.append(f"{'.':>{w}}")
        print(f"{lbl(a):>{w}}" + "".join(cells))

    print("\n=== leitura de enlaces ===")
    seen = set()
    for a in macs:
        for b in macs:
            if a == b or (b, a) in seen:
                continue
            seen.add((a, b))
            fa = matrix[a].get(b)
            fb = matrix[b].get(a)
            if not fa and not fb:
                continue
            va = sum(fa) / len(fa) if fa else None
            vb = sum(fb) / len(fb) if fb else None
            best = max(v for v in (va, vb) if v is not None)
            state = ("FIRME" if best >= -82 else "BORDA" if best >= -90 else "FRACO")
            pa = f"{va:.0f}" if va is not None else "·"
            pb = f"{vb:.0f}" if vb is not None else "·"
            print(f"  {lbl(a)} <-> {lbl(b)}: {pa}/{pb} dBm  [{state}]")
    print("\n('.' = nao ouviu; alvo elo = -70..-82 FIRME; pulo-de-2 deve ficar < -90)")


if __name__ == "__main__":
    main()
