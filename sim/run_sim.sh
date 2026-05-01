#!/usr/bin/env zsh

set -euo pipefail

usage() {
  cat <<'EOF'
uso:
  bash sim/run_sim.sh [VARIANT]

variantes:
  basic     3 nos A-B-C, descoberta + retry de ACK + late-join (padrao)
  large     6 nos em cadeia A-B-C-D-E-F, RERR e reconvergencia
  100       grade 10x10, 100 nos, parede central
  1000      grade 32x32, 1024 nos, smart city com falhas (lento)

exemplos:
  bash sim/run_sim.sh
  bash sim/run_sim.sh basic
  bash sim/run_sim.sh large
  bash sim/run_sim.sh 100

observacao:
  variantes 100 e 1000 setam config.route_table_size e config.neighbor_table_size,
  mas esses campos nao crescem os arrays em runtime (sao compile-time em
  AODV_EN_ROUTE_TABLE_SIZE e AODV_EN_NEIGHBOR_TABLE_SIZE em
  firmware/components/aodv_en/include/aodv_en_limits.h). Para experimentos
  realmente grandes, recompile a lib com -DAODV_EN_ROUTE_TABLE_SIZE=N etc.
EOF
}

VARIANT="${1:-basic}"

case "$VARIANT" in
  basic)
    SIM_SRC="sim/aodv_en_sim.c"
    OUT_BIN="/tmp/aodv_en_sim_basic"
    ;;
  large)
    SIM_SRC="sim/aodv_en_sim_large.c"
    OUT_BIN="/tmp/aodv_en_sim_large"
    ;;
  100)
    SIM_SRC="sim/aodv_en_sim_100.c"
    OUT_BIN="/tmp/aodv_en_sim_100"
    ;;
  1000)
    SIM_SRC="sim/aodv_en_sim_1000.c"
    OUT_BIN="/tmp/aodv_en_sim_1000"
    ;;
  -h|--help|help)
    usage
    exit 0
    ;;
  *)
    echo "variante invalida: $VARIANT" >&2
    usage
    exit 1
    ;;
esac

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

cd "$ROOT_DIR"

cc -std=c11 -Wall -Wextra \
    -Ifirmware/components/aodv_en/include \
    firmware/components/aodv_en/src/aodv_en_mac.c \
    firmware/components/aodv_en/src/aodv_en_neighbors.c \
    firmware/components/aodv_en/src/aodv_en_routes.c \
    firmware/components/aodv_en/src/aodv_en_rreq_cache.c \
    firmware/components/aodv_en/src/aodv_en_peers.c \
    firmware/components/aodv_en/src/aodv_en_node.c \
    "$SIM_SRC" \
    -o "$OUT_BIN"

"$OUT_BIN"
