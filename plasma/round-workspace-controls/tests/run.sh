#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
build=$(mktemp -d "${TMPDIR:-/tmp}/linux-rond-controls-test.XXXXXXXX")
trap 'rm -rf -- "$build"' EXIT
qt_bins=$(qmake6 -query QT_INSTALL_BINS)
qt_libexec=$(qmake6 -query QT_INSTALL_LIBEXECS)
"$qt_libexec/moc" "$root/dbus_server.cpp" -o "$build/dbus_server.moc"
c++ -std=c++23 -fPIC -I "$build" "$root/dbus_server.cpp" $(pkg-config --cflags --libs Qt6Core Qt6DBus) -o "$build/dbus-server"
mkdir -m 700 "$build/runtime" "$build/config" "$build/data" "$build/cache" "$build/state"
export XDG_RUNTIME_DIR="$build/runtime" XDG_CONFIG_HOME="$build/config" XDG_DATA_HOME="$build/data"
export XDG_CACHE_HOME="$build/cache" XDG_STATE_HOME="$build/state"
export QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software QT_QPA_PLATFORMTHEME=generic QT_NO_XDG_DESKTOP_PORTAL=1
unset DISPLAY WAYLAND_DISPLAY
dbus-run-session -- bash -c '
    "$2" -input "$4" || exit $?
    "$1" & server=$!
    trap '\''kill "$server" 2>/dev/null || true'\'' EXIT
    "$2" -input "$3"
' bash "$build/dbus-server" "$qt_bins/qmltestrunner" "$root/dbus" "$root/tst_controls.qml"

if command -v node >/dev/null; then node "$root/configure_test.js"; fi
