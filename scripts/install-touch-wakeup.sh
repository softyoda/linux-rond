#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
for tool in cmake kwin_wayland qdbus6 realpath; do command -v "$tool" >/dev/null; done
qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.LoadedPlugins >/dev/null
build=$(mktemp -d "${TMPDIR:-/tmp}/linux-rond-touch-build.XXXXXXXX")
trap 'rm -rf -- "$build"' EXIT
cmake -S "$root/kwin/touch-wakeup" -B "$build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build" --parallel 2
ctest --test-dir "$build" --output-on-failure
destination="${XDG_DATA_HOME:-$HOME/.local/share}/linux-rond/touch-wakeup"
mkdir -p "$destination"
# Une mise à jour ne doit pas écraser le fichier mappé dans KWin.
qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.UnloadPlugin linux_rond_touch_wakeup >/dev/null
install -m 644 "$build/linux_rond_touch_wakeup.so" "$destination/linux_rond_touch_wakeup.so.new"
mv -f "$destination/linux_rond_touch_wakeup.so.new" "$destination/linux_rond_touch_wakeup.so"
kwin_wayland --version > "$destination/kwin-version"
install -m 755 "$root/scripts/load-touch-wakeup.sh" "$destination/load.sh"
"$destination/load.sh"
python3 - "$destination/load.sh" "${XDG_CONFIG_HOME:-$HOME/.config}/autostart/linux-rond-touch-wakeup.desktop" <<'PY'
import sys
from pathlib import Path
# Desktop Entry quoting differs from shell quoting; quote for Exec explicitly.
command='"'+sys.argv[1].replace('\\','\\\\').replace('"','\\"').replace('`','\\`').replace('$','\\$').replace('%','%%')+'"'
p=Path(sys.argv[2]);p.parent.mkdir(parents=True,exist_ok=True)
p.write_text('[Desktop Entry]\nType=Application\nName=Linux Rond — réveil tactile\nExec='+command+'\nOnlyShowIn=KDE;\nX-KDE-autostart-phase=2\n')
PY
printf '%s\n' 'Réveil tactile installé dans le compte utilisateur et activé au démarrage de Plasma.'
