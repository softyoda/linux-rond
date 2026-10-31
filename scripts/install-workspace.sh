#!/usr/bin/env bash
set -euo pipefail
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source "$script_dir/workspace-common.sh"
source_root=$(cd -- "$script_dir/.." && pwd)
mode=${1:---prepare}
[[ $# -le 1 && ( $mode == --prepare || $mode == --enable ) ]] || {
    printf '%s\n' 'Usage: install-workspace.sh [--prepare|--enable]'; exit 2;
}
for tool in cmake c++ python3 kwin_wayland; do command -v "$tool" >/dev/null; done
build=$(mktemp -d "${TMPDIR:-/tmp}/linux-rond-workspace-build.XXXXXXXX")
trap 'rm -rf -- "$build"' EXIT
cmake -S "$source_root/kwin/round-workspace" -B "$build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build" --parallel 2
ctest --test-dir "$build" --output-on-failure
if command -v node >/dev/null; then node "$source_root/plasma/round-workspace-controls/tests/configure_test.js"; fi
mkdir -p "$workspace_root/releases"
release=$(mktemp -d "$workspace_root/releases/build-XXXXXXXX")
mkdir -p "$release/plugins/kwin/effects/plugins"
install -m 644 "$build/linux_rond_workspace.so" "$release/plugins/kwin/effects/plugins/linux_rond_workspace.so"
install -m 644 "$build/linux_rond_workspace_loader.so" "$release/linux_rond_workspace_loader.so"
kwin_wayland --version > "$release/kwin-version"
mkdir -p "$release/plasma"
cp -a "$source_root/plasma/org.softyoda.arcpanel" "$release/plasma/"
cp -a "$source_root/plasma/round-workspace-controls" "$release/plasma/"
for file in workspace-common.sh workspace-backup.py load-workspace.sh disable-workspace.sh; do
    install -m 755 "$script_dir/$file" "$workspace_root/$file"
done
ln -sfn "$release" "$workspace_root/prepared.new"
mv -Tf "$workspace_root/prepared.new" "$workspace_root/prepared"
printf 'Prepared: %s\n' "$release"
if [[ $mode == --enable ]]; then "$workspace_root/load-workspace.sh" --activate-prepared; fi
