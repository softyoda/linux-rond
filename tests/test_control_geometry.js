const fs = require("node:fs");
const vm = require("node:vm");
const assert = require("node:assert/strict");
const path = require("node:path");
const source = fs.readFileSync(path.join(__dirname,"../plasma/round-workspace-controls/contents/ui/ControlGeometry.js"),"utf8").replace(/^\.pragma library\s*/,"");
const geometry = vm.createContext({});
vm.runInContext(source,geometry);
for (const radius of [280,332,512]) {
    for (const size of [40,56]) {
        const width = size*4.8+4;
        for (const top of [true,false]) {
            const center = {x:width/2,y:size/2+(top?radius:-radius)};
            const slots = [size*.9,...[0,1,2].map(index=>size*1.8+index*(size+2)+size/2)];
            const points = slots.map(slot=>geometry.arcPoint(slot,width,size,radius,top));
            for (const point of points) {
                assert(Math.abs(Math.hypot(point.x-center.x,point.y-center.y)-radius)<1e-8);
            }
            for (let i=1;i<points.length;++i) assert(points[i].x>points[i-1].x);
            const middle=geometry.arcPoint(width/2,width,size,radius,top);
            assert.equal(middle.x,width/2);
            assert.equal(middle.y,size/2);
            assert(Math.abs(middle.rotation)<1e-8);
        }
    }
}
console.log("Window-button and title centers follow the same circle");
