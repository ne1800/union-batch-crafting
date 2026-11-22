#!/usr/bin/env bash
#MISE description="Start a version or pin the built DLL hashes with --apply"
set -euo pipefail
exec python3 -B tools/release/prepare-WRITES.py "$@"
