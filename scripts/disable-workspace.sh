#!/usr/bin/env bash
set -euo pipefail
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source "$script_dir/workspace-common.sh"
rm -f -- "$workspace_autostart"
kwriteconfig6 --file linux-rondrc --group Workspace --key Enabled false
workspace_unload
backup="$workspace_root/rollback"
[[ -f $backup/state.json ]] || { printf '%s\n' 'Workspace disabled; no saved panel state.'; exit 0; }
release=$(readlink -f "$workspace_root/current" 2>/dev/null || true)
if [[ ! -f $release/plasma/round-workspace-controls/rollback.js ]]; then release=$(readlink -f "$workspace_root/prepared"); fi
safe_value=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["safeAreaValue"])' "$backup/state.json")
safe_loaded=$(python3 -c 'import json,sys; print(str(json.load(open(sys.argv[1]))["safeAreaLoaded"]).lower())' "$backup/state.json")
if [[ $safe_value == __UNSET__ ]]; then
    kwriteconfig6 --file kwinrc --group Plugins --key round-safe-areaEnabled --delete
else
    kwriteconfig6 --file kwinrc --group Plugins --key round-safe-areaEnabled "$safe_value"
fi
workspace_bus /KWin reconfigure
if [[ $safe_loaded == true ]]; then
    if [[ $(workspace_bus /Scripting org.kde.kwin.Scripting.isScriptLoaded round-safe-area) != true ]]; then
        workspace_bus /Scripting org.kde.kwin.Scripting.loadScript "$workspace_data/kwin/scripts/round-safe-area/contents/code/main.js" round-safe-area >/dev/null
        workspace_bus /Scripting org.kde.kwin.Scripting.start
    fi
else
    workspace_bus /Scripting org.kde.kwin.Scripting.unloadScript round-safe-area >/dev/null
fi
qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript "$(cat "$release/plasma/round-workspace-controls/rollback.js")" >/dev/null
systemctl --user stop plasma-plasmashell.service
trap 'systemctl --user start plasma-plasmashell.service' EXIT
python3 "$script_dir/workspace-backup.py" restore "$backup" "$workspace_data"
systemctl --user start plasma-plasmashell.service
trap - EXIT
mv "$backup" "$workspace_root/rollback-restored-$(date +%s)"
printf '%s\n' 'Workspace disabled; prior panels, packages, and safe-area state restored.'
