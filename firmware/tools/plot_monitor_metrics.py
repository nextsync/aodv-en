#!/usr/bin/env python3

import argparse
import json
from pathlib import Path
from typing import List

import matplotlib.pyplot as plt
import pandas as pd


def must_read_csv(path: Path) -> pd.DataFrame:
    if not path.exists():
        raise FileNotFoundError(f"missing CSV: {path}")
    return pd.read_csv(path)


def load_context(analysis_dir: Path) -> dict:
    summary_path = analysis_dir / "summary.json"
    if not summary_path.exists():
        return {}
    return json.loads(summary_path.read_text(encoding="utf-8"))


def save_plot(fig: plt.Figure, path: Path) -> None:
    fig.tight_layout()
    fig.savefig(path, dpi=160)
    plt.close(fig)


def plot_ack_fail_per_minute(minute_df: pd.DataFrame, out: Path) -> None:
    fig, ax = plt.subplots(figsize=(11, 4.5))
    x = minute_df["minute_index"]
    ax.plot(x, minute_df["ack_count"], marker="o", linewidth=1.8, label="ACK/min")
    ax.plot(x, minute_df["send_fail_count"], marker="o", linewidth=1.8, label="ESP-NOW fail/min")
    ax.set_title("ACK vs ESP-NOW Send Fail por Minuto")
    ax.set_xlabel("Minuto")
    ax.set_ylabel("Eventos")
    ax.grid(True, alpha=0.3)
    ax.legend()
    save_plot(fig, out)


def plot_queue_status_per_minute(minute_df: pd.DataFrame, out: Path) -> None:
    fig, ax1 = plt.subplots(figsize=(11, 4.8))
    x = minute_df["minute_index"]

    ax1.bar(x, minute_df["queued_route_count"], label="queued_to_route/min", alpha=0.75)
    ax1.bar(
        x,
        minute_df["queued_discovery_count"],
        bottom=minute_df["queued_route_count"],
        label="queued_discovery/min",
        alpha=0.75,
    )
    ax1.set_xlabel("Minuto")
    ax1.set_ylabel("Eventos de Fila")
    ax1.set_title("Fila de DATA por Minuto (+ fila cheia: status=-2)")
    ax1.grid(True, axis="y", alpha=0.25)

    ax2 = ax1.twinx()
    ax2.plot(
        x,
        minute_df["status_neg2_count"],
        color="black",
        marker="x",
        linewidth=1.5,
        label="fila_cheia_status=-2/min",
    )
    ax2.set_ylabel("fila cheia (status=-2) por minuto")

    h1, l1 = ax1.get_legend_handles_labels()
    h2, l2 = ax2.get_legend_handles_labels()
    ax1.legend(h1 + h2, l1 + l2, loc="upper right")
    save_plot(fig, out)


def plot_route_state_timeline(route_df: pd.DataFrame, out: Path) -> None:
    state_map = {"absent": 0, "hops_1": 1, "hops_2": 2}
    route_df = route_df.copy()
    route_df["state_num"] = route_df["state_label"].map(lambda s: state_map.get(s, 3))

    fig, ax = plt.subplots(figsize=(12, 3.8))
    ax.step(route_df["elapsed_s"], route_df["state_num"], where="post", linewidth=1.5)
    ax.set_yticks([0, 1, 2, 3])
    ax.set_yticklabels(["absent", "hops=1", "hops=2", "other"])
    ax.set_xlabel("Tempo (s)")
    ax.set_ylabel("Estado da rota alvo")
    ax.set_title("Timeline do Estado de Rota para o Destino Alvo")
    ax.grid(True, alpha=0.3)
    save_plot(fig, out)


def plot_route_distribution(route_df: pd.DataFrame, out: Path) -> None:
    counts = route_df["state_label"].value_counts()
    labels = ["hops_2", "hops_1", "absent"]
    values = [int(counts.get(lbl, 0)) for lbl in labels]
    total = max(1, sum(values))
    pct = [(v * 100.0) / total for v in values]

    fig, ax = plt.subplots(figsize=(7.2, 4.5))
    bars = ax.bar(["hops=2", "hops=1", "absent"], values)
    ax.set_title("Distribuicao de Estado da Rota Alvo")
    ax.set_ylabel("Snapshots")
    ax.grid(True, axis="y", alpha=0.25)
    for i, b in enumerate(bars):
        ax.text(b.get_x() + b.get_width() / 2, b.get_height(), f"{values[i]} ({pct[i]:.1f}%)", ha="center", va="bottom")
    save_plot(fig, out)


