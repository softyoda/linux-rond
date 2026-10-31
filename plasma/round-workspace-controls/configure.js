// Add controls and journal only the settings we change.
var plugin = "org.softyoda.roundworkspacecontrols";
var journalGroup = ["LinuxRondWorkspace"];
var arcs = panels().filter(function(panel) { return panel.type === "org.softyoda.arcpanel"; });
if (!arcs.length) throw new Error("No arc panels found");
function ensure(panel, role, added) {
    var existing = panel.widgets().find(function(widget) {
        if (widget.type !== plugin) return false;
        widget.currentConfigGroup = ["General"];
        return widget.readConfig("role", "right") === role;
    });
    if (existing) return;
    var widget = panel.addWidget(plugin);
    widget.currentConfigGroup = ["General"];
    widget.writeConfig("role", role);
    added.push(String(widget.id));
    panel.currentConfigGroup = journalGroup;
    panel.writeConfig("addedWidgetIds", JSON.stringify(added));
}
arcs.forEach(function(panel) {
    if (panel.location !== "top" && panel.location !== "bottom") return;
    panel.currentConfigGroup = journalGroup;
    var active = String(panel.readConfig("active", false)) === "true";
    if (!active) {
        panel.writeConfig("originalHeight", panel.height);
        panel.writeConfig("appliedHeight", Math.max(panel.height, 260));
        panel.currentConfigGroup = ["General"];
        var tasksIndex = panel.readConfig("tasksIndex", 1);
        panel.currentConfigGroup = journalGroup;
        panel.writeConfig("originalTasksIndex", tasksIndex);
        panel.writeConfig("addedWidgetIds", "[]");
        panel.writeConfig("addedNotificationIds", "[]");
        panel.writeConfig("movedNotifications", "[]");
        panel.writeConfig("active", true);
    }
    var added = JSON.parse(panel.readConfig("addedWidgetIds", "[]"));
    if (panel.location === "top") {
        var appliedHeight = Number(panel.readConfig("appliedHeight", 260));
        if (!active) {
            panel.height = appliedHeight;
        } else if (appliedHeight < 260) {
            if (panel.height !== appliedHeight) panel.writeConfig("originalHeight", panel.height);
            panel.height = Math.max(panel.height, 260);
            panel.writeConfig("appliedHeight", panel.height);
        }
        ensure(panel, "left", added);
        ensure(panel, "right", added);
    } else {
        ensure(panel, "overview", added);
        if (!active) {
            panel.currentConfigGroup = ["General"];
            panel.writeConfig("tasksIndex", 2);
        }
    }
});
var notificationsPlugin = "org.kde.plasma.notifications";
arcs.filter(function(panel) { return panel.location === "top"; }).forEach(function(top) {
    if (top.widgets().some(function(widget) { return widget.type === notificationsPlugin; })) return;
    var bottoms = arcs.filter(function(panel) { return panel.location === "bottom" && panel.screen === top.screen; });
    var source = null;
    var notification = null;
    bottoms.some(function(panel) {
        notification = panel.widgets().find(function(widget) { return widget.type === notificationsPlugin; });
        if (notification) source = panel;
        return !!notification;
    });
    if (notification) {
        source.currentConfigGroup = journalGroup;
        var moved = JSON.parse(source.readConfig("movedNotifications", "[]"));
        moved.push({ id: String(notification.id), target: String(top.id), index: notification.index });
        source.writeConfig("movedNotifications", JSON.stringify(moved));
        top.addWidget(notification);
    } else {
        var added = top.addWidget(notificationsPlugin);
        top.currentConfigGroup = journalGroup;
        top.writeConfig("addedNotificationIds", JSON.stringify([String(added.id)]));
    }
});
