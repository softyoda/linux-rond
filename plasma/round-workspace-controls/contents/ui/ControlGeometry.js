.pragma library

function arcPoint(slotCenter, width, buttonSize, radius, atTop) {
    const angle = (slotCenter - width / 2) / radius;
    return {
        x: width / 2 + radius * Math.sin(angle),
        y: buttonSize / 2 + (atTop ? 1 : -1) * radius * (1 - Math.cos(angle)),
        rotation: (atTop ? 1 : -1) * angle * 180 / Math.PI
    };
}
