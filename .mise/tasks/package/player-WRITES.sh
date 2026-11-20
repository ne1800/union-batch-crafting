#!/usr/bin/env bash
#MISE description="Package the permanent player DLL and bilingual guides with --apply"
set -euo pipefail
exec python3 -B tools/package/package-player-WRITES.py "$@"
