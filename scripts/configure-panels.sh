#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript 'print("Linux Rond : configuration des panneaux")' >/dev/null
systemctl --user is-active --quiet plasma-plasmashell.service
rc="${XDG_CONFIG_HOME:-$HOME/.config}/plasma-org.kde.plasma.desktop-appletsrc"
backup=$(mktemp "${rc}.linux-rond-XXXXXXXX")
cp -p -- "$rc" "$backup"
printf 'Sauvegarde : %s\n' "$backup"
qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript "$(cat "$root/plasma/panels.js")" >/dev/null
systemctl --user stop plasma-plasmashell.service
trap 'systemctl --user start plasma-plasmashell.service' EXIT
python3 "$root/scripts/configure-tray.py" "$rc"
systemctl --user start plasma-plasmashell.service
trap - EXIT
