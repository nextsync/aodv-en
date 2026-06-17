#!/usr/bin/env python3
"""serial_capture.py - captura serial exclusiva de N portas em paralelo.

Cada porta e aberta (exclusiva), lida por DURATION segundos e fechada.
Sem monitor interativo. Saidas em results/.

Uso:
  serial_capture.py --duration 60 --out-prefix m10-aodv-s4 \
      N1:/dev/cu.usbserial-214420 N2:/dev/cu.usbserial-214430 N3:/dev/cu.usbserial-214440
Saidas:
  results/<out-prefix>-<label>.log  (uma por porta)
"""

import argparse
import threading
import time
from pathlib import Path

import serial

ROOT = Path("/Users/huaksonlima/Documents/tcc/aodv-en")
OUTDIR = ROOT / "results"


def capture(label, port, duration, baud, started, results):
    out = OUTDIR / f"{started}-{label}.log"
    lines = 0
    try:
        with serial.Serial(port, baud, timeout=1, exclusive=True) as ser, open(out, "w") as fh:
            deadline = time.monotonic() + duration
            while time.monotonic() < deadline:
                raw = ser.readline()
                if not raw:
                    continue
                try:
                    txt = raw.decode("utf-8", errors="replace")
                except Exception:
                    txt = repr(raw) + "\n"
                fh.write(txt)
                fh.flush()
                lines += 1
        results[label] = (str(out), lines)
    except Exception as exc:
        results[label] = (str(out), f"ERRO: {exc}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--duration", type=int, default=60)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--out-prefix", required=True)
    ap.add_argument("ports", nargs="+", help="label:port pairs")
    args = ap.parse_args()

    OUTDIR.mkdir(parents=True, exist_ok=True)
    pairs = []
    for p in args.ports:
        label, _, port = p.partition(":")
        pairs.append((label, port))

    results = {}
    threads = []
    for label, port in pairs:
        t = threading.Thread(target=capture,
                             args=(label, port, args.duration, args.baud, args.out_prefix, results))
        t.start()
        threads.append(t)
    for t in threads:
        t.join()

    for label, port in pairs:
        path, info = results.get(label, ("?", "sem resultado"))
        print(f"{label} {port} -> {path} ({info} linhas)")


if __name__ == "__main__":
    main()
