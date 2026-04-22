#!/usr/bin/env python3

import argparse
import csv
import json
import re
from collections import Counter, defaultdict
from pathlib import Path
from typing import Dict, List, Optional

ANSI_RE = re.compile(r"\x1B\[[0-?]*[ -/]*[@-~]")
LOG_RE = re.compile(r"^\s*([IWE]) \((\d+)\) ([^:]+): (.*)$")

NODE_RE = re.compile(
    r"node=([A-Z0-9_]+)\s+self_mac=([0-9A-F:]{17})\s+channel=(\d+)\s+network_id=0x([0-9A-Fa-f]+)"
)
TARGET_RE = re.compile(r"periodic DATA enabled target=([0-9A-F:]{17})")
ACK_RE = re.compile(r"ACK received from ([0-9A-F:]{17}) for seq=(\d+)")
SNAPSHOT_RE = re.compile(r"routes=(\d+)\s+neighbors=(\d+)\s+tx=(\d+)\s+rx=(\d+)\s+delivered=(\d+)")
ROUTE_RE = re.compile(
    r"route\[(\d+)\]\s+dest=([0-9A-F:]{17})\s+via=([0-9A-F:]{17})\s+hops=(\d+)\s+metric=(\d+)\s+state=(\d+)\s+expires=(\d+)"
)
SEND_FAIL_RE = re.compile(r"ESP-NOW send fail to ([0-9A-F:]{17})")
DATA_STATUS_RE = re.compile(r"DATA send status=(-?\d+)")


def strip_ansi(line: str) -> str:
    return ANSI_RE.sub("", line).replace("\r", "").rstrip("\n")


def safe_tag(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_-]+", "_", value).strip("_")


def write_csv(path: Path, rows: List[Dict], headers: List[str]) -> None:
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=headers)
        writer.writeheader()
        for row in rows:
            writer.writerow({k: row.get(k, "") for k in headers})


def compute_target_route_series(
    snapshots: List[Dict],
    routes_by_snapshot: Dict[int, List[Dict]],
    target_mac: Optional[str],
) -> List[Dict]:
    series = []
    for snap in snapshots:
        ts = snap["ts_ms"]
        entries = routes_by_snapshot.get(ts, [])
        target_entry = None
        if target_mac:
            for route in entries:
                if route["dest"] == target_mac:
                    target_entry = route
                    break

        if target_entry is None:
            label = "absent"
            series.append(
                {
                    "ts_ms": ts,
                    "present": 0,
                    "hops": "",
                    "via": "",
                    "metric": "",
                    "state": "",
                    "state_label": label,
                }
            )
            continue

        hops = int(target_entry["hops"])
        if hops == 1:
            label = "hops_1"
        elif hops == 2:
            label = "hops_2"
        else:
            label = f"hops_{hops}"
        series.append(
            {
                "ts_ms": ts,
                "present": 1,
                "hops": hops,
                "via": target_entry["via"],
                "metric": target_entry["metric"],
                "state": target_entry["state"],
                "state_label": label,
            }
        )
    return series


def compute_discovery_windows(events: List[Dict], last_ts: int) -> List[Dict]:
    filtered = [e for e in events if e["event_type"] in {"data_queued_discovery", "data_queued_route", "ack_received"}]
    filtered.sort(key=lambda x: x["ts_ms"])

    windows = []
    open_start = None
    for event in filtered:
        if event["event_type"] == "data_queued_discovery":
            if open_start is None:
                open_start = event["ts_ms"]
            continue

        if open_start is not None and event["event_type"] in {"data_queued_route", "ack_received"}:
            windows.append(
                {
                    "start_ms": open_start,
                    "end_ms": event["ts_ms"],
                    "duration_ms": max(0, event["ts_ms"] - open_start),
                    "closed_by": event["event_type"],
                }
            )
            open_start = None

    if open_start is not None:
        windows.append(
            {
                "start_ms": open_start,
                "end_ms": last_ts,
                "duration_ms": max(0, last_ts - open_start),
                "closed_by": "end_of_log",
            }
        )

    return windows


