# Linux Rond — shared UX concept (binding for every component)

All agents and features follow this document so the round desktop feels like one product.
When something is missing here, agree with the other owners (see "Coordination") and add it.

## 1. Geometry (logical px; output 720×720, scale 1.5, physical 1080×1080)

| Name | Value | Meaning |
|---|---|---|
| `R` | 360 | visible circle radius, center (360, 360) |
| `bandOuter` | 358 | outer edge of every capsule band |
| `bandInner` | 306 | inner edge of the clock capsule / dock bands |
| `contentRadius` | 306 | round app content and round sheets stop here (no wallpaper ring) |
| `square` | 509 at (105.4, 105.4) | largest visible square (square mode, 1:1) |
| side capsules | centered at ±90° | window controls (left: title / tabs, right: minimize · round/square · close) |

Nothing interactive may sit outside the circle. Text and controls keep a 12 px margin from any circle edge
they face (use the chord width at their y).

## 2. Visual language

- Theme: Kirigami theme of We10XOSDark; never hard-code colors except the destructive red of close actions.
- Capsule = arc band with round caps, fill `Kirigami.Theme.backgroundColor` at **0.82** alpha, no border, no shadow.
- Round sheet (every popup: launcher, Wi-Fi, audio, brightness, USB, calendar, notifications) =
  a **circle of radius `contentRadius`** centered on the screen, same fill, behind it a 40 % dim of the desktop.
  A sheet always shows: curved title along its top arc, content in the middle, close by tapping outside or swiping down.
- Lists inside a circle use **chord-width rows**: row width = chord at the row's y minus 2×16 px, centered;
  rows fade (opacity) within 24 px of the circle edge. No rectangular list that overflows the circle.
- Values use **arc sliders** on a ring (radius ≈ 250), thumb 28 px, track 6 px, accent fill.
- Curved text only along arcs (titles, tabs in side capsules); body text stays horizontal.
- Icons: theme icons, monochrome where Plasma uses symbolic icons; buttons keep their on-screen orientation.
- Active state: accent arc of 3 px (as the dock indicators); minimized = 45 % opacity.

## 3. Motion

- Open sheet: 180 ms `Easing.OutCubic`, scale 0.92 → 1 + opacity 0 → 1, origin = the tapped icon.
- Close sheet: 140 ms `Easing.InCubic`, reverse.
- Gesture-driven UI follows the finger 1:1 while dragging; on release it settles in 200 ms `OutCubic`;
  cancelling reverts with the same curve. Thresholds: 24 px to start a drag, 30 % of travel to commit.
- Never show a delayed frame: preload sheets, react to D-Bus signals (`SignalWatcher`), no polling, no layout jumps.

## 4. Touch

- Minimum target 44 px (side-capsule buttons 48). Spacing 8 px. Long-press 450 ms only for secondary actions,
  never as the only way to reach a primary action.
- Edge gestures: top edge swipe down = show panels (immersive exit); clock capsule drag up = immersive;
  bottom edge swipe up = launcher; overview: drag preview to a side = split attach, drag down = close target.

## 5. Shared code

`plasma/linux-rond-ui/` — QML module `org.softyoda.linuxrond.ui` (installed with the other packages):

| Component | Owner | Purpose |
|---|---|---|
| `Tokens` (singleton) | launcher | the values of sections 1–3 |
| `RoundSheet` | launcher | full-circle popup window (frameless, centered, dimmed backdrop, open/close motion, swipe-down to close) |
| `ChordListView` | launcher | vertical list whose rows follow the chord width |
| `ArcText` | firefox | text laid out along an arc (used by side capsules and sheet titles) |
| `ArcSlider` | quicksettings | value slider on a ring |
| `RoundIconButton` | quicksettings | 44–48 px round touch button |

Owners implement first and publish the API in `docs/COORDINATION.md` of their branch; others reuse it
(read the owner's worktree directly, or ask the owner) instead of writing their own variant.

## 6. Coordination

Use the available collaboration channel for short API proposals, hooks, and blockers.
Discover local worktree locations with `git worktree list`; do not publish private paths.
The orchestrator integrates branches in this order: ui-kit pieces → launcher → quicksettings → calendar →
gestures → overview → firefox → kate.
