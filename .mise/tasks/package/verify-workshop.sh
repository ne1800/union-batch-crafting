#!/usr/bin/env bash
#MISE description="Read the Workshop VDF and verify the player pin and licence notices"
set -euo pipefail
exec python3 -B -m tools.package.workshop verify "$@"
