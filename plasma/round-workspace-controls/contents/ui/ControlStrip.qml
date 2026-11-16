// SPDX-License-Identifier: GPL-2.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import org.kde.plasma.components as PlasmaComponents3
import QtQuick.Shapes
import org.kde.kirigami as Kirigami
import "ControlGeometry.js" as ControlGeometry

Item {
    id: strip
    required property var backend
    property string role: "right"
    property int buttonSize: 40
    property real arcRadius: 332
    property bool arcAtTop: true
    property real titleMaxWidth: -1
    readonly property bool overviewButton: role === "overview"
    // Rotation applied to the whole strip by the arc panel (set by the panel); keeps buttons upright.
    property real screenRotation: 0
    readonly property bool split: !!backend.workspaceState.splitLeftId && !!backend.workspaceState.splitRightId
    // Single window: "left" shows the title, "right" the buttons. Split: each side shows both.
    readonly property bool hasWindow: backend.workspaceState.enabled && !backend.workspaceState.overview
        && !!target && !target.minimized
    readonly property bool showTitle: !overviewButton && hasWindow && (split || role === "left")
    readonly property bool showButtons: !overviewButton && hasWindow && (split || role === "right")
    readonly property bool shown: overviewButton || showTitle || showButtons
    readonly property string targetId: split
        ? (role === "left" ? backend.workspaceState.splitLeftId : backend.workspaceState.splitRightId)
        : (backend.workspaceState.activeId || "")
    readonly property var target: backend.findWindow(targetId)
    readonly property bool available: shown && backend.connected && backend.workspaceState.enabled && (overviewButton || !!target)
    readonly property int buttonSpacing: 2
    // Title slot fits the title, up to six buttons wide (or the arc limit).
    readonly property real titleLimit: titleMaxWidth >= 0 ? Math.min(titleMaxWidth, buttonSize * 6) : buttonSize * 6
    // Fixed width (same as the button row): the panel never relayouts when the title changes.
    readonly property real rowWidth: 3 * buttonSize + 2 * buttonSpacing
    readonly property real titleSlotWidth: showTitle ? (split ? Math.min(titleLimit, rowWidth) : rowWidth) : 0
    readonly property real buttonsOffset: titleSlotWidth + (showTitle && showButtons ? 2 * buttonSpacing : 0)
    implicitWidth: !shown ? 0 : (overviewButton ? buttonSize
        : buttonsOffset + (showButtons ? 3 * buttonSize + 2 * buttonSpacing : 0))
    implicitHeight: buttonSize
    visible: shown

    FontMetrics { id: titleMetrics; font: Kirigami.Theme.defaultFont }
    TextMetrics { id: fullTitle; font: titleMetrics.font; text: elidedTitle.text }
    TextMetrics {
        id: elidedTitle
        font: titleMetrics.font
        text: strip.target ? strip.target.title || strip.target.appId || "" : ""
        elide: Text.ElideRight
        elideWidth: Math.max(0, strip.titleSlotWidth - 16)
    }
    TextMetrics { id: displayedTitle; font: titleMetrics.font; text: elidedTitle.elidedText }
    function titleGlyphs(text, totalWidth, height) {
        let prefix = "";
        return Array.from(text).map(character => {
            const start = titleMetrics.advanceWidth(prefix);
            prefix += character;
            const end = titleMetrics.advanceWidth(prefix);
            return { text: character, width: end - start, height: height,
                center: (titleSlotWidth - totalWidth) / 2 + (start + end) / 2 };
        });
    }
    Item {
        id: titleArc
        objectName: "window-title"
        width: strip.width
        height: strip.buttonSize
        visible: strip.showTitle
        readonly property string title: elidedTitle.text
        Repeater {
            model: strip.titleGlyphs(displayedTitle.text, displayedTitle.advanceWidth, titleMetrics.height)
            delegate: Text {
                required property var modelData
                readonly property var point: ControlGeometry.arcPoint(modelData.center, strip.width, strip.buttonSize, strip.arcRadius, strip.arcAtTop)
                text: modelData.text
                font: titleMetrics.font
                color: strip.Kirigami.Theme.textColor
                renderType: Text.QtRendering
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                width: modelData.width
                height: modelData.height
                x: point.x - width / 2
                y: point.y - height / 2
                rotation: point.rotation
            }
        }
    }

    Item {
        objectName: "window-actions"
        anchors.fill: parent
        visible: strip.showButtons
        Repeater {
            model: !strip.showButtons ? [] : [
                { icon: "window-minimize", label: qsTr("Minimize"), method: "Minimize" },
                { icon: strip.split ? "view-restore" : (strip.target && strip.target.circular ? "view-fullscreen" : "draw-circle"),
                  label: strip.split ? qsTr("Leave split-screen") : qsTr("Switch square / circle"),
                  method: strip.split ? "ExitSplitFor" : "ToggleMode" },
                { icon: "window-close", label: qsTr("Close"), method: "Close" }
            ]
            delegate: PlasmaComponents3.ToolButton {
                id: actionButton
                required property var modelData
                required property int index
                readonly property var point: ControlGeometry.arcPoint(strip.buttonsOffset + index * (strip.buttonSize + strip.buttonSpacing) + strip.buttonSize / 2, strip.width, strip.buttonSize, strip.arcRadius, strip.arcAtTop)
                x: point.x - width / 2
                y: point.y - height / 2
                // Icons keep their on-screen orientation (a rotated "minimize" bar reads as an arrow).
                rotation: -strip.screenRotation
                flat: true
                objectName: "action-" + modelData.method
                width: strip.buttonSize
                height: strip.buttonSize
                icon.name: modelData.icon
                enabled: strip.available
                Accessible.name: (strip.split ? (strip.role === "left" ? qsTr("Left window: ") : qsTr("Right window: ")) : "") + modelData.label
                PlasmaComponents3.ToolTip {
                    text: actionButton.Accessible.name + (strip.target ? " — " + strip.target.title : "")
                }
                onClicked: strip.backend.call(modelData.method, [strip.targetId])
            }
        }
    }
    PlasmaComponents3.ToolButton {
        id: overviewControl
        flat: true
        objectName: "action-ToggleOverview"
        visible: strip.shown && strip.overviewButton
        width: strip.buttonSize
        height: strip.buttonSize
        contentItem: Item {
            implicitWidth: 31
            implicitHeight: 31
            Shape {
                objectName: "overview-glyph"
                anchors.centerIn: parent
                width: 24
                height: 24
                scale: 31 / 24
                opacity: overviewControl.enabled ? (strip.backend.workspaceState.overview ? 1 : 0.92) : 0.45
                ShapePath {
                    strokeColor: strip.Kirigami.Theme.textColor
                    strokeWidth: 2
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin
                    PathSvg {
                        path: "M8 3H19A2 2 0 0 1 21 5V15 M5 7H15A2 2 0 0 1 17 9V19A2 2 0 0 1 15 21H5A2 2 0 0 1 3 19V9A2 2 0 0 1 5 7Z M3 11H17"
                    }
                }
            }
        }
        enabled: strip.backend.connected && strip.backend.workspaceState.enabled
        Accessible.name: qsTr("Window overview")
        PlasmaComponents3.ToolTip { text: qsTr("Window overview (Super+Tab)") }
        onClicked: strip.backend.call("ToggleOverview", [])
    }
}
