#!/usr/bin/env bash
#MISE description="Prepare the local Windows SDK with --apply"
set -euo pipefail
python3 tools/setup-toolchain-WRITES.py "$@"
