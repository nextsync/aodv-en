#!/usr/bin/env python3
"""plot_c4_selfheal.py - figura de self-healing do cenario C4: entregas acumuladas
vs tempo numa repeticao real, evidenciando os patamares (falha do rele on-path) e
a retomada (re-rota/recuperacao). Fonte: log real da origem (results/camp-C4-*).
"""

import re
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path("/Users/huaksonlima/Documents/tcc/aodv-en")
LOG = ROOT / "results" / "camp-C4-aodv-en-r11-ORIGIN.log"
OUT = ROOT / "tcc_latex" / "figuras" / "fig_c4_selfheal.png"
AODV = "#1a365d"
IDEAL = "#999999"
GAP = "#c05621"


def main():
    ts = []
    for line in LOG.read_text(errors="replace").splitlines():
        if "LAT seq=" in line:
            m = re.search(r"I \((\d+)\)", line)
            if m:
                ts.append(int(m.group(1)) / 1000.0)
    if len(ts) < 5:
        print("poucas amostras")
        sys.exit(1)
    t0 = ts[0]
    rel = [t - t0 for t in ts]
    cum = list(range(1, len(rel) + 1))

    fig, ax = plt.subplots(figsize=(8.5, 4.6))
    ax.step(rel, cum, where="post", color=AODV, lw=2, label="Entregas acumuladas (AODV-EN)")
    ideal_t = [0, rel[-1]]
    ideal_y = [0, rel[-1]]
    ax.plot(ideal_t, ideal_y, "--", color=IDEAL, lw=1.5, label="Ideal (1 pacote/s, sem perda)")

    for i in range(1, len(rel)):
        gap = rel[i] - rel[i - 1]
        if gap >= 3:
            ax.axvspan(rel[i - 1], rel[i], color=GAP, alpha=0.15)
    ax.axvspan(0, 0, color=GAP, alpha=0.15, label="Interrupcao (falha do rele + re-descoberta)")

    ax.set_xlabel("Tempo na repeticao (s)")
    ax.set_ylabel("Pacotes entregues (acumulado)")
    ax.set_title("C4 self-healing: entrega ao longo do tempo sob falha periodica de rele on-path")
    ax.grid(True, alpha=0.3)
    ax.legend(loc="upper left", fontsize=9)
    fig.tight_layout()
    fig.savefig(OUT, dpi=140)
    print("gerado:", OUT)
    print(f"  {len(rel)} entregas em {rel[-1]:.1f}s; patamares (gaps>=3s) destacados")


if __name__ == "__main__":
    main()
