#!/usr/bin/env python3
"""plot_c3_figures.py - figuras do cenario C3 (malha densa) a partir dos JSONs reais:
  fig_c3_compare.png        - aodv vs flood em C3 (PDR, latencia, RX por entrega)
  fig_topology_dependence.png - PDR aodv vs flood em C1 (cadeia) e C3 (malha): o gap encolhe
"""

import json
import math
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path("/Users/huaksonlima/Documents/tcc/aodv-en")
OUT = ROOT / "tcc_latex" / "figuras"
AODV = "#1a365d"
FLOOD = "#c05621"


def per_rep(scn, algo):
    return json.loads((ROOT / "results" / f"campaign-{scn}-{algo}.json").read_text())["per_rep"]


def ic(v):
    v = [x for x in v if x is not None]
    n = len(v)
    m = sum(v) / n
    s = math.sqrt(sum((x - m) ** 2 for x in v) / (n - 1)) if n > 1 else 0
    return m, (1.96 * s / math.sqrt(n) if n > 1 else 0)


def pdr(r):
    return r.get("pdr_pct")


def lat(r):
    return r["latency_oneway_ms"]["mean"]


def rxd(r):
    return r["sum_rx"] / r["sum_delivered"] if r.get("sum_delivered", 0) > 0 else None


a = per_rep("C3", "aodv-en")
f = per_rep("C3", "flooding")

specs = [("PDR de entrega (%)", pdr), ("Latencia one-way (ms)", lat),
         ("RX por pacote entregue", rxd)]
fig, axs = plt.subplots(1, 3, figsize=(11, 4))
for ax, (title, fn) in zip(axs, specs):
    ma, ea = ic([fn(r) for r in a])
    mf, ef = ic([fn(r) for r in f])
    ax.bar(["AODV-EN", "Flooding"], [ma, mf], yerr=[ea, ef], capsize=5, color=[AODV, FLOOD])
    ax.set_title(title)
    ax.grid(True, axis="y", alpha=0.3)
    for i, v in enumerate([ma, mf]):
        ax.text(i, v, f"{v:.1f}", ha="center", va="bottom", fontsize=9)
fig.suptitle("Cenario C3 (malha densa, 5 saltos, 10 nos): AODV-EN vs Flooding (barra=IC95)")
fig.tight_layout()
fig.savefig(OUT / "fig_c3_compare.png", dpi=140)
plt.close(fig)

c1a, _ = ic([pdr(r) for r in per_rep("C1", "aodv-en")])
c1f, _ = ic([pdr(r) for r in per_rep("C1", "flooding")])
c3a, _ = ic([pdr(r) for r in a])
c3f, _ = ic([pdr(r) for r in f])
fig, ax = plt.subplots(figsize=(7.5, 4.6))
x = [0, 1]
w = 0.36
ax.bar([i - w / 2 for i in x], [c1a, c3a], w, color=AODV, label="AODV-EN")
ax.bar([i + w / 2 for i in x], [c1f, c3f], w, color=FLOOD, label="Flooding")
ax.set_xticks(x)
ax.set_xticklabels(["C1 (cadeia fina, 3-4 saltos)", "C3 (malha densa, 5 saltos)"])
ax.set_ylabel("PDR de entrega (%)")
ax.set_ylim(0, 105)
ax.set_title("Dependencia de topologia: vantagem do roteamento encolhe na malha densa")
ax.grid(True, axis="y", alpha=0.3)
ax.legend()
for i, (va, vf) in zip(x, [(c1a, c1f), (c3a, c3f)]):
    ax.text(i - w / 2, va, f"{va:.0f}", ha="center", va="bottom", fontsize=9)
    ax.text(i + w / 2, vf, f"{vf:.0f}", ha="center", va="bottom", fontsize=9)
fig.tight_layout()
fig.savefig(OUT / "fig_topology_dependence.png", dpi=140)
plt.close(fig)
print("gerado:", OUT / "fig_c3_compare.png")
print("gerado:", OUT / "fig_topology_dependence.png")
