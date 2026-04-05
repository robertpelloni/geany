#!/usr/bin/env sh
set -eu

BUILD_DIR=${1:-build/geany-bobgui}

if ! command -v meson >/dev/null 2>&1; then
  echo "[geany-bobgui] meson is required but was not found in PATH." >&2
  exit 1
fi

meson setup "$BUILD_DIR" variants/geany-bobgui --reconfigure
meson compile -C "$BUILD_DIR"
