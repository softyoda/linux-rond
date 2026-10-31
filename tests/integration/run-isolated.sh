#!/usr/bin/env bash
set -euo pipefail
if [[ ${LINUX_ROND_TEST_SESSION:-} != 1 ]]; then
    reference_config=${XDG_CONFIG_HOME:-$HOME/.config}
    reference_data=${XDG_DATA_HOME:-$HOME/.local/share}
    run=$(mktemp -d /tmp/linux-rond-test.XXXXXXXX)
    export LINUX_ROND_TEST_RUN="$run" WAYLAND_DISPLAY="linux-rond-test-$$"
    export XDG_RUNTIME_DIR="$run/runtime" XDG_CONFIG_HOME="$run/config" XDG_DATA_HOME="$run/data"
    export XDG_CACHE_HOME="$run/cache" XDG_STATE_HOME="$run/state"
    mkdir -m 700 "$XDG_RUNTIME_DIR" "$XDG_CONFIG_HOME" "$XDG_DATA_HOME" "$XDG_CACHE_HOME" "$XDG_STATE_HOME"
    export QT_QPA_PLATFORM=wayland LIBGL_ALWAYS_SOFTWARE=1
    export QT_QPA_PLATFORMTHEME=generic QT_NO_XDG_DESKTOP_PORTAL=1
    export XDG_CONFIG_DIRS=/etc/xdg XDG_DATA_DIRS=/usr/local/share:/usr/share
    if [[ ${LINUX_ROND_REFERENCE_THEME:-0} == 1 ]]; then
        python3 "$(dirname "$0")/copy-appearance.py" "$reference_config" "$XDG_CONFIG_HOME"
        export XDG_DATA_DIRS="$reference_data:/usr/local/share:/usr/share"
        if [[ -d $reference_data/fonts ]]; then ln -s "$reference_data/fonts" "$XDG_DATA_HOME/fonts"; fi
    fi
    unset DISPLAY
    if [[ ${LINUX_ROND_REFERENCE_THEME:-0} == 1 ]]; then export QT_QPA_PLATFORMTHEME=kde; fi
    exec dbus-run-session -- env LINUX_ROND_TEST_SESSION=1 "$0" "$@"
fi
build=${1:?Usage: run-isolated.sh /path/to/test-build [plugin.so]}
plugin=${2:-}
run=${LINUX_ROND_TEST_RUN:?}
export QT_FORCE_STDERR_LOGGING=1 QT_LOGGING_TO_CONSOLE=1
if [[ -n $plugin ]]; then
    mkdir -p "$run/plugins/kwin/effects/plugins"
    cp "$plugin" "$run/plugins/kwin/effects/plugins/linux_rond_workspace.so"
    export QT_PLUGIN_PATH="$run/plugins"
fi
printf '%s\n' "$run" > /tmp/linux-rond-latest-test-path
printf 'Test artifacts: %s\n' "$run"
extra_args=()
if [[ ${LINUX_ROND_XWAYLAND:-0} == 1 ]]; then
    export LINUX_ROND_TEST_RUN="$run"
    cat > "$run/export-x11.sh" <<'X11'
#!/usr/bin/env bash
python3 - <<'PYTHON'
import json,os
from pathlib import Path
(Path(os.environ['LINUX_ROND_TEST_RUN'])/'x11-env.json').write_text(json.dumps({k:os.environ[k] for k in ['DISPLAY','XAUTHORITY'] if k in os.environ}))
PYTHON
X11
    chmod +x "$run/export-x11.sh"
    extra_args=(--xwayland "$run/export-x11.sh")
fi
pixels=${LINUX_ROND_TEST_PIXELS:-720}
QT_QPA_PLATFORM=offscreen kwin_wayland --virtual --width "$pixels" --height "$pixels" --scale 1 --socket "$WAYLAND_DISPLAY" \
    --no-lockscreen --no-global-shortcuts --no-kactivities "${extra_args[@]}" > "$run/kwin.log" 2>&1 &
compositor_pid=$!
probe_pid=
plasma_pid=
cleanup() {
    [[ -z $probe_pid ]] || kill "$probe_pid" 2>/dev/null || true
    [[ -z $plasma_pid ]] || kill "$plasma_pid" 2>/dev/null || true
    kill "$compositor_pid" 2>/dev/null || true
}
trap cleanup EXIT
for attempt in {1..100}; do
    if qdbus6 org.kde.KWin /KWin supportInformation > "$run/support.txt" 2>/dev/null; then break; fi
    kill -0 "$compositor_pid" 2>/dev/null || { cat "$run/kwin.log"; exit 1; }
    sleep .1
