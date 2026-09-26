#!/usr/bin/env bash
# Builds and runs the planner unit tests. Needs cmake, a C++20 compiler and GoogleTest.
# On NixOS: nix-shell -p cmake gtest --run tests/run.sh
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S tests -B build/tests -DCMAKE_BUILD_TYPE=Debug
cmake --build build/tests -j
ctest --test-dir build/tests --output-on-failure
