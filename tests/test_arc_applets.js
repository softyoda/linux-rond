const fs = require("node:fs");
const vm = require("node:vm");
const assert = require("node:assert/strict");
const path = require("node:path");
const source = fs.readFileSync(path.join(__dirname,"../plasma/org.softyoda.arcpanel/contents/ui/main.qml"),"utf8");
const start = source.indexOf("function elements() {");
let end = source.indexOf("{",start), depth = 1;
while (depth && ++end < source.length) {
    if (source[end] === "{") ++depth;
    if (source[end] === "}") --depth;
}
assert.equal(depth,0);
const controlsPlugin="org.softyoda.roundworkspacecontrols";
function item(plugin, role="", status=2) {
    return {plasmoid:{pluginName:plugin,status,configuration:{role}},visible:true,enabled:true,opacity:1,
        compactRepresentationItem:{implicitWidth:role==="left"?0:40,backend:{}}};
}
const context=vm.createContext({
    atTop:true,controlsPlugin,workspaceBackend:null,
    PlasmaCore:{Types:{PassiveStatus:1}},Plasmoid:{configuration:{showTasks:false}},
    pluginOf:item=>item.plasmoid.pluginName,
    controlRole:item=>item.plasmoid.configuration.role,
    elementWidth:item=>item.compactRepresentationItem.implicitWidth
});
vm.runInContext(source.slice(start,end+1),context);
const volume=item("org.kde.plasma.volume"),clock=item("org.kde.plasma.digitalclock");
const device=item("org.kde.plasma.devicenotifier","",1),brightness=item("org.kde.plasma.brightness");
const notification=item("org.kde.plasma.notifications"),left=item(controlsPlugin,"left"),right=item(controlsPlugin,"right");
context.appletItems=[volume,clock,device,brightness,notification,left,right];
let placed=context.elements();
assert(!placed.includes(device));
assert.equal(device.visible,true);
assert.equal(device.opacity,0);
assert.equal(device.enabled,false);
assert.equal(left.visible,true);
assert.equal(left.opacity,0);
assert.equal(left.enabled,false);
assert(!placed.includes(left));
assert(placed.indexOf(notification)<placed.indexOf(brightness));
device.plasmoid.status=2;
placed=context.elements();
assert.equal(device.opacity,1);
assert.equal(device.enabled,true);
assert.equal(placed.indexOf(notification),placed.indexOf(device)+1);
context.atTop=false;
context.appletItems=[notification,right];
assert(!context.elements().includes(notification));
assert.equal(notification.opacity,0);
context.appletItems=[notification];
assert(context.elements().includes(notification));
assert.equal(notification.opacity,1);
console.log("Applet visibility, notification placement, and lazy-loading tests passed");
