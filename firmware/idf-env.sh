#!/usr/bin/env zsh

# Shared ESP-IDF bootstrap for firmware scripts.
# Resolution order:
# 1) ESP_IDF_EXPORT
# 2) IDF_PATH/export.sh
# 3) $HOME/esp/esp-idf/export.sh
# 4) idf.py in PATH

aodv_en_idf_env_die() {
  echo "[aodv-en] $*" >&2
  return 1 2>/dev/null || exit 1
}

aodv_en_find_idf_export() {
  if [[ -n "${ESP_IDF_EXPORT:-}" ]]; then
    printf '%s\n' "${ESP_IDF_EXPORT}"
    return 0
  fi

  if [[ -n "${IDF_PATH:-}" && -f "${IDF_PATH}/export.sh" ]]; then
    printf '%s\n' "${IDF_PATH}/export.sh"
    return 0
  fi

  if [[ -f "${HOME}/esp/esp-idf/export.sh" ]]; then
    printf '%s\n' "${HOME}/esp/esp-idf/export.sh"
    return 0
  fi

  return 1
}

if [[ "${AODV_EN_IDF_ENV_LOADED:-0}" != "1" ]]; then
  IDF_EXPORT_SCRIPT=""

  if [[ -n "${ESP_IDF_EXPORT:-}" && ! -f "${ESP_IDF_EXPORT}" ]]; then
    aodv_en_idf_env_die "ESP_IDF_EXPORT points to a missing file: ${ESP_IDF_EXPORT}"
    return 1 2>/dev/null || exit 1
  fi

  if IDF_EXPORT_SCRIPT="$(aodv_en_find_idf_export)"; then
    source "${IDF_EXPORT_SCRIPT}" >/dev/null
  elif ! command -v idf.py >/dev/null 2>&1; then
    aodv_en_idf_env_die "could not find ESP-IDF environment. Set ESP_IDF_EXPORT or IDF_PATH."
    return 1 2>/dev/null || exit 1
  fi

  if ! command -v idf.py >/dev/null 2>&1; then
    aodv_en_idf_env_die "idf.py not found after ESP-IDF bootstrap."
    return 1 2>/dev/null || exit 1
  fi

  export IDF_COMPONENT_MANAGER=0
  export AODV_EN_IDF_ENV_LOADED=1
fi

unset -f aodv_en_find_idf_export
unset -f aodv_en_idf_env_die
