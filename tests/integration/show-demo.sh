#!/usr/bin/env bash
set -euo pipefail
build=${1:?Usage: show-demo.sh test-build plugin.so host-wayland-socket}
plugin=${2:?}
host_socket=${3:?}
source_root=$(cd "$(dirname "$0")/../.." && pwd)
if [[ ${LINUX_ROND_DEMO_SESSION:-0} != 1 ]]; then
    [[ -S $host_socket ]] || { printf '%s\n' 'Host Wayland socket not found.' >&2; exit 1; }
    reference_config=${XDG_CONFIG_HOME:-$HOME/.config}
    reference_data=${XDG_DATA_HOME:-$HOME/.local/share}
    run=$(mktemp -d /tmp/linux-rond-demo.XXXXXXXX)
    export LINUX_ROND_TEST_RUN="$run" WAYLAND_DISPLAY="linux-rond-demo-$$"
    export XDG_RUNTIME_DIR="$run/runtime" XDG_CONFIG_HOME="$run/config" XDG_DATA_HOME="$run/data"
    export XDG_CACHE_HOME="$run/cache" XDG_STATE_HOME="$run/state"
    mkdir -m 700 "$XDG_RUNTIME_DIR" "$XDG_CONFIG_HOME" "$XDG_DATA_HOME" "$XDG_CACHE_HOME" "$XDG_STATE_HOME"
    export XDG_CONFIG_DIRS=/etc/xdg XDG_DATA_DIRS=/usr/local/share:/usr/share
    if [[ ${LINUX_ROND_REFERENCE_THEME:-0} == 1 ]]; then
        python3 "$(dirname "$0")/copy-appearance.py" "$reference_config" "$XDG_CONFIG_HOME"
        export XDG_DATA_DIRS="$reference_data:/usr/local/share:/usr/share"
        if [[ -d $reference_data/fonts ]]; then ln -s "$reference_data/fonts" "$XDG_DATA_HOME/fonts"; fi
    fi
    export QT_QPA_PLATFORM=wayland QT_QPA_PLATFORMTHEME=generic QT_NO_XDG_DESKTOP_PORTAL=1
    export QT_FORCE_STDERR_LOGGING=1 QT_LOGGING_TO_CONSOLE=1
    unset DISPLAY
    if [[ ${LINUX_ROND_REFERENCE_THEME:-0} == 1 ]]; then export QT_QPA_PLATFORMTHEME=kde; fi
    exec dbus-run-session -- env LINUX_ROND_DEMO_SESSION=1 "$0" "$@"
fi
run=${LINUX_ROND_TEST_RUN:?}
python3 - "$run/session.json" <<'SESSION'
import json,os,sys
keys=['DBUS_SESSION_BUS_ADDRESS','XDG_RUNTIME_DIR','XDG_CONFIG_HOME','XDG_DATA_HOME','XDG_CACHE_HOME','XDG_STATE_HOME','XDG_DATA_DIRS','XDG_CONFIG_DIRS','WAYLAND_DISPLAY','QT_QPA_PLATFORM','QT_QPA_PLATFORMTHEME']
with open(sys.argv[1],'w') as file:json.dump({k:os.environ[k] for k in keys if k in os.environ},file)
SESSION
mkdir -p "$run/plugins/kwin/effects/plugins"
cp "$plugin" "$run/plugins/kwin/effects/plugins/linux_rond_workspace.so"
export QT_PLUGIN_PATH="$run/plugins"
printf '%s\n' "$run" > /tmp/linux-rond-latest-demo-path
printf 'Demo artifacts: %s\n' "$run"
QT_QPA_PLATFORM=offscreen kwin_wayland --wayland-display "$host_socket" --width 720 --height 720 \
    --fullscreen true --socket "$WAYLAND_DISPLAY" --no-lockscreen --no-global-shortcuts --no-kactivities \
    > "$run/kwin.log" 2>&1 &
