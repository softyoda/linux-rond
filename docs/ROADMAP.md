# Roadmap

Status as of October 31, 2026. Native workspace development is underway in three
parallel workstreams: rendering/window management, input mapping, and overview/split UX.
The experimental workspace is installed on the target at the user’s request.
Unchecked items still need final user or compatibility validation; see [the validation tracker](VALIDATION.md).

## Completed

- [x] Arc panels, tilted icons, and curved task indicators.
- [x] Safe-area window placement on the circular display.
- [x] Visually approved M5-A square-to-disk mesh and geometric checks.
- [x] Standalone public repository; dotfiles reduced to a pinned installer.
- [x] Single-tap display wakeup, user installation, and autostart; physically confirmed on KWin 6.8.1.
- [x] English source documentation and concise code comments.

## 1. Live projection and input

- [ ] Give main windows a common, fixed square canvas with sufficient source resolution.
- [ ] Render the approved mesh in KWin with a reversible enable/disable setting.
- [ ] Map mouse and touchscreen input through the inverse of the displayed geometry.
- [ ] Handle menus, popups, dialogs, subsurfaces, client-side decorations, and non-resizable apps.
- [ ] Validate both Wayland and XWayland applications, including drag-and-drop and multitouch.
- [ ] Measure sharpness, frame rate, and resource use on the target display.

## 2. Window controls and saved modes

Depends on live projection and input.

- [ ] Keep minimize, square/circle toggle, and close controls outside app content, in the top OS arc.
- [ ] Show separate left/right controls and curved app titles beside the clock in split-screen.
- [ ] Make the maximize action switch between square and circular projection.
- [ ] Persist the mode by application ID across new windows and restarts.
- [ ] Disable free window movement and resizing in this mode.
- [ ] Expose mode settings near the clock or in the launcher.
- [ ] Restore the previous safe layout when the mode is disabled.

## 3. Bubble overview

Depends on window mode management.

- [ ] Open the overview with a button near the launcher and `Super+Tab`.
- [ ] Pack all window previews without overlap, with touch-friendly targets.
- [ ] Show circular apps as circles and square apps as rounded squares.
- [ ] Give non-minimized windows slightly larger previews than minimized windows.
- [ ] Select and activate a window by tapping its preview.

## 4. Split-screen

Depends on the overview and transformed input.

- [ ] Drag an overview preview left or right to place it in that half of the display.
- [ ] Show the remaining previews in the opposite half to select the second window.
- [ ] Display both apps in complementary half-disks with matching input coordinates.
- [ ] Use vertical canvases during split-screen to preserve text proportions; restore square canvases afterward.
- [ ] Support leaving split-screen and restoring each app's saved mode.

## Reference and validation

The approved M5-A mesh has 49 × 49 vertices and 48 × 48 faces. Area, stretch, and curvature
were regularized together, with extra corner protection; uniform texel density alone does not ensure legible text.
`projection/approved-mesh.*` remains the geometry reference.

KWin 6.8.1 and 6.8.2 APIs have been inspected. Rendering and input extensions are implemented and integration-tested. Binary extensions must match the installed KWin version.

- [ ] Test rendering/input round trips on mesh edges and interiors; reject folds and holes.
- [ ] Keep input handling behind session-lock protections.
- [ ] Test preference persistence, clean unloading, and layout restoration.
- [ ] Validate in an isolated compositor before enabling changes in the main session.
- [ ] Confirm interactions on physical touch hardware; do not claim universal compatibility.
