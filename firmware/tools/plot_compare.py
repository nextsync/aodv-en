#!/usr/bin/env python3
"""
plot_compare.py - gera graficos comparando AODV-EN vs flooding controlado.

Entrada: CSV produzido por `bash sim/run_sim.sh compare` (sweep em grid).
  colunas: protocol,side,nodes,hops,sent,delivered,ack,total_tx,tx_per_delivered

Saidas (docs/evidencias/):
  m5-tx-por-entrega.png   - linha: tx/entrega vs numero de nos (sim) + ponto hardware (hub)
  m5-tx-barras.png        - barras agrupadas: tx/entrega por tamanho de grid
  m5-entrega.png          - taxa de entrega por tamanho de grid

Uso:
  python3 firmware/tools/plot_compare.py [caminho_csv]
  (default: roda sim/run_sim.sh compare e usa a saida)
"""

import csv
import io
import subprocess
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "docs" / "evidencias"

HARDWARE_HUB = {
    "nodes": 3,
    "aodv_tx_per_delivered": 5.33,
    "flood_tx_per_delivered": 4.00,
}

AODV_COLOR = "#1a365d"
FLOOD_COLOR = "#c05621"


def load_csv(path):
    if path is not None:
        text = Path(path).read_text(encoding="utf-8")
    else:
        proc = subprocess.run(
            ["bash", str(ROOT / "sim" / "run_sim.sh"), "compare"],
            capture_output=True,
            text=True,
            check=True,
        )
        text = proc.stdout

    rows = list(csv.DictReader(io.StringIO(text)))
    aodv = [r for r in rows if r["protocol"] == "aodv"]
    flood = [r for r in rows if r["protocol"] == "flood"]
    aodv.sort(key=lambda r: int(r["nodes"]))
    flood.sort(key=lambda r: int(r["nodes"]))
    return aodv, flood


def plot_tx_line(aodv, flood):
    fig, ax = plt.subplots(figsize=(8, 5))
    ax.plot(
        [int(r["nodes"]) for r in aodv],
        [float(r["tx_per_delivered"]) for r in aodv],
        marker="o",
        color=AODV_COLOR,
        label="AODV-EN (sim grid)",
        linewidth=2,
    )
    ax.plot(
        [int(r["nodes"]) for r in flood],
        [float(r["tx_per_delivered"]) for r in flood],
        marker="s",
        color=FLOOD_COLOR,
        label="Flooding controlado (sim grid)",
        linewidth=2,
    )
    ax.scatter(
        [HARDWARE_HUB["nodes"]],
        [HARDWARE_HUB["aodv_tx_per_delivered"]],
        color=AODV_COLOR,
        marker="*",
        s=220,
        edgecolor="black",
        zorder=5,
        label="AODV-EN (hardware hub, 1 hop)",
    )
    ax.scatter(
        [HARDWARE_HUB["nodes"]],
        [HARDWARE_HUB["flood_tx_per_delivered"]],
        color=FLOOD_COLOR,
        marker="*",
        s=220,
        edgecolor="black",
        zorder=5,
        label="Flooding (hardware hub, 1 hop)",
    )
    ax.set_xlabel("Numero de nos")
    ax.set_ylabel("Transmissoes por entrega")
    ax.set_title("Custo de canal: AODV-EN vs Flooding (entrega 100% em ambos)")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    out = OUT_DIR / "m5-tx-por-entrega.png"
    fig.savefig(out, dpi=130)
    plt.close(fig)
    return out


def plot_tx_bars(aodv, flood):
    labels = [f"{r['side']}x{r['side']}\n({r['nodes']} nos)" for r in aodv]
    x = range(len(labels))
    width = 0.38
    fig, ax = plt.subplots(figsize=(8, 5))
    ax.bar(
        [i - width / 2 for i in x],
        [float(r["tx_per_delivered"]) for r in aodv],
        width,
        color=AODV_COLOR,
        label="AODV-EN",
    )
    ax.bar(
        [i + width / 2 for i in x],
        [float(r["tx_per_delivered"]) for r in flood],
        width,
        color=FLOOD_COLOR,
        label="Flooding",
    )
    ax.set_xticks(list(x))
    ax.set_xticklabels(labels)
    ax.set_ylabel("Transmissoes por entrega")
    ax.set_title("Transmissoes por entrega por tamanho de grid")
    ax.grid(True, axis="y", alpha=0.3)
    ax.legend()
    fig.tight_layout()
    out = OUT_DIR / "m5-tx-barras.png"
    fig.savefig(out, dpi=130)
    plt.close(fig)
    return out


def plot_delivery(aodv, flood):
    labels = [f"{r['side']}x{r['side']}" for r in aodv]
    x = range(len(labels))
    width = 0.38

    def ratio(rows):
        return [100.0 * int(r["delivered"]) / max(int(r["sent"]), 1) for r in rows]

    fig, ax = plt.subplots(figsize=(8, 5))
    ax.bar([i - width / 2 for i in x], ratio(aodv), width, color=AODV_COLOR, label="AODV-EN")
    ax.bar([i + width / 2 for i in x], ratio(flood), width, color=FLOOD_COLOR, label="Flooding")
    ax.set_xticks(list(x))
    ax.set_xticklabels(labels)
    ax.set_ylabel("Taxa de entrega (%)")
    ax.set_ylim(0, 110)
    ax.set_title("Taxa de entrega por tamanho de grid")
    ax.grid(True, axis="y", alpha=0.3)
    ax.legend()
    fig.tight_layout()
    out = OUT_DIR / "m5-entrega.png"
    fig.savefig(out, dpi=130)
    plt.close(fig)
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else None
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    aodv, flood = load_csv(path)
    outs = [plot_tx_line(aodv, flood), plot_tx_bars(aodv, flood), plot_delivery(aodv, flood)]
    for o in outs:
        print(f"gerado: {o}")


if __name__ == "__main__":
    main()
