// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtQuick.Window
import QtTest
import org.kde.kirigami as Kirigami
import "../contents/ui" as Controls

Item {
    id: scene
    readonly property color foreground: Kirigami.Theme.textColor
    Rectangle { anchors.fill: parent; color: Kirigami.Theme.backgroundColor }
    width: 500
    height: 200
    Item {
        id: backend
        property bool connected: true
        property var calls: []
        property var workspaceState: ({ enabled: true, overview: false, activeId: "one", splitLeftId: "", splitRightId: "", windows: [] })
        function findWindow(id) { return (workspaceState.windows || []).find(window => window.id === id) || (id ? { id: id, title: id, circular: true } : null); }
        function call(method, args) { calls.push({ method: method, args: args }); }
    }
    Controls.ControlStrip { id: right; backend: backend; role: "right"; width: implicitWidth; height: implicitHeight }
    Controls.ControlStrip { id: left; y: 50; backend: backend; role: "left"; width: implicitWidth; height: implicitHeight }
    Controls.ControlStrip { id: overview; y: 100; backend: backend; role: "overview"; width: implicitWidth; height: implicitHeight }
    TestCase {
        name: "RoundControls"
        when: windowShown
        function init() {
            left.visible = Qt.binding(() => left.shown);
            right.visible = Qt.binding(() => right.shown);
            backend.connected = true;
            backend.calls = [];
            backend.workspaceState = { enabled: true, overview: false, activeId: "one", splitLeftId: "", splitRightId: "", windows: [] };
            wait(20);
        }
        function test_normalControls() {
            compare(left.implicitWidth, 0);
            verify(right.implicitWidth >= 120);
            compare(right.targetId, "one");
            mouseClick(findChild(right, "action-Minimize"));
            compare(backend.calls[0].method, "Minimize");
            compare(backend.calls[0].args[0], "one");
        }
        function test_splitControls() {
            backend.workspaceState = { enabled: true, overview: false, activeId: "one", splitLeftId: "left-id", splitRightId: "right-id", windows: [] };
            verify(left.implicitWidth >= 120);
            compare(left.targetId, "left-id");
            compare(right.targetId, "right-id");
            waitForRendering(right);
            mouseClick(findChild(left, "action-Close"));
            compare(backend.calls[0].args[0], "left-id");
            mouseClick(findChild(right, "action-ExitSplitFor"));
            compare(backend.calls[1].method, "ExitSplitFor");
            compare(backend.calls[1].args[0], "right-id");
        }
        function test_overviewGlyphContrast() {
            const image = grabImage(scene);
            const scale = scene.Screen.devicePixelRatio;
            let foregroundPixels = 0;
            for (let y = 105; y < 135; ++y) {
                for (let x = 5; x < 35; ++x) {
                    const px = Math.floor(x * scale), py = Math.floor(y * scale);
                    if (px >= image.width || py >= image.height) continue;
                    const pixel = image.pixel(px, py);
                    if (Math.abs(pixel.r - scene.foreground.r) < .10
                        && Math.abs(pixel.g - scene.foreground.g) < .10
                        && Math.abs(pixel.b - scene.foreground.b) < .10) ++foregroundPixels;
                }
            }
            verify(foregroundPixels > 40, "Overview glyph must use the theme foreground");
        }
        function test_buttonsFollowArc() {
            for (const top of [true, false]) {
                right.arcAtTop = top;
                for (const method of ["Minimize", "ToggleMode", "Close"]) {
                    const button = findChild(right, "action-" + method);
                    const dx = button.x + button.width / 2 - right.width / 2;
                    const dy = button.y + button.height / 2 - (right.buttonSize / 2 + (top ? right.arcRadius : -right.arcRadius));
                    verify(Math.abs(Math.hypot(dx, dy) - right.arcRadius) < 1, "Button center must lie on the arc");
                    verify(button.flat);
                }
            }
            right.arcAtTop = true;
            verify(!findChild(overview, "action-ToggleOverview").checkable);
        }
        function test_overview() {
            mouseClick(findChild(overview, "action-ToggleOverview"));
            compare(backend.calls[0].method, "ToggleOverview");
        }
        function test_hiddenDuringOverview() {
            backend.workspaceState = { enabled: true, overview: true, activeId: "one", splitLeftId: "left-id", splitRightId: "right-id", windows: [] };
            compare(left.implicitWidth, 0);
            compare(right.implicitWidth, 0);
            verify(!findChild(right, "window-title").visible);
            verify(overview.visible);
        }
        function test_hostCannotRevealHiddenActions() {
            left.visible = true;
            verify(!left.shown);
            verify(!findChild(left, "window-actions").visible);
            verify(!findChild(left, "action-Close").enabled);
            backend.workspaceState = { enabled: true, overview: true, activeId: "one", splitLeftId: "", splitRightId: "", windows: [] };
            right.visible = true;
            verify(!findChild(right, "window-actions").visible);
            verify(!findChild(right, "window-title").visible);
        }
        function test_lateEnable() {
            backend.workspaceState = { enabled: false, overview: false, activeId: "", splitLeftId: "", splitRightId: "", windows: [] };
            compare(right.implicitWidth, 0);
            wait(30);
            backend.workspaceState = { enabled: true, overview: false, activeId: "one", splitLeftId: "", splitRightId: "", windows: [] };
            tryCompare(right, "implicitWidth", 196);
            verify(right.visible);
            compare(right.targetId, "one");
        }
        function test_noWindowTarget() {
            backend.workspaceState = { enabled: true, overview: false, activeId: "", splitLeftId: "", splitRightId: "", windows: [] };
            compare(right.implicitWidth, 0);
            verify(!findChild(right, "window-actions").visible);
            backend.workspaceState = { enabled: true, overview: false, activeId: "one", splitLeftId: "", splitRightId: "", windows: [{id:"one",title:"one",minimized:true}] };
            compare(right.implicitWidth, 0);
            verify(!findChild(right, "window-actions").visible);
        }
        function test_title() {
            const title = findChild(right, "window-title");
            verify(title.visible);
            compare(title.title, "one");
            compare(right.arcRadius, 332);
            compare(title.height, right.buttonSize);
            compare(findChild(right, "window-actions").x, 0);
        }
        function test_disconnectedControls() {
            backend.connected = false;
            verify(!findChild(right, "action-Close").enabled);
            verify(!findChild(overview, "action-ToggleOverview").enabled);
        }
    }
}
