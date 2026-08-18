import type { PointerInput, PointerIntent, WheelInput } from "./input.js";

export interface PointerCapturePort {
  hasPointerCapture(pointerId: number): boolean;
  setPointerCapture(pointerId: number): void;
  releasePointerCapture(pointerId: number): void;
}

export interface FramePort {
  requestFrame(callback: () => void): number;
  cancelFrame(handle: number): void;
}

export interface ToolContext {
  capturePointer(): void;
  releasePointer(): void;
  invalidatePreview(): void;
}

export interface EditorTool {
  pointerDown?(input: PointerInput, intent: PointerIntent | undefined, context: ToolContext): void;
  pointerMove?(input: PointerInput, context: ToolContext): void;
  pointerUp?(input: PointerInput, context: ToolContext): void;
  wheel?(input: WheelInput, context: ToolContext): void;
  cancel?(context: ToolContext): void;
}

function throwLifecycleErrors(errors: readonly unknown[], message: string): void {
  if (errors.length === 1) throw errors[0];
  if (errors.length > 1) throw new AggregateError(errors, message);
}

export class ToolController {
  private tool: EditorTool;
  private activePointerId: number | undefined;
  private capturedPointerId: number | undefined;
  private dispatchPointerId: number | undefined;
  private previewFrame: number | undefined;
  private disposed = false;

  constructor(
    tool: EditorTool,
    private readonly capturePort: PointerCapturePort,
    private readonly framePort: FramePort,
    private readonly renderPreview: () => void,
  ) {
    this.tool = tool;
  }

  setTool(tool: EditorTool): void {
    this.assertActive();
    this.cancel();
    this.tool = tool;
  }

  pointerDown(input: PointerInput, intent: PointerIntent | undefined): void {
    this.assertActive();
    if (this.activePointerId !== undefined) return;
    this.activePointerId = input.pointerId;
    try {
      this.withPointer(input.pointerId, (context) =>
        this.tool.pointerDown?.(input, intent, context),
      );
    } catch (error) {
      const errors: unknown[] = [error];
      this.terminateGesture(true, true, errors);
      throwLifecycleErrors(errors, "Pointer-down failure and cleanup both failed.");
    }
  }

  pointerMove(input: PointerInput): void {
    this.assertActive();
    if (this.activePointerId !== undefined && this.activePointerId !== input.pointerId) return;
    this.withPointer(input.pointerId, (context) => this.tool.pointerMove?.(input, context));
  }

  pointerUp(input: PointerInput): void {
    this.assertActive();
    if (this.activePointerId !== input.pointerId) return;
    const errors: unknown[] = [];
    this.terminateGesture(false, false, errors, (context) => this.tool.pointerUp?.(input, context));
    if (errors.length > 0) this.tryCancelPreview(errors);
    throwLifecycleErrors(errors, "Pointer-up callback and cleanup both failed.");
  }

  pointerCancel(pointerId: number): void {
    this.assertActive();
    if (this.activePointerId !== pointerId) return;
    const errors: unknown[] = [];
    this.terminateGesture(true, true, errors);
    throwLifecycleErrors(errors, "Pointer cancellation cleanup failed.");
  }

  pointerCaptureLost(pointerId: number): void {
    this.assertActive();
    if (this.activePointerId !== pointerId && this.capturedPointerId !== pointerId) return;
    const errors: unknown[] = [];
    this.terminateGesture(true, true, errors, undefined, false);
    throwLifecycleErrors(errors, "Lost-pointer-capture cleanup failed.");
  }

  wheel(input: WheelInput): void {
    this.assertActive();
    this.tool.wheel?.(input, this.context());
  }

  cancel(): void {
    if (this.disposed) return;
    if (this.activePointerId === undefined) {
      this.cancelPreview();
      return;
    }
    const errors: unknown[] = [];
    this.terminateGesture(true, true, errors);
    throwLifecycleErrors(errors, "Tool cancellation cleanup failed.");
  }

