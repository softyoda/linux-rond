#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
data=${XDG_DATA_HOME:-$HOME/.local/share}
[[ $(kreadconfig6 --file kwinrc --group Plugins --key round-safe-areaEnabled) == true ]]
diff -qr "$root/plasma/org.softyoda.arcpanel" "$data/plasma/plasmoids/org.softyoda.arcpanel" >/dev/null
diff -qr "$root/kwin/round-safe-area" "$data/kwin/scripts/round-safe-area" >/dev/null
printf '%s\n' 'Linux Rond: installed packages match; safe area enabled.'
