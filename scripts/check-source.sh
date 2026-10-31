#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$root"
python3 -m unittest discover -s tests
for script in scripts/*.sh; do bash -n "$script"; done
git diff --check
