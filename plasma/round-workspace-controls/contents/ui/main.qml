// SPDX-License-Identifier: GPL-2.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore

PlasmoidItem {
    id: root
    readonly property string controlRole: Plasmoid.configuration.role
    readonly property bool split: !!workspaceBus.workspaceState.splitLeftId && !!workspaceBus.workspaceState.splitRightId
    readonly property string targetId: split
        ? (controlRole === "left" ? workspaceBus.workspaceState.splitLeftId : workspaceBus.workspaceState.splitRightId)
        : (controlRole === "left" ? "" : workspaceBus.workspaceState.activeId || "")
    readonly property var targetWindow: workspaceBus.findWindow(targetId)
    readonly property bool groupVisible: controlRole === "overview" || (workspaceBus.workspaceState.enabled && !workspaceBus.workspaceState.overview && !!targetWindow && !targetWindow.minimized && (controlRole !== "left" || split))
    readonly property int buttonSize: Plasmoid.configuration.buttonSize
    readonly property int groupWidth: groupVisible ? (controlRole === "overview" ? buttonSize : Math.round(buttonSize * 4.8 + 4)) : 0
    Layout.minimumWidth: groupWidth
    Layout.preferredWidth: groupWidth
    Layout.minimumHeight: buttonSize
    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground
    preferredRepresentation: compactRepresentation
    toolTipMainText: qsTr("Linux Rond")
    toolTipSubText: workspaceBus.connected ? qsTr("Window controls") : qsTr("Round workspace is unavailable")

    WorkspaceBus { id: workspaceBus }
    compactRepresentation: ControlStrip {
        backend: workspaceBus
        role: root.controlRole
        buttonSize: root.buttonSize
    }
    fullRepresentation: ColumnLayout {
        Label { text: qsTr("Round workspace") }
        Switch {
            text: qsTr("Enable round workspace")
            checked: workspaceBus.workspaceState.enabled
            enabled: workspaceBus.connected
            onToggled: workspaceBus.setEnabled(checked)
        }
        Label {
            visible: !workspaceBus.connected
            text: qsTr("Load the Linux Rond KWin extension first.")
            wrapMode: Text.WordWrap
            Layout.preferredWidth: 240
        }
    }
    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: workspaceBus.workspaceState.enabled ? qsTr("Disable round workspace") : qsTr("Enable round workspace")
            icon.name: "preferences-system-windows"
            enabled: workspaceBus.connected
            onTriggered: workspaceBus.setEnabled(!workspaceBus.workspaceState.enabled)
        }
    ]
}
