#!/usr/bin/env zsh

set -euo pipefail

usage() {
  cat <<'EOF'
uso:
  zsh firmware/monitor_log.sh -p <PORTA> [-B <BUILD_DIR>] [-t <RUN_TAG>] [-l <LABEL>]

opcoes:
  -p <PORTA>      porta serial (obrigatorio), ex.: /dev/ttyUSB0
  -B <BUILD_DIR>  diretorio de build (padrao: build)
  -t <RUN_TAG>    tag opcional da rodada (ex.: tc004_soak)
  -l <LABEL>      label opcional para nome do arquivo (padrao: basename do build dir)
  -h              ajuda

exemplos:
  zsh firmware/monitor_log.sh -p /dev/ttyUSB0
  zsh firmware/monitor_log.sh -p /dev/ttyUSB0 -B build/tc002_node_a -t tc004_soak -l node_a
  zsh firmware/monitor_log.sh -p /dev/ttyUSB1 -B build/tc002_node_b -t tc004_soak -l node_b

saida:
  salva o monitor em firmware/logs/serial/<label>_<timestamp>.log
  ou firmware/logs/serial/<label>_<run_tag>_<timestamp>.log

observacao:
  interrompa com Ctrl-] (atalho do monitor do ESP-IDF)
EOF
}

PORT=""
BUILD_DIR_OPT="build"
RUN_TAG=""
LABEL=""

while getopts ":p:B:t:l:h" opt; do
  case "$opt" in
    p) PORT="$OPTARG" ;;
    B) BUILD_DIR_OPT="$OPTARG" ;;
    t) RUN_TAG="$OPTARG" ;;
    l) LABEL="$OPTARG" ;;
    h)
      usage
      exit 0
      ;;
    :)
      echo "opcao -$OPTARG exige valor" >&2
      usage
      exit 1
      ;;
    \?)
      echo "opcao invalida: -$OPTARG" >&2
      usage
      exit 1
      ;;
  esac
done

if [[ -z "$PORT" ]]; then
  echo "porta serial obrigatoria (-p <PORTA>)" >&2
  usage
  exit 1
fi

FW_DIR="$(cd "$(dirname "$0")" && pwd)"
if [[ "$BUILD_DIR_OPT" == /* ]]; then
  BUILD_DIR="$BUILD_DIR_OPT"
else
  BUILD_DIR="$FW_DIR/$BUILD_DIR_OPT"
fi

if [[ ! -d "$BUILD_DIR" ]]; then
  echo "build nao encontrado: $BUILD_DIR" >&2
  echo "dica: rode build/flash antes (ex.: firmware/tests/tc002/build_flash.sh ...)" >&2
  exit 1
fi

if [[ -z "$LABEL" ]]; then
  LABEL="$(basename "$BUILD_DIR")"
fi

SAFE_LABEL="${LABEL//[^[:alnum:]_-]/_}"
SAFE_TAG="${RUN_TAG//[^[:alnum:]_-]/_}"
TIMESTAMP="$(date +%Y%m%d-%H%M%S)"

LOG_DIR="$FW_DIR/logs/serial"
mkdir -p "$LOG_DIR"

if [[ -n "$SAFE_TAG" ]]; then
  LOG_FILE="$LOG_DIR/${SAFE_LABEL}_${SAFE_TAG}_${TIMESTAMP}.log"
else
  LOG_FILE="$LOG_DIR/${SAFE_LABEL}_${TIMESTAMP}.log"
fi

source "$FW_DIR/idf-env.sh"

echo "build dir: $BUILD_DIR"
echo "porta: $PORT"
echo "gravando monitor em: $LOG_FILE"
echo "interrompa com Ctrl-]"
echo

idf.py -B "$BUILD_DIR" -p "$PORT" monitor 2>&1 | tee "$LOG_FILE"

