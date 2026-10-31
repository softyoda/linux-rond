#!/usr/bin/env bash
# Shared user-only workspace deployment paths.
workspace_root="${XDG_DATA_HOME:-$HOME/.local/share}/linux-rond/workspace"
workspace_config="${XDG_CONFIG_HOME:-$HOME/.config}"
workspace_data="${XDG_DATA_HOME:-$HOME/.local/share}"
workspace_plugin=linux_rond_workspace
workspace_loader=linux_rond_workspace_loader
workspace_autostart="$workspace_config/autostart/linux-rond-workspace.desktop"

workspace_bus() { qdbus6 org.kde.KWin "$@"; }
workspace_state() { workspace_bus /LinuxRond org.softyoda.LinuxRond.StateJson; }
workspace_set_enabled() { workspace_bus /LinuxRond org.softyoda.LinuxRond.SetEnabled "$1"; }
workspace_unload() {
    if workspace_state >/dev/null 2>&1; then
        workspace_set_enabled false
        for attempt in {1..300}; do
            if workspace_state | python3 -c 'import json,sys; s=json.load(sys.stdin); sys.exit(1 if s.get("enabled") or s.get("captureActive") else 0)'; then break; fi
            sleep .1
        done
        workspace_state | python3 -c 'import json,sys; s=json.load(sys.stdin); sys.exit(1 if s.get("enabled") or s.get("captureActive") else 0)' || {
            printf '%s\n' 'Release active pointers/touches, then retry disabling the workspace.' >&2
            return 1
        }
    fi
    workspace_bus /Effects org.kde.kwin.Effects.unloadEffect "$workspace_plugin" >/dev/null
    workspace_bus /Plugins org.kde.KWin.Plugins.UnloadPlugin "$workspace_loader" >/dev/null
}
workspace_safe_area_off() {
    kwriteconfig6 --file kwinrc --group Plugins --key round-safe-areaEnabled false
    workspace_bus /Scripting org.kde.kwin.Scripting.unloadScript round-safe-area >/dev/null
}
workspace_install_package() {
    local source=$1 name=$2
    if [[ -d $workspace_data/plasma/plasmoids/$name ]]; then
        kpackagetool6 --type Plasma/Applet --upgrade "$source" >/dev/null
    else
        kpackagetool6 --type Plasma/Applet --install "$source" >/dev/null
    fi
}
