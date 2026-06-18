#!/usr/bin/env python3
"""plot_c1_figures.py - figuras do cenario C1 (HW, 10 nos) a partir dos JSONs
reais da campanha (results/campaign-C1-{aodv-en,flooding}.json).

Saidas (commitadas como figuras do TCC):
  tcc_latex/figuras/fig_hw_metrics.png  (PDR entrega, latencia, tx/entrega, energia; IC95)
  tcc_latex/figuras/fig_pdr_rep.png     (PDR de entrega por repeticao, aodv vs flood)
"""

import json
import math
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path("/Users/huaksonlima/Documents/tcc/aodv-en")
RES = ROOT / "results"
OUT = ROOT / "tcc_latex" / "figuras"
AODV = "#1a365d"
FLOOD = "#c05621"


def ic95(xs):
    xs = [x for x in xs if x is not None]
    n = len(xs)
    if n == 0:
        return 0.0, 0.0
    m = sum(xs) / n
    s = math.sqrt(sum((x - m) ** 2 for x in xs) / (n - 1)) if n > 1 else 0.0
    return m, 1.96 * s / math.sqrt(n) if n > 1 else 0.0


def load(algo):
    d = json.loads((RES / f"campaign-C1-{algo}.json").read_text())
    return d["per_rep"]


def pdr_deliv(r):
    return r.get("pdr_pct")


def txd(r):
    sd = r.get("sum_delivered", 0)
    return r["sum_tx"] / sd if sd > 0 else None


def main():
    a = load("aodv-en")
    f = load("flooding")

    specs = [
        ("PDR de entrega (%)", [pdr_deliv(r) for r in a], [pdr_deliv(r) for r in f]),
        ("Latencia one-way (ms)", [r["latency_oneway_ms"]["mean"] for r in a],
         [r["latency_oneway_ms"]["mean"] for r in f]),
        ("Transmissoes por entrega", [txd(r) for r in a], [txd(r) for r in f]),
        ("Energia (J, estimada)", [r["energy_j"] for r in a], [r["energy_j"] for r in f]),
    ]
    fig, axs = plt.subplots(2, 2, figsize=(9.5, 7.5))
    for ax, (title, av, fv) in zip(axs.flat, specs):
        am, ae = ic95(av)
        fm, fe = ic95(fv)
        ax.bar(["AODV-EN", "Flooding"], [am, fm], yerr=[ae, fe], capsize=5,
               color=[AODV, FLOOD])
        ax.set_title(title)
        ax.grid(True, axis="y", alpha=0.3)
        for i, v in enumerate([am, fm]):
            ax.text(i, v, f"{v:.3g}", ha="center", va="bottom", fontsize=9)
    fig.suptitle("AODV-EN vs Flooding - cenario C1 multi-hop (10 nos, 30 reps, barra=IC95)")
    fig.tight_layout()
    fig.savefig(OUT / "fig_hw_metrics.png", dpi=140)
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(8, 4.5))
    ax.plot(range(1, len(a) + 1), [pdr_deliv(r) for r in a], marker="o",
            color=AODV, lw=1.8, label="AODV-EN")
    ax.plot(range(1, len(f) + 1), [pdr_deliv(r) for r in f], marker="s",
            color=FLOOD, lw=1.8, label="Flooding")
    ax.set_xlabel("Repeticao")
    ax.set_ylabel("PDR de entrega (%)")
    ax.set_ylim(0, 105)
    ax.set_title("Taxa de entrega por repeticao - C1 multi-hop (10 nos)")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(OUT / "fig_pdr_rep.png", dpi=140)
    plt.close(fig)
    print("gerado:", OUT / "fig_hw_metrics.png")
    print("gerado:", OUT / "fig_pdr_rep.png")


if __name__ == "__main__":
    main()
