#!/usr/bin/env zsh

set -euo pipefail

source ~/.zshrc
idfenv >/dev/null

export IDF_COMPONENT_MANAGER=0
