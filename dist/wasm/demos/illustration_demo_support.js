export const MESH_QUALITY_PRESETS = {
    draft: {
        linearDeflectionMm: 0.25,
        angularDeflectionDegrees: 40,
        hlrDeflectionCoefficient: 0.008,
        hlrAngularDeflectionDegrees: 40,
    },
    balanced: {
        linearDeflectionMm: 0.1,
        angularDeflectionDegrees: (0.5 * 180) / Math.PI,
        hlrDeflectionCoefficient: 0.004,
        hlrAngularDeflectionDegrees: (0.5 * 180) / Math.PI,
    },
    fine: {
        linearDeflectionMm: 0.03,
        angularDeflectionDegrees: 15,
        hlrDeflectionCoefficient: 0.002,
        hlrAngularDeflectionDegrees: 15,
    },
    "extra-fine": {
        linearDeflectionMm: 0.01,
        angularDeflectionDegrees: 8,
        hlrDeflectionCoefficient: 0.001,
        hlrAngularDeflectionDegrees: 8,
    },
};
export const DEMO_MODEL_ORDER = [
    "SOT-23.STEP",
    "SOIC-8-W.step",
    "sot223.stp",
    "Cap_SMT_Aluminum_F.STEP",
    "BGA90-8X13mm.step",
];
export function formatBytes(value) {
    return value >= 1024 * 1024
        ? `${(value / (1024 * 1024)).toFixed(2)} MiB`
        : `${Math.max(1, Math.round(value / 1024))} KiB`;
}
export function formatMs(value) {
    return value >= 1000 ? `${(value / 1000).toFixed(2)} s` : `${Math.max(1, Math.round(value))} ms`;
}
export function colorFromHex(value) {
    const normalized = value.replace(/^#/u, "");
    const numeric = Number.parseInt(normalized, 16);
    return [((numeric >> 16) & 255) / 255, ((numeric >> 8) & 255) / 255, (numeric & 255) / 255];
}
export function surfaceMeshKey(settings) {
    const angularRadians = (settings.angularDeflectionDegrees * Math.PI) / 180;
    return `${settings.linearDeflectionMm.toPrecision(12)}|${angularRadians.toPrecision(12)}`;
}
export const DEFAULT_SURFACE_MESH_KEY = surfaceMeshKey(MESH_QUALITY_PRESETS.balanced);
export function normalizeTuple(value) {
    const length = Math.hypot(value[0], value[1], value[2]) || 1;
    return [value[0] / length, value[1] / length, value[2] / length];
}
