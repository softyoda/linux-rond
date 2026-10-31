#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$root"
python3 -m unittest discover -s tests
for script in scripts/*.sh; do bash -n "$script"; done
git diff --check

if command -v c++ >/dev/null; then
    test_dir=$(mktemp -d "${TMPDIR:-/tmp}/linux-rond-tap-test.XXXXXXXX")
    trap 'rm -rf -- "$test_dir"' EXIT
    c++ -std=c++23 -Wall -Wextra -Werror kwin/touch-wakeup/tapstate_test.cpp -o "$test_dir/tapstate"
    "$test_dir/tapstate"
fi