def add_elapsed(rows: List[Dict], first_ts: int, key: str = "ts_ms") -> None:
    for row in rows:
        if key not in row:
            continue
        ts = row[key]
        try:
            elapsed_ms = int(ts) - first_ts
        except Exception:
            continue
        row["elapsed_ms"] = elapsed_ms
        row["elapsed_s"] = round(elapsed_ms / 1000.0, 3)


def main() -> None:
    parser = argparse.ArgumentParser(description="Extract metrics from ESP-IDF monitor logs for AODV-EN tests.")
    parser.add_argument("log_file", type=Path, help="Path to monitor .log file")
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=None,
        help="Output directory (default: firmware/logs/analysis/<log_basename>)",
    )
    parser.add_argument(
        "--target-mac",
        default=None,
        help="Optional target MAC to focus route analysis (default: detect from log/acks)",
    )
    args = parser.parse_args()

    log_file = args.log_file
    if not log_file.exists():
        raise SystemExit(f"log file not found: {log_file}")

    repo_root = Path(__file__).resolve().parents[2]
    default_out = repo_root / "firmware" / "logs" / "analysis" / safe_tag(log_file.stem)
    out_dir = args.out_dir if args.out_dir else default_out
    out_dir.mkdir(parents=True, exist_ok=True)

    raw_lines = log_file.read_text(encoding="utf-8", errors="replace").splitlines()
    clean_lines = [strip_ansi(line) for line in raw_lines]

    resets = sum(1 for line in clean_lines if "rst:0x" in line)

    events = []
    snapshots = []
    routes = []
    acks = []
    fails = []
    data_status = []

    node_name = None
    self_mac = None
    network_id = None
    channel = None
    detected_target = None
    last_snapshot_ts = None
    first_ts = None
    last_ts = None

    for line in clean_lines:
        if not line:
            continue

        match = LOG_RE.match(line)
        if not match:
            continue

        level, ts_s, tag, message = match.groups()
        ts = int(ts_s)
        if first_ts is None:
            first_ts = ts
        last_ts = ts

        node_match = NODE_RE.search(message)
        if node_match:
            node_name, self_mac, channel_s, network_hex = node_match.groups()
            channel = int(channel_s)
            network_id = f"0x{network_hex.upper()}"

        target_match = TARGET_RE.search(message)
        if target_match:
            detected_target = target_match.group(1)

        snap_match = SNAPSHOT_RE.search(message)
        if snap_match:
            routes_count, neighbors, tx, rx, delivered = snap_match.groups()
            snapshot = {
                "ts_ms": ts,
                "routes": int(routes_count),
                "neighbors": int(neighbors),
                "tx": int(tx),
                "rx": int(rx),
                "delivered": int(delivered),
            }
            snapshots.append(snapshot)
            events.append({"ts_ms": ts, "event_type": "snapshot", "detail": message, "mac": "", "seq": "", "status": ""})
            last_snapshot_ts = ts
            continue

        route_match = ROUTE_RE.search(message)
        if route_match:
            idx, dest, via, hops, metric, state, expires = route_match.groups()
            route = {
                "ts_ms": ts,
                "snapshot_ts": last_snapshot_ts if last_snapshot_ts is not None else "",
                "route_index": int(idx),
                "dest": dest,
                "via": via,
                "hops": int(hops),
                "metric": int(metric),
                "state": int(state),
                "expires_ms": int(expires),
            }
            routes.append(route)
            continue

        ack_match = ACK_RE.search(message)
        if ack_match:
            mac, seq = ack_match.groups()
            row = {"ts_ms": ts, "mac": mac, "seq": int(seq)}
            acks.append(row)
            events.append({"ts_ms": ts, "event_type": "ack_received", "detail": message, "mac": mac, "seq": int(seq), "status": ""})
            continue

        if "DATA queued while route discovery is in progress" in message:
            events.append(
                {"ts_ms": ts, "event_type": "data_queued_discovery", "detail": message, "mac": "", "seq": "", "status": ""}
            )
            continue

        if "DATA queued to route" in message:
            events.append({"ts_ms": ts, "event_type": "data_queued_route", "detail": message, "mac": "", "seq": "", "status": ""})
            continue

        status_match = DATA_STATUS_RE.search(message)
        if status_match:
            status = int(status_match.group(1))
            row = {"ts_ms": ts, "status": status}
            data_status.append(row)
            events.append({"ts_ms": ts, "event_type": "data_send_status", "detail": message, "mac": "", "seq": "", "status": status})
            continue

        fail_match = SEND_FAIL_RE.search(message)
        if fail_match:
            mac = fail_match.group(1)
            row = {"ts_ms": ts, "mac": mac}
            fails.append(row)
            events.append({"ts_ms": ts, "event_type": "espnow_send_fail", "detail": message, "mac": mac, "seq": "", "status": ""})
            continue

    if first_ts is None or last_ts is None:
        raise SystemExit("no timestamped log lines found")

    target_mac = args.target_mac or detected_target
    if target_mac is None and acks:
        target_mac = Counter(a["mac"] for a in acks).most_common(1)[0][0]

    snapshots.sort(key=lambda x: x["ts_ms"])
    routes.sort(key=lambda x: (x["ts_ms"], x["route_index"]))
    acks.sort(key=lambda x: x["ts_ms"])
    events.sort(key=lambda x: x["ts_ms"])

    if snapshots:
        base_tx = snapshots[0]["tx"]
        base_rx = snapshots[0]["rx"]
        base_delivered = snapshots[0]["delivered"]
        for snap in snapshots:
            snap["tx_delta"] = snap["tx"] - base_tx
            snap["rx_delta"] = snap["rx"] - base_rx
            snap["delivered_delta"] = snap["delivered"] - base_delivered

    routes_by_snapshot = defaultdict(list)
    for route in routes:
        snap_ts = route["snapshot_ts"] if route["snapshot_ts"] != "" else route["ts_ms"]
        routes_by_snapshot[int(snap_ts)].append(route)

    target_route_series = compute_target_route_series(snapshots, routes_by_snapshot, target_mac)

    flap_count = 0
    prev_label = None
    transitions = []
    for row in target_route_series:
        label = row["state_label"]
        if prev_label is not None and label != prev_label:
            flap_count += 1
            transitions.append(
                {
                    "ts_ms": row["ts_ms"],
                    "from_state": prev_label,
                    "to_state": label,
                }
            )
        prev_label = label

    absent_windows = []
    open_absent = None
    for row in target_route_series:
        if row["state_label"] == "absent":
            if open_absent is None:
                open_absent = row["ts_ms"]
            continue
        if open_absent is not None:
            absent_windows.append(max(0, row["ts_ms"] - open_absent))
            open_absent = None
    if open_absent is not None:
        absent_windows.append(max(0, last_ts - open_absent))

    discovery_windows = compute_discovery_windows(events, last_ts)

    # Add elapsed time columns for plotting.
    add_elapsed(events, first_ts)
    add_elapsed(snapshots, first_ts)
    add_elapsed(routes, first_ts)
    add_elapsed(target_route_series, first_ts)
    add_elapsed(acks, first_ts)
    add_elapsed(fails, first_ts)
    add_elapsed(data_status, first_ts)
    add_elapsed(discovery_windows, first_ts, key="start_ms")
    add_elapsed(transitions, first_ts)
    for window in discovery_windows:
        window["start_elapsed_s"] = round((window["start_ms"] - first_ts) / 1000.0, 3)
        window["end_elapsed_s"] = round((window["end_ms"] - first_ts) / 1000.0, 3)
    for tr in transitions:
        tr["elapsed_s"] = round((tr["ts_ms"] - first_ts) / 1000.0, 3)

    # Build minute buckets to simplify charts.
    minute_buckets: Dict[int, Dict] = {}

    def ensure_bucket(idx: int) -> Dict:
        if idx not in minute_buckets:
            start_ms = first_ts + idx * 60000
            minute_buckets[idx] = {
                "minute_index": idx,
                "start_ms": start_ms,
                "end_ms": start_ms + 59999,
                "ack_count": 0,
                "send_fail_count": 0,
                "queued_route_count": 0,
                "queued_discovery_count": 0,
                "status_neg2_count": 0,
                "snapshot_count": 0,
                "sum_neighbors": 0,
                "sum_routes": 0,
                "hops1_snapshots": 0,
                "hops2_snapshots": 0,
                "target_absent_snapshots": 0,
            }
        return minute_buckets[idx]

    for ack in acks:
        idx = int((ack["ts_ms"] - first_ts) // 60000)
        ensure_bucket(idx)["ack_count"] += 1

    for fail in fails:
        idx = int((fail["ts_ms"] - first_ts) // 60000)
        ensure_bucket(idx)["send_fail_count"] += 1

    for status in data_status:
        idx = int((status["ts_ms"] - first_ts) // 60000)
        if status["status"] == -2:
            ensure_bucket(idx)["status_neg2_count"] += 1

    for event in events:
        idx = int((event["ts_ms"] - first_ts) // 60000)
        bucket = ensure_bucket(idx)
        if event["event_type"] == "data_queued_route":
            bucket["queued_route_count"] += 1
        elif event["event_type"] == "data_queued_discovery":
            bucket["queued_discovery_count"] += 1

    for snap in snapshots:
        idx = int((snap["ts_ms"] - first_ts) // 60000)
        bucket = ensure_bucket(idx)
        bucket["snapshot_count"] += 1
        bucket["sum_neighbors"] += snap["neighbors"]
        bucket["sum_routes"] += snap["routes"]

    for row in target_route_series:
        idx = int((row["ts_ms"] - first_ts) // 60000)
        bucket = ensure_bucket(idx)
        if row["state_label"] == "hops_1":
            bucket["hops1_snapshots"] += 1
        elif row["state_label"] == "hops_2":
            bucket["hops2_snapshots"] += 1
        elif row["state_label"] == "absent":
            bucket["target_absent_snapshots"] += 1

    minute_metrics = []
    for idx in sorted(minute_buckets.keys()):
        bucket = minute_buckets[idx]
        sc = bucket["snapshot_count"]
        minute_metrics.append(
            {
                "minute_index": bucket["minute_index"],
                "start_ms": bucket["start_ms"],
                "end_ms": bucket["end_ms"],
                "start_elapsed_s": round((bucket["start_ms"] - first_ts) / 1000.0, 3),
                "end_elapsed_s": round((bucket["end_ms"] - first_ts) / 1000.0, 3),
                "ack_count": bucket["ack_count"],
                "send_fail_count": bucket["send_fail_count"],
                "queued_route_count": bucket["queued_route_count"],
                "queued_discovery_count": bucket["queued_discovery_count"],
                "status_neg2_count": bucket["status_neg2_count"],
                "snapshot_count": sc,
                "avg_neighbors": round(bucket["sum_neighbors"] / sc, 4) if sc > 0 else 0.0,
                "avg_routes": round(bucket["sum_routes"] / sc, 4) if sc > 0 else 0.0,
                "hops1_snapshots": bucket["hops1_snapshots"],
                "hops2_snapshots": bucket["hops2_snapshots"],
                "target_absent_snapshots": bucket["target_absent_snapshots"],
            }
        )

    ack_sequences = [row["seq"] for row in acks]
    ack_unique = len(set(ack_sequences))
    ack_duplicates = max(0, len(ack_sequences) - ack_unique)
    ack_out_of_order = 0
    for i in range(1, len(ack_sequences)):
        if ack_sequences[i] < ack_sequences[i - 1]:
            ack_out_of_order += 1

    ack_gap_total = 0
    sorted_unique = sorted(set(ack_sequences))
    for i in range(1, len(sorted_unique)):
        gap = sorted_unique[i] - sorted_unique[i - 1] - 1
        if gap > 0:
            ack_gap_total += gap

    fail_by_mac = Counter(row["mac"] for row in fails)
    status_by_code = Counter(row["status"] for row in data_status)
    event_counts = Counter(row["event_type"] for row in events)

    duration_ms = last_ts - first_ts
    duration_s = duration_ms / 1000.0 if duration_ms > 0 else 0.0
    duration_min = duration_s / 60.0 if duration_s > 0 else 0.0

    tx_delta = snapshots[-1]["tx"] - snapshots[0]["tx"] if len(snapshots) >= 2 else 0
    rx_delta = snapshots[-1]["rx"] - snapshots[0]["rx"] if len(snapshots) >= 2 else 0
    delivered_delta = snapshots[-1]["delivered"] - snapshots[0]["delivered"] if len(snapshots) >= 2 else 0

    summary = {
        "input_log": str(log_file),
        "output_dir": str(out_dir),
        "context": {
            "node_name": node_name,
            "self_mac": self_mac,
            "target_mac": target_mac,
            "network_id": network_id,
            "channel": channel,
        },
        "timeline": {
            "first_ts_ms": first_ts,
            "last_ts_ms": last_ts,
            "duration_ms": duration_ms,
            "duration_s": round(duration_s, 3),
            "duration_min": round(duration_min, 3),
            "reset_events_detected": resets,
        },
        "counts": {
            "snapshots": len(snapshots),
            "route_entries": len(routes),
            "acks": len(acks),
            "ack_unique": ack_unique,
            "ack_duplicates": ack_duplicates,
            "ack_out_of_order": ack_out_of_order,
            "ack_missing_estimated": ack_gap_total,
            "data_queued_to_route": event_counts.get("data_queued_route", 0),
            "data_queued_discovery": event_counts.get("data_queued_discovery", 0),
            "data_send_status_events": len(data_status),
            "espnow_send_fail": len(fails),
        },
        "deltas_from_snapshots": {
            "tx_delta": tx_delta,
            "rx_delta": rx_delta,
            "delivered_delta": delivered_delta,
        },
        "rates_per_min": {
            "ack_per_min": round((len(acks) / duration_min), 3) if duration_min > 0 else 0.0,
            "tx_delta_per_min": round((tx_delta / duration_min), 3) if duration_min > 0 else 0.0,
            "rx_delta_per_min": round((rx_delta / duration_min), 3) if duration_min > 0 else 0.0,
        },
        "route_target_analysis": {
            "target_mac": target_mac,
            "snapshots_with_target": sum(1 for row in target_route_series if row["present"] == 1),
            "snapshots_without_target": sum(1 for row in target_route_series if row["present"] == 0),
            "hops_1_count": sum(1 for row in target_route_series if row["state_label"] == "hops_1"),
            "hops_2_count": sum(1 for row in target_route_series if row["state_label"] == "hops_2"),
            "other_hops_count": sum(
                1 for row in target_route_series if row["state_label"].startswith("hops_") and row["state_label"] not in {"hops_1", "hops_2"}
            ),
            "flap_count": flap_count,
            "absent_windows_count": len(absent_windows),
            "absent_window_max_ms": max(absent_windows) if absent_windows else 0,
            "absent_window_avg_ms": round((sum(absent_windows) / len(absent_windows)), 3) if absent_windows else 0,
        },
        "discovery_windows": {
            "count": len(discovery_windows),
            "max_ms": max((w["duration_ms"] for w in discovery_windows), default=0),
            "avg_ms": round((sum(w["duration_ms"] for w in discovery_windows) / len(discovery_windows)), 3)
            if discovery_windows
            else 0,
        },
        "failures_by_mac": dict(fail_by_mac),
        "data_send_status_by_code": {str(k): v for k, v in status_by_code.items()},
    }

    summary_json = out_dir / "summary.json"
    summary_txt = out_dir / "summary.txt"
    summary_json.write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    txt_lines = [
        f"log: {log_file}",
        f"output_dir: {out_dir}",
        f"node: {node_name} self_mac={self_mac} target={target_mac} network_id={network_id} channel={channel}",
        f"timeline: {first_ts}..{last_ts} ms (duration={duration_ms} ms, {duration_min:.2f} min), resets={resets}",
        f"acks: total={len(acks)} unique={ack_unique} dup={ack_duplicates} out_of_order={ack_out_of_order} missing_estimated={ack_gap_total}",
        f"events: queued_route={event_counts.get('data_queued_route', 0)} queued_discovery={event_counts.get('data_queued_discovery', 0)} "
        f"status_events={len(data_status)} send_fail={len(fails)}",
        f"deltas: tx={tx_delta} rx={rx_delta} delivered={delivered_delta}",
        f"rates/min: ack={summary['rates_per_min']['ack_per_min']} tx_delta={summary['rates_per_min']['tx_delta_per_min']} "
        f"rx_delta={summary['rates_per_min']['rx_delta_per_min']}",
        f"target_route: present={summary['route_target_analysis']['snapshots_with_target']} absent={summary['route_target_analysis']['snapshots_without_target']} "
        f"hops1={summary['route_target_analysis']['hops_1_count']} hops2={summary['route_target_analysis']['hops_2_count']} flaps={flap_count}",
        f"target_absence: windows={summary['route_target_analysis']['absent_windows_count']} "
        f"max_ms={summary['route_target_analysis']['absent_window_max_ms']} "
        f"avg_ms={summary['route_target_analysis']['absent_window_avg_ms']}",
        f"discovery_windows: count={summary['discovery_windows']['count']} max_ms={summary['discovery_windows']['max_ms']} "
        f"avg_ms={summary['discovery_windows']['avg_ms']}",
        f"failures_by_mac: {dict(fail_by_mac)}",
        f"data_send_status_by_code: {summary['data_send_status_by_code']}",
    ]
    summary_txt.write_text("\n".join(txt_lines) + "\n", encoding="utf-8")

    write_csv(
        out_dir / "events.csv",
        events,
        headers=["ts_ms", "elapsed_ms", "elapsed_s", "event_type", "mac", "seq", "status", "detail"],
    )
    write_csv(
        out_dir / "snapshots.csv",
        snapshots,
        headers=[
            "ts_ms",
            "elapsed_ms",
            "elapsed_s",
            "routes",
            "neighbors",
            "tx",
            "rx",
            "delivered",
            "tx_delta",
            "rx_delta",
            "delivered_delta",
        ],
    )
    write_csv(
        out_dir / "routes.csv",
        routes,
        headers=[
            "ts_ms",
            "elapsed_ms",
            "elapsed_s",
            "snapshot_ts",
            "route_index",
            "dest",
            "via",
            "hops",
            "metric",
            "state",
            "expires_ms",
        ],
    )
    write_csv(
        out_dir / "target_route_series.csv",
        target_route_series,
        headers=["ts_ms", "elapsed_ms", "elapsed_s", "present", "hops", "via", "metric", "state", "state_label"],
    )
    write_csv(out_dir / "ack_events.csv", acks, headers=["ts_ms", "elapsed_ms", "elapsed_s", "mac", "seq"])
    write_csv(out_dir / "fail_events.csv", fails, headers=["ts_ms", "elapsed_ms", "elapsed_s", "mac"])
    write_csv(out_dir / "data_status_events.csv", data_status, headers=["ts_ms", "elapsed_ms", "elapsed_s", "status"])
    write_csv(
        out_dir / "discovery_windows.csv",
        discovery_windows,
        headers=[
            "start_ms",
            "end_ms",
            "duration_ms",
            "closed_by",
            "elapsed_ms",
            "elapsed_s",
            "start_elapsed_s",
            "end_elapsed_s",
        ],
    )
    write_csv(
        out_dir / "target_route_transitions.csv",
        transitions,
        headers=["ts_ms", "elapsed_ms", "elapsed_s", "from_state", "to_state"],
    )
    write_csv(
        out_dir / "minute_metrics.csv",
        minute_metrics,
        headers=[
            "minute_index",
            "start_ms",
            "end_ms",
            "start_elapsed_s",
            "end_elapsed_s",
            "ack_count",
            "send_fail_count",
            "queued_route_count",
            "queued_discovery_count",
            "status_neg2_count",
            "snapshot_count",
            "avg_neighbors",
            "avg_routes",
            "hops1_snapshots",
            "hops2_snapshots",
            "target_absent_snapshots",
        ],
    )

    print(f"analysis generated at: {out_dir}")
    print(f"summary: {summary_txt}")
    print(
        "csv: snapshots.csv, routes.csv, target_route_series.csv, target_route_transitions.csv, "
        "ack_events.csv, fail_events.csv, data_status_events.csv, discovery_windows.csv, minute_metrics.csv, events.csv"
    )


if __name__ == "__main__":
    main()
