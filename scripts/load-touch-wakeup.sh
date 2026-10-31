#!/usr/bin/env bash
set -euo pipefail
root="${XDG_DATA_HOME:-$HOME/.local/share}/linux-rond/touch-wakeup"
plugin=linux_rond_touch_wakeup
binary="$root/$plugin.so"
[[ -f $binary && -f $root/kwin-version ]] || exit 1
current=$(kwin_wayland --version)
[[ $current == "$(cat "$root/kwin-version")" ]] || {
    printf '%s\n' 'Linux Rond: KWin version changed; rerun install-touch-wakeup.sh.' >&2
    exit 1
}
# KWin prefixes plugin paths; resolve from the root across Qt search paths.
relative="../../../../../../../../${binary#/}"
relative=${relative%.so}
for attempt in {1..30}; do
    if loaded=$(qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.LoadedPlugins 2>/dev/null); then
        if printf '%s\n' "$loaded" | grep -Fxq "$plugin"; then exit 0; fi
        result=$(qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.LoadPlugin "$relative")
        [[ $result == true ]] || { printf '%s\n' 'Failed to load touch wakeup.' >&2; exit 1; }
        printf '%s\n' 'Linux Rond: single-tap wakeup enabled.'
        exit 0
    fi
    sleep 1
done
printf '%s\n' 'KWin session unavailable.' >&2
exit 1
