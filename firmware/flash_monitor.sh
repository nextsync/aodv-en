#!/usr/bin/env zsh

set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "uso: ./flash_monitor.sh /dev/cu.usbserial-XXXX"
    exit 1
fi

PORT="$1"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

source "$SCRIPT_DIR/idf-env.sh"

cd "$SCRIPT_DIR"
idf.py -p "$PORT" flash monitor
