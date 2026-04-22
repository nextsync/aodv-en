#!/usr/bin/env python3

import argparse
import json
import re
from pathlib import Path
from typing import Dict, List

import matplotlib.pyplot as plt
import pandas as pd


def safe_tag(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_-]+", "_", value).strip("_")


def load_summary(path: Path) -> Dict:
    if not path.exists():
        raise FileNotFoundError(f"summary not found: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def build_row(summary_path: Path, label: str, idx: int, change_index: int) -> Dict:
    summary = load_summary(summary_path)
    duration_min = max(0.001, float(summary["timeline"]["duration_min"]))
    duration_h = duration_min / 60.0
    counts = summary["counts"]
    route = summary["route_target_analysis"]

    snapshots_total = int(route["snapshots_with_target"]) + int(route["snapshots_without_target"])
    status_codes = summary.get("data_send_status_by_code", {})
    status_neg2 = int(status_codes.get("-2", 0))

    row = {
        "run_index": idx,
        "run_label": label,
        "summary_path": str(summary_path),
        "post_change": 1 if idx >= change_index else 0,
        "duration_min": duration_min,
        "ack_per_min": float(summary["rates_per_min"]["ack_per_min"]),
        "send_fail_per_min": float(counts["espnow_send_fail"]) / duration_min,
        "status_neg2_per_min": status_neg2 / duration_min,
        "queued_discovery_per_min": float(counts["data_queued_discovery"]) / duration_min,
        "flap_per_hour": float(route["flap_count"]) / duration_h,
        "target_absent_ratio": (float(route["snapshots_without_target"]) / max(1, snapshots_total)),
        "absent_window_max_s": float(route["absent_window_max_ms"]) / 1000.0,
        "hops1_ratio": float(route["hops_1_count"]) / max(1, snapshots_total),
        "hops2_ratio": float(route["hops_2_count"]) / max(1, snapshots_total),
        "ack_missing_estimated": int(counts["ack_missing_estimated"]),
        "ack_out_of_order": int(counts["ack_out_of_order"]),
    }
    return row


def save_plot(fig: plt.Figure, out_path: Path) -> None:
    fig.tight_layout()
    fig.savefig(out_path, dpi=170)
    plt.close(fig)


def style_axis_common(ax: plt.Axes, title: str, ylabel: str) -> None:
    ax.set_title(title)
    ax.set_ylabel(ylabel)
    ax.grid(True, axis="y", alpha=0.25)


def add_change_marker(ax: plt.Axes, df: pd.DataFrame, change_index: int) -> None:
    if change_index <= 0 or change_index >= len(df):
        return
    marker_x = change_index - 0.5
    ax.axvline(marker_x, color="black", linestyle="--", linewidth=1.1)
    ymax = ax.get_ylim()[1]
    ax.text(marker_x + 0.03, ymax * 0.95, "mudanca de codigo", rotation=90, va="top", ha="left", fontsize=9)


def plot_core_metrics(df: pd.DataFrame, out_path: Path, change_index: int) -> None:
    colors = ["#4E79A7" if v == 0 else "#F28E2B" for v in df["post_change"]]
    labels = df["run_label"].tolist()
    x = range(len(df))

    fig, axes = plt.subplots(2, 2, figsize=(13, 8))
    plots = [
        ("ack_per_min", "ACK por Minuto (maior = melhor)", "eventos/min"),
        ("send_fail_per_min", "ESP-NOW Fail por Minuto (menor = melhor)", "eventos/min"),
        (
            "status_neg2_per_min",
            "Fila cheia do buffer (status=-2) por Minuto (menor = melhor)",
            "eventos/min",
        ),
        ("queued_discovery_per_min", "Fila em Discovery por Minuto (menor = melhor)", "eventos/min"),
    ]

    for ax, (key, title, ylabel) in zip(axes.flat, plots):
        ax.bar(x, df[key], color=colors)
        ax.set_xticks(list(x))
        ax.set_xticklabels(labels, rotation=15, ha="right")
        style_axis_common(ax, title, ylabel)
        add_change_marker(ax, df, change_index)

    fig.suptitle("Comparativo de Performance - TC004", fontsize=14, y=1.02)
    save_plot(fig, out_path)


def plot_stability(df: pd.DataFrame, out_path: Path, change_index: int) -> None:
    colors = ["#4E79A7" if v == 0 else "#F28E2B" for v in df["post_change"]]
    labels = df["run_label"].tolist()
    x = range(len(df))

    fig, axes = plt.subplots(1, 3, figsize=(14, 4.5))
    plots = [
        ("flap_per_hour", "Flaps por Hora", "transicoes/h"),
        ("target_absent_ratio", "Rota alvo ausente", "fracao (0..1)"),
        ("absent_window_max_s", "Maior blackout", "segundos"),
    ]

    for ax, (key, title, ylabel) in zip(axes.flat, plots):
        ax.bar(x, df[key], color=colors)
        ax.set_xticks(list(x))
        ax.set_xticklabels(labels, rotation=15, ha="right")
        style_axis_common(ax, title, ylabel)
        add_change_marker(ax, df, change_index)

    fig.suptitle("Comparativo de Estabilidade - TC004", fontsize=14, y=1.03)
    save_plot(fig, out_path)


def plot_route_profile(df: pd.DataFrame, out_path: Path, change_index: int) -> None:
    x = range(len(df))
    labels = df["run_label"].tolist()
    hops2 = df["hops2_ratio"]
    hops1 = df["hops1_ratio"]
    absent = df["target_absent_ratio"]

    fig, ax = plt.subplots(figsize=(11, 4.8))
    ax.bar(x, hops2, label="hops=2", color="#59A14F")
    ax.bar(x, hops1, bottom=hops2, label="hops=1", color="#4E79A7")
    ax.bar(x, absent, bottom=hops2 + hops1, label="absent", color="#E15759")
    ax.set_xticks(list(x))
    ax.set_xticklabels(labels, rotation=15, ha="right")
    ax.set_ylim(0, 1.0)
    style_axis_common(ax, "Perfil de Rota para o Destino Alvo", "fracao de snapshots")
    ax.legend(loc="upper right")
    add_change_marker(ax, df, change_index)
    save_plot(fig, out_path)


def parse_runs(run_args: List[str]) -> List[Dict[str, str]]:
    parsed = []
    for arg in run_args:
        if "::" in arg:
            label, summary = arg.split("::", 1)
        else:
            summary = arg
            label = Path(summary).parent.name
        parsed.append({"label": label, "summary": summary})
    return parsed


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate comparison plots from multiple monitor summary.json files.")
    parser.add_argument(
        "--run",
        action="append",
        required=True,
        help="Format: label::/path/to/summary.json (label optional)",
    )
    parser.add_argument(
        "--change-index",
        type=int,
        default=0,
        help="Index of first run considered post-change (default: 0)",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=None,
        help="Output directory (default: firmware/logs/analysis/comparisons/<auto-tag>)",
    )
    args = parser.parse_args()

    runs = parse_runs(args.run)
    if not runs:
        raise SystemExit("at least one --run is required")
    if args.change_index < 0 or args.change_index > len(runs):
        raise SystemExit(f"--change-index must be in [0, {len(runs)}]")

    repo_root = Path(__file__).resolve().parents[2]
    auto_tag = "__".join(safe_tag(run["label"]) for run in runs)
    out_dir = args.out_dir or (repo_root / "firmware" / "logs" / "analysis" / "comparisons" / auto_tag)
    out_dir.mkdir(parents=True, exist_ok=True)

    rows = []
    for idx, run in enumerate(runs):
        rows.append(build_row(Path(run["summary"]), run["label"], idx, args.change_index))
    df = pd.DataFrame(rows).sort_values("run_index").reset_index(drop=True)
    df.to_csv(out_dir / "comparison_metrics.csv", index=False)

    plot_core_metrics(df, out_dir / "01_core_metrics.png", args.change_index)
    plot_stability(df, out_dir / "02_stability_metrics.png", args.change_index)
    plot_route_profile(df, out_dir / "03_route_profile.png", args.change_index)

    manifest = {
        "out_dir": str(out_dir),
        "change_index": args.change_index,
        "pre_change_labels": df[df["post_change"] == 0]["run_label"].tolist(),
        "post_change_labels": df[df["post_change"] == 1]["run_label"].tolist(),
        "files": [
            str(out_dir / "comparison_metrics.csv"),
            str(out_dir / "01_core_metrics.png"),
            str(out_dir / "02_stability_metrics.png"),
            str(out_dir / "03_route_profile.png"),
        ],
    }
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print(f"comparison generated at: {out_dir}")
    for path in manifest["files"]:
        print(path)


if __name__ == "__main__":
    main()
