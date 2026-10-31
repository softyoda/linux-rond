# Experimental round workspace

This native KWin effect uses the approved projection mesh for rendering and the same triangles
for inverse input mapping. It is separate from the validated safe-area setup.

## Prepare without activation

Requires C++23, CMake, the installed KWin development package, and matching Qt/KF dependencies.

```sh
./scripts/install-workspace.sh --prepare
```

Preparation compiles the effect and its user-path loader, runs CTest, and stages a versioned
release under `${XDG_DATA_HOME:-~/.local/share}/linux-rond/workspace/`. It does not load an
effect, change windows, install Plasma packages, or modify the active panel configuration.
The default action is `--prepare`; no system packages are installed.

## Enable after testing

Use an isolated KWin session first. Activation requires a running Plasma user session with
existing Linux Rond arc panels and exactly one square output, unless `Workspace/OutputName`
is set in `linux-rondrc`.

```sh
./scripts/install-workspace.sh --enable
```

Activation:

1. Saves the previous safe-area script state and installed arc/control packages.
2. Disables and unloads `round-safe-area` before enabling the new geometry manager.
3. Installs the control applet and arc containment, journaling added widgets and changed settings.
4. Restarts **Plasma Shell**, not KWin or the login session.
5. Loads the user-path helper, then the effect, with automatic activation initially disabled.
6. Enables the effect, checks its DBus readiness/output, and creates its own KDE Autostart entry.

The helper registers only the release's Qt plugin directory. It is needed because KWin's
effect loader discovers named plugins rather than accepting arbitrary file paths. No library
is copied into `/usr`, and no global `QT_PLUGIN_PATH` is changed. KWin updates require rebuilding.

## Disable and restore

```sh
./scripts/disable-workspace.sh
# Also available without this checkout:
~/.local/share/linux-rond/workspace/disable-workspace.sh
```

Presentation changes wait for active pointer/touch gestures to finish. The effect then
restores saved window geometry, decorations, fullscreen and maximize states.
The script removes its autostart entry, restores the previous safe-area configuration/loading
state, removes only widget IDs recorded by activation, returns any moved notification applet
to its original panel with its configuration intact, and restores panel height/tasksIndex
only when those values still equal the values it applied. Unrelated widgets and subsequent
user edits are retained.

Previous arc/control packages are restored from a local backup. If package files were edited
after activation, rollback preserves them and reports the backup location instead of overwriting
them. Successful backups are retained as `rollback-restored-*`. Failed activation attempts run
rollback automatically; any reported incomplete restoration should be reviewed before retrying.

## Interaction

- Main windows share a fixed square canvas on the selected output. Split uses vertical
  rectangular canvases and restores square canvases when leaving split.
- Window mode is saved by desktop application ID, falling back to the resource class.
- The top arc exposes minimize, mode toggle, and close controls.
- `Super+Tab` or the overview button opens touch-friendly previews.
- Drag a preview to a side, then tap another preview to select a split partner.
- Each split half has its own controls. Leaving split restores the saved per-app modes.

Server-side decorations can be removed. Client-side headerbars remain app content; arbitrary
cropping would remove controls. Apps that reject the requested canvas size are reported by
`canvasAccepted` in the diagnostic state rather than subjected to repeated resize requests.

## API and checks

Service `org.kde.KWin`, object `/LinuxRond`, interface `org.softyoda.LinuxRond`:

- `StateJson()` and `StateChanged(json)` expose window IDs, modes, layout, and geometry diagnostics.
- `SetEnabled(bool)`, `ToggleOverview()`, `Activate(id)`.
- `Minimize(id)`, `ToggleMode(id)`, `Close(id)` and active-window wrappers.
- `BeginSplit(id, side)`, `ChooseSplit(id)`, `ExitSplit()`, `ExitSplitFor(id)`; side is `-1` or `1`.

```sh
cmake -S kwin/round-workspace -B build-workspace
cmake --build build-workspace
ctest --test-dir build-workspace --output-on-failure
node plasma/round-workspace-controls/tests/configure_test.js
```

Geometry tests cover round trips, clipped popup areas, disk/half-disk containment, and saved modes.
Layout tests independently check shape intersections and density. Isolated compositor tests have
exercised projected mouse/touch input, popups, overview, split interactions, and targeted controls
on KWin 6.8.1/6.8.2; broad application and physical touchscreen workspace validation remains open.
