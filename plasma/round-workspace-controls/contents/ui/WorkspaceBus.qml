// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.kde.plasma.workspace.dbus as DBus

Item {
    id: bus
    visible: false
    property var workspaceState: ({ enabled: false, overview: false, windows: [], activeId: "", splitLeftId: "", splitRightId: "" })
    property bool connected: false
    property bool pending: false
    property string errorText: ""

    function acceptState(value) {
        try {
            workspaceState = JSON.parse(value);
            connected = true;
            errorText = "";
        } catch (error) {
            connected = false;
            errorText = "Invalid workspace state";
        }
    }
    function call(member, args, complete) {
        DBus.SessionBus.asyncCall({
            service: "org.kde.KWin", path: "/LinuxRond", iface: "org.softyoda.LinuxRond",
            member: member, arguments: args || []
        }, result => {
            if (complete) complete(result.value);
            if (member !== "StateJson") refresh();
        }, result => {
            connected = false;
            pending = false;
            errorText = result.error.message;
        });
    }
    function refresh() {
        if (pending) return;
        pending = true;
        call("StateJson", [], value => {
            pending = false;
            acceptState(value);
        });
    }
    function findWindow(id) {
        return (workspaceState.windows || []).find(window => window.id === id) || null;
    }
    function setEnabled(value) { call("SetEnabled", [new DBus.bool(value)]); }

    Timer { interval: bus.connected ? 500 : 2000; repeat: true; running: true; onTriggered: bus.refresh() }
    Component.onCompleted: refresh()
}
