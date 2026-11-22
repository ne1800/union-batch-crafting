#!/usr/bin/env bash
# Check the commits introduced by a push or PR. All-zero base denotes a new branch.
# The pull request title has its own workflow, pr-title.yml.
set -euo pipefail
base=${1:?expected base SHA}
head=${2:?expected head SHA}
[[ "$base" =~ ^[[:xdigit:]]{40}$ && "$head" =~ ^[[:xdigit:]]{40}$ ]]
range=$base..$head
if [[ "$base" =~ ^0+$ ]]; then
  range=$head
fi
git log --format=%s "$range" -- | python3 tools/lint/commit-subject.py --stdin
