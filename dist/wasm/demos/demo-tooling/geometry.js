export function addVec2(left, right) {
    return { x: left.x + right.x, y: left.y + right.y };
}
export function subtractVec2(left, right) {
    return { x: left.x - right.x, y: left.y - right.y };
}
export function scaleVec2(value, scale) {
    return { x: value.x * scale, y: value.y * scale };
}
export function interpolateVec2(from, to, amount) {
    return {
        x: from.x + (to.x - from.x) * amount,
        y: from.y + (to.y - from.y) * amount,
    };
}
