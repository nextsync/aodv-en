#!/usr/bin/env python3
"""
tcc_metrics.py - computa as 4 metricas do TCC (PDR, latencia, NRL, energia) a
partir de logs serial REAIS dos nos (AODV-EN ou flooding). Complementa o
extract_monitor_metrics.py (que faz analise de rotas AODV).

So usa numeros PARSEADOS do log. As constantes de energia sao ESTIMATIVAS de
datasheet (ESP32-WROOM-32), rotuladas - nao medidas.

Fontes no log (instrumentacao add em 2026-05-31):
  - latencia: linhas 'LAT seq=<n> rtt_ms=<r>' (RTT medido na origem, mesmo clock).
  - PDR: data enviado vs ACK recebido na origem.
      flood: 'flood DATA broadcast' (envio) ; 'ACK received' (ack)
      aodv:  'DATA queued to route' / 'DATA queued while' (envio) ; 'ACK received' (ack)
  - NRL/energia: ultima linha de stats por no.
      aodv:  'routes=.. neighbors=.. tx=N rx=N delivered=N control=N acks=N'
      flood: 'stats tx=N rx=N rebroadcast=.. delivered=N ack=.. dup=.. ttl_drop=..' (control=0)

Uso:
  python3 tcc_metrics.py --algo aodv-en --origin N2.log --node N1.log --node N3.log --duration-s 48
  (origin = log do no de origem dos DATA; --node = todos os nos p/ energia/NRL de rede)
"""

import argparse
import json
import math
import re
import sys
from pathlib import Path

ANSI = re.compile(r"\x1B\[[0-?]*[ -/]*[@-~]")
LAT_RE = re.compile(r"LAT seq=(\d+) rtt_ms=(\d+)")
AODV_STATS_RE = re.compile(
    r"routes=(\d+)\s+neighbors=(\d+)\s+tx=(\d+)\s+rx=(\d+)\s+delivered=(\d+)\s+control=(\d+)\s+acks=(\d+)"
)
FLOOD_STATS_RE = re.compile(
    r"stats tx=(\d+) rx=(\d+) rebroadcast=(\d+) delivered=(\d+) ack=(\d+) dup=(\d+) ttl_drop=(\d+)"
)
STATSREP_RE = re.compile(
    r"STATSREP node=([0-9A-Fa-f:]{17}) tx=(\d+) rx=(\d+) control=(\d+) delivered=(\d+)"
)

# Constantes do modelo de energia (ESP32-WROOM-32, datasheet tipico, ROTULADO estimativa)
ENERGY_DEFAULTS = {
    "v": 3.3,        # V
    "i_tx": 0.240,   # A (RF TX 802.11b ativo)
    "i_rx": 0.100,   # A (RF RX ativo)
    "i_idle": 0.020, # A (modem-sleep)
    "t_pkt": 0.001,  # s (airtime por frame ESP-NOW ~72B, estimado)
}


def clean(text):
    return ANSI.sub("", text)


def parse_origin(path):
    text = clean(Path(path).read_text(encoding="utf-8", errors="replace"))
    rtts = [int(m.group(2)) for m in LAT_RE.finditer(text)]
    acks = len(re.findall(r"ACK received", text))
    flood_sent = len(re.findall(r"flood DATA broadcast", text))
    aodv_sent = len(re.findall(r"DATA queued", text))
    data_sent = flood_sent if flood_sent > 0 else aodv_sent
    statsrep = {}
    for m in STATSREP_RE.finditer(text):
        mac = m.group(1).upper()
        statsrep[mac] = {
            "tx": int(m.group(2)),
            "rx": int(m.group(3)),
            "control": int(m.group(4)),
            "delivered": int(m.group(5)),
        }
    return {"rtts": rtts, "acks": acks, "data_sent": data_sent, "statsrep": statsrep}


def parse_node_final(path):
    text = clean(Path(path).read_text(encoding="utf-8", errors="replace"))
    tx = rx = delivered = control = 0
    found = False
    for m in AODV_STATS_RE.finditer(text):
        _, _, tx, rx, delivered, control, _acks = (int(x) for x in m.groups())
        found = True
    if not found:
        for m in FLOOD_STATS_RE.finditer(text):
            tx, rx, _reb, delivered, _ack, _dup, _ttl = (int(x) for x in m.groups())
            control = 0
            found = True
    return {"tx": tx, "rx": rx, "delivered": delivered, "control": control, "found": found}


