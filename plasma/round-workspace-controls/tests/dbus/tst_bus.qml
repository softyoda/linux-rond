// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtTest
import "../../contents/ui" as Controls
Item {
    Controls.WorkspaceBus { id: backend }
    TestCase {
        name: "WorkspaceBus"
        when: windowShown
        function test_connectionAndActions() {
            tryCompare(backend, "connected", true);
            compare(backend.workspaceState.activeId, "one");
            backend.call("Minimize", ["left-id"]);
            tryVerify(() => backend.workspaceState.lastAction === "minimize:left-id");
            backend.setEnabled(false);
            tryVerify(() => backend.workspaceState.enabled === false);
        }
    }
}
