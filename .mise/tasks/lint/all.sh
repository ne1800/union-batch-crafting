#!/usr/bin/env bash
#MISE description="Run the repository hooks on all tracked files"
set -euo pipefail
exec prek run --all-files
