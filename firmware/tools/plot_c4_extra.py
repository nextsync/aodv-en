#!/usr/bin/env python3
"""plot_c4_extra.py - figuras complementares do cenario C4 a partir dos JSONs reais:
  fig_c4_pdr_rep.png   - PDR de entrega por repeticao no C4 (variabilidade da recuperacao)
  fig_c4_metrics.png   - impacto da falha: PDR e latencia, C1 (sem falha) vs C4 (sob falha)
"""

import json
import math
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path("/Users/huaksonlima/Documents/tcc/aodv-en")
OUT = ROOT / "tcc_latex" / "figuras"
C1C = "#1a365d"
C4C = "#c05621"


def load(scn):
    return json.loads((ROOT / "results" / f"campaign-{scn}-aodv-en.json").read_text())["per_rep"] if False else \
        json.loads((ROOT / "results" / f"campaign-{scn}-aodv-en.json").read_text())["per_rep"]


def ic95(xs):
    xs = [x for x in xs if x is not None]
    n = len(xs)
    m = sum(xs) / n
    s = math.sqrt(sum((x - m) ** 2 for x in xs) / (n - 1)) if n > 1 else 0
    return m, (1.96 * s / math.sqrt(n) if n > 1 else 0)


scn = None
c1 = json.loads((ROOT / "results" / "campaign-C1-aodv-en.json").read_text())["per_rep"]
c4 = json.loads((ROOT / "results" / "campaign-C4-aodv-en.json").read_text())["per_rep"]


def pdr(r):
    if r.get("pdr_source") == "delivery_dest":
        return r.get("pdr_delivery_pct")
    return None


def lat(r):
    return r["latency_oneway_ms"]["mean"]


fig, ax = plt.subplots(figsize=(8.5, 4.3))
y = [pdr(r) for r in c4 if pdr(r) is not None]
ax.plot(range(1, len(y) + 1), y, marker="o", color=C4C, lw=1.8)
ax.axhline(sum(v for v in y) / len(y), ls="--", color="#555", lw=1, label=f"media {sum(y)/len(y):.1f}%")
ax.set_xlabel("Repeticao")
ax.set_ylabel("PDR de entrega (%)")
ax.set_ylim(0, 105)
ax.set_title("C4: taxa de entrega por repeticao sob falha periodica (recuperacao estocastica)")
ax.grid(True, alpha=0.3)
ax.legend()
fig.tight_layout()
fig.savefig(OUT / "fig_c4_pdr_rep.png", dpi=140)
plt.close(fig)

fig, axs = plt.subplots(1, 2, figsize=(9, 4.2))
for ax, (title, f) in zip(axs, [("PDR de entrega (%)", pdr), ("Latencia one-way (ms)", lat)]):
    m1, e1 = ic95([f(r) for r in c1])
    m4, e4 = ic95([f(r) for r in c4])
    ax.bar(["C1 (sem falha)", "C4 (sob falha)"], [m1, m4], yerr=[e1, e4], capsize=5, color=[C1C, C4C])
    ax.set_title(title)
    ax.grid(True, axis="y", alpha=0.3)
    for i, v in enumerate([m1, m4]):
        ax.text(i, v, f"{v:.1f}", ha="center", va="bottom", fontsize=9)
fig.suptitle("Impacto da falha periodica de rele on-path (AODV-EN, barra=IC95)")
fig.tight_layout()
fig.savefig(OUT / "fig_c4_metrics.png", dpi=140)
plt.close(fig)
print("gerado:", OUT / "fig_c4_pdr_rep.png")
print("gerado:", OUT / "fig_c4_metrics.png")
