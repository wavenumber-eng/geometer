function throwLifecycleErrors(errors, message) {
    if (errors.length === 1)
        throw errors[0];
    if (errors.length > 1)
        throw new AggregateError(errors, message);
}
export class ToolController {
    capturePort;
    framePort;
    renderPreview;
    tool;
    activePointerId;
    capturedPointerId;
    dispatchPointerId;
    previewFrame;
    disposed = false;
    constructor(tool, capturePort, framePort, renderPreview) {
        this.capturePort = capturePort;
        this.framePort = framePort;
        this.renderPreview = renderPreview;
        this.tool = tool;
    }
    setTool(tool) {
        this.assertActive();
        this.cancel();
        this.tool = tool;
    }
    pointerDown(input, intent) {
        this.assertActive();
        if (this.activePointerId !== undefined)
            return;
        this.activePointerId = input.pointerId;
        try {
            this.withPointer(input.pointerId, (context) => this.tool.pointerDown?.(input, intent, context));
        }
        catch (error) {
            const errors = [error];
            this.terminateGesture(true, true, errors);
            throwLifecycleErrors(errors, "Pointer-down failure and cleanup both failed.");
        }
    }
    pointerMove(input) {
        this.assertActive();
        if (this.activePointerId !== undefined && this.activePointerId !== input.pointerId)
            return;
        this.withPointer(input.pointerId, (context) => this.tool.pointerMove?.(input, context));
    }
    pointerUp(input) {
        this.assertActive();
        if (this.activePointerId !== input.pointerId)
            return;
        const errors = [];
        this.terminateGesture(false, false, errors, (context) => this.tool.pointerUp?.(input, context));
        if (errors.length > 0)
            this.tryCancelPreview(errors);
        throwLifecycleErrors(errors, "Pointer-up callback and cleanup both failed.");
    }
    pointerCancel(pointerId) {
        this.assertActive();
        if (this.activePointerId !== pointerId)
            return;
        const errors = [];
        this.terminateGesture(true, true, errors);
        throwLifecycleErrors(errors, "Pointer cancellation cleanup failed.");
    }
    pointerCaptureLost(pointerId) {
        this.assertActive();
        if (this.activePointerId !== pointerId && this.capturedPointerId !== pointerId)
            return;
        const errors = [];
        this.terminateGesture(true, true, errors, undefined, false);
        throwLifecycleErrors(errors, "Lost-pointer-capture cleanup failed.");
    }
    wheel(input) {
        this.assertActive();
        this.tool.wheel?.(input, this.context());
    }
    cancel() {
        if (this.disposed)
            return;
        if (this.activePointerId === undefined) {
            this.cancelPreview();
            return;
        }
        const errors = [];
        this.terminateGesture(true, true, errors);
        throwLifecycleErrors(errors, "Tool cancellation cleanup failed.");
    }
    dispose() {
        if (this.disposed)
            return;
        this.disposed = true;
        const errors = [];
        if (this.activePointerId !== undefined)
            this.terminateGesture(true, true, errors);
        else
            this.tryCancelPreview(errors);
        throwLifecycleErrors(errors, "Tool disposal cleanup failed.");
    }
    terminateGesture(notifyCancel, cancelPreview, errors, terminalCallback, releasePhysicalCapture = true) {
        const capturedPointerId = this.capturedPointerId;
        this.activePointerId = undefined;
        this.capturedPointerId = undefined;
        this.dispatchPointerId = undefined;
        if (cancelPreview)
            this.tryCancelPreview(errors);
        const context = this.terminalContext();
        try {
            if (notifyCancel)
                this.tool.cancel?.(context);
            else
                terminalCallback?.(context);
        }
        catch (error) {
            errors.push(error);
        }
        if (cancelPreview)
            this.tryCancelPreview(errors);
        if (releasePhysicalCapture && capturedPointerId !== undefined)
            this.tryReleasePhysicalCapture(capturedPointerId, errors);
    }
    withPointer(pointerId, callback) {
        this.dispatchPointerId = pointerId;
        try {
            callback(this.context());
        }
        finally {
            this.dispatchPointerId = undefined;
        }
    }
    context() {
        return {
            capturePointer: () => this.captureCurrentPointer(),
            releasePointer: () => this.releaseCapturedPointer(),
            invalidatePreview: () => this.invalidatePreview(),
        };
    }
    terminalContext() {
        return {
            capturePointer: () => {
                throw new Error("A terminal tool callback cannot capture a pointer.");
            },
            releasePointer: () => undefined,
            invalidatePreview: () => this.invalidatePreview(),
        };
    }
    captureCurrentPointer() {
        const pointerId = this.dispatchPointerId;
        if (pointerId === undefined || this.activePointerId !== pointerId)
            throw new Error("Pointer capture requires the active pointer dispatch.");
        if (this.capturedPointerId !== undefined && this.capturedPointerId !== pointerId)
            throw new Error("A different pointer is already captured.");
        if (!this.capturePort.hasPointerCapture(pointerId))
            this.capturePort.setPointerCapture(pointerId);
        this.capturedPointerId = pointerId;
    }
    releaseCapturedPointer() {
        const pointerId = this.capturedPointerId;
        if (pointerId === undefined)
            return;
        this.capturedPointerId = undefined;
        const errors = [];
        this.tryReleasePhysicalCapture(pointerId, errors);
        throwLifecycleErrors(errors, "Pointer release failed.");
    }
    tryReleasePhysicalCapture(pointerId, errors) {
        try {
            if (this.capturePort.hasPointerCapture(pointerId))
                this.capturePort.releasePointerCapture(pointerId);
        }
        catch (error) {
            errors.push(error);
        }
    }
    invalidatePreview() {
        if (this.previewFrame !== undefined)
            return;
        this.previewFrame = this.framePort.requestFrame(() => {
            this.previewFrame = undefined;
            if (!this.disposed)
                this.renderPreview();
        });
    }
    cancelPreview() {
        const errors = [];
        this.tryCancelPreview(errors);
        throwLifecycleErrors(errors, "Preview-frame cancellation failed.");
    }
    tryCancelPreview(errors) {
        const handle = this.previewFrame;
        if (handle === undefined)
            return;
        this.previewFrame = undefined;
        try {
            this.framePort.cancelFrame(handle);
        }
        catch (error) {
            errors.push(error);
        }
    }
    assertActive() {
        if (this.disposed)
            throw new Error("ToolController is disposed.");
    }
}
