#!/usr/bin/env python3
"""campaign.py - orquestra a campanha de 30 reps por cenario/algoritmo (Fase 1).

Modelo de coleta (decidido no PLANO_CAMPANHA_30REPS.md):
  - so a ORIGEM fica na serial; nos espalhados reportam via telemetria in-band (STATSREP).
  - cada repeticao = capturar o log da origem por --rep-seconds; a origem ja loga LAT (RTT),
    ACK received (PDR) e STATSREP node=... (NRL/energia da rede inteira).
  - 30 reps -> tcc_metrics.py por rep -> agrega media/desvio/IC95.

Este script NAO builda/flasha (isso e por-no, manual ou via build_flash). Ele assume que os
nos ja estao flashados para o cenario/algoritmo corrente (origem com REPORT desligado e
TARGET=destino; demais com REPORT_TO_MAC=origem). Ele:
  1. captura N reps do log da origem (serial_capture),
  2. roda tcc_metrics por rep,
  3. agrega (media/desvio/IC95) e grava results/campaign-<cenario>-<algo>.json.

Uso:
  campaign.py --scenario C1 --algo aodv-en --origin-port /dev/cu.usbserial-XXXX \
      --reps 30 --rep-seconds 60 [--settle 8] [--duration-s 60]
"""

import argparse
import json
import math
import subprocess
import time
from pathlib import Path

ROOT = Path("/Users/huaksonlima/Documents/tcc/aodv-en")
RESULTS = ROOT / "results"
IDFPY = "/Users/huaksonlima/.espressif/python_env/idf6.0_py3.14_env/bin/python"
METRICS = ROOT / "firmware" / "tools" / "tcc_metrics.py"
CAPTURE = ROOT / "firmware" / "tools" / "serial_capture.py"


def capture_rep(origin_port, prefix, seconds, baud):
    subprocess.run(
        [IDFPY, str(CAPTURE), "--duration", str(seconds), "--baud", str(baud),
         "--out-prefix", prefix, f"ORIGIN:{origin_port}"],
        check=True, capture_output=True, text=True)
    return RESULTS / f"{prefix}-ORIGIN.log"


def run_metrics(algo, origin_log, duration_s, scenario, seed):
    r = subprocess.run(
        [IDFPY, str(METRICS), "--algo", algo, "--origin", str(origin_log),
         "--duration-s", str(duration_s), "--scenario", scenario, "--seed", str(seed)],
        check=True, capture_output=True, text=True)
    return json.loads(r.stdout)


def agg(values):
    vals = [v for v in values if v is not None]
    n = len(vals)
    if n == 0:
        return {"n": 0, "mean": None, "std": None, "ic95": None}
    mean = sum(vals) / n
    if n > 1:
        std = math.sqrt(sum((v - mean) ** 2 for v in vals) / (n - 1))
        ic95 = 1.96 * std / math.sqrt(n)
    else:
        std = ic95 = 0.0
    return {"n": n, "mean": round(mean, 3), "std": round(std, 3), "ic95": round(ic95, 3)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--scenario", required=True)
    ap.add_argument("--algo", required=True, choices=["aodv-en", "flooding"])
    ap.add_argument("--origin-port", required=True)
    ap.add_argument("--reps", type=int, default=30)
    ap.add_argument("--rep-seconds", type=int, default=60)
    ap.add_argument("--duration-s", type=float, default=None,
                    help="janela p/ energia/idle no tcc_metrics (default = rep-seconds)")
    ap.add_argument("--settle", type=int, default=8, help="espera entre reps (s)")
    ap.add_argument("--baud", type=int, default=115200)
    args = ap.parse_args()

    duration_s = args.duration_s if args.duration_s is not None else float(args.rep_seconds)
    RESULTS.mkdir(parents=True, exist_ok=True)
    per_rep = []
    print(f"campanha {args.scenario}/{args.algo}: {args.reps} reps x {args.rep_seconds}s")

    for rep in range(1, args.reps + 1):
        prefix = f"camp-{args.scenario}-{args.algo}-r{rep:02d}"
        log = capture_rep(args.origin_port, prefix, args.rep_seconds, args.baud)
        m = run_metrics(args.algo, log, duration_s, args.scenario, rep)
        per_rep.append(m)
        print(f"  rep {rep:02d}: PDR={m['pdr_pct']:.1f} "
              f"lat_ow={m['latency_oneway_ms']['mean']} "
              f"NRL={m['nrl']} E={m['energy_j']} nodes={m['n_nodes']} src={m['stats_source']}")
        if rep < args.reps:
            time.sleep(args.settle)

    summary = {
        "scenario": args.scenario,
        "algo": args.algo,
        "reps": len(per_rep),
        "rep_seconds": args.rep_seconds,
        "pdr_pct": agg([m["pdr_pct"] for m in per_rep]),
        "latency_oneway_ms": agg([m["latency_oneway_ms"]["mean"] for m in per_rep]),
        "nrl": agg([m["nrl"] for m in per_rep]),
        "energy_j": agg([m["energy_j"] for m in per_rep]),
        "n_nodes_seen": agg([m["n_nodes"] for m in per_rep]),
        "per_rep": per_rep,
    }
    out = RESULTS / f"campaign-{args.scenario}-{args.algo}.json"
    out.write_text(json.dumps(summary, indent=2, ensure_ascii=False))
    print(f"\n=== {args.scenario}/{args.algo} ({len(per_rep)} reps) ===")
    for k in ("pdr_pct", "latency_oneway_ms", "nrl", "energy_j"):
        s = summary[k]
        print(f"  {k:18} mean={s['mean']} std={s['std']} ic95={s['ic95']} (n={s['n']})")
    print(f"gravado: {out}")


if __name__ == "__main__":
    main()
