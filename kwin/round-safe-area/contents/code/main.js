/*
 * Zone sûre écran rond — KWin 6
 * Sur un écran rond (sortie carrée, ou OutputName), le cadre visible est le
 * cercle inscrit : on garde les fenêtres dans le carré inscrit (côté = d·Inset).
 *  - AndroidMode : chaque fenêtre redimensionnable occupe tout le carré.
 *  - sinon : les fenêtres sont seulement ramenées dans le carré.
 * Les fenêtres plein écran (vidéo, jeux) ne sont pas touchées, sauf ConstrainFullscreen.
 */
const outputName = String(readConfig("OutputName", ""));
const inset = Number(readConfig("Inset", 0.7071));
const androidMode = readConfig("AndroidMode", true) == true;
const constrainFullscreen = readConfig("ConstrainFullscreen", false) == true;

function rect(x, y, w, h) {
    return { x: Math.round(x), y: Math.round(y), width: Math.round(w), height: Math.round(h) };
}

function isRound(output) {
    if (!output) return false;
    if (outputName) return output.name === outputName;
    const g = output.geometry;
    return g.width === g.height;
}

function safeRect(window) {
    const g = window.output.geometry;
    const d = Math.min(g.width, g.height);
    const s = Math.floor(d * inset);
    const sq = { x: g.x + (g.width - s) / 2, y: g.y + (g.height - s) / 2, w: s, h: s };
    // Intersection avec la zone hors panneaux
    const a = workspace.clientArea(KWin.MaximizeArea, window);
    const x1 = Math.max(sq.x, a.x), y1 = Math.max(sq.y, a.y);
    const x2 = Math.min(sq.x + sq.w, a.x + a.width), y2 = Math.min(sq.y + sq.h, a.y + a.height);
    return rect(x1, y1, x2 - x1, y2 - y1);
}

function managed(window) {
    return window.normalWindow || window.dialog || window.utility;
}

const busy = new Set();

function fit(window, force) {
    if (!managed(window) || busy.has(window) || !isRound(window.output)) return;
    if (window.fullScreen && !constrainFullscreen) return;
    const r = safeRect(window);
    const f = window.frameGeometry;
    let w = f.width, h = f.height, x = f.x, y = f.y;
    if (window.normalWindow && window.resizeable && (androidMode && force)) {
        x = r.x; y = r.y; w = r.width; h = r.height;
    } else {
        if (window.resizeable) { w = Math.min(w, r.width); h = Math.min(h, r.height); }
        x = Math.min(Math.max(x, r.x), r.x + r.width - w);
        y = Math.min(Math.max(y, r.y), r.y + r.height - h);
        if (force && !window.normalWindow) { // dialogues : centrés
            x = r.x + (r.width - w) / 2; y = r.y + (r.height - h) / 2;
        }
    }
    if (x === f.x && y === f.y && w === f.width && h === f.height) return;
    busy.add(window);
    window.frameGeometry = rect(x, y, w, h);
    busy.delete(window);
}

function setup(window) {
    if (!managed(window)) return;
    fit(window, true);
    window.outputChanged.connect(() => fit(window, true));
    window.fullScreenChanged.connect(() => fit(window, androidMode));
    if (window.interactiveMoveResizeFinished)
        window.interactiveMoveResizeFinished.connect(() => fit(window, false));
    if (window.maximizedChanged)
        window.maximizedChanged.connect(() => fit(window, androidMode));
    window.frameGeometryChanged.connect(() => {
        if (!window.move && !window.resize) fit(window, false);
    });
}

workspace.windowAdded.connect(setup);
workspace.windowList().forEach(setup);
workspace.screensChanged.connect(() => workspace.windowList().forEach(w => fit(w, true)));
