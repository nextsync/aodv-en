#!/usr/bin/env zsh

set -euo pipefail

usage() {
  cat <<'EOF'
uso:
  zsh firmware/tests/flood/build_flash.sh <PORTA> <TARGET_MAC>

descricao:
  Builda o firmware no modo flooding controlado (app_flood) e grava na porta.
  TARGET_MAC e o MAC do no destino do flooding (deve estar presente na malha).
  O proprio no destino, ao detectar self==target, vira apenas relay.

exemplo:
  zsh firmware/tests/flood/build_flash.sh /dev/cu.usbserial-214430 28:05:A5:34:99:34
EOF
}

if [[ $# -lt 2 ]]; then
  usage
  exit 1
fi

PORT="$1"
TARGET_MAC="$2"

if [[ ! "$TARGET_MAC" =~ ^[0-9A-Fa-f]{2}(:[0-9A-Fa-f]{2}){5}$ ]]; then
  echo "TARGET_MAC invalido: $TARGET_MAC" >&2
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FW_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

source "$FW_DIR/idf-env.sh"

PROFILE="$SCRIPT_DIR/flood.defaults"
BUILD_DIR="$FW_DIR/build/flood"
TMP_OVERRIDES="$(mktemp)"

cleanup() {
  rm -f "$TMP_OVERRIDES"
}
trap cleanup EXIT

UPPER_TARGET_MAC="${TARGET_MAC:u}"
echo "CONFIG_AODV_EN_APP_TARGET_MAC=\"${UPPER_TARGET_MAC}\"" >"$TMP_OVERRIDES"

export SDKCONFIG_DEFAULTS="$FW_DIR/sdkconfig.defaults;$PROFILE;$TMP_OVERRIDES"
export SDKCONFIG="$BUILD_DIR/sdkconfig"

cd "$FW_DIR"
idf.py -B "$BUILD_DIR" set-target esp32 >/dev/null
idf.py -B "$BUILD_DIR" build
idf.py -B "$BUILD_DIR" -p "$PORT" -b 115200 erase-flash flash

echo
echo "ok: flood gravado em $PORT (target=$UPPER_TARGET_MAC)"
echo "monitor:"
echo "  idf.py -B $BUILD_DIR -p $PORT monitor"
