# Workspace controls

Plasma applet `org.softyoda.roundworkspacecontrols`, with three `General/role` values:

- `right`: focused-window controls, or controls for the right split window.
- `left`: controls for the left split window; zero width outside split-screen.
- `overview`: overview button next to the launcher.

The arc containment orders these groups and respects their implicit width.
`configure.js` adds missing instances without rebuilding panels. Install the package
and updated arc containment before running it through Plasma's scripting API.
The top panel is raised to at least 160 logical pixels to contain both split-control
groups along the arc. The bottom task insertion index becomes 2, after the overview button.

DBus: `org.kde.KWin`, `/LinuxRond`, `org.softyoda.LinuxRond`.
The applet reads `StateJson()` and calls `Minimize(id)`, `ToggleMode(id)`, `Close(id)`,
`ExitSplitFor(id)`, `ToggleOverview()`, or `SetEnabled(bool)`. Exiting split restores
saved per-app modes. Actions never infer their target from a previous focus change.

The context menu enables or disables the workspace. Window controls and curved
titles stay in the OS arc and are hidden during overview and partner selection.
Split groups target their respective IDs; titles are elided to fit their arc span.
The containment passes `arcRadius` and `arcAtTop` to the compact representation.
Controls become unavailable when the extension is not loaded.

```sh
plasma/round-workspace-controls/tests/run.sh
```
