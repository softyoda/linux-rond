const fs = require("node:fs");
const vm = require("node:vm");
const assert = require("node:assert/strict");
const path = require("node:path");
const code = fs.readFileSync(path.join(__dirname, "../plasma/org.softyoda.arcpanel/contents/ui/ArcGeometry.js"), "utf8").replace(/^\.pragma library\s*/, "");
const geometry = vm.createContext({});
vm.runInContext(code, geometry);
for (const dpr of [1, 1.25, 1.5, 2]) {
    const band = geometry.backgroundBand(332, 40, 296, dpr);
    assert.equal(band.outer, 358);
    assert(Math.abs((296 - band.inner) * dpr - 1) < 1e-8);
    assert(Math.abs(band.center - band.width / 2 - band.inner) < 1e-8);
    const edgeBand = geometry.backgroundBand(332, 40, 296, dpr, 360);
    assert(Math.abs((edgeBand.outer - 360) * dpr - 1) < 1e-8);
    assert.equal(edgeBand.inner, band.inner);
    assert.equal(edgeBand.layoutCenter, band.center);
    const inactive = geometry.backgroundBand(332, 40, 0, dpr, 360);
    assert.equal(inactive.outer, 358);
    assert.equal(inactive.center, 332);

}
const fallback = geometry.backgroundBand(332, 40, 0, 1.5);
assert.equal(fallback.center, 332);
assert.equal(fallback.width, 52);
console.log("Arc seam tests passed");
for (const leftVisible of [false, true]) {
    const widths = leftVisible ? [124, 40, 40, 70, 40, 40, 124] : [40, 40, 70, 40, 40, 124];
    const kinds = leftVisible ? ["left", "system", "system", "clock", "system", "system", "right"] : ["system", "system", "clock", "system", "system", "right"];
    const slots = geometry.layoutSlots(widths, kinds, 8, true);
    const clock = kinds.indexOf("clock");
    assert.equal(slots.starts[clock] + widths[clock] / 2, 0);
    const system = slots.segments.find(segment => segment.kind === "system");
    const right = slots.segments.find(segment => segment.kind === "right");
    assert.equal(right.start - system.end, 8);
    if (leftVisible) {
        const left = slots.segments.find(segment => segment.kind === "left");
        assert.equal(system.start - left.end, 8);
    }
    assert.equal(system.start, -131);
}
const connectedBand = geometry.backgroundBand(332, 40, 296, 1.5);
const titleSpan = geometry.titleWidth(124, 332, connectedBand, 300);
assert(titleSpan > 65 && titleSpan < 100);
console.log("Separate capsules, clock anchoring, and title fitting tests passed");

assert(geometry.titleWidth(144,332,connectedBand,300)>95);

for (const taskCount of [1, 2, 4]) {
    const widths = [40, 40, 2, ...Array(taskCount).fill(40), 2, 40];
    const kinds = ["system", "overview", "system", ...Array(taskCount).fill("tasks"), "system", "system"];
    const baseline = geometry.layoutSlots(widths, kinds, 8, false, false);
    const expected = widths.reduce((a,b)=>a+b,0) + 8 * (widths.length-1);
    assert.equal(baseline.start,-expected/2);
    assert.equal(baseline.end,expected/2);
    const anchored = geometry.layoutSlots(widths, kinds, 8, false, true);
    const first = 3, last = first + taskCount - 1;
    assert.equal((anchored.starts[first] + anchored.starts[last] + widths[last])/2,0);
    assert(anchored.starts[0] < anchored.starts[first]);
    assert(anchored.starts.at(-1) > anchored.starts[last]);
    assert.equal(anchored.segments.length,1);
}
console.log("Task-group anchoring and unchanged baseline layout tests passed");
