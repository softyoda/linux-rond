/*
 * Barre en arc pour écran rond (conteneur de panneau Plasma 6).
 * Les widgets (et, en option, les applis ouvertes) sont placés le long du cercle
 * inscrit dans l'écran, sur un fond courbe. Bord haut ou bas selon le panneau.
 */
import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami
import org.kde.taskmanager as TaskManager

ContainmentItem {
    id: root

    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground

    readonly property int iconSize: Plasmoid.configuration.iconSize
    readonly property int gap: Plasmoid.configuration.gap
    readonly property int edgeMargin: Plasmoid.configuration.edgeMargin
    readonly property bool atTop: Plasmoid.location === PlasmaCore.Types.TopEdge
    // Cercle inscrit dans l'écran (coordonnées du panneau)
    readonly property real radius: Math.min(Screen.width, Screen.height) / 2
    readonly property real cx: width / 2
    readonly property real cy: atTop ? radius : height - radius
    // Rayon de la bande (fond) ; les icônes sont décalées vers le bord quand des
    // traits d'état sont affichés, pour que « trait + icône » reste centré dans la bande.
    readonly property real track: radius - edgeMargin - iconSize / 2
    readonly property real indicatorGap: 4
    readonly property real iconShift: Plasmoid.configuration.showTasks ? (indicatorGap + 3) / 2 : 0
    readonly property real iconTrack: track + iconShift

    property var appletItems: []

    function addApplet(applet) {
        const item = root.itemFor(applet);
        if (!item || appletItems.indexOf(item) >= 0) return;
        item.parent = arcLayer;
        item.implicitWidthChanged.connect(() => Qt.callLater(root.relayout));
        if (item.compactRepresentationItem)
            item.compactRepresentationItem.implicitWidthChanged.connect(() => Qt.callLater(root.relayout));
        item.visible = true;
        appletItems.push(item);
        appletItems = appletItems.slice();
        relayout();
    }
    function removeApplet(applet) {
        const item = root.itemFor(applet);
        appletItems = appletItems.filter(i => i !== item);
        relayout();
    }

    // Liste ordonnée des éléments à placer : widgets + (option) applis ouvertes
    function elements() {
        const out = appletItems.slice();
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

    // Toutes les icônes ont la même taille ; seuls l'horloge (texte) et la zone de
    // notification (plusieurs icônes) sont plus larges.
    readonly property var wideApplets: ({ "org.kde.plasma.digitalclock": 2.6 })
    // Icônes en couleur qui remplissent leur case : réduites pour s'aligner sur les pictos
    readonly property var iconScale: ({ "org.kde.plasma.devicenotifier": 0.62, "org.kde.plasma.notifications": 1.25 })
    readonly property real defaultAppletScale: 0.92
    readonly property string trayPlugin: "org.kde.plasma.systemtray"

    function pluginOf(item) { return item && item.plasmoid ? item.plasmoid.pluginName : ""; }

    function elementWidth(item) {
        if (item === sepBefore || item === sepAfter) return 2;
        const name = pluginOf(item);
        if (wideApplets[name]) {
            // largeur réelle du texte de l'horloge, pour qu'elle soit vraiment centrée
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

    property real arcStart: 0
    property real arcEnd: 0
    // indicateurs des applis : { phi, span, state } (state : active | open | minimized)
    property var indicators: []

    function relayout() {
        const els = elements();
        sepBefore.visible = els.indexOf(sepBefore) >= 0;
        sepAfter.visible = els.indexOf(sepAfter) >= 0;
        if (!els.length || track <= 0) { arcStart = arcEnd = 0; bg.requestPaint(); return; }
        const widths = els.map(elementWidth);
        const total = widths.reduce((a, b) => a + b, 0) + gap * (els.length - 1);
        let s = -total / 2;
        const ind = [];
        for (let i = 0; i < els.length; ++i) {
            const it = els[i], w = widths[i];
            const phi = (s + w / 2) / track;           // angle depuis l'axe vertical
            const px = cx + iconTrack * Math.sin(phi);
            const py = atTop ? cy - iconTrack * Math.cos(phi) : cy + iconTrack * Math.cos(phi);
            const isSep = it === sepBefore || it === sepAfter;
            it.width = w;
            it.height = isSep ? Math.round(iconSize * 0.6) : iconSize;
            it.x = px - w / 2;
            it.y = py - it.height / 2;
            // les éléments larges (zone de notification) sont inclinés pour suivre la courbe
            const tilt = Plasmoid.configuration.tilt || isSep;
            it.rotation = tilt ? (atTop ? phi : -phi) * 180 / Math.PI : 0;
            it.scale = isSep || !pluginOf(it) ? 1 : (iconScale[pluginOf(it)] || defaultAppletScale);
            if (it.isTask) {
                const st = it.model.IsActive ? "active" : (it.model.IsMinimized ? "minimized" : "open");
                ind.push({ phi: phi, span: w / track, state: st });
            }
            s += w + gap;
        }
        indicators = ind;
        arcStart = -total / 2 / track;
        arcEnd = total / 2 / track;
        bg.requestPaint();
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

    // Fond courbe : un trait épais, arrondi aux bouts, qui suit le cercle
    Canvas {
        id: bg
        anchors.fill: parent
        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            if (root.arcEnd <= root.arcStart) return;
            // les bouts arrondis débordent de lineWidth/2 : on raccourcit l'arc pour garder une petite marge
            const pad = -(root.iconSize / 2) / root.track;
            const a0 = root.atTop ? -Math.PI / 2 + root.arcStart - pad : Math.PI / 2 - root.arcEnd - pad;
            const a1 = root.atTop ? -Math.PI / 2 + root.arcEnd + pad : Math.PI / 2 - root.arcStart + pad;
            ctx.beginPath();
            ctx.arc(root.cx, root.cy, root.track, a0, a1, false);
            ctx.lineWidth = root.iconSize + 12;
            ctx.lineCap = "round";
            const c = Kirigami.Theme.backgroundColor;
            ctx.strokeStyle = Qt.rgba(c.r, c.g, c.b, 0.82);
            ctx.stroke();

            // traits d'état des applis, côté intérieur (vers le centre de l'écran)
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

    // Fins séparateurs (orientés vers le centre du cercle)
    component Separator: Rectangle {
        parent: arcLayer
        radius: width / 2
        color: Kirigami.Theme.textColor
        opacity: 0.35
        visible: false
    }
    Separator { id: sepBefore }
    Separator { id: sepAfter }

    // Applis ouvertes (option showTasks)
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