def plot_neighbors_routes_per_minute(minute_df: pd.DataFrame, out: Path) -> None:
    fig, ax = plt.subplots(figsize=(11, 4.5))
    x = minute_df["minute_index"]
    ax.plot(x, minute_df["avg_neighbors"], marker="o", linewidth=1.8, label="avg_neighbors")
    ax.plot(x, minute_df["avg_routes"], marker="o", linewidth=1.8, label="avg_routes")
    ax.set_title("Media de Neighbors e Rotas por Minuto")
    ax.set_xlabel("Minuto")
    ax.set_ylabel("Media por snapshot no minuto")
    ax.grid(True, alpha=0.3)
    ax.legend()
    save_plot(fig, out)


def plot_discovery_windows(discovery_df: pd.DataFrame, out: Path) -> None:
    fig, ax = plt.subplots(figsize=(11, 4.5))
    if discovery_df.empty:
        ax.text(0.5, 0.5, "Sem janelas de discovery", ha="center", va="center")
        ax.set_axis_off()
        save_plot(fig, out)
        return

    x = list(range(len(discovery_df)))
    duration_s = discovery_df["duration_ms"] / 1000.0
    ax.bar(x, duration_s, alpha=0.85)
    mean_s = duration_s.mean()
    max_s = duration_s.max()
    ax.axhline(mean_s, linestyle="--", linewidth=1.3, label=f"media={mean_s:.2f}s")
    ax.axhline(max_s, linestyle=":", linewidth=1.3, label=f"max={max_s:.2f}s")
    ax.set_title("Duracao das Janelas de Discovery")
    ax.set_xlabel("Indice da janela")
    ax.set_ylabel("Duracao (s)")
    ax.grid(True, axis="y", alpha=0.25)
    ax.legend()
    save_plot(fig, out)


def plot_tx_rx_trend(snapshots_df: pd.DataFrame, out: Path) -> None:
    fig, ax = plt.subplots(figsize=(11, 4.5))
    ax.plot(snapshots_df["elapsed_s"], snapshots_df["tx_delta"], linewidth=1.8, label="tx_delta")
    ax.plot(snapshots_df["elapsed_s"], snapshots_df["rx_delta"], linewidth=1.8, label="rx_delta")
    ax.set_title("Evolucao de TX/RX (delta desde primeiro snapshot)")
    ax.set_xlabel("Tempo (s)")
    ax.set_ylabel("Delta")
    ax.grid(True, alpha=0.3)
    ax.legend()
    save_plot(fig, out)


def build_plots(analysis_dir: Path) -> List[Path]:
    minute_df = must_read_csv(analysis_dir / "minute_metrics.csv")
    route_df = must_read_csv(analysis_dir / "target_route_series.csv")
    discovery_df = must_read_csv(analysis_dir / "discovery_windows.csv")
    snapshots_df = must_read_csv(analysis_dir / "snapshots.csv")
    context = load_context(analysis_dir)

    out_dir = analysis_dir / "plots"
    out_dir.mkdir(parents=True, exist_ok=True)

    generated = []
    plot_specs = [
        ("01_ack_vs_fail_per_minute.png", lambda p: plot_ack_fail_per_minute(minute_df, p)),
        ("02_queue_and_status_per_minute.png", lambda p: plot_queue_status_per_minute(minute_df, p)),
        ("03_route_state_timeline.png", lambda p: plot_route_state_timeline(route_df, p)),
        ("04_route_state_distribution.png", lambda p: plot_route_distribution(route_df, p)),
        ("05_neighbors_routes_per_minute.png", lambda p: plot_neighbors_routes_per_minute(minute_df, p)),
        ("06_discovery_window_durations.png", lambda p: plot_discovery_windows(discovery_df, p)),
        ("07_tx_rx_delta_over_time.png", lambda p: plot_tx_rx_trend(snapshots_df, p)),
    ]

    for name, fn in plot_specs:
        target = out_dir / name
        fn(target)
        generated.append(target)

    manifest = {
        "analysis_dir": str(analysis_dir),
        "plots_dir": str(out_dir),
        "node_name": context.get("context", {}).get("node_name"),
        "self_mac": context.get("context", {}).get("self_mac"),
        "target_mac": context.get("context", {}).get("target_mac"),
        "generated": [str(p) for p in generated],
    }
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return generated


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate plots from monitor metrics analysis directory.")
    parser.add_argument("analysis_dir", type=Path, help="Path produced by extract_monitor_metrics.py")
    args = parser.parse_args()

    analysis_dir = args.analysis_dir
    if not analysis_dir.exists():
        raise SystemExit(f"analysis dir not found: {analysis_dir}")

    generated = build_plots(analysis_dir)
    print(f"plots generated: {len(generated)}")
    for path in generated:
        print(path)


if __name__ == "__main__":
    main()
