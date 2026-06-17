#!/usr/bin/env python3

import argparse
import csv
import json
import os
import subprocess
from collections import defaultdict
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Tuple


@dataclass
class MetricRow:
    scenario: str
    protocol: str
    profile: str
    seed: int
    duration_ms: int
    offered: int
    delivered: int
    pdr: float
    latency_avg_ms: float
    nrl: float
    nrl_control: float
    energy_est_mj: float
    tx_total: int
    rx_total: int
    tx_control: int
    tx_data: int
    tx_failures: int
    route_discoveries: int
    route_repairs: int
    duplicate_rreq_drops: int
    flood_duplicate_drops: int


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run campaign comparing AODV-EN vs flooding baseline and generate summary artifacts."
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=None,
        help="Output directory (default: sim/results/campaign_<UTC_TIMESTAMP>)",
    )
    parser.add_argument(
        "--binary",
        type=Path,
        default=Path("/tmp/aodv_en_campaign_compare"),
        help="Temporary binary path for campaign simulator",
    )
    return parser.parse_args()


def run_cmd(command: List[str], cwd: Path) -> None:
    subprocess.run(command, cwd=str(cwd), check=True)


def compile_campaign_binary(repo_root: Path, binary_path: Path) -> None:
    compile_cmd = [
        "cc",
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-O2",
        "-Ifirmware/components/aodv_en/include",
        "firmware/components/aodv_en/src/aodv_en_mac.c",
        "firmware/components/aodv_en/src/aodv_en_neighbors.c",
        "firmware/components/aodv_en/src/aodv_en_routes.c",
        "firmware/components/aodv_en/src/aodv_en_rreq_cache.c",
        "firmware/components/aodv_en/src/aodv_en_peers.c",
        "firmware/components/aodv_en/src/aodv_en_node.c",
        "sim/campaign_compare.c",
        "-o",
        str(binary_path),
    ]
    run_cmd(compile_cmd, repo_root)


def parse_row(raw: Dict[str, str]) -> MetricRow:
    return MetricRow(
        scenario=raw["scenario"],
        protocol=raw["protocol"],
        profile=raw["profile"],
        seed=int(raw["seed"]),
        duration_ms=int(raw["duration_ms"]),
        offered=int(raw["offered"]),
        delivered=int(raw["delivered"]),
        pdr=float(raw["pdr"]),
        latency_avg_ms=float(raw["latency_avg_ms"]),
        nrl=float(raw["nrl"]),
        nrl_control=float(raw["nrl_control"]),
        energy_est_mj=float(raw["energy_est_mj"]),
        tx_total=int(raw["tx_total"]),
        rx_total=int(raw["rx_total"]),
        tx_control=int(raw["tx_control"]),
        tx_data=int(raw["tx_data"]),
        tx_failures=int(raw["tx_failures"]),
        route_discoveries=int(raw["route_discoveries"]),
        route_repairs=int(raw["route_repairs"]),
        duplicate_rreq_drops=int(raw["duplicate_rreq_drops"]),
        flood_duplicate_drops=int(raw["flood_duplicate_drops"]),
    )


def read_rows(csv_path: Path) -> List[MetricRow]:
    rows: List[MetricRow] = []
    with csv_path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for raw in reader:
            rows.append(parse_row(raw))
    return rows


def aggregate(rows: List[MetricRow], key_fn) -> Dict[Tuple, Dict[str, float]]:
    grouped: Dict[Tuple, List[MetricRow]] = defaultdict(list)
    for row in rows:
        grouped[key_fn(row)].append(row)

    aggregated: Dict[Tuple, Dict[str, float]] = {}
    for key, bucket in grouped.items():
        count = float(len(bucket))
        aggregated[key] = {
            "runs": len(bucket),
            "pdr_avg": sum(r.pdr for r in bucket) / count,
            "latency_avg_ms": sum(r.latency_avg_ms for r in bucket) / count,
            "nrl_avg": sum(r.nrl for r in bucket) / count,
            "nrl_control_avg": sum(r.nrl_control for r in bucket) / count,
            "energy_avg_mj": sum(r.energy_est_mj for r in bucket) / count,
            "offered_avg": sum(r.offered for r in bucket) / count,
            "delivered_avg": sum(r.delivered for r in bucket) / count,
            "tx_total_avg": sum(r.tx_total for r in bucket) / count,
            "tx_control_avg": sum(r.tx_control for r in bucket) / count,
        }
    return aggregated


