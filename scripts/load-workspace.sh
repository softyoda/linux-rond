#!/usr/bin/env bash
set -euo pipefail
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source "$script_dir/workspace-common.sh"
mode=${1:---resume}
[[ $# -le 1 && ( $mode == --resume || $mode == --activate-prepared ) ]] || exit 2
release=$(readlink -f "$workspace_root/$([[ $mode == --resume ]] && printf current || printf prepared)")
[[ -f $release/plugins/kwin/effects/plugins/linux_rond_workspace.so && -f $release/linux_rond_workspace_loader.so && -f $release/kwin-version ]]
for attempt in {1..30}; do
    if workspace_bus /Effects org.kde.kwin.Effects.loadedEffects >/dev/null 2>&1; then break; fi
    sleep 1
done
workspace_bus /Effects org.kde.kwin.Effects.loadedEffects >/dev/null
backup="$workspace_root/rollback"
if [[ $mode == --activate-prepared ]]; then
    systemctl --user is-active --quiet plasma-plasmashell.service
    safe_value=$(kreadconfig6 --file kwinrc --group Plugins --key round-safe-areaEnabled --default __UNSET__)
    safe_loaded=$(workspace_bus /Scripting org.kde.kwin.Scripting.isScriptLoaded round-safe-area)
    python3 "$script_dir/workspace-backup.py" save "$backup" "$workspace_data" "$release" "$safe_value" "$safe_loaded"
else
    [[ -f $backup/state.json ]] || { printf '%s\n' 'Workspace rollback state is missing.' >&2; exit 1; }
fi
rollback_on_error() {
    code=$?
    if (( code != 0 )); then
        printf '%s\n' 'Workspace activation failed; restoring the saved state.' >&2
        "$script_dir/disable-workspace.sh" || true
    fi
    exit "$code"
}
trap rollback_on_error EXIT
[[ $(kwin_wayland --version) == "$(cat "$release/kwin-version")" ]] || {
    printf '%s\n' 'KWin version changed; prepare the workspace again.' >&2; exit 1;
}
workspace_unload
workspace_safe_area_off
kwriteconfig6 --file linux-rondrc --group Workspace --key Enabled false
if [[ $mode == --activate-prepared ]]; then
    workspace_install_package "$release/plasma/org.softyoda.arcpanel" org.softyoda.arcpanel
    workspace_install_package "$release/plasma/round-workspace-controls" org.softyoda.roundworkspacecontrols
    qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript "$(cat "$release/plasma/round-workspace-controls/configure.js")" >/dev/null
    systemctl --user restart plasma-plasmashell.service
fi
relative="../../../../../../../../${release#/}/linux_rond_workspace_loader"
result=$(workspace_bus /Plugins org.kde.KWin.Plugins.LoadPlugin "$relative")
[[ $result == true ]] || { printf '%s\n' 'KWin rejected the user plugin loader.' >&2; exit 1; }
result=$(workspace_bus /Effects org.kde.kwin.Effects.loadEffect "$workspace_plugin")
[[ $result == true ]] || { printf '%s\n' 'KWin rejected the workspace effect.' >&2; exit 1; }
workspace_set_enabled true
workspace_state | python3 -c 'import json,sys; s=json.load(sys.stdin); sys.exit(0 if s.get("ready") and s.get("enabled") and s.get("output",{}).get("width",0)>0 else 1)'
if [[ $mode == --activate-prepared ]]; then
    ln -sfn "$release" "$workspace_root/current.new"
    mv -Tf "$workspace_root/current.new" "$workspace_root/current"
    python3 - "$workspace_root/load-workspace.sh" "$workspace_autostart" <<'PY'
import sys
from pathlib import Path
command='"'+sys.argv[1].replace('\\','\\\\').replace('"','\\"').replace('`','\\`').replace('$','\\$').replace('%','%%')+'"'
p=Path(sys.argv[2]);p.parent.mkdir(parents=True,exist_ok=True)
p.write_text('[Desktop Entry]\nType=Application\nName=Linux Rond workspace\nExec='+command+'\nOnlyShowIn=KDE;\nX-KDE-autostart-phase=2\n')
PY
fi
trap - EXIT
printf '%s\n' 'Linux Rond workspace enabled.'
