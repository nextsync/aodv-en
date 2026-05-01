#!/usr/bin/env zsh

set -euo pipefail

usage() {
  cat <<'EOF'
uso:
  zsh firmware/tests/tc005/build_flash.sh node_a <PORTA> <TARGET_MAC_NODE_D>
  zsh firmware/tests/tc005/build_flash.sh node_b <PORTA>
  zsh firmware/tests/tc005/build_flash.sh node_c <PORTA>
  zsh firmware/tests/tc005/build_flash.sh node_d <PORTA>

exemplos:
  zsh firmware/tests/tc005/build_flash.sh node_a /dev/ttyUSB0 28:05:A5:34:99:34
  zsh firmware/tests/tc005/build_flash.sh node_b /dev/ttyUSB1
  zsh firmware/tests/tc005/build_flash.sh node_c /dev/ttyUSB2
  zsh firmware/tests/tc005/build_flash.sh node_d /dev/ttyUSB3

observacao:
  para node_a, o TARGET_MAC deve ser o MAC do NODE_D (nunca o proprio MAC do NODE_A)
EOF
}

if [[ $# -lt 2 ]]; then
  usage
  exit 1
fi

ROLE="$1"
PORT="$2"
TARGET_MAC="${3:-}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FW_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

case "$ROLE" in
  node_a|node_b|node_c|node_d)
    ;;
  *)
    echo "papel invalido: $ROLE (use node_a, node_b, node_c ou node_d)" >&2
    exit 1
    ;;
esac

if [[ "$ROLE" == "node_a" ]]; then
  if [[ -z "$TARGET_MAC" ]]; then
    echo "node_a exige TARGET_MAC_NODE_D" >&2
    usage
    exit 1
  fi

  if [[ ! "$TARGET_MAC" =~ ^[0-9A-Fa-f]{2}(:[0-9A-Fa-f]{2}){5}$ ]]; then
    echo "TARGET_MAC invalido: $TARGET_MAC" >&2
    exit 1
  fi
fi

source "$FW_DIR/idf-env.sh"

PROFILE="$SCRIPT_DIR/${ROLE}.defaults"
BUILD_DIR="$FW_DIR/build/tc005_${ROLE}"
TMP_OVERRIDES="$(mktemp)"

cleanup() {
  rm -f "$TMP_OVERRIDES"
}
trap cleanup EXIT

if [[ "$ROLE" == "node_a" ]]; then
  UPPER_TARGET_MAC="${TARGET_MAC:u}"
  {
    echo "CONFIG_AODV_EN_APP_TARGET_MAC=\"${UPPER_TARGET_MAC}\""
  } >"$TMP_OVERRIDES"
fi

if [[ -s "$TMP_OVERRIDES" ]]; then
  export SDKCONFIG_DEFAULTS="$FW_DIR/sdkconfig.defaults;$PROFILE;$TMP_OVERRIDES"
else
  export SDKCONFIG_DEFAULTS="$FW_DIR/sdkconfig.defaults;$PROFILE"
fi

export SDKCONFIG="$BUILD_DIR/sdkconfig"

cd "$FW_DIR"
idf.py -B "$BUILD_DIR" set-target esp32 >/dev/null
idf.py -B "$BUILD_DIR" build
idf.py -B "$BUILD_DIR" -p "$PORT" -b 115200 erase-flash flash

echo
echo "ok: $ROLE gravado em $PORT"
echo "monitor:"
echo "  idf.py -B $BUILD_DIR -p $PORT monitor"
