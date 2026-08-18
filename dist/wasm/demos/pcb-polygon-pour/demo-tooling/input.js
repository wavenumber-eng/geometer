const DEFAULT_INTENT_BINDINGS = [
    {
        intent: "cancel",
        priority: 400,
        matches: (input) => input.button === 2 && input.modifiers.alt,
    },
    { intent: "context", priority: 300, matches: (input) => input.button === 2 },
    {
        intent: "pan",
        priority: 200,
        matches: (input) => input.button === 1 || (input.button === 0 && input.modifiers.shift),
    },
    { intent: "primary", priority: 100, matches: (input) => input.button === 0 },
];
function modifiersOf(event) {
    return {
        alt: event.altKey,
        control: event.ctrlKey,
        meta: event.metaKey,
        shift: event.shiftKey,
    };
}
function localPosition(surface, clientX, clientY) {
    const bounds = surface.getBoundingClientRect();
    const position = { x: clientX - bounds.left, y: clientY - bounds.top };
    if (!Number.isFinite(position.x) || !Number.isFinite(position.y))
        throw new RangeError("Pointer position must be finite.");
    return position;
}
export function normalizePointerInput(event, surface) {
    if (!Number.isSafeInteger(event.pointerId))
        throw new RangeError("pointerId must be a safe integer.");
    if (!Number.isFinite(event.pressure))
        throw new RangeError("pressure must be finite.");
    return {
        pointerId: event.pointerId,
        pointerType: event.pointerType,
        position: localPosition(surface, event.clientX, event.clientY),
        button: event.button,
        buttons: event.buttons,
        modifiers: modifiersOf(event),
        pressure: event.pressure,
    };
}
export function normalizeWheelInput(event, surface) {
    if (event.deltaMode !== 0 && event.deltaMode !== 1 && event.deltaMode !== 2)
        throw new RangeError("deltaMode must be pixel, line, or page mode.");
    const unit = event.deltaMode === 1 ? 16 : event.deltaMode === 2 ? surface.clientHeight : 1;
    const deltaX = event.deltaX * unit;
    const deltaY = event.deltaY * unit;
    if (!Number.isFinite(deltaX) || !Number.isFinite(deltaY))
        throw new RangeError("Wheel delta must be finite.");
    return {
        position: localPosition(surface, event.clientX, event.clientY),
        deltaX,
        deltaY,
        modifiers: modifiersOf(event),
    };
}
export function resolvePointerIntent(input, bindings = DEFAULT_INTENT_BINDINGS) {
    let winner;
    for (const binding of bindings) {
        if (!Number.isFinite(binding.priority))
            throw new RangeError("Intent priority must be finite.");
        if (binding.matches(input) && (winner === undefined || binding.priority > winner.priority))
            winner = binding;
    }
    return winner?.intent;
}
