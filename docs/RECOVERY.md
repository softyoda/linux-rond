# Recovery audit — November 16, 2026

## Repository findings

The working checkout started clean on `wip/round-ui` at `79dd11e`. All seven agent
worktrees were also clean at that commit. Their branch names describe assignments,
not completed implementations. The shared UI module specified in `UX.md` is absent.
Upstream main remains `cb66cd7`, confirmed by a remote refresh during recovery.

The most recent implementation commit, `c68b9de`, explicitly describes an unvalidated
prototype. Historical integration results use synthetic Qt probes. They do not prove
legibility or usability with everyday applications, and the user's latest report
rejects the current experience as usable.

## Reproduced failures and repairs

- Two Python installer tests failed because the configuration test expected the old
  260 px panel height. The current side capsules require the configured 560 px host.
  Updated the configuration and migration expectations; targeted rollback remains tested.
- Four offscreen QML tests expected the old layout: no left title and a combined title
  and actions on the right. Updated them for the implemented separate capsules while
  retaining action targeting, hidden controls, and theme foreground checks.
- The failed-load rollback test could pass when preparation failed before attempting
  to load the effect. It now requires evidence that effect loading was attempted.
- The source check entry point omitted the standalone JavaScript suites. It now runs
  them and explicitly requires Node and the C++ compiler instead of silently omitting
  those checks.

The target is a separate circular-display device; the local workstation is only the
source/development host. Read-only inspection confirmed an enabled KWin 6.8.1 workspace
on a 720 × 720 logical output, with a 612 × 612 circular content area.

The first isolated integration run reproduced a native diagnostic regression:
`canvasAccepted` compared fractional requested geometry with pixel-rounded client
geometry using exact equality. Four of five inspected target windows differed only by
rounding; one exceeded the requested size substantially. Geometry comparisons now
allow one logical pixel of quantization, including resize retry decisions. Unit tests
retain rejection of oversized and displaced windows.

The next integration run exposed a stale split scenario: it assumed both applications
used half-disks even when one retained square mode. The scenario now checks the square
half's portrait rectangle, circle containment, and matching pointer/touch coordinates.

These repairs restore meaningful development checks. They do not certify the reported
visual problems as fixed. After isolated checks, the recovery build was installed using the versioned installer
and existing rollback journal. The active workspace now includes the geometry correction
and panel recovery. Physical acceptance remains pending.

## Verification scope

- `scripts/check-source.sh`: eight Python tests, standalone JavaScript suites,
  configuration/rollback JavaScript tests, shell syntax, diff whitespace, and the C++
  touch-wakeup state test passed.
- `plasma/round-workspace-controls/tests/run.sh`: 13 controls results and three D-Bus
  results passed, including setup/cleanup, plus configuration/rollback checks.
- QML tests used Qt 6.12.0, the offscreen platform and software rendering, with simulated
  mouse actions and a private D-Bus test service. No physical touch or window system
  compatibility was established by these tests.
- The development workstation reports KWin 6.8.2 and lacks CMake. The separate target
  has KWin 6.8.1 and build dependencies. The effect was rebuilt there in a temporary
  directory; all three CTest cases passed. A separate virtual compositor with private
  runtime/configuration/data directories exercised the build, leaving the active session
  unchanged.
- After the geometry correction and split-scenario update, all 34 extended integration
  checks passed on KWin 6.8.1 with virtual Wayland and XWayland probes at 720 × 720,
  scale 1: pointer, synthetic single/two-contact touch, popup actions, overview, mixed
  split modes, deferred actions, and new-process preferences. Physical touch and general
  application compatibility remain unverified in this recovery pass.
- Historical KWin 6.8.1/6.8.2 Wayland/XWayland probe evidence remains historical; see
  `VALIDATION.md`. The development-PC graphics restriction recorded there remains in force.

## Visual inspection and remaining evidence

An isolated Plasma session at 1080 × 1080 and 150% scaling rendered synthetic probe
content in square and circular modes. Mouse and synthetic touch reached the probe.
The circular projection visibly compresses/skews text near the boundary. This remains
a product readability problem even when coordinate mapping passes. The session used
the default test appearance, not the target user's theme.

Next: validate real application minimum sizes, readable edge controls, popups, and the
actual target theme; physically review touch and readability before deployment. The
complete shared UI and circular system sheets remain pending.

## Installed panel recovery

A private inspection of the actual desktop found both arc panel views at 32 logical
pixels, floating, with default visibility. This clipped the top and bottom controls and
hid the side capsules. The configuration journal still described the older taller layout.
The trigger that reset the views is not yet established.

Configuration now restores the minimum top/bottom heights, nonfloating full-width views,
and retains rollback values. The loader reapplies configuration on resume and after its
Plasma restart. A new installer test checks that resume configures panels before activation.
The isolated integration runner can reproduce collapsed views with
`LINUX_ROND_PANEL_RECOVERY=1 LINUX_ROND_PLASMA=1`.

The reproduction passed on KWin 6.8.1 at 1080 × 1080, 150% scale, with synthetic mouse
and touch. After deployment, a private screenshot confirmed complete top/dock controls
and visible side capsules. The existing bottom journal restored a 260 px panel view;
the top is 560 px. Private screenshots and configuration backups remain outside the
repository. No private desktop image is published as validation evidence.

User feedback requested: launcher and square/circle button touch usability, and whether
the dock obscures content. Circular text distortion, real-app compatibility, and recovery
after an actual display hotplug/cold login remain pending.

## November 17 follow-up

The user still rejects the overall experience: missing top drag and circular system sheets,
wallpaper misalignment, and inaccessible transcoding controls. The existing control window
was found minimized and restored without launching or stopping any transcoding operation.
Multiple Python windows share a task group. A new task action helper opens overview for
multiple managed windows; a single minimized window calls `Activate`, even if the reported
active ID is stale. Source tests cover those cases. Other task providers retain their fallback.

Panel setup now uses `hiding = "windowsgobelow"` and journals its previous mode. It replaces
manual configuration writes that were not reapplied on resume. The [Plasma scripting
implementation](https://raw.githubusercontent.com/KDE/plasma-workspace/master/shell/scripting/panel.cpp)
accepts that value but its getter can report `none`; this was reproduced on the target.
The isolated configuration file confirms `panelVisibility=3`. Rollback accommodates the
getter behavior; a manual change to `none` cannot be distinguished through that getter.

Checks: eight Python tests, JavaScript suites including task dispatch, isolated Plasma
at 1080 × 1080 / 150%, synthetic mouse/touch, and three native CTest cases passed. The
new release was deployed through the versioned installer. A private screenshot confirms
the control window is visible; physical task selection remains pending. No new claim is
made about the 34-check compositor suite, which passed in the previous recovery step.

The wallpaper uses the installed blur plugin; no wallpaper replacement or crop adjustment
was made without identifying which alignment the user means. Gestures and circular sheets
remain implementation work, not installed functionality.
