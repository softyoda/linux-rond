var scr = -1;
var candidates = 0;
for (var i = 0; i < screenCount; ++i) { var g = screenGeometry(i); if (g.width == g.height) { scr = i; candidates++; } }
if (candidates > 1) throw new Error("Multiple square outputs: explicit configuration required");
if (scr < 0) throw new Error("No square output detected");
var launcherIcon = "";
panels().forEach(function (panel) {
    if (panel.screen !== scr) return;
    panel.widgets().forEach(function (widget) {
        if (!["org.kde.plasma.kickoff", "org.kde.plasma.kicker", "org.kde.plasma.kickerdash"].includes(widget.type)) return;
        widget.currentConfigGroup = ["General"];
        launcherIcon = String(widget.readConfig("icon", "")) || launcherIcon;
    });
});
panels().forEach(function (p) { if (p.screen == scr) p.remove(); });

function arcPanel(loc, h) {
    var p = new Panel("org.softyoda.arcpanel");
    p.screen = scr; p.location = loc; p.height = h;
    p.lengthMode = "fill"; p.floating = false;
    p.currentConfigGroup = ["General"];
    return p;
}

// Top: volume, network, clock, devices, notifications, brightness.
var top = arcPanel("top", 120);
top.addWidget("org.kde.plasma.volume");
top.addWidget("org.kde.plasma.networkmanagement");
var clock = top.addWidget("org.kde.plasma.digitalclock");
clock.currentConfigGroup = ["Appearance"];
clock.writeConfig("showDate", false);
top.addWidget("org.kde.plasma.devicenotifier");
top.addWidget("org.kde.plasma.notifications");
top.addWidget("org.kde.plasma.brightness");

// Bottom: launcher, tasks, tray.
var dock = arcPanel("bottom", 120);
dock.writeConfig("showTasks", true);
dock.writeConfig("tasksIndex", 1);
var menu = dock.addWidget("org.kde.plasma.kickoff");
menu.currentConfigGroup = ["General"];
menu.writeConfig("icon", launcherIcon || menu.readConfig("icon", "start-here-kde") || "start-here-kde");
var tray = dock.addWidget("org.kde.plasma.systemtray");
