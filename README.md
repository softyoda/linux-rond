# Linux Rond

A **KDE Plasma 6 / KWin** interface for circular touchscreens, independent of personal dotfiles.

## Status

- **Working:** arc panels with tilted icons and curved task indicators; a KWin safe-area script; optional single-tap display wakeup.
- **Visually approved:** the balanced M5 square-to-disk mesh ([JSON](projection/approved-mesh.json), [OBJ with UVs](projection/approved-mesh.obj)).
- **Pending:** live projection, transformed input, per-app modes, bubble overview, and split-screen. See the [task checklist](docs/ROADMAP.md).

The projection is still a simulation, not an installed KWin effect. Applications currently
use the safe square. Personal screenshots used for validation are not distributed.

## Install

Requires a Plasma 6 session, `kpackagetool6`, `kwriteconfig6`, `qdbus6`, Python 3,
and the `plasma-plasmashell.service` user service. These scripts install no system packages.

```sh
git clone https://github.com/softyoda/linux-rond.git
cd linux-rond
./scripts/install.sh
# Optional: replace panels on the single square output.
./scripts/configure-panels.sh
```

`install.sh` installs the arc containment and enables the safe-area script for the current user.
`configure-panels.sh` backs up the layout, **replaces panels on the square output**, and
restarts Plasma. It leaves other outputs unchanged and refuses ambiguous output selection.
Set display scaling in System Settings → Display; no machine-specific scale is imposed.

```sh
./scripts/check.sh
./scripts/restore-panels.sh /path/to/backup
# Optional screenshot preview; requires Pillow.
python3 tools/preview.py capture.png preview.png
```

Restoring a backup restores the saved Plasma layout and disables the safe-area script.

## Single-tap wakeup

KWin normally requires a double tap to wake a sleeping display
([upstream implementation](https://github.com/KDE/kwin/blob/master/src/dpmsinputeventfilter.cpp)).
The optional native plugin wakes it when the last finger is lifted after one tap.
It does not unlock the session, synthesize clicks, change idle timeouts, or wake a suspended computer.

```sh
./scripts/install-touch-wakeup.sh
# Restore native KWin behavior.
./scripts/disable-touch-wakeup.sh
```

Requires C++23, CMake, and matching KWin/Qt/KF development files. Tested with **KWin 6.8.1**,
including isolated load/unload tests and a physical single-tap test confirmed on October 31, 2026.
The plugin installs in the user account and loads through KDE Autostart; no direct `/dev/input`
access or system configuration changes are required.

**Re-run the installer after a KWin update.** The loader rejects binaries built for a different
version. Logic tests cover single-touch, multitouch, and unexpected releases; other hardware
still needs physical validation.

## Development

```sh
./scripts/check-source.sh
```

Use English identifiers, comments, docstrings, logs, and documentation. Keep comments concise;
retain explicit UI translations. Report the distribution, Plasma/KWin versions, display
resolution, and input devices when testing. GNOME and other compositors are not supported.

The initial implementation came from `softyoda/dotfiles` at `84db998` (arc panels first
validated in `34c32c5`). Dotfiles now reference this repository at an immutable revision.

License: GPL-2.0-or-later. KDE dependencies retain their respective licenses.
