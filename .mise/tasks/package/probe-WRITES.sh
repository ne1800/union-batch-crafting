#!/usr/bin/env bash
#MISE description="Write the diagnostic package to dist with --apply"
set -euo pipefail
python3 tools/package/package-WRITES.py "$@"
