#!/usr/bin/env python3
"""plot_graphs_unit.py - gera cada grafico do TCC como PNG UNITARIO em graphs/.

Diferente de plot_tcc_figures.py (que junta 4 metricas num grid 2x2), aqui cada
metrica/grafico sai num arquivo proprio. Dados REAIS:
  - results/experiments-ledger.json (HW, media+desvio por metrica/algo)
  - results/m10-*-metrics.json (contadores de canal por seed)
  - results/m-bench-sim-compare.csv (varredura de simulacao)

Saidas (graphs/):
  pdr.png latencia.png nrl.png energia.png
  canal_tx_rx.png latencia_por_seed.png sim_crossover.png

Uso: $IDFPY firmware/tools/plot_graphs_unit.py
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
OUT = ROOT / "graphs"
OUT.mkdir(parents=True, exist_ok=True)

AODV = "#1a365d"
FLOOD = "#c05621"


def load_ledger():
    runs = json.loads(LEDGER.read_text())
    by = {"aodv-en": {}, "flooding": {}}
    for r in runs:
        for k, v in r["metrics"].items():
            by.setdefault(r["algo"], {}).setdefault(k, []).append(float(v))
    return by


def mean_std(xs):
    n = len(xs)
    if n == 0:
        return 0.0, 0.0
    m = sum(xs) / n
    s = math.sqrt(sum((x - m) ** 2 for x in xs) / (n - 1)) if n > 1 else 0.0
    return m, s


def bar_metric(by, key, title, fname, n):
    am, asd = mean_std(by["aodv-en"].get(key, []))
    fm, fsd = mean_std(by["flooding"].get(key, []))
    fig, ax = plt.subplots(figsize=(5, 4.2))
    ax.bar(["AODV-EN", "Flooding"], [am, fm], yerr=[asd, fsd], capsize=6,
           color=[AODV, FLOOD])
    ax.set_title(f"{title}\n(hardware, 3 ESP32 hub, media de {n} seeds)", fontsize=11)
    ax.grid(True, axis="y", alpha=0.3)
    for i, (v, sd) in enumerate([(am, asd), (fm, fsd)]):
        ax.text(i, v, f"{v:.3g}", ha="center", va="bottom", fontsize=10)
    fig.tight_layout()
    p = OUT / fname
    fig.savefig(p, dpi=150)
    plt.close(fig)
    return p


def channel_tx_rx():
    rows = {"aodv-en": {"tx": [], "rx": []}, "flooding": {"tx": [], "rx": []}}
    pats = [("aodv-en", "m10-aodv-metrics.json")]
    pats += [("aodv-en", f"m10-aodv-s{i}-metrics.json") for i in range(2, 7)]
    pats += [("flooding", "m10-flood-metrics.json")]
    pats += [("flooding", f"m10-flood-s{i}-metrics.json") for i in range(2, 7)]
    for algo, fn in pats:
        f = ROOT / "results" / fn
        if f.exists():
            d = json.loads(f.read_text())
            rows[algo]["tx"].append(d["sum_tx"])
            rows[algo]["rx"].append(d["sum_rx"])

    def mean(xs):
        return sum(xs) / len(xs) if xs else 0

    fig, ax = plt.subplots(figsize=(6, 4.5))
    x = [0, 1]
    w = 0.38
    ax.bar([i - w / 2 for i in x], [mean(rows["aodv-en"]["tx"]), mean(rows["flooding"]["tx"])],
           w, color=AODV, label="TX (transmissoes)")
    ax.bar([i + w / 2 for i in x], [mean(rows["aodv-en"]["rx"]), mean(rows["flooding"]["rx"])],
           w, color=FLOOD, label="RX (recepcoes)")
    ax.set_xticks(x)
    ax.set_xticklabels(["AODV-EN", "Flooding"])
    ax.set_ylabel("Quadros (media, rede toda)")
    n = len(rows["aodv-en"]["tx"])
    ax.set_title(f"Custo de canal: TX e RX por algoritmo\n(rede de 3 nos, media de {n} seeds)", fontsize=11)
    ax.grid(True, axis="y", alpha=0.3)
    ax.legend()
    for i, vals in zip(x, [(mean(rows["aodv-en"]["tx"]), mean(rows["aodv-en"]["rx"])),
                           (mean(rows["flooding"]["tx"]), mean(rows["flooding"]["rx"]))]):
        ax.text(i - w / 2, vals[0], f"{vals[0]:.0f}", ha="center", va="bottom", fontsize=9)
        ax.text(i + w / 2, vals[1], f"{vals[1]:.0f}", ha="center", va="bottom", fontsize=9)
    fig.tight_layout()
    p = OUT / "canal_tx_rx.png"
    fig.savefig(p, dpi=150)
    plt.close(fig)
    return p


def latency_per_seed(by):
    al = by["aodv-en"].get("latency_ms", [])
    fl = by["flooding"].get("latency_ms", [])
    seeds = list(range(1, max(len(al), len(fl)) + 1))
    fig, ax = plt.subplots(figsize=(6.5, 4.2))
    ax.plot(seeds[:len(al)], al, marker="o", color=AODV, lw=2, label="AODV-EN")
    ax.plot(seeds[:len(fl)], fl, marker="s", color=FLOOD, lw=2, label="Flooding")
    ax.set_xlabel("Seed")
    ax.set_ylabel("Latencia one-way (ms)")
    ax.set_xticks(seeds)
    ax.set_title("Latencia one-way por seed (AODV-EN vs Flooding)", fontsize=11)
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    p = OUT / "latencia_por_seed.png"
    fig.savefig(p, dpi=150)
    plt.close(fig)
    return p


def sim_crossover():
    if not SIMCSV.exists():
        return None
    rows = list(csv.DictReader(open(SIMCSV)))
    a = sorted([r for r in rows if r["protocol"] == "aodv"], key=lambda r: int(r["nodes"]))
    f = sorted([r for r in rows if r["protocol"] == "flood"], key=lambda r: int(r["nodes"]))
    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.plot([int(r["nodes"]) for r in a], [float(r["tx_per_delivered"]) for r in a],
            marker="o", color=AODV, lw=2, label="AODV-EN")
    ax.plot([int(r["nodes"]) for r in f], [float(r["tx_per_delivered"]) for r in f],
            marker="s", color=FLOOD, lw=2, label="Flooding")
    ax.set_xlabel("Numero de nos (grid)")
    ax.set_ylabel("Transmissoes por entrega")
    ax.set_title("Simulacao: custo de canal vs escala", fontsize=11)
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    p = OUT / "sim_crossover.png"
    fig.savefig(p, dpi=150)
    plt.close(fig)
    return p


def main():
    by = load_ledger()
    n = len(by["aodv-en"].get("pdr", []))
    outs = [
        bar_metric(by, "pdr", "PDR (%)", "pdr.png", n),
        bar_metric(by, "latency_ms", "Latencia one-way (ms)", "latencia.png", n),
        bar_metric(by, "nrl", "NRL (controle/dados)", "nrl.png", n),
        bar_metric(by, "energy_j", "Energia (J, estimada)", "energia.png", n),
        channel_tx_rx(),
        latency_per_seed(by),
        sim_crossover(),
    ]
    for o in outs:
        if o:
            print("gerado:", o)


if __name__ == "__main__":
    main()