  dispose(): void {
    if (this.disposed) return;
    this.disposed = true;
    const errors: unknown[] = [];
    if (this.activePointerId !== undefined) this.terminateGesture(true, true, errors);
    else this.tryCancelPreview(errors);
    throwLifecycleErrors(errors, "Tool disposal cleanup failed.");
  }

  private terminateGesture(
    notifyCancel: boolean,
    cancelPreview: boolean,
    errors: unknown[],
    terminalCallback?: (context: ToolContext) => void,
    releasePhysicalCapture = true,
  ): void {
    const capturedPointerId = this.capturedPointerId;
    this.activePointerId = undefined;
    this.capturedPointerId = undefined;
    this.dispatchPointerId = undefined;
    if (cancelPreview) this.tryCancelPreview(errors);
    const context = this.terminalContext();
    try {
      if (notifyCancel) this.tool.cancel?.(context);
      else terminalCallback?.(context);
    } catch (error) {
      errors.push(error);
    }
    if (cancelPreview) this.tryCancelPreview(errors);
    if (releasePhysicalCapture && capturedPointerId !== undefined)
      this.tryReleasePhysicalCapture(capturedPointerId, errors);
  }

  private withPointer(pointerId: number, callback: (context: ToolContext) => void): void {
    this.dispatchPointerId = pointerId;
    try {
      callback(this.context());
    } finally {
      this.dispatchPointerId = undefined;
    }
  }

  private context(): ToolContext {
    return {
      capturePointer: () => this.captureCurrentPointer(),
      releasePointer: () => this.releaseCapturedPointer(),
      invalidatePreview: () => this.invalidatePreview(),
    };
  }

  private terminalContext(): ToolContext {
    return {
      capturePointer: () => {
        throw new Error("A terminal tool callback cannot capture a pointer.");
      },
      releasePointer: () => undefined,
      invalidatePreview: () => this.invalidatePreview(),
    };
  }

  private captureCurrentPointer(): void {
    const pointerId = this.dispatchPointerId;
    if (pointerId === undefined || this.activePointerId !== pointerId)
      throw new Error("Pointer capture requires the active pointer dispatch.");
    if (this.capturedPointerId !== undefined && this.capturedPointerId !== pointerId)
      throw new Error("A different pointer is already captured.");
    if (!this.capturePort.hasPointerCapture(pointerId))
      this.capturePort.setPointerCapture(pointerId);
    this.capturedPointerId = pointerId;
  }

  private releaseCapturedPointer(): void {
    const pointerId = this.capturedPointerId;
    if (pointerId === undefined) return;
    this.capturedPointerId = undefined;
    const errors: unknown[] = [];
    this.tryReleasePhysicalCapture(pointerId, errors);
    throwLifecycleErrors(errors, "Pointer release failed.");
  }

  private tryReleasePhysicalCapture(pointerId: number, errors: unknown[]): void {
    try {
      if (this.capturePort.hasPointerCapture(pointerId))
        this.capturePort.releasePointerCapture(pointerId);
    } catch (error) {
      errors.push(error);
    }
  }

  private invalidatePreview(): void {
    if (this.previewFrame !== undefined) return;
    this.previewFrame = this.framePort.requestFrame(() => {
      this.previewFrame = undefined;
      if (!this.disposed) this.renderPreview();
    });
  }

  private cancelPreview(): void {
    const errors: unknown[] = [];
    this.tryCancelPreview(errors);
    throwLifecycleErrors(errors, "Preview-frame cancellation failed.");
  }

  private tryCancelPreview(errors: unknown[]): void {
    const handle = this.previewFrame;
    if (handle === undefined) return;
    this.previewFrame = undefined;
    try {
      this.framePort.cancelFrame(handle);
    } catch (error) {
      errors.push(error);
    }
  }

  private assertActive(): void {
    if (this.disposed) throw new Error("ToolController is disposed.");
  }
}
