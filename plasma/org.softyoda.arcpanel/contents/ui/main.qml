// Plasma 6 panel containment with widgets and tasks arranged along an arc.
import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami
import org.kde.taskmanager as TaskManager
import "ArcGeometry.js" as ArcGeometry

ContainmentItem {
    id: root

    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground

    readonly property int iconSize: Plasmoid.configuration.iconSize
    readonly property int gap: Plasmoid.configuration.gap
    readonly property int edgeMargin: Plasmoid.configuration.edgeMargin
    readonly property bool atTop: Plasmoid.location === PlasmaCore.Types.TopEdge
    // Inscribed circle in panel coordinates.
    readonly property real radius: Math.min(Screen.width, Screen.height) / 2
    readonly property real cx: width / 2
    readonly property real cy: atTop ? radius : height - radius
    // Offset icons outward to center each icon and its status indicator.
    readonly property real track: radius - edgeMargin - iconSize / 2
    readonly property real indicatorGap: 4
    readonly property real iconShift: Plasmoid.configuration.showTasks ? (indicatorGap + 3) / 2 : 0
    readonly property real iconTrack: (!atTop && contentRadius > 0 ? backgroundBand.layoutCenter : track) + iconShift

    property var appletItems: []
    property var appletWatchers: []
    property bool layoutBusy: false
    property var workspaceBackend: null
    readonly property real dpr: Math.max(1, Screen.devicePixelRatio)
    readonly property real contentRadius: workspaceBackend && workspaceBackend.connected
        && workspaceBackend.workspaceState.enabled && workspaceBackend.workspaceState.content
        ? workspaceBackend.workspaceState.content.width / 2 : 0
    readonly property var backgroundBand: ArcGeometry.backgroundBand(track, iconSize, contentRadius, dpr, radius)

    onContentRadiusChanged: Qt.callLater(root.relayout)

    function addApplet(applet) {
        const item = root.itemFor(applet);
        if (!item || appletItems.indexOf(item) >= 0) return;
        item.parent = arcLayer;
        item.visible = true;
        if (pluginOf(item) === controlsPlugin) item.opacity = 0;
        const watcher = appletWatcherComponent.createObject(root, { item: item, workspaceControl: pluginOf(item) === controlsPlugin });
        watcher.changed.connect(root.relayout);
        appletWatchers.push(watcher);
        appletItems.push(item);
        appletItems = appletItems.slice();
        relayout();
    }
    function removeApplet(applet) {
        const item = root.itemFor(applet);
        appletItems = appletItems.filter(i => i !== item);
        const removed = appletWatchers.filter(watcher => watcher.item === item);
        appletWatchers = appletWatchers.filter(watcher => watcher.item !== item);
        removed.forEach(watcher => watcher.destroy());
        relayout();
    }
    Component { id: appletWatcherComponent; AppletWatcher {} }

    // Insert optional tasks among the ordered applets.
    function elements() {
        const controls = appletItems.filter(item => pluginOf(item) === controlsPlugin);
        workspaceBackend = controls.map(item => item.compactRepresentationItem)
            .filter(rep => rep && rep.backend).map(rep => rep.backend)[0] || null;
        const out = appletItems.filter(item => {
            const plugin = pluginOf(item);
            const hidden = (plugin === "org.kde.plasma.devicenotifier" && item.plasmoid.status === PlasmaCore.Types.PassiveStatus)
                || (!atTop && controls.length > 0 && plugin === "org.kde.plasma.notifications");
            if (plugin === "org.kde.plasma.devicenotifier" || (!atTop && plugin === "org.kde.plasma.notifications")) {
                item.visible = true;
                item.opacity = hidden ? 0 : 1;
                item.enabled = !hidden;
            }
            return plugin !== controlsPlugin && !hidden;
        });
        if (atTop) {
            const notifications = out.filter(item => pluginOf(item) === "org.kde.plasma.notifications");
            notifications.forEach(item => out.splice(out.indexOf(item), 1));
            const device = out.findIndex(item => pluginOf(item) === "org.kde.plasma.devicenotifier");
            const brightness = out.findIndex(item => pluginOf(item) === "org.kde.plasma.brightness");
            out.splice(device >= 0 ? device + 1 : (brightness >= 0 ? brightness : out.length), 0, ...notifications);
        }
        // Keep wrappers visible so Plasma can load their compact representations.
        controls.forEach(item => {
            const placed = elementWidth(item) > 0;
            item.visible = true;
            item.opacity = placed ? 1 : 0;
            item.enabled = placed;
        });
        const shown = controls.filter(item => elementWidth(item) > 0);
        const overview = shown.filter(item => controlRole(item) === "overview");
        const launcher = out.findIndex(item => ["org.kde.plasma.kickoff", "org.kde.plasma.kicker"].includes(pluginOf(item)));
        out.splice(Math.max(0, launcher + 1), 0, ...overview);
        out.unshift(...shown.filter(item => controlRole(item) === "left"));
        out.push(...shown.filter(item => controlRole(item) === "right"));
        if (Plasmoid.configuration.showTasks) {
            const t = [];
            for (let i = 0; i < taskRepeater.count; ++i) t.push(taskRepeater.itemAt(i));
            const at = Math.max(0, Math.min(Plasmoid.configuration.tasksIndex, out.length));
            const group = t.filter(x => x);
            if (group.length) {
                if (at > 0) group.unshift(sepBefore);
                if (at < out.length) group.push(sepAfter);
            }
            out.splice(at, 0, ...group);
        }
        return out;
    }

    // Clock text and tray contents may span multiple icon slots.
    readonly property var wideApplets: ({ "org.kde.plasma.digitalclock": 2.6 })
    // Match the apparent size of filled icons and symbolic icons.
    readonly property var iconScale: ({ "org.kde.plasma.devicenotifier": 0.62, "org.kde.plasma.notifications": 1.25 })
    readonly property real defaultAppletScale: 0.92
    readonly property string trayPlugin: "org.kde.plasma.systemtray"
    readonly property string controlsPlugin: "org.softyoda.roundworkspacecontrols"

    function controlRole(item) { return String(item.plasmoid.configuration.role || "right"); }

    function pluginOf(item) { return item && item.plasmoid ? item.plasmoid.pluginName : ""; }
    function visualScale(item) {
        const plugin = pluginOf(item);
        return !plugin || plugin === controlsPlugin ? 1 : (iconScale[plugin] || defaultAppletScale);
    }

    function elementWidth(item) {
        if (item === sepBefore || item === sepAfter) return 2;
        const name = pluginOf(item);
        if (name === controlsPlugin) {
            const rep = item.compactRepresentationItem;
            return rep ? Math.max(0, rep.implicitWidth) : Math.max(0, item.Layout.minimumWidth || 0);
        }
        if (wideApplets[name]) {
            // Center the actual clock text.
            const rep = item.compactRepresentationItem;
            const m = Math.max(rep ? (rep.Layout.minimumWidth || 0) : 0, rep ? (rep.implicitWidth || 0) : 0, item.Layout.minimumWidth || 0);
            return isFinite(m) && m > iconSize / 2 && m < 6 * iconSize ? Math.round(m) : Math.round(iconSize * wideApplets[name]);
        }
        if (name === trayPlugin) {
            const rep = item.compactRepresentationItem;
            const w = Math.max(item.Layout.minimumWidth || 0, item.Layout.preferredWidth || 0, item.implicitWidth || 0,
                               rep ? (rep.Layout.minimumWidth || 0) : 0, rep ? (rep.implicitWidth || 0) : 0,
                               rep ? rep.childrenRect.width : 0, item.childrenRect.width);
            return isFinite(w) && w > iconSize ? Math.min(w, 8 * iconSize) : iconSize;
        }
        return iconSize;
    }

    property var backgroundSegments: []
    property real arcStart: 0
    property real arcEnd: 0
    // Indicators: { phi, span, state: active | open | minimized }.
    property var indicators: []

    function relayout() {
        if (layoutBusy) { Qt.callLater(root.relayout); return; }
        layoutBusy = true;
        try {
            const els = elements();
            sepBefore.visible = els.indexOf(sepBefore) >= 0;
            sepAfter.visible = els.indexOf(sepAfter) >= 0;
            if (!els.length || track <= 0) { backgroundSegments = []; arcStart = arcEnd = 0; bg.requestPaint(); return; }
            const widths = els.map(elementWidth);
            const kinds = els.map(item => item.isTask ? "tasks" : pluginOf(item) === controlsPlugin ? controlRole(item)
                : (pluginOf(item) === "org.kde.plasma.digitalclock" ? "clock" : "system"));
            const slots = ArcGeometry.layoutSlots(widths, kinds, gap, atTop && contentRadius > 0, !atTop && contentRadius > 0);
            const ind = [];
            for (let i = 0; i < els.length; ++i) {
                const it = els[i], w = widths[i];
                const phi = (slots.starts[i] + w / 2) / track;           // Angle from the vertical axis.
                const px = cx + iconTrack * Math.sin(phi);
                const py = atTop ? cy - iconTrack * Math.cos(phi) : cy + iconTrack * Math.cos(phi);
                const isSep = it === sepBefore || it === sepAfter;
                it.width = w;
                it.height = isSep ? Math.round(iconSize * 0.6) : iconSize;
                it.x = px - w / 2;
                it.y = py - it.height / 2;
                // Separators always follow the arc; icon tilt is optional.
                const tilt = Plasmoid.configuration.tilt || isSep || pluginOf(it) === controlsPlugin;
                it.rotation = tilt ? (atTop ? phi : -phi) * 180 / Math.PI : 0;
                if (pluginOf(it) === controlsPlugin && it.compactRepresentationItem) {
                    it.compactRepresentationItem.arcRadius = iconTrack;
                    it.compactRepresentationItem.arcAtTop = atTop;
                    it.compactRepresentationItem.titleMaxWidth = ArcGeometry.titleWidth(w, track, backgroundBand, iconTrack);
                }
                it.scale = visualScale(it);
                if (it.isTask) {
                    const st = it.model.IsActive ? "active" : (it.model.IsMinimized ? "minimized" : "open");
                    ind.push({ phi: phi, span: w / track, state: st });
                }
            }
            indicators = ind;
            arcStart = slots.start / track;
            arcEnd = slots.end / track;
            backgroundSegments = slots.segments.map(segment => ({ start: segment.start / track, end: segment.end / track }));
            bg.requestPaint();
        } finally {
            layoutBusy = false;
        }
    }

    onWidthChanged: relayout()
    onHeightChanged: relayout()
    onAtTopChanged: relayout()
    onIconSizeChanged: relayout()

    Connections {
        target: Plasmoid
        function onAppletAdded(applet, geometry) { root.addApplet(applet); }
        function onAppletRemoved(applet) { root.removeApplet(applet); }
    }
    Timer { interval: 1500; repeat: true; running: true; onTriggered: root.relayout() }

    Component.onCompleted: {
        Plasmoid.applets.forEach(a => addApplet(a));
        relayout();
    }

    // Arc background with round caps.
    Canvas {
        id: bg
        anchors.fill: parent
        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            if (!root.backgroundSegments.length) return;
            const band = root.backgroundBand;
            const pad = root.contentRadius > 0 ? Math.asin(Math.min(1, band.width / (2 * band.center))) : root.iconSize / (2 * root.track);
            const c = Kirigami.Theme.backgroundColor;
            ctx.strokeStyle = ctx.fillStyle = Qt.rgba(c.r, c.g, c.b, 0.82);
            ctx.lineWidth = band.width;
            ctx.lineCap = "round";
            root.backgroundSegments.forEach(segment => {
                const a0 = root.atTop ? -Math.PI / 2 + segment.start + pad : Math.PI / 2 - segment.end + pad;
                const a1 = root.atTop ? -Math.PI / 2 + segment.end - pad : Math.PI / 2 - segment.start - pad;
                ctx.beginPath();
                if (a1 > a0) {
                    ctx.arc(root.cx, root.cy, band.center, a0, a1, false);
                    ctx.stroke();
                } else {
                    const mid = (a0 + a1) / 2;
                    ctx.arc(root.cx + band.center * Math.cos(mid), root.cy + band.center * Math.sin(mid), band.width / 2, 0, 2 * Math.PI);
                    ctx.fill();
                }
            });

            // Status arcs face the screen center.
            const r = root.iconTrack - root.iconSize / 2 - root.indicatorGap;
            const hl = Kirigami.Theme.highlightColor, tx = Kirigami.Theme.textColor;
            ctx.lineCap = "round";
            ctx.lineWidth = 3;
            root.indicators.forEach(i => {
                const frac = i.state === "active" ? 0.62 : 0.24;
                const half = i.span * frac / 2;
                const mid = root.atTop ? -Math.PI / 2 + i.phi : Math.PI / 2 - i.phi;
                ctx.beginPath();
                ctx.arc(root.cx, root.cy, r, mid - half, mid + half, false);
                ctx.strokeStyle = i.state === "minimized" ? Qt.rgba(tx.r, tx.g, tx.b, 0.35)
                                : Qt.rgba(hl.r, hl.g, hl.b, i.state === "active" ? 1 : 0.6);
                ctx.stroke();
            });
        }
    }

    Item { id: arcLayer; anchors.fill: parent }

    // Radial separators.
    component Separator: Rectangle {
        parent: arcLayer
        radius: width / 2
        color: Kirigami.Theme.textColor
        opacity: 0.35
        visible: false
    }
    Separator { id: sepBefore }
    Separator { id: sepAfter }

    // Optional task list.
    TaskManager.TasksModel {
        id: tasksModel
        filterByVirtualDesktop: false
        filterByScreen: false
        filterByActivity: true
        groupMode: TaskManager.TasksModel.GroupApplications
        sortMode: TaskManager.TasksModel.SortManual
        launcherList: []
    }
    Connections {
        target: tasksModel
        function onDataChanged() { Qt.callLater(root.relayout); }
    }
    Repeater {
        id: taskRepeater
        model: Plasmoid.configuration.showTasks ? tasksModel : null
        onCountChanged: Qt.callLater(root.relayout)
        delegate: Item {
            id: task
            parent: arcLayer
            required property var model
            required property int index
            readonly property bool isTask: true
            Kirigami.Icon {
                anchors.fill: parent
                anchors.margins: parent.width * 0.1
                source: task.model.decoration
                opacity: task.model.IsMinimized ? 0.45 : 1
                active: mouse.containsMouse
            }
            MouseArea {
                id: mouse
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                onClicked: event => {
                    const idx = tasksModel.makeModelIndex(task.index);
                    if (event.button === Qt.MiddleButton) tasksModel.requestNewInstance(idx);
                    else if (task.model.IsActive) tasksModel.requestToggleMinimized(idx);
                    else tasksModel.requestActivate(idx);
                }
            }
        }
    }
}
