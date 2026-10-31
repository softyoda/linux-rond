const fs = require("node:fs");
const vm = require("node:vm");
const assert = require("node:assert/strict");
const path = require("node:path");
const code = fs.readFileSync(path.join(__dirname,"../plasma/panels.js"),"utf8");
function run(existingIcon, defaultIcon) {
    const all=[];
    function Widget(type, initialIcon) {
        this.type=type;
        this.values={};
        if (initialIcon) this.values.icon=initialIcon;
        this.readConfig=(key,fallback)=>this.values[key] ?? (key==="icon" ? defaultIcon : null) ?? fallback;
        this.writeConfig=(key,value)=>{this.values[key]=value;};
    }
    function Panel(type) {
        this.type=type;this.screen=0;this.removed=false;this.values={};this.applets=[];
        this.widgets=()=>this.applets;
        this.addWidget=type=>{const widget=new Widget(type);this.applets.push(widget);return widget;};
        this.writeConfig=(key,value)=>{this.values[key]=value;};
        this.remove=()=>{this.removed=true;};
        all.push(this);
    }
    const previous=new Panel("org.softyoda.arcpanel");
    if (existingIcon) previous.applets.push(new Widget("org.kde.plasma.kickoff",existingIcon));
    const other=new Panel("org.kde.panel");other.screen=1;
    other.applets.push(new Widget("org.kde.plasma.kickoff","unrelated.svg"));
    vm.runInNewContext(code,{screenCount:1,screenGeometry:()=>({width:720,height:720}),panels:()=>all.filter(p=>!p.removed),Panel});
    assert(!other.removed);
    const top=all.find(p=>!p.removed && p.location==="top");
    const dock=all.find(p=>!p.removed && p.location==="bottom");
    assert(top.applets.some(w=>w.type==="org.kde.plasma.notifications"));
    assert(!dock.applets.some(w=>w.type==="org.kde.plasma.notifications"));
    return dock.applets.find(w=>w.type==="org.kde.plasma.kickoff").values.icon;
}
assert.equal(run("custom-launcher.svg","distribution-logo"),"custom-launcher.svg");
assert.equal(run(null,"distribution-logo"),"distribution-logo");
assert.equal(run(null,null),"start-here-kde");
console.log("Launcher preservation and panel placement tests passed");
