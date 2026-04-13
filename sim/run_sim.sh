#!/usr/bin/env zsh

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT_BIN="/tmp/aodv_en_sim"

cd "$ROOT_DIR"

cc -std=c11 -Wall -Wextra \
    -Ifirmware/components/aodv_en/include \
    firmware/components/aodv_en/src/aodv_en_mac.c \
    firmware/components/aodv_en/src/aodv_en_neighbors.c \
    firmware/components/aodv_en/src/aodv_en_routes.c \
    firmware/components/aodv_en/src/aodv_en_rreq_cache.c \
    firmware/components/aodv_en/src/aodv_en_peers.c \
    firmware/components/aodv_en/src/aodv_en_node.c \
    sim/aodv_en_sim.c \
    -o "$OUT_BIN"

"$OUT_BIN"
