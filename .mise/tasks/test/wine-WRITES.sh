#!/usr/bin/env bash
#MISE description="Run the DLL load test in a temporary Wine prefix with --apply"
set -euo pipefail
python3 tools/test/smoke-test-WRITES.py "$@"
