# Validation tracker

Updated October 31, 2026. **Working code, automated checks, and user approval are separate statuses.**
The experimental workspace is running in a separate demo session, not as a replacement for the main desktop.
The live baseline remains the existing arc panels and the validated single-tap wakeup plugin.

## Current review

The user confirmed on the target touchscreen:

- The window button switches between square and circular presentation.
- Touch targets align at the center and near all four corners in circular mode.

This approves those interactions only. **The dock appearance, overview icon, inactive-window
controls, and latest capsule layout are not approved.** A subsequent request requires sharp
corners in normal square mode; rounded corners belong only to overview thumbnails.

## Task list

| ID | Request | Status | Evidence / next check |
| --- | --- | --- | --- |
| R01 | Common fixed square canvas; no free movement | Automated checks passed; sharp-square correction in progress | Native move requests are cancelled; size constraints of other apps still need testing |
| R02 | Square ↔ circular button; fuller-corner mesh A | Toggle and circular touch behavior approved by user; exact square corners pending | [Live demo](validation/2026-10-31/01-live-circle.png) |
| R03 | Mouse coordinates follow projection | Automated checks passed on Wayland/XWayland probes | [Pointer target](validation/2026-10-31/02-pointer-test.png); test actual apps next |
| R04 | Touch coordinates follow projection | **User confirmed center and four corners**; multitouch automated | [Touch target](validation/2026-10-31/03-touch-test.png); real menus and gestures still to review |
| R05 | Window controls outside app content; separate capsules; curved title before buttons | **In progress — latest appearance rejected** | Hide controls with no active window; move title before buttons; review next screenshot |
| R06 | Remember shape per app across new windows/restarts | New-process persistence tested; cold session restart pending | [Reopened probe](validation/2026-10-31/07-restored-mode.png) |
| R07 | Dense overview; equal-area square/circle previews; larger non-minimized windows | Geometry and selection tested; visual review pending | [Overview WIP](validation/2026-10-31/04-overview-wip.png); fix dark icon contrast |
| R08 | Full half-disks with separate controls | Design approved; automated interactions passed; manual review pending | [Split WIP](validation/2026-10-31/06-split-wip.png); portrait canvases selected by user |
| R09 | Drag to a side, then pick the other window | Automated pointer drag + touch selection passed | [Partner selection WIP](validation/2026-10-31/05-partner-wip.png); physical drag test next |
| R10 | No wallpaper gap between content and panel backgrounds | Implemented; visual review pending | Preserve icon centering while extending panel backgrounds |
| R11 | Notifications at top; hide empty device widget | Implemented; hardware hotplug review pending | Verify notification interaction and USB insertion/removal |
| R12 | Dark theme, existing Wi-Fi/brightness/clock sizes, CachyOS launcher, clear overview icon | **In progress — latest dock rejected** | Keep validated baseline; fix icon contrast and centering |
| R13 | Wake the display with one tap | **User validated; installed and persistent** | Physical test confirmed; a screenshot cannot prove display wakeup |
| R14 | Firefox tabs on the left | Requested; native setting under investigation | Validate in Firefox before changing its normal profile |
| R15 | Enable/disable and restore previous setup | Build, package backup, and rollback checks passed; live rollback review pending | No main-desktop workspace activation yet |
| R16 | Per-step reports with screenshots | First report published; ongoing | Append decisions and mark user approvals explicitly |

## Evidence scope

The [automated results](validation/2026-10-31/checks.json) contain 34 integration checks
from an isolated KWin session on the target machine. These exercise instrumented Qt
Wayland/XWayland windows, not arbitrary production applications. Screenshots contain
synthetic test content; titles such as “Terminal” are labels on the test probe.

Verified cases include corner/center coordinates, two simultaneous touch contacts,
menu actions, native move cancellation, `Super+Tab`, overview selection, split selection,
input in both half-disks, mode restoration, targeted minimize/close, deferred mode changes
during held input, and preferences restored in a new process.

Remaining compatibility checks: client-side headerbars, non-resizable apps, large/nested
popups, native browser interactions, drag-and-drop, pointer constraints, and sustained performance.

## Review sequence

1. **Done:** confirm square/circle switching and circular touch alignment.
2. **Next:** correct the dock and window controls without changing the approved baseline.
3. Review overview selection and physical drag-to-split.
4. Test Firefox with vertical tabs and other actual applications.
5. Verify persistence and clean rollback before replacing the main workspace.

New requests receive an ID here. A passing test alone never changes a row to “user validated”.
