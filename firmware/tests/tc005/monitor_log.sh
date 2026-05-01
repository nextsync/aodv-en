#!/usr/bin/env zsh

set -euo pipefail

usage() {
  cat <<'EOF'
uso:
  zsh firmware/tests/tc005/monitor_log.sh node_a <PORTA> [RUN_TAG]
  zsh firmware/tests/tc005/monitor_log.sh node_b <PORTA> [RUN_TAG]
  zsh firmware/tests/tc005/monitor_log.sh node_c <PORTA> [RUN_TAG]
  zsh firmware/tests/tc005/monitor_log.sh node_d <PORTA> [RUN_TAG]

observacao:
  este script e um wrapper para firmware/monitor_log.sh (global)
EOF
}

if [[ $# -lt 2 ]]; then
  usage
  exit 1
fi

ROLE="$1"
PORT="$2"
RUN_TAG="${3:-}"

case "$ROLE" in
  node_a|node_b|node_c|node_d)
    ;;
  *)
    echo "papel invalido: $ROLE (use node_a, node_b, node_c ou node_d)" >&2
    usage
    exit 1
    ;;
esac

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FW_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="build/tc005_${ROLE}"

if [[ -n "$RUN_TAG" ]]; then
  exec zsh "$FW_DIR/monitor_log.sh" -p "$PORT" -B "$BUILD_DIR" -l "$ROLE" -t "$RUN_TAG"
else
  exec zsh "$FW_DIR/monitor_log.sh" -p "$PORT" -B "$BUILD_DIR" -l "$ROLE"
fi
