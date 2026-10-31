// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick

QtObject {
    id: watcher
    property var item: null
    property bool workspaceControl: false
    signal changed()
    property Connections itemSignals: Connections {
        target: watcher.item
        ignoreUnknownSignals: true
        function onImplicitWidthChanged() { watcher.changed(); }
        function onCompactRepresentationItemChanged() { watcher.changed(); }
    }
    property Connections representationSignals: Connections {
        target: watcher.item ? watcher.item.compactRepresentationItem : null
        ignoreUnknownSignals: true
        function onImplicitWidthChanged() { watcher.changed(); }
        function onImplicitHeightChanged() { watcher.changed(); }
        function onShownChanged() { watcher.changed(); }
        function onBackendChanged() { watcher.changed(); }
    }
    property Connections backendSignals: Connections {
        target: {
            const rep = watcher.item ? watcher.item.compactRepresentationItem : null;
            return watcher.workspaceControl && rep && rep.backend ? rep.backend : null;
        }
        ignoreUnknownSignals: true
        function onWorkspaceStateChanged() { watcher.changed(); }
        function onConnectedChanged() { watcher.changed(); }
    }
    property Connections statusSignals: Connections {
        target: watcher.item ? watcher.item.plasmoid : null
        ignoreUnknownSignals: true
        function onStatusChanged() { watcher.changed(); }
    }
}
