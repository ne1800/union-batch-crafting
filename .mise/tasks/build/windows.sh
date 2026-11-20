#!/usr/bin/env bash
#MISE description="Build the diagnostic DLL and Windows x86 test host"
set -euo pipefail
cmake -S . -B build/windows -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/windows-x86-clang.cmake \
  -DUBC_WINDOWS_SDK="$PWD/.tools/xwin"
cmake --build build/windows
