// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import QtTest
import "../contents/ui" as Arc
Item {
    id: scene
    QtObject { id: backendObject; property var workspaceState: ({enabled:false}); property bool connected:false }
    QtObject { id: plasmoidObject; property int status:0 }
    QtObject {
        id: applet
        property var compactRepresentationItem:null
        property var plasmoid:plasmoidObject
        property real implicitWidth:0
    }
    Component {
        id: representation
        QtObject {
            property var backend:backendObject
            property real implicitWidth:0
            property real implicitHeight:40
            property bool shown:false
        }
    }
    Arc.AppletWatcher { id: watcher; item:applet; workspaceControl:true }
    SignalSpy { id: spy; target:watcher; signalName:"changed" }
    TestCase {
        name:"AppletWatcher"
        function test_lateRepresentationSignals() {
            const rep=representation.createObject(scene);
            applet.compactRepresentationItem=rep;
            verify(spy.count>0);
            spy.clear();
            rep.implicitWidth=124;
            compare(spy.count,1);
            rep.shown=true;
            compare(spy.count,2);
            backendObject.workspaceState={enabled:true};
            compare(spy.count,3);
            applet.compactRepresentationItem=null;
            spy.clear();
            rep.implicitWidth=40;
            compare(spy.count,0);
            rep.destroy();
        }
    }
}