done
export QT_QPA_PLATFORM=wayland
if [[ ${LINUX_ROND_TEST_SCALE:-1} != 1 ]]; then
    kscreen-doctor "output.1.scale.${LINUX_ROND_TEST_SCALE}" > "$run/scaling.log"
    sleep .4
fi
if [[ ${LINUX_ROND_PLASMA:-0} == 1 ]]; then
    source_root=$(cd "$(dirname "$0")/../.." && pwd)
    kpackagetool6 --type Plasma/Applet --install "$source_root/plasma/org.softyoda.arcpanel" > "$run/packages.log" 2>&1
    kpackagetool6 --type Plasma/Applet --install "$source_root/plasma/round-workspace-controls" >> "$run/packages.log" 2>&1
    plasmashell --no-respawn > "$run/plasma.log" 2>&1 &
    plasma_pid=$!
    for attempt in {1..150}; do
        if qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript 'print("test-ready")' >/dev/null 2>&1; then break; fi
        kill -0 "$plasma_pid" 2>/dev/null || { cat "$run/plasma.log"; exit 1; }
        sleep .1
    done
    qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript "$(cat "$source_root/plasma/panels.js")" > "$run/panels.log"

if [[ -f $XDG_CONFIG_HOME/reference-launcher.json ]]; then
    python3 - "$XDG_CONFIG_HOME/reference-launcher.json" > "$run/launcher.js" <<'PYTHON'
import json,sys
icon=json.load(open(sys.argv[1]))
print('panels().forEach(p => p.widgets().forEach(w => { if (["org.kde.plasma.kickoff","org.kde.plasma.kicker"].includes(w.type)) { w.currentConfigGroup=["General"]; w.writeConfig("icon",'+json.dumps(icon)+'); }}));')
PYTHON
    qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript "$(cat "$run/launcher.js")" >/dev/null
fi

    qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript "$(cat "$source_root/plasma/round-workspace-controls/configure.js")" >> "$run/panels.log"
    sleep 1
    kill "$plasma_pid";wait "$plasma_pid" || true
    python3 "$source_root/scripts/configure-tray.py" "$XDG_CONFIG_HOME/plasma-org.kde.plasma.desktop-appletsrc" >> "$run/packages.log"
    plasmashell --no-respawn >> "$run/plasma.log" 2>&1 &
    plasma_pid=$!
    for attempt in {1..150}; do
        if qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript 'print("test-ready")' >/dev/null 2>&1; then break; fi
        sleep .1
    done

fi
"$build/round_probe" A "$run/events.jsonl" > "$run/probe.log" 2>&1 &
probe_pid=$!
sleep 2
cat > "$run/windows.js" <<'JS'
workspace.windowList().forEach(w => console.log("TEST_WINDOW", w.caption, JSON.stringify(w.frameGeometry), w.minimized));
JS
script_id=$(qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.loadScript "$run/windows.js" linux-rond-test-windows)
qdbus6 org.kde.KWin "/Scripting/Script$script_id" org.kde.kwin.Script.run
if [[ -n $plugin ]]; then
    qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadEffect linux_rond_workspace
    qdbus6 org.kde.KWin /LinuxRond org.softyoda.LinuxRond.SetEnabled true
    if [[ ${LINUX_ROND_PLASMA:-0} == 1 ]]; then sleep 3; else sleep .5; fi
    qdbus6 org.kde.KWin /LinuxRond org.softyoda.LinuxRond.StateJson > "$run/state.json"
fi
if [[ -n $plugin ]]; then
    timeout 12 spectacle -b -n -f -o "$run/square.png" > "$run/screenshot.log" 2>&1
    qdbus6 org.kde.KWin /LinuxRond org.softyoda.LinuxRond.ToggleActiveMode
    sleep .4
    qdbus6 org.kde.KWin /LinuxRond org.softyoda.LinuxRond.StateJson > "$run/state-round.json"
    if command -v spectacle >/dev/null; then timeout 12 spectacle -b -n -f -o "$run/preview.png" > "$run/screenshot.log" 2>&1 || true; fi
fi
"$build/round_input_driver" click 360 360
"$build/round_input_driver" touch 300 300
sleep .3
python3 - "$run/events.jsonl" <<'PY'
import json,sys
rows=[json.loads(line) for line in open(sys.argv[1])]
assert any(r['event']=='mouse-up' for r in rows),rows
assert any(r['event'].startswith('touch-') for r in rows),rows
print('Mouse and touch reached the isolated probe.')
PY
if [[ -n $plugin && ${LINUX_ROND_EXTENDED:-0} == 1 ]]; then
    python3 "$(dirname "$0")/scenarios.py" "$build" "$run"
fi
if [[ -n $plugin ]]; then
    qdbus6 org.kde.KWin /LinuxRond org.softyoda.LinuxRond.SetEnabled false
    qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.unloadEffect linux_rond_workspace
fi
