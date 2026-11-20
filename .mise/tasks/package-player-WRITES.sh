#!/usr/bin/env bash
#MISE description="Package the permanent player DLL and bilingual guides with --apply"
set -euo pipefail
exec python3 -B tools/package-player-WRITES.py "$@"
