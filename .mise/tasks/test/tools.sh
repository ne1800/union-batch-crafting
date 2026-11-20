#!/usr/bin/env bash
#MISE description="Test the write and cleanup boundaries of helper scripts"
set -euo pipefail
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests -p 'test_*.py' -v
