#!/bin/bash
# Verification gate — change-scoped (plan 29) on ~/.claude/scripts/gate/scope.sh.
# Builds the slicer when source or build files changed; CMake is reconfigured
# (no -b) when a CMake input changed, since -b would silently skip new files.
set -euo pipefail
cd "$(dirname "$0")/.."
. "$HOME/.claude/scripts/gate/scope.sh"
scope_init
scope_skip_if_unchanged

build_flags=(-x -s -a arm64 -b)
if scope_changed '*CMakeLists.txt' 'cmake/*'; then
  build_flags=(-x -s -a arm64)
fi
if scope_changed 'src/*' '*CMakeLists.txt' 'cmake/*'; then
  echo "== build (./build_release_macos.sh ${build_flags[*]}) =="
  ./build_release_macos.sh "${build_flags[@]}"
fi

scope_uncovered
scope_green
