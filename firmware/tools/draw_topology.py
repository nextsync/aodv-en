#!/usr/bin/env python3

import argparse
import csv
import json
import re
import subprocess
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Optional, Tuple


def safe_tag(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_-]+", "_", value).strip("_")


def mac_key(mac: str) -> str:
    return (mac or "").strip().upper()


def mac_short(mac: str) -> str:
    m = mac_key(mac)
    if len(m) != 17:
        return m
    return m[-5:]


def node_id(mac: str) -> str:
    clean = mac_key(mac).replace(":", "")
    if not clean:
        clean = "UNKNOWN"
    return f"N_{clean}"


def load_summary(path: Path) -> Dict:
    if not path.exists():
        raise FileNotFoundError(f"summary.json not found: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def load_routes(path: Path) -> List[Dict]:
    if not path.exists():
        raise FileNotFoundError(f"routes.csv not found: {path}")

    rows: List[Dict] = []
    with path.open("r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            row["dest"] = mac_key(row.get("dest", ""))
            row["via"] = mac_key(row.get("via", ""))
            row["snapshot_ts"] = int(row.get("snapshot_ts") or 0)
            row["hops"] = int(row.get("hops") or 0)
            row["metric"] = int(row.get("metric") or 0)
            row["state"] = int(row.get("state") or 0)
            rows.append(row)
    return rows


def select_snapshot_ts(routes: List[Dict], requested: Optional[int]) -> int:
    if not routes:
        return 0

    snapshots = sorted({r["snapshot_ts"] for r in routes if r["snapshot_ts"] > 0})
    if not snapshots:
        return 0

    if requested is None:
        return snapshots[-1]

    # Pick the closest snapshot <= requested, else the earliest.
    prior = [ts for ts in snapshots if ts <= requested]
    if prior:
        return prior[-1]
    return snapshots[0]


def label_for_mac(mac: str, known_nodes: Dict[str, str]) -> str:
    m = mac_key(mac)
    if not m:
        return "UNKNOWN"
    if m in known_nodes:
        return f"{known_nodes[m]}\\n{m}"
    return f"{mac_short(m)}\\n{m}"


def build_topology(
    analysis_dirs: List[Path],
    snapshot_ts: Optional[int],
    mode: str,
) -> Tuple[Dict[str, str], Dict[Tuple[str, str, str], Dict], List[Dict]]:
    known_nodes: Dict[str, str] = {}
    edges: Dict[Tuple[str, str, str], Dict] = {}
    context_rows: List[Dict] = []

    loaded = []
    for analysis_dir in analysis_dirs:
        summary = load_summary(analysis_dir / "summary.json")
        routes = load_routes(analysis_dir / "routes.csv")
        loaded.append((analysis_dir, summary, routes))

        ctx = summary.get("context", {})
        self_mac = mac_key(ctx.get("self_mac", ""))
        node_name = (ctx.get("node_name") or analysis_dir.name).strip()
        if self_mac:
            known_nodes[self_mac] = node_name

    for analysis_dir, summary, routes in loaded:
        ctx = summary.get("context", {})
        self_mac = mac_key(ctx.get("self_mac", ""))
        node_name = known_nodes.get(self_mac, analysis_dir.name)
        selected_snapshot = select_snapshot_ts(routes, snapshot_ts)
        if mode == "observed":
            selected_rows = list(routes)
        else:
            selected_rows = [r for r in routes if r["snapshot_ts"] == selected_snapshot]

        context_rows.append(
            {
                "analysis_dir": str(analysis_dir),
                "node_name": node_name,
                "self_mac": self_mac,
                "snapshot_ts": selected_snapshot,
                "routes_at_snapshot": len(selected_rows),
                "mode": mode,
            }
        )

        if not self_mac:
            continue

        for row in selected_rows:
            dest = mac_key(row["dest"])
            via = mac_key(row["via"])
            hops = int(row["hops"])
            metric = int(row["metric"])
            state = int(row["state"])

            if not dest or not via:
                continue

            key_nh = (self_mac, via, "next_hop")
            info_nh = edges.setdefault(
                key_nh,
                {
                    "src": self_mac,
                    "dst": via,
                    "kind": "next_hop",
                    "destinations": set(),
                    "max_hops": 0,
                    "count": 0,
                },
            )
            info_nh["destinations"].add(dest)
            info_nh["max_hops"] = max(info_nh["max_hops"], hops)
            info_nh["count"] += 1

            # Multi-hop inference: we only know next hop and final dest from this node view.
            if hops > 1 and via != dest:
                key_inf = (via, dest, "inferred")
                info_inf = edges.setdefault(
                    key_inf,
                    {
                        "src": via,
                        "dst": dest,
                        "kind": "inferred",
                        "destinations": set([dest]),
                        "max_hops": hops,
                        "count": 0,
                    },
                )
                info_inf["max_hops"] = max(info_inf["max_hops"], hops)
                info_inf["count"] += 1

            # Store extra nodes in map with generic labels if unknown.
            if via not in known_nodes:
                known_nodes[via] = mac_short(via)
            if dest not in known_nodes:
                known_nodes[dest] = mac_short(dest)

    return known_nodes, edges, context_rows


def render_mermaid(nodes: Dict[str, str], edges: Dict[Tuple[str, str, str], Dict]) -> str:
    lines = ["graph TD"]

    for mac, name in sorted(nodes.items()):
        nid = node_id(mac)
        label = label_for_mac(mac, nodes)
        lines.append(f'    {nid}["{label}"]')

    for _, info in sorted(edges.items(), key=lambda kv: (kv[1]["src"], kv[1]["dst"], kv[1]["kind"])):
        src = node_id(info["src"])
        dst = node_id(info["dst"])
        if info["kind"] == "next_hop":
            label = f"next-hop ({len(info['destinations'])} dest)"
            lines.append(f"    {src} -->|{label}| {dst}")
        else:
            label = f"infer h<={info['max_hops']}"
            lines.append(f"    {src} -.->|{label}| {dst}")

    return "\n".join(lines) + "\n"


def render_dot(nodes: Dict[str, str], edges: Dict[Tuple[str, str, str], Dict]) -> str:
    lines = [
        "digraph AODV_EN_TOPOLOGY {",
        '  rankdir=LR;',
        '  node [shape=box, style="rounded,filled", fillcolor="#f7f7f7", color="#444444"];',
        '  edge [color="#444444"];',
    ]

    for mac in sorted(nodes.keys()):
        nid = node_id(mac)
        label = label_for_mac(mac, nodes).replace("\\n", "\\n")
        lines.append(f'  {nid} [label="{label}"];')

    for _, info in sorted(edges.items(), key=lambda kv: (kv[1]["src"], kv[1]["dst"], kv[1]["kind"])):
        src = node_id(info["src"])
        dst = node_id(info["dst"])
        if info["kind"] == "next_hop":
            label = f"next-hop ({len(info['destinations'])} dest)"
            lines.append(f'  {src} -> {dst} [label="{label}", penwidth=1.6];')
        else:
            label = f"infer h<={info['max_hops']}"
            lines.append(f'  {src} -> {dst} [label="{label}", style=dashed, color="#888888"];')

    lines.append("}")
    return "\n".join(lines) + "\n"


def try_render_svg(dot_path: Path, svg_path: Path) -> bool:
    try:
        result = subprocess.run(
            ["dot", "-Tsvg", str(dot_path), "-o", str(svg_path)],
            check=False,
            capture_output=True,
            text=True,
        )
    except FileNotFoundError:
        return False

    return result.returncode == 0 and svg_path.exists()


def main() -> None:
    parser = argparse.ArgumentParser(description="Draw AODV-EN topology from analysis directories.")
    parser.add_argument(
        "analysis_dir",
        nargs="+",
        type=Path,
        help="One or more analysis dirs generated by extract_monitor_metrics.py",
    )
    parser.add_argument(
        "--snapshot-ts",
        type=int,
        default=None,
        help="Optional snapshot ts_ms to render (default: latest snapshot in each analysis dir).",
    )
    parser.add_argument(
        "--mode",
        choices=["latest", "observed"],
        default="latest",
        help="latest = only selected snapshot, observed = union of all snapshots in each analysis dir.",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=None,
        help="Output directory (default: first analysis dir / topology).",
    )
    parser.add_argument(
        "--name",
        default="topology",
        help="Base name for output files (default: topology).",
    )
    args = parser.parse_args()

    analysis_dirs = [p.resolve() for p in args.analysis_dir]
    for p in analysis_dirs:
        if not p.exists():
            raise SystemExit(f"analysis dir not found: {p}")

    default_out = analysis_dirs[0] / "topology"
    out_dir = args.out_dir.resolve() if args.out_dir else default_out
    out_dir.mkdir(parents=True, exist_ok=True)

    nodes, edges, context_rows = build_topology(analysis_dirs, args.snapshot_ts, args.mode)
    if not nodes:
        raise SystemExit("no topology data found in provided analysis dirs")

    mermaid_text = render_mermaid(nodes, edges)
    dot_text = render_dot(nodes, edges)

    base = safe_tag(args.name) or "topology"
    mermaid_path = out_dir / f"{base}.mmd"
    dot_path = out_dir / f"{base}.dot"
    svg_path = out_dir / f"{base}.svg"
    json_path = out_dir / f"{base}.json"

    mermaid_path.write_text(mermaid_text, encoding="utf-8")
    dot_path.write_text(dot_text, encoding="utf-8")

    svg_generated = try_render_svg(dot_path, svg_path)
    if not svg_generated and svg_path.exists():
        svg_path.unlink()

    # Make JSON summary serializable.
    edge_rows = []
    for info in edges.values():
        edge_rows.append(
            {
                "src": info["src"],
                "dst": info["dst"],
                "kind": info["kind"],
                "destinations": sorted(info["destinations"]),
                "max_hops": int(info["max_hops"]),
                "count": int(info["count"]),
            }
        )

    summary = {
        "analysis_dirs": [str(p) for p in analysis_dirs],
        "snapshot_ts_requested": args.snapshot_ts,
        "mode": args.mode,
        "contexts": context_rows,
        "node_count": len(nodes),
        "edge_count": len(edge_rows),
        "outputs": {
            "mermaid": str(mermaid_path),
            "dot": str(dot_path),
            "svg": str(svg_path) if svg_generated else None,
        },
        "nodes": [{"mac": mac, "name": name} for mac, name in sorted(nodes.items())],
        "edges": sorted(edge_rows, key=lambda e: (e["src"], e["dst"], e["kind"])),
    }
    json_path.write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print(f"topology generated at: {out_dir}")
    print(f"mermaid: {mermaid_path}")
    print(f"dot: {dot_path}")
    if svg_generated:
        print(f"svg: {svg_path}")
    else:
        print("svg: not generated (graphviz 'dot' not available)")
    print(f"summary: {json_path}")


if __name__ == "__main__":
    main()
