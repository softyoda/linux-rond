// SPDX-License-Identifier: GPL-2.0-or-later
const fs = require("node:fs");
const vm = require("node:vm");
const assert = require("node:assert/strict");
const path = require("node:path");
const configure = fs.readFileSync(path.join(__dirname, "../configure.js"), "utf8");
const rollback = fs.readFileSync(path.join(__dirname, "../rollback.js"), "utf8");
let nextId = 1;
function settings(object) {
    object.currentConfigGroup = ["General"];
    object.config = {};
    object.readConfig = function(key, fallback) { return this.config[this.currentConfigGroup.join("/") + "/" + key] ?? fallback; };
    object.writeConfig = function(key, value) { this.config[this.currentConfigGroup.join("/") + "/" + key] = value; };
    return object;
}
function panel(type, location) {
    const widgets = [];
    return settings({
        type, location, height: 120, id: nextId++, screen: 0,
        widgets: () => widgets.slice(),
        addWidget(type) {
            if (typeof type !== "string") {
                type.owner.splice(type.owner.indexOf(type), 1);
                type.owner = widgets;
                widgets.push(type);
                return type;
            }
            const widget = settings({type, id: nextId++, index: -1, owner: widgets,
                                     remove() { this.owner.splice(this.owner.indexOf(this), 1); }});
            widgets.push(widget);
            return widget;
        }
    });
}
const top = panel("org.softyoda.arcpanel", "top");
const bottom = panel("org.softyoda.arcpanel", "bottom");
const other = panel("org.kde.panel", "top");
const preexisting = top.addWidget("org.softyoda.roundworkspacecontrols");
preexisting.writeConfig("role", "left");
bottom.writeConfig("tasksIndex", 3);
const notification = bottom.addWidget("org.kde.plasma.notifications");
notification.writeConfig("customSetting", "preserve");
const context = vm.createContext({panels: () => [top, bottom, other]});
vm.runInContext(configure, context);
vm.runInContext(configure, context);
assert.equal(top.widgets().length, 3);
assert.ok(top.widgets().includes(notification));
assert.equal(notification.config["General/customSetting"], "preserve");
assert.equal(bottom.widgets().length, 1);
assert.equal(other.widgets().length, 0);
assert.equal(top.height, 260);
top.height = 160;
top.currentConfigGroup = ["LinuxRondWorkspace"];
top.writeConfig("appliedHeight", 160);
vm.runInContext(configure, context);
assert.equal(top.height, 260);
assert.equal(bottom.config["General/tasksIndex"], 2);
vm.runInContext(rollback, context);
assert.deepEqual(top.widgets(), [preexisting]);
assert.deepEqual(bottom.widgets(), [notification]);
assert.equal(top.height, 120);
assert.equal(bottom.config["General/tasksIndex"], 3);
vm.runInContext(configure, context);
top.height = 190;
bottom.currentConfigGroup = ["General"];
bottom.writeConfig("tasksIndex", 7);
vm.runInContext(rollback, context);
vm.runInContext(rollback, context);
assert.equal(top.height, 190);
assert.equal(bottom.config["General/tasksIndex"], 7);
assert.deepEqual(top.widgets(), [preexisting]);
assert.deepEqual(bottom.widgets(), [notification]);
const existingTop = top.addWidget("org.kde.plasma.notifications");
vm.runInContext(configure, context);
assert.ok(top.widgets().includes(existingTop));
assert.ok(bottom.widgets().includes(notification));
vm.runInContext(rollback, context);
assert.ok(top.widgets().includes(existingTop));
assert.ok(bottom.widgets().includes(notification));
assert.throws(() => vm.runInNewContext(configure, {panels: () => []}), /No arc panels/);
console.log("Panel configuration and targeted rollback tests passed");
