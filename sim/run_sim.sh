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
  flood     3 nos A-B-C, baseline flooding controlado (TTL + dedup origem,seq)
  compare   sweep em grid (2x2..5x5) AODV-EN vs flooding, saida CSV de metricas

exemplos:
  bash sim/run_sim.sh
  bash sim/run_sim.sh basic
  bash sim/run_sim.sh large
  bash sim/run_sim.sh 100
  bash sim/run_sim.sh flood
  bash sim/run_sim.sh compare

observacao:
  variantes 100 e 1000 setam config.route_table_size e config.neighbor_table_size,
  mas esses campos nao crescem os arrays em runtime (sao compile-time em
  AODV_EN_ROUTE_TABLE_SIZE e AODV_EN_NEIGHBOR_TABLE_SIZE em
  firmware/components/aodv_en/include/aodv_en_limits.h). Para experimentos
  realmente grandes, recompile a lib com -DAODV_EN_ROUTE_TABLE_SIZE=N etc.
EOF
}

VARIANT="${1:-basic}"

AODV_INC=(-Ifirmware/components/aodv_en/include)
AODV_SRCS=(
  firmware/components/aodv_en/src/aodv_en_mac.c
  firmware/components/aodv_en/src/aodv_en_neighbors.c
  firmware/components/aodv_en/src/aodv_en_routes.c
  firmware/components/aodv_en/src/aodv_en_rreq_cache.c
  firmware/components/aodv_en/src/aodv_en_peers.c
  firmware/components/aodv_en/src/aodv_en_node.c
)
FLOOD_INC=(-Ifirmware/components/flood_en/include)
FLOOD_SRCS=(firmware/components/flood_en/src/flood_en.c)

case "$VARIANT" in
  basic)
    SIM_SRC="sim/aodv_en_sim.c"
    OUT_BIN="/tmp/aodv_en_sim_basic"
    INCLUDES=("${AODV_INC[@]}")
    LIB_SRCS=("${AODV_SRCS[@]}")
    ;;
  large)
    SIM_SRC="sim/aodv_en_sim_large.c"
    OUT_BIN="/tmp/aodv_en_sim_large"
    INCLUDES=("${AODV_INC[@]}")
    LIB_SRCS=("${AODV_SRCS[@]}")
    ;;
  100)
    SIM_SRC="sim/aodv_en_sim_100.c"
    OUT_BIN="/tmp/aodv_en_sim_100"
    INCLUDES=("${AODV_INC[@]}")
    LIB_SRCS=("${AODV_SRCS[@]}")
    ;;
  1000)
    SIM_SRC="sim/aodv_en_sim_1000.c"
    OUT_BIN="/tmp/aodv_en_sim_1000"
    INCLUDES=("${AODV_INC[@]}")
    LIB_SRCS=("${AODV_SRCS[@]}")
    ;;
  flood)
    SIM_SRC="sim/flood_en_sim.c"
    OUT_BIN="/tmp/flood_en_sim"
    INCLUDES=("${FLOOD_INC[@]}")
    LIB_SRCS=("${FLOOD_SRCS[@]}")
    ;;
  compare)
    SIM_SRC="sim/compare_sim.c"
    OUT_BIN="/tmp/compare_sim"
    INCLUDES=("${AODV_INC[@]}" "${FLOOD_INC[@]}")
    LIB_SRCS=("${AODV_SRCS[@]}" "${FLOOD_SRCS[@]}")
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
    "${INCLUDES[@]}" \
    "${LIB_SRCS[@]}" \
    "$SIM_SRC" \
    -o "$OUT_BIN"

"$OUT_BIN"
