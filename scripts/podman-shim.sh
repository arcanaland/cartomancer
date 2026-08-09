#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

set -euo pipefail

image="${CARTOMANCER_IMAGE:-cartomancer-builder}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# TODO: replace when we get conan set up
arcana_prefix="${ARCANA_PREFIX:-$root/../libarcana/build/RelWithDebInfo/stage/usr/local}"

if [ ! -d "$arcana_prefix/lib/cmake/arcana" ]; then
  echo "no arcana install prefix at: $arcana_prefix" >&2
  exit 1
fi

arcana_prefix="$(cd "$arcana_prefix" && pwd)"

# this is a file that `just` creates from the recipe body and passes to us.
script="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"

# forward a tty when there is one so we get color and stuff
tty_flag=()
[ -t 0 ] && [ -t 1 ] && tty_flag=(-t)

exec podman run --rm -i "${tty_flag[@]}" \
  -v "$root:/src:Z" \
  -v "$arcana_prefix:/opt/arcana:ro,z" \
  -v cartomancer-conan:/root/.conan2 \
  -v "$script:/tmp/recipe.sh:ro,z" \
  -w /src \
  "$image" \
  bash -euo pipefail /tmp/recipe.sh