def choose_best_profile(rows: List[MetricRow]) -> str:
    profile_rows = [r for r in rows if r.protocol == "aodv_en"]
    by_profile = aggregate(profile_rows, key_fn=lambda r: (r.profile,))

    ranking = []
    for (profile,), metrics in by_profile.items():
        ranking.append(
            (
                -metrics["pdr_avg"],
                metrics["latency_avg_ms"],
                metrics["nrl_avg"],
                metrics["energy_avg_mj"],
                profile,
            )
        )

    ranking.sort()
    return ranking[0][4]


def write_csv(path: Path, headers: List[str], rows: List[Dict[str, object]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=headers)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def build_summary(
    rows: List[MetricRow],
    best_profile: str,
    scenario_protocol_agg: Dict[Tuple, Dict[str, float]],
    profile_agg: Dict[Tuple, Dict[str, float]],
    out_dir: Path,
) -> Dict[str, object]:
    scenarios = sorted({r.scenario for r in rows})
    comparison_rows = []

    for scenario in scenarios:
        a_key = (scenario, "aodv_en", best_profile)
        f_key = (scenario, "flooding", "baseline")
        a = scenario_protocol_agg.get(a_key, {})
        f = scenario_protocol_agg.get(f_key, {})
        comparison_rows.append(
            {
                "scenario": scenario,
                "aodv_profile": best_profile,
                "aodv_pdr": round(a.get("pdr_avg", 0.0), 6),
                "flooding_pdr": round(f.get("pdr_avg", 0.0), 6),
                "aodv_latency_ms": round(a.get("latency_avg_ms", 0.0), 3),
                "flooding_latency_ms": round(f.get("latency_avg_ms", 0.0), 3),
                "aodv_nrl": round(a.get("nrl_avg", 0.0), 6),
                "flooding_nrl": round(f.get("nrl_avg", 0.0), 6),
                "aodv_energy_mj": round(a.get("energy_avg_mj", 0.0), 3),
                "flooding_energy_mj": round(f.get("energy_avg_mj", 0.0), 3),
            }
        )

    improvement = {}
    if comparison_rows:
        pdr_delta = [row["aodv_pdr"] - row["flooding_pdr"] for row in comparison_rows]
        latency_delta = [row["aodv_latency_ms"] - row["flooding_latency_ms"] for row in comparison_rows]
        nrl_delta = [row["aodv_nrl"] - row["flooding_nrl"] for row in comparison_rows]
        energy_delta = [row["aodv_energy_mj"] - row["flooding_energy_mj"] for row in comparison_rows]
        improvement = {
            "pdr_delta_avg": sum(pdr_delta) / len(pdr_delta),
            "latency_delta_ms_avg": sum(latency_delta) / len(latency_delta),
            "nrl_delta_avg": sum(nrl_delta) / len(nrl_delta),
            "energy_delta_mj_avg": sum(energy_delta) / len(energy_delta),
        }

    return {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "rows_total": len(rows),
        "best_profile": best_profile,
        "profiles": {
            profile: metrics for (profile,), metrics in profile_agg.items()
        },
        "scenario_comparison_best_profile_vs_flooding": comparison_rows,
        "average_deltas_aodv_minus_flooding": improvement,
        "artifacts": {
            "raw_csv": str((out_dir / "campaign_raw.csv").resolve()),
            "agg_scenario_protocol_csv": str((out_dir / "campaign_agg_scenario_protocol.csv").resolve()),
            "agg_profile_csv": str((out_dir / "campaign_agg_profiles.csv").resolve()),
        },
    }


def write_summary_markdown(path: Path, summary: Dict[str, object]) -> None:
    lines: List[str] = []
    lines.append("# Campanha Comparativa AODV-EN vs Flooding\n")
    lines.append(f"- Gerado em: `{summary['generated_at_utc']}`")
    lines.append(f"- Total de execucoes: `{summary['rows_total']}`")
    lines.append(f"- Melhor perfil AODV para metrica hibrida: `{summary['best_profile']}`\n")

    lines.append("## Perfis AODV (media global)\n")
    lines.append("| Perfil | PDR | Latencia (ms) | NRL | Energia (mJ) |")
    lines.append("|---|---:|---:|---:|---:|")
    profiles = summary.get("profiles", {})
    for profile_name, metrics in sorted(profiles.items()):
        lines.append(
            f"| {profile_name} | {metrics['pdr_avg']:.4f} | {metrics['latency_avg_ms']:.3f} | "
            f"{metrics['nrl_avg']:.4f} | {metrics['energy_avg_mj']:.2f} |"
        )
    lines.append("")

    lines.append("## Comparativo por Cenario (melhor AODV vs Flooding)\n")
    lines.append("| Cenario | AODV PDR | Flooding PDR | AODV Lat(ms) | Flooding Lat(ms) | AODV NRL | Flooding NRL |")
    lines.append("|---|---:|---:|---:|---:|---:|---:|")
    for row in summary.get("scenario_comparison_best_profile_vs_flooding", []):
        lines.append(
            f"| {row['scenario']} | {row['aodv_pdr']:.4f} | {row['flooding_pdr']:.4f} | "
            f"{row['aodv_latency_ms']:.3f} | {row['flooding_latency_ms']:.3f} | "
            f"{row['aodv_nrl']:.4f} | {row['flooding_nrl']:.4f} |"
        )
    lines.append("")

    deltas = summary.get("average_deltas_aodv_minus_flooding", {})
    if deltas:
        lines.append("## Delta medio (AODV - Flooding)\n")
        lines.append(f"- `PDR`: `{deltas['pdr_delta_avg']:+.4f}`")
        lines.append(f"- `Latencia ms`: `{deltas['latency_delta_ms_avg']:+.3f}`")
        lines.append(f"- `NRL`: `{deltas['nrl_delta_avg']:+.4f}`")
        lines.append(f"- `Energia mJ`: `{deltas['energy_delta_mj_avg']:+.2f}`")
        lines.append("")

    lines.append("## Leituras para o Plano C/D/E/F\n")
    lines.append("- `Plano C`: perfil hibrido calibrado por ranking global de PDR/latencia/NRL/energia.")
    lines.append("- `Plano D`: baseline flooding implementado e comparado no mesmo conjunto de cenarios e seeds.")
    lines.append("- `Plano E`: campanha multi-cenario com repeticoes automatizada e reproduzivel.")
    lines.append("- `Plano F`: resultado em formato tabela + deltas prontos para inserir em capitulo de resultados/discussao.")
    lines.append("")

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[1]
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    out_dir = args.out_dir or (repo_root / "sim" / "results" / f"campaign_{timestamp}")
    out_dir.mkdir(parents=True, exist_ok=True)

    raw_csv = out_dir / "campaign_raw.csv"
    binary_path = args.binary

    compile_campaign_binary(repo_root, binary_path)
    run_cmd([str(binary_path), str(raw_csv)], repo_root)

    rows = read_rows(raw_csv)
    if not rows:
        raise SystemExit("campaign produced no rows")

    profile_agg = aggregate([r for r in rows if r.protocol == "aodv_en"], key_fn=lambda r: (r.profile,))
    best_profile = choose_best_profile(rows)

    scenario_protocol_agg = aggregate(
        [r for r in rows if (r.protocol == "flooding" or (r.protocol == "aodv_en" and r.profile == best_profile))],
        key_fn=lambda r: (r.scenario, r.protocol, r.profile),
    )

    agg_scenario_rows = []
    for (scenario, protocol, profile), metrics in sorted(scenario_protocol_agg.items()):
        agg_scenario_rows.append(
            {
                "scenario": scenario,
                "protocol": protocol,
                "profile": profile,
                "runs": metrics["runs"],
                "pdr_avg": metrics["pdr_avg"],
                "latency_avg_ms": metrics["latency_avg_ms"],
                "nrl_avg": metrics["nrl_avg"],
                "nrl_control_avg": metrics["nrl_control_avg"],
                "energy_avg_mj": metrics["energy_avg_mj"],
                "offered_avg": metrics["offered_avg"],
                "delivered_avg": metrics["delivered_avg"],
                "tx_total_avg": metrics["tx_total_avg"],
                "tx_control_avg": metrics["tx_control_avg"],
            }
        )

    agg_profile_rows = []
    for (profile,), metrics in sorted(profile_agg.items()):
        agg_profile_rows.append(
            {
                "profile": profile,
                "runs": metrics["runs"],
                "pdr_avg": metrics["pdr_avg"],
                "latency_avg_ms": metrics["latency_avg_ms"],
                "nrl_avg": metrics["nrl_avg"],
                "nrl_control_avg": metrics["nrl_control_avg"],
                "energy_avg_mj": metrics["energy_avg_mj"],
            }
        )

    write_csv(
        out_dir / "campaign_agg_scenario_protocol.csv",
        headers=list(agg_scenario_rows[0].keys()) if agg_scenario_rows else ["scenario"],
        rows=agg_scenario_rows,
    )
    write_csv(
        out_dir / "campaign_agg_profiles.csv",
        headers=list(agg_profile_rows[0].keys()) if agg_profile_rows else ["profile"],
        rows=agg_profile_rows,
    )

    summary = build_summary(rows, best_profile, scenario_protocol_agg, profile_agg, out_dir)
    (out_dir / "summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    write_summary_markdown(out_dir / "summary.md", summary)

    print(f"campaign output: {out_dir}")
    print(f"best profile: {best_profile}")
    print(f"raw csv: {raw_csv}")


if __name__ == "__main__":
    os.environ.setdefault("LC_ALL", "C")
    main()

