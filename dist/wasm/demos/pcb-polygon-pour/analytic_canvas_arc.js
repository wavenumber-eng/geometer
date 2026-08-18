/** Resolve the governed endpoint/radius/branch arc for a Canvas context reflected in Y. */
export function resolveReflectedCanvasArc(startX, startY, endX, endY, radius, direction, majorArc) {
    const dx = endX - startX;
    const dy = endY - startY;
    const chord = Math.hypot(dx, dy);
    const diameter = radius * 2;
    const tolerance = Number.EPSILON * Math.max(1, Math.abs(diameter), chord) * 16;
    if (chord === 0 || chord > diameter + tolerance)
        return undefined;
    const midpointX = (startX + endX) / 2;
    const midpointY = (startY + endY) / 2;
    const height = Math.sqrt(Math.max(0, radius * radius - (chord * chord) / 4));
    const leftX = -dy / chord;
    const leftY = dx / chord;
    const candidates = [
        { x: midpointX + leftX * height, y: midpointY + leftY * height },
        { x: midpointX - leftX * height, y: midpointY - leftY * height },
    ];
    const center = candidates.find((candidate) => {
        const startAngle = Math.atan2(startY - candidate.y, startX - candidate.x);
        const endAngle = Math.atan2(endY - candidate.y, endX - candidate.x);
        const sweep = direction === "ccw"
            ? positiveAngle(endAngle - startAngle)
            : positiveAngle(startAngle - endAngle);
        return majorArc ? sweep > Math.PI : sweep <= Math.PI + 1e-9;
    });
    if (center === undefined)
        return undefined;
    return {
        centerX: center.x,
        centerY: center.y,
        counterclockwise: direction === "cw",
        endAngle: Math.atan2(endY - center.y, endX - center.x),
        radius,
        startAngle: Math.atan2(startY - center.y, startX - center.x),
    };
}
function positiveAngle(value) {
    const tau = Math.PI * 2;
    return ((value % tau) + tau) % tau;
}