def stats_summary(values):
    n = len(values)
    if n == 0:
        return {"n": 0, "mean": None, "std": None, "ic95": None}
    mean = sum(values) / n
    if n > 1:
        var = sum((v - mean) ** 2 for v in values) / (n - 1)
        std = math.sqrt(var)
        ic95 = 1.96 * std / math.sqrt(n)
    else:
        std = 0.0
        ic95 = 0.0
    return {"n": n, "mean": round(mean, 3), "std": round(std, 3), "ic95": round(ic95, 3)}


def main():
    p = argparse.ArgumentParser(description="Metricas TCC (PDR/latencia/NRL/energia) de logs reais.")
    p.add_argument("--algo", required=True)
    p.add_argument("--origin", required=True, help="log serial do no de origem")
    p.add_argument("--node", action="append", default=[], help="log de cada no (p/ NRL/energia de rede)")
    p.add_argument("--duration-s", type=float, required=True)
    p.add_argument("--scenario", default="C1-3n-hub")
    p.add_argument("--seed", type=int, default=0)
    for k, v in ENERGY_DEFAULTS.items():
        p.add_argument(f"--{k}", type=float, default=v)
    args = p.parse_args()

    org = parse_origin(args.origin)
    pdr_raw = (100.0 * org["acks"] / org["data_sent"]) if org["data_sent"] > 0 else 0.0
    # PDR nao pode exceder 100%: acks>data_sent indica efeito de borda de janela
    # (acks de DATA enviados antes do inicio da captura). Clampa e sinaliza.
    pdr = min(100.0, pdr_raw)
    pdr_boundary = pdr_raw > 100.0
    lat_rtt = stats_summary(org["rtts"])
    lat_oneway = stats_summary([r / 2.0 for r in org["rtts"]])

    statsrep = org.get("statsrep", {})
    if statsrep:
        nodes = list(statsrep.values())
        stats_source = "statsrep_inband"
    else:
        nodes = [parse_node_final(n) for n in args.node]
        nodes = [n for n in nodes if n["found"]]
        stats_source = "node_serial_logs"
    sum_tx = sum(n["tx"] for n in nodes)
    sum_rx = sum(n["rx"] for n in nodes)
    sum_control = sum(n["control"] for n in nodes)
    sum_delivered = sum(n["delivered"] for n in nodes)

    nrl = (sum_control / sum_delivered) if sum_delivered > 0 else 0.0

    e_tx = args.v * args.i_tx * args.t_pkt
    e_rx = args.v * args.i_rx * args.t_pkt
    p_idle = args.v * args.i_idle
    energy_j = sum_tx * e_tx + sum_rx * e_rx + len(nodes) * args.duration_s * p_idle

    out = {
        "algo": args.algo,
        "scenario": args.scenario,
        "seed": args.seed,
        "duration_s": args.duration_s,
        "n_nodes": len(nodes),
        "stats_source": stats_source,
        "data_sent": org["data_sent"],
        "acks": org["acks"],
        "pdr_pct": round(pdr, 2),
        "pdr_raw_pct": round(pdr_raw, 2),
        "pdr_boundary_effect": pdr_boundary,
        "latency_rtt_ms": lat_rtt,
        "latency_oneway_ms": lat_oneway,
        "sum_tx": sum_tx,
        "sum_rx": sum_rx,
        "sum_control_tx": sum_control,
        "sum_delivered": sum_delivered,
        "nrl": round(nrl, 4),
        "energy_j": round(energy_j, 4),
        "energy_constants": {
            "note": "ESTIMATIVA datasheet ESP32-WROOM-32 (nao medido)",
            "V": args.v, "I_tx": args.i_tx, "I_rx": args.i_rx, "I_idle": args.i_idle, "t_pkt_s": args.t_pkt,
        },
    }
    print(json.dumps(out, indent=1))


if __name__ == "__main__":
    main()
