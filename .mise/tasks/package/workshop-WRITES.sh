#!/usr/bin/env bash
#MISE description="Build the pinned Workshop VDF in dist with --apply"
set -euo pipefail
exec python3 -B -m tools.package.workshop build "$@"
