#!/usr/bin/env bash
set -euo pipefail
root="${XDG_DATA_HOME:-$HOME/.local/share}/linux-rond/touch-wakeup"
plugin=linux_rond_touch_wakeup
binary="$root/$plugin.so"
[[ -f $binary && -f $root/kwin-version ]] || exit 1
current=$(kwin_wayland --version)
[[ $current == "$(cat "$root/kwin-version")" ]] || {
    printf '%s\n' 'Linux Rond : version KWin modifiée ; relancer install-touch-wakeup.sh.' >&2
    exit 1
}
# KWin préfixe les noms avec kwin/plugins, puis Qt cherche dans plusieurs
# dossiers. Rejoindre la racine avant le chemin utilisateur fonctionne dans
# chacun de ces dossiers, sans installation système ni QT_PLUGIN_PATH global.
relative="../../../../../../../../${binary#/}"
relative=${relative%.so}
for attempt in {1..30}; do
    if loaded=$(qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.LoadedPlugins 2>/dev/null); then
        if printf '%s\n' "$loaded" | grep -Fxq "$plugin"; then exit 0; fi
        result=$(qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.LoadPlugin "$relative")
        [[ $result == true ]] || { printf '%s\n' 'Échec du chargement du réveil tactile.' >&2; exit 1; }
        printf '%s\n' 'Linux Rond : réveil au premier toucher activé.'
        exit 0
    fi
    sleep 1
done
printf '%s\n' 'Session KWin indisponible.' >&2
exit 1
