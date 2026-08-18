const DEFAULT_MIN_SCALE = 1e-6;
const DEFAULT_MAX_SCALE = 1e6;
const DEFAULT_WHEEL_SENSITIVITY = 0.002;
function requireFinitePositive(value, label) {
    if (!Number.isFinite(value) || value <= 0)
        throw new RangeError(`${label} must be finite and > 0.`);
    return value;
}
function requireFiniteVec(value, label) {
    if (!Number.isFinite(value.x) || !Number.isFinite(value.y))
        throw new RangeError(`${label} must contain finite coordinates.`);
    return value;
}
function clamp(value, minimum, maximum) {
    return Math.min(maximum, Math.max(minimum, value));
}
export class Camera2D {
    centerValue;
    scaleValue;
    viewportValue;
    minPixelsPerWorldUnit;
    maxPixelsPerWorldUnit;
    wheelSensitivity;
    constructor(viewport, options = {}) {
        this.viewportValue = Camera2D.validateViewport(viewport);
        this.centerValue = {
            ...requireFiniteVec(options.center ?? { x: 0, y: 0 }, "center"),
        };
        this.minPixelsPerWorldUnit = requireFinitePositive(options.minPixelsPerWorldUnit ?? DEFAULT_MIN_SCALE, "minPixelsPerWorldUnit");
        this.maxPixelsPerWorldUnit = requireFinitePositive(options.maxPixelsPerWorldUnit ?? DEFAULT_MAX_SCALE, "maxPixelsPerWorldUnit");
        if (this.minPixelsPerWorldUnit > this.maxPixelsPerWorldUnit)
            throw new RangeError("minPixelsPerWorldUnit cannot exceed maxPixelsPerWorldUnit.");
        this.wheelSensitivity = requireFinitePositive(options.wheelSensitivity ?? DEFAULT_WHEEL_SENSITIVITY, "wheelSensitivity");
        this.scaleValue = this.clampScale(options.pixelsPerWorldUnit ?? 1);
    }
    get center() {
        return { ...this.centerValue };
    }
    get pixelsPerWorldUnit() {
        return this.scaleValue;
    }
    get viewport() {
        return { ...this.viewportValue };
    }
    setViewport(viewport) {
        this.viewportValue = Camera2D.validateViewport(viewport);
    }
    setCenter(center) {
        this.centerValue = { ...requireFiniteVec(center, "center") };
    }
    worldToScreen(world) {
        requireFiniteVec(world, "world");
        return requireFiniteVec({
            x: this.viewportValue.width / 2 + (world.x - this.centerValue.x) * this.scaleValue,
            y: this.viewportValue.height / 2 - (world.y - this.centerValue.y) * this.scaleValue,
        }, "screen result");
    }
    screenToWorld(screen) {
        requireFiniteVec(screen, "screen");
        return requireFiniteVec({
            x: this.centerValue.x + (screen.x - this.viewportValue.width / 2) / this.scaleValue,
            y: this.centerValue.y - (screen.y - this.viewportValue.height / 2) / this.scaleValue,
        }, "world result");
    }
    panByScreen(delta) {
        requireFiniteVec(delta, "delta");
        this.centerValue = requireFiniteVec({
            x: this.centerValue.x - delta.x / this.scaleValue,
            y: this.centerValue.y + delta.y / this.scaleValue,
        }, "panned center");
    }
    zoomAt(screenAnchor, wheelDeltaY) {
        if (!Number.isFinite(wheelDeltaY))
            throw new RangeError("wheelDeltaY must be finite.");
        requireFiniteVec(screenAnchor, "screenAnchor");
        const exponent = -wheelDeltaY * this.wheelSensitivity;
        const minimumExponent = Math.log(this.minPixelsPerWorldUnit) - Math.log(this.scaleValue);
        const maximumExponent = Math.log(this.maxPixelsPerWorldUnit) - Math.log(this.scaleValue);
        const nextScale = exponent <= minimumExponent
            ? this.minPixelsPerWorldUnit
            : exponent >= maximumExponent
                ? this.maxPixelsPerWorldUnit
                : this.scaleValue * Math.exp(exponent);
        this.setScaleAt(screenAnchor, nextScale);
    }
    zoomByFactorAt(screenAnchor, factor) {
        requireFiniteVec(screenAnchor, "screenAnchor");
        requireFinitePositive(factor, "factor");
        const nextScale = factor >= this.maxPixelsPerWorldUnit / this.scaleValue
            ? this.maxPixelsPerWorldUnit
            : factor <= this.minPixelsPerWorldUnit / this.scaleValue
                ? this.minPixelsPerWorldUnit
                : this.scaleValue * factor;
        this.setScaleAt(screenAnchor, nextScale);
    }
    setScaleAt(screenAnchor, nextScale) {
        const worldAnchor = this.screenToWorld(screenAnchor);
        const boundedScale = this.clampScale(nextScale);
        const nextCenter = requireFiniteVec({
            x: worldAnchor.x - (screenAnchor.x - this.viewportValue.width / 2) / boundedScale,
            y: worldAnchor.y + (screenAnchor.y - this.viewportValue.height / 2) / boundedScale,
        }, "zoomed center");
        this.scaleValue = boundedScale;
        this.centerValue = nextCenter;
    }
    fit(bounds, paddingPixels = 24) {
        const min = requireFiniteVec(bounds.min, "bounds.min");
        const max = requireFiniteVec(bounds.max, "bounds.max");
        if (max.x < min.x || max.y < min.y)
            throw new RangeError("bounds must be ordered.");
        if (!Number.isFinite(paddingPixels) || paddingPixels < 0)
            throw new RangeError("paddingPixels must be finite and >= 0.");
        const availableWidth = this.viewportValue.width - paddingPixels * 2;
        const availableHeight = this.viewportValue.height - paddingPixels * 2;
        if (availableWidth <= 0 || availableHeight <= 0)
            throw new RangeError("paddingPixels leaves no visible viewport.");
        const width = max.x - min.x;
        const height = max.y - min.y;
        const widthScale = width === 0 ? Number.POSITIVE_INFINITY : availableWidth / width;
        const heightScale = height === 0 ? Number.POSITIVE_INFINITY : availableHeight / height;
        const candidate = Math.min(widthScale, heightScale);
        this.scaleValue =
            candidate <= 0
                ? this.minPixelsPerWorldUnit
                : this.clampScale(Number.isFinite(candidate) ? candidate : this.maxPixelsPerWorldUnit);
        this.centerValue = { x: min.x / 2 + max.x / 2, y: min.y / 2 + max.y / 2 };
    }
    gridScale(targetMinorPixels = 24, majorEvery = 5) {
        requireFinitePositive(targetMinorPixels, "targetMinorPixels");
        if (!Number.isSafeInteger(majorEvery) || majorEvery < 1)
            throw new RangeError("majorEvery must be a positive safe integer.");
        const idealWorldSpacing = targetMinorPixels / this.scaleValue;
        if (!Number.isFinite(idealWorldSpacing))
            throw new RangeError("targetMinorPixels is too large for grid math.");
        const decade = 10 ** Math.floor(Math.log10(idealWorldSpacing));
        if (!Number.isFinite(decade) || decade <= 0)
            throw new RangeError("Grid spacing is outside the finite numeric range.");
        const normalized = idealWorldSpacing / decade;
        const step = normalized <= 1 ? 1 : normalized <= 2 ? 2 : normalized <= 5 ? 5 : 10;
        const minorWorldSpacing = step * decade;
        const majorWorldSpacing = minorWorldSpacing * majorEvery;
        const minorPixelSpacing = minorWorldSpacing * this.scaleValue;
        if (!Number.isFinite(minorWorldSpacing) ||
            !Number.isFinite(majorWorldSpacing) ||
            !Number.isFinite(minorPixelSpacing))
            throw new RangeError("Grid spacing is outside the finite numeric range.");
        return {
            minorWorldSpacing,
            majorWorldSpacing,
            minorPixelSpacing,
            majorEvery,
        };
    }
    snapWorldToGrid(world, spacing = this.gridScale().minorWorldSpacing) {
        requireFiniteVec(world, "world");
        requireFinitePositive(spacing, "spacing");
        return requireFiniteVec({
            x: Math.round(world.x / spacing) * spacing,
            y: Math.round(world.y / spacing) * spacing,
        }, "snapped world");
    }
    clampScale(scale) {
        return clamp(requireFinitePositive(scale, "pixelsPerWorldUnit"), this.minPixelsPerWorldUnit, this.maxPixelsPerWorldUnit);
    }
    static validateViewport(viewport) {
        return {
            width: requireFinitePositive(viewport.width, "viewport.width"),
            height: requireFinitePositive(viewport.height, "viewport.height"),
        };
    }
}
