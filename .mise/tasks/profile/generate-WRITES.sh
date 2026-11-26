#!/usr/bin/env bash
#MISE description="Generate a script profile header from a reference GOTHIC.DAT; writes only with --apply"
set -euo pipefail
python3 -B tools/profile/generate-script-profile-WRITES.py "$@"
