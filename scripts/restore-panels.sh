#!/usr/bin/env bash
set -euo pipefail
[[ $# == 1 && -f $1 && -s $1 ]] || { printf '%s\n' 'Usage : restore-panels.sh /chemin/sauvegarde'; exit 2; }
rc="${XDG_CONFIG_HOME:-$HOME/.config}/plasma-org.kde.plasma.desktop-appletsrc"
backup=$(mktemp "${rc}.before-restore-XXXXXXXX")
cp -p -- "$rc" "$backup"
systemctl --user stop plasma-plasmashell.service
trap 'systemctl --user start plasma-plasmashell.service' EXIT
cp -- "$1" "$rc"
kwriteconfig6 --file kwinrc --group Plugins --key round-safe-areaEnabled false
qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.unloadScript round-safe-area >/dev/null 2>&1 || true
qdbus6 org.kde.KWin /KWin reconfigure
systemctl --user start plasma-plasmashell.service
trap - EXIT
printf 'Disposition restaurée. Sauvegarde avant restauration : %s\n' "$backup"
