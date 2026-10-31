#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
data=${XDG_DATA_HOME:-$HOME/.local/share}
if [[ $(kreadconfig6 --file kwinrc --group Plugins --key round-safe-areaEnabled) != true ]]; then
    qdbus6 org.kde.KWin /LinuxRond org.softyoda.LinuxRond.StateJson 2>/dev/null | python3 -c 'import json,sys; sys.exit(0 if json.load(sys.stdin).get("enabled") else 1)'
fi
diff -qr "$root/plasma/org.softyoda.arcpanel" "$data/plasma/plasmoids/org.softyoda.arcpanel" >/dev/null
diff -qr "$root/kwin/round-safe-area" "$data/kwin/scripts/round-safe-area" >/dev/null
printf '%s\n' 'Linux Rond: installed packages match; window management enabled.'
