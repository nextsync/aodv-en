#!/usr/bin/env python3
"""
plot_tcc_figures.py - gera as figuras do relatorio TCC a partir de dados REAIS:
  - ledger de hardware (results/experiments-ledger.json): media+desvio por metrica/algo
  - varredura de simulacao (results/m-bench-sim-compare.csv): tx/entrega vs nos

Saidas (commitadas, sao figuras do documento):
  docs/img/tcc/fig-hw-metrics.png    (PDR, latencia, NRL, energia: AODV vs flood, media+std)
  docs/img/tcc/fig-hw-channel.png    (tx/rx de rede: custo de canal)
  docs/img/tcc/fig-sim-crossover.png (tx/entrega vs numero de nos, sim grid)
  docs/img/tcc/fig-latency-seeds.png (latencia one-way por seed)

Uso: $IDFPY firmware/tools/plot_tcc_figures.py
"""

import csv
import json
import math
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path("/Users/huaksonlima/Documents/tcc/aodv-en")
LEDGER = ROOT / "results" / "experiments-ledger.json"
SIMCSV = ROOT / "results" / "m-bench-sim-compare.csv"
OUT = ROOT / "docs" / "img" / "tcc"
OUT.mkdir(parents=True, exist_ok=True)

AODV = "#1a365d"
FLOOD = "#c05621"


def load_ledger():
    data = json.loads(LEDGER.read_text())
    runs = data if isinstance(data, list) else data.get("runs", [])
    by = {"aodv-en": {}, "flooding": {}}
    for r in runs:
        algo = r["algo"]
        for k, v in r["metrics"].items():
            by.setdefault(algo, {}).setdefault(k, []).append(float(v))
    return by


def mean_std(xs):
    n = len(xs)
    if n == 0:
        return 0.0, 0.0
    m = sum(xs) / n
    s = math.sqrt(sum((x - m) ** 2 for x in xs) / (n - 1)) if n > 1 else 0.0
    return m, s


def fig_hw_metrics(by):
    specs = [("pdr", "PDR (%)"), ("latency_ms", "Latencia one-way (ms)"),
             ("nrl", "NRL (controle/dados)"), ("energy_j", "Energia (J, estimada)")]
    fig, axs = plt.subplots(2, 2, figsize=(9.5, 7.5))
    for ax, (k, title) in zip(axs.flat, specs):
        am, asd = mean_std(by["aodv-en"].get(k, []))
        fm, fsd = mean_std(by["flooding"].get(k, []))
        ax.bar(["AODV-EN", "Flooding"], [am, fm], yerr=[asd, fsd], capsize=5,
               color=[AODV, FLOOD])
        ax.set_title(title)
        ax.grid(True, axis="y", alpha=0.3)
        for i, (v, sd) in enumerate([(am, asd), (fm, fsd)]):
            ax.text(i, v, f"{v:.3g}", ha="center", va="bottom", fontsize=9)
    n = len(by["aodv-en"].get("pdr", []))
    fig.suptitle(f"AODV-EN vs Flooding — hardware (3 ESP32, hub, media de {n} seeds, barra=desvio)")
    fig.tight_layout()
    p = OUT / "fig-hw-metrics.png"
    fig.savefig(p, dpi=140); plt.close(fig)
    return p, (am if False else None)


