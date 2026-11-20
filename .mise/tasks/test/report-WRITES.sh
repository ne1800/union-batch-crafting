#!/usr/bin/env bash
#MISE description="Run regression tests and write a summary with --apply, without launching the game"
set -euo pipefail
exec python3 tools/test/test-report-WRITES.py "$@"
