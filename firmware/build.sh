#!/usr/bin/env zsh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

source "$SCRIPT_DIR/idf-env.sh"

cd "$SCRIPT_DIR"
idf.py set-target esp32 >/dev/null
idf.py build