def fig_hw_channel():
    # contadores de rede por seed, dos JSONs de metricas
    rows = {"aodv-en": {"tx": [], "rx": []}, "flooding": {"tx": [], "rx": []}}
    pats = [("aodv-en", "m10-aodv-metrics.json"), ("aodv-en", "m10-aodv-s2-metrics.json"),
            ("aodv-en", "m10-aodv-s3-metrics.json"), ("aodv-en", "m10-aodv-s4-metrics.json"),
            ("flooding", "m10-flood-metrics.json"), ("flooding", "m10-flood-s2-metrics.json"),
            ("flooding", "m10-flood-s3-metrics.json"), ("flooding", "m10-flood-s4-metrics.json")]
    for algo, fn in pats:
        f = ROOT / "results" / fn
        if f.exists():
            d = json.loads(f.read_text())
            rows[algo]["tx"].append(d["sum_tx"]); rows[algo]["rx"].append(d["sum_rx"])
    def mean(xs): return sum(xs) / len(xs) if xs else 0
    fig, ax = plt.subplots(figsize=(7.5, 5))
    x = [0, 1]; w = 0.38
    ax.bar([i - w/2 for i in x], [mean(rows["aodv-en"]["tx"]), mean(rows["flooding"]["tx"])],
           w, color=AODV, label="TX (transmissoes)")
    ax.bar([i + w/2 for i in x], [mean(rows["aodv-en"]["rx"]), mean(rows["flooding"]["rx"])],
           w, color=FLOOD, label="RX (recepcoes)")
    ax.set_xticks(x); ax.set_xticklabels(["AODV-EN", "Flooding"])
    ax.set_ylabel("Quadros (media de 3 seeds, rede toda)")
    ax.set_title("Custo de canal: TX e RX por algoritmo (rede de 3 nos)")
    ax.grid(True, axis="y", alpha=0.3); ax.legend()
    for i, vals in zip(x, [(mean(rows["aodv-en"]["tx"]), mean(rows["aodv-en"]["rx"])),
                           (mean(rows["flooding"]["tx"]), mean(rows["flooding"]["rx"]))]):
        ax.text(i - w/2, vals[0], f"{vals[0]:.0f}", ha="center", va="bottom", fontsize=9)
        ax.text(i + w/2, vals[1], f"{vals[1]:.0f}", ha="center", va="bottom", fontsize=9)
    fig.tight_layout(); p = OUT / "fig-hw-channel.png"; fig.savefig(p, dpi=140); plt.close(fig)
    return p


def fig_sim_crossover():
    if not SIMCSV.exists():
        return None
    rows = list(csv.DictReader(open(SIMCSV)))
    a = sorted([r for r in rows if r["protocol"] == "aodv"], key=lambda r: int(r["nodes"]))
    f = sorted([r for r in rows if r["protocol"] == "flood"], key=lambda r: int(r["nodes"]))
    fig, ax = plt.subplots(figsize=(8, 5))
    ax.plot([int(r["nodes"]) for r in a], [float(r["tx_per_delivered"]) for r in a],
            marker="o", color=AODV, lw=2, label="AODV-EN")
    ax.plot([int(r["nodes"]) for r in f], [float(r["tx_per_delivered"]) for r in f],
            marker="s", color=FLOOD, lw=2, label="Flooding")
    ax.set_xlabel("Numero de nos (grid)"); ax.set_ylabel("Transmissoes por entrega")
    ax.set_title("Simulacao: custo de canal vs escala (entrega 100% em ambos)")
    ax.grid(True, alpha=0.3); ax.legend()
    fig.tight_layout(); p = OUT / "fig-sim-crossover.png"; fig.savefig(p, dpi=140); plt.close(fig)
    return p


def fig_latency_seeds(by):
    al = by["aodv-en"].get("latency_ms", [])
    fl = by["flooding"].get("latency_ms", [])
    seeds = list(range(1, max(len(al), len(fl)) + 1))
    fig, ax = plt.subplots(figsize=(7.5, 4.5))
    ax.plot(seeds[:len(al)], al, marker="o", color=AODV, lw=2, label="AODV-EN")
    ax.plot(seeds[:len(fl)], fl, marker="s", color=FLOOD, lw=2, label="Flooding")
    ax.set_xlabel("Seed"); ax.set_ylabel("Latencia one-way (ms)")
    ax.set_xticks(seeds)
    ax.set_title("Latencia one-way por seed (variacao do flooding no seed 3 = ACK em 2 hops)")
    ax.grid(True, alpha=0.3); ax.legend()
    fig.tight_layout(); p = OUT / "fig-latency-seeds.png"; fig.savefig(p, dpi=140); plt.close(fig)
    return p


def main():
    by = load_ledger()
    outs = [fig_hw_metrics(by)[0], fig_hw_channel(), fig_sim_crossover(), fig_latency_seeds(by)]
    for o in outs:
        if o:
            print("gerado:", o)


if __name__ == "__main__":
    main()
