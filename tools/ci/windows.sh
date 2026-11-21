#!/usr/bin/env bash
# Tar preserves xwin's case aliases when the CI cache skips symbolic links.
set -euo pipefail
cache=.cache/ci-sdk.tar.gz
mkdir -p .cache .tools
if [[ -f "$cache" ]]; then
  tar -xzf "$cache" -C .tools
fi
mise run setup:windows-WRITES -- --apply
if [[ ! -f "$cache" ]]; then
  tar -czf "$cache.tmp" -C .tools xwin toolchain-lock.json xwin-0.10.0-x86_64-unknown-linux-musl.tar.gz
  mv "$cache.tmp" "$cache"
fi
mise run build:windows
mkdir -p reports
(
  clang-cl --version
  lld-link --version
  cd build/windows
  sha256sum {player/,}UnionBatchCraftingExtended.dll
  sha256sum --check ../../tools/ci/dll-0.0.59.sha256
) | tee reports/windows-hashes.txt
mise run package:player-WRITES -- --apply
