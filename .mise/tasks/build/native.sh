#!/usr/bin/env bash
#MISE description="Build the portable core and tests"
set -euo pipefail
cmake -S . -B build/native -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/native
