#!/usr/bin/env bash
set -euo pipefail
qdbus6 org.kde.KWin /Plugins org.kde.KWin.Plugins.UnloadPlugin linux_rond_touch_wakeup
rm -f -- "${XDG_CONFIG_HOME:-$HOME/.config}/autostart/linux-rond-touch-wakeup.desktop"
printf '%s\n' 'Touch wakeup disabled; native KWin behavior restored.'
