// Preserve unrelated widgets and settings changed after activation.
var plugin = "org.softyoda.roundworkspacecontrols";
panels().filter(function(panel) { return panel.type === "org.softyoda.arcpanel"; }).forEach(function(panel) {
    panel.currentConfigGroup = ["LinuxRondWorkspace"];
    if (String(panel.readConfig("active", false)) !== "true") return;
    var added = JSON.parse(panel.readConfig("addedWidgetIds", "[]"));
    var addedNotifications = JSON.parse(panel.readConfig("addedNotificationIds", "[]"));
    var movedNotifications = JSON.parse(panel.readConfig("movedNotifications", "[]"));
    var originalHeight = Number(panel.readConfig("originalHeight", panel.height));
    var appliedHeight = Number(panel.readConfig("appliedHeight", panel.height));
    var originalTasksIndex = panel.readConfig("originalTasksIndex", 1);
    panel.widgets().forEach(function(widget) {
        if (widget.type === plugin && added.indexOf(String(widget.id)) >= 0) widget.remove();
        if (widget.type === "org.kde.plasma.notifications" && addedNotifications.indexOf(String(widget.id)) >= 0) widget.remove();
    });
    movedNotifications.forEach(function(record) {
        var target = panels().find(function(candidate) { return String(candidate.id) === record.target; });
        if (!target) return;
        var widget = target.widgets().find(function(candidate) {
            return String(candidate.id) === record.id && candidate.type === "org.kde.plasma.notifications";
        });
        if (widget) {
            panel.addWidget(widget);
            if (typeof record.index === "number" && record.index >= 0) widget.index = record.index;
        }
    });
    if (panel.location === "top" && panel.height === appliedHeight) panel.height = originalHeight;
    if (panel.location === "bottom") {
        panel.currentConfigGroup = ["General"];
        if (Number(panel.readConfig("tasksIndex", 1)) === 2) panel.writeConfig("tasksIndex", originalTasksIndex);
    }
    panel.currentConfigGroup = ["LinuxRondWorkspace"];
    panel.writeConfig("active", false);
    panel.writeConfig("addedWidgetIds", "[]");
    panel.writeConfig("addedNotificationIds", "[]");
    panel.writeConfig("movedNotifications", "[]");
});
