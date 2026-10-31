#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
for tool in kpackagetool6 kwriteconfig6 qdbus6; do command -v "$tool" >/dev/null; done
qdbus6 org.kde.KWin /KWin supportInformation >/dev/null
install_package() {
    local type=$1 source=$2 destination=$3
    if [[ -d $destination ]]; then
        kpackagetool6 --type "$type" --upgrade "$source"
    else
        kpackagetool6 --type "$type" --install "$source"
    fi
}
install_package Plasma/Applet "$root/plasma/org.softyoda.arcpanel" "${XDG_DATA_HOME:-$HOME/.local/share}/plasma/plasmoids/org.softyoda.arcpanel"
install_package KWin/Script "$root/kwin/round-safe-area" "${XDG_DATA_HOME:-$HOME/.local/share}/kwin/scripts/round-safe-area"
kwriteconfig6 --file kwinrc --group Plugins --key round-safe-areaEnabled true
qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.unloadScript round-safe-area >/dev/null 2>&1 || true
qdbus6 org.kde.KWin /KWin reconfigure
printf '%s\n' 'Arc panel and safe area installed. Existing panels preserved.'