kwin_pid=$!
children=()
cleanup() {
    for child in "${children[@]}"; do kill "$child" 2>/dev/null || true; done
    kill "$kwin_pid" 2>/dev/null || true
}
trap cleanup EXIT
for attempt in {1..100}; do
    if qdbus6 org.kde.KWin /KWin supportInformation > "$run/support.txt" 2>/dev/null; then break; fi
    kill -0 "$kwin_pid" 2>/dev/null || { cat "$run/kwin.log"; exit 1; }
    sleep .1
done
kpackagetool6 --type Plasma/Applet --install "$source_root/plasma/org.softyoda.arcpanel" > "$run/packages.log" 2>&1
kpackagetool6 --type Plasma/Applet --install "$source_root/plasma/round-workspace-controls" >> "$run/packages.log" 2>&1
start_plasma() {
    plasmashell --no-respawn >> "$run/plasma.log" 2>&1 &
    plasma_pid=$!;children+=("$plasma_pid")
    for attempt in {1..150}; do
        if qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript 'print("demo-ready")' >/dev/null 2>&1; then return; fi
        kill -0 "$plasma_pid" 2>/dev/null || return 1
        sleep .1
    done
    return 1
}
start_plasma
qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript "$(cat "$source_root/plasma/panels.js")"

if [[ -f $XDG_CONFIG_HOME/reference-launcher.json ]]; then
    python3 - "$XDG_CONFIG_HOME/reference-launcher.json" > "$run/launcher.js" <<'PYTHON'
import json,sys
icon=json.load(open(sys.argv[1]))
print('panels().forEach(p => p.widgets().forEach(w => { if (["org.kde.plasma.kickoff","org.kde.plasma.kicker"].includes(w.type)) { w.currentConfigGroup=["General"]; w.writeConfig("icon",'+json.dumps(icon)+'); }}));')
PYTHON
    qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript "$(cat "$run/launcher.js")" >/dev/null
fi

qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript "$(cat "$source_root/plasma/round-workspace-controls/configure.js")"
# Let Plasma flush the new panel tree before configuring its tray.
sleep 1
kill "$plasma_pid";wait "$plasma_pid" || true
python3 "$source_root/scripts/configure-tray.py" "$XDG_CONFIG_HOME/plasma-org.kde.plasma.desktop-appletsrc"
start_plasma
"$build/round_demo_control" > "$run/control.log" 2>&1 &
control_pid=$!;children+=("$control_pid")
mkdir -p "$XDG_DATA_HOME/applications"
cat > "$XDG_DATA_HOME/applications/linux-rond-exit-demo.desktop" <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=Exit demo
Icon=application-exit
Exec=qdbus6 org.softyoda.LinuxRondDemo /Demo org.softyoda.LinuxRondDemo.Quit
Terminal=false
DESKTOP
chmod +x "$XDG_DATA_HOME/applications/linux-rond-exit-demo.desktop"
python3 - "$XDG_DATA_HOME/applications/linux-rond-exit-demo.desktop" > "$run/exit-widget.js" <<'PY'
import json,sys
from pathlib import Path
print('var dock = panels().filter(p => p.location === "bottom")[0];')
print('if (!dock) throw new Error("Demo dock not found");')
print('var exit = dock.addWidget("org.kde.plasma.icon"); exit.currentConfigGroup=["General"];')
print('exit.writeConfig("url", '+json.dumps(Path(sys.argv[1]).as_uri())+');')
PY
qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript "$(cat "$run/exit-widget.js")"
qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadEffect linux_rond_workspace
qdbus6 org.kde.KWin /LinuxRond org.softyoda.LinuxRond.SetEnabled true
read -r -a demo_apps <<< "${LINUX_ROND_DEMO_APPS:-Calibration}"
for name in "${demo_apps[@]}"; do
    "$build/round_probe" "$name" "$run/events.jsonl" > "$run/$name.log" 2>&1 &
    children+=("$!")
done
sleep 3
qdbus6 org.kde.KWin /LinuxRond org.softyoda.LinuxRond.ToggleActiveMode
sleep 1
spectacle -b -n -f -o "$run/preview.png" > "$run/screenshot.log" 2>&1 || true
printf '%s\n' 'Demo ready. The exit icon in the bottom dock closes only this nested session.'
wait "$control_pid"
