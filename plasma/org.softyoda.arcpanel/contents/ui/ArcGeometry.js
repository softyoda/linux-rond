.pragma library

function backgroundBand(track, iconSize, contentRadius, dpr, screenRadius) {
    const pixel = 1 / Math.max(1, dpr);
    const originalOuter = track + (iconSize + 12) / 2;
    const originalInner = contentRadius > 0
        ? Math.max(0, Math.min(contentRadius, originalOuter) - pixel)
        : track - (iconSize + 12) / 2;
    const outer = contentRadius > 0 && screenRadius > 0 ? screenRadius + pixel : originalOuter;
    const inner = contentRadius > 0 ? Math.max(0, Math.min(contentRadius, outer) - pixel) : originalInner;
    return {
        inner: inner, outer: outer, center: (inner + outer) / 2, width: outer - inner,
        layoutCenter: (originalInner + originalOuter) / 2
    };
}

function layoutSlots(widths, kinds, gap, anchorClock, anchorTasks) {
    const starts = [];
    const segments = [];
    function span(indices) {
        return indices.reduce((total, index) => total + widths[index], 0) + Math.max(0, indices.length - 1) * gap;
    }
    function place(indices, start, kind) {
        let position = start;
        indices.forEach(index => { starts[index] = position; position += widths[index] + gap; });
        const end = indices.length ? position - gap : start;
        if (indices.length) segments.push({ start: start, end: end, kind: kind });
        return end;
    }
    const all = widths.map((_, index) => index);
    const tasks = all.filter(index => kinds[index] === "tasks");
    if (anchorTasks && tasks.length) {
        const before = all.slice(0, tasks[0]);
        const anchor = span(before) + (before.length ? gap : 0) + span(tasks) / 2;
        const end = place(all, -anchor, "system");
        return { starts: starts, segments: segments, start: -anchor, end: end };
    }
    if (!anchorClock) {
        const total = span(all);
        place(all, -total / 2, "system");
        return { starts: starts, segments: segments, start: -total / 2, end: total / 2 };
    }
    const left = all.filter(index => kinds[index] === "left");
    const right = all.filter(index => kinds[index] === "right");
    const system = all.filter(index => kinds[index] !== "left" && kinds[index] !== "right");
    let anchor = span(system) / 2;
    let position = 0;
    system.forEach(index => {
        if (kinds[index] === "clock") anchor = position + widths[index] / 2;
        position += widths[index] + gap;
    });
    const systemStart = -anchor;
    const systemEnd = place(system, systemStart, "system");
    place(left, systemStart - gap - span(left), "left");
    place(right, systemEnd + gap, "right");
    return {
        starts: starts, segments: segments,
        start: segments.length ? Math.min.apply(null, segments.map(segment => segment.start)) : 0,
        end: segments.length ? Math.max.apply(null, segments.map(segment => segment.end)) : 0
    };
}

function titleWidth(slotWidth, track, band, titleRadius) {
    if (titleRadius <= 0 || band.center <= 0) return 0;
    const capRadius = band.width / 2;
    const capPad = Math.asin(Math.min(1, capRadius / band.center));
    const cosine = (band.center * band.center + titleRadius * titleRadius - capRadius * capRadius) / (2 * band.center * titleRadius);
    const capAtTitle = Math.acos(Math.max(-1, Math.min(1, cosine)));
    return Math.max(0, (slotWidth / track - 2 * capPad + 2 * capAtTitle) * titleRadius - 8);
}
