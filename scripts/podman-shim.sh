#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

set -euo pipefail

image="${CARTOMANCER_IMAGE:-cartomancer-builder}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# ARCANA_SOURCE builds against a local libarcana checkout instead of the pinned tag
arcana_mount=()
if [ -n "${ARCANA_SOURCE:-}" ]; then
  if [ ! -f "$ARCANA_SOURCE/CMakeLists.txt" ]; then
    echo "ARCANA_SOURCE is not a libarcana checkout: $ARCANA_SOURCE" >&2
    exit 1
  fi
  arcana_mount=(-v "$(cd "$ARCANA_SOURCE" && pwd):/opt/arcana-src:ro,z"
    -e CARTOMANCER_ARCANA_SOURCE=/opt/arcana-src)
fi

# this is a file that `just` creates from the recipe body and passes to us.
script="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"

# forward a tty when there is one so we get color and stuff
tty_flag=()
[ -t 0 ] && [ -t 1 ] && tty_flag=(-t)

exec podman run --rm -i "${tty_flag[@]}" \
  -v "$root:/src:Z" \
  "${arcana_mount[@]}" \
  -v cartomancer-conan:/root/.conan2 \
  -v "$script:/tmp/recipe.sh:ro,z" \
  -w /src \
  "$image" \
  bash -euo pipefail /tmp/recipe.sh
