var scr = -1;
var candidates = 0;
for (var i = 0; i < screenCount; ++i) { var g = screenGeometry(i); if (g.width == g.height) { scr = i; candidates++; } }
if (candidates > 1) throw new Error("Multiple square outputs: explicit configuration required");
if (scr < 0) throw new Error("No square output detected");
panels().forEach(function (p) { if (p.screen == scr) p.remove(); });

function arcPanel(loc, h) {
    var p = new Panel("org.softyoda.arcpanel");
    p.screen = scr; p.location = loc; p.height = h;
    p.lengthMode = "fill"; p.floating = false;
    p.currentConfigGroup = ["General"];
    return p;
}

// Top: volume, network, clock, devices, brightness.
var top = arcPanel("top", 120);
top.addWidget("org.kde.plasma.volume");
top.addWidget("org.kde.plasma.networkmanagement");
var clock = top.addWidget("org.kde.plasma.digitalclock");
clock.currentConfigGroup = ["Appearance"];
clock.writeConfig("showDate", false);
top.addWidget("org.kde.plasma.devicenotifier");
top.addWidget("org.kde.plasma.brightness");

// Bottom: launcher, tasks, notifications, tray.
var dock = arcPanel("bottom", 120);
dock.writeConfig("showTasks", true);
dock.writeConfig("tasksIndex", 1);
var menu = dock.addWidget("org.kde.plasma.kickoff");
menu.currentConfigGroup = ["General"];
menu.writeConfig("icon", "start-here");
dock.addWidget("org.kde.plasma.notifications");
var tray = dock.addWidget("org.kde.plasma.systemtray");
