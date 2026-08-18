import { interpolateVec2, type Vec2 } from "./geometry.js";

export type Easing = (amount: number) => number;
export type AnimationCancelReason = "cancelled" | "retargeted";

export interface AnimationBinding<Value> {
  read(): Value;
  write(value: Value): void;
}

export interface AnimationOptions {
  readonly durationMilliseconds: number;
  readonly easing?: Easing;
  readonly onSettle?: () => void;
  readonly onCancel?: (reason: AnimationCancelReason) => void;
}

export interface AnimationFrameClock {
  now(): number;
  requestFrame(callback: (timestamp: number) => void): number;
  cancelFrame(handle: number): void;
}

interface ActiveAnimation {
  sample(timestamp: number): boolean;
  settle(): void;
  cancel(reason: AnimationCancelReason): void;
  abandon(): void;
}

export const linearEasing: Easing = (amount) => amount;

export function scalarBinding(
  read: () => number,
  write: (value: number) => void,
): AnimationBinding<number> {
  return { read, write };
}

export function vec2Binding(
  read: () => Vec2,
  write: (value: Vec2) => void,
): AnimationBinding<Vec2> {
  return { read, write };
}

function throwAnimationErrors(errors: readonly unknown[]): void {
  if (errors.length === 1) throw errors[0];
  if (errors.length > 1) throw new AggregateError(errors, "Multiple animation callbacks failed.");
}

function finiteScalar(value: number, label: string): number {
  if (!Number.isFinite(value)) throw new RangeError(`${label} must be finite.`);
  return value;
}

function finiteVec2(value: Vec2, label: string): Vec2 {
  return {
    x: finiteScalar(value.x, `${label}.x`),
    y: finiteScalar(value.y, `${label}.y`),
  };
}

function requireFunction<FunctionType extends (...arguments_: never[]) => unknown>(
  value: FunctionType,
  label: string,
): FunctionType {
  if (typeof value !== "function") throw new TypeError(`${label} must be a function.`);
  return value;
}

function optionalFunction<FunctionType extends (...arguments_: never[]) => unknown>(
  value: FunctionType | undefined,
  label: string,
): FunctionType | undefined {
  return value === undefined ? undefined : requireFunction(value, label);
}

export class AnimationScheduler {
  private readonly animations = new Map<string, ActiveAnimation>();
  private frameHandle: number | undefined;

  constructor(private readonly clock: AnimationFrameClock) {}

  get isIdle(): boolean {
    return this.animations.size === 0;
  }

  animateScalar(
    name: string,
    binding: AnimationBinding<number>,
    target: number,
    options: AnimationOptions,
  ): void {
    this.animate(
      name,
      binding,
      target,
      (from, to, amount) => from + (to - from) * amount,
      options,
      finiteScalar,
    );
  }

  animateVec2(
    name: string,
    binding: AnimationBinding<Vec2>,
    target: Vec2,
    options: AnimationOptions,
  ): void {
    this.animate(name, binding, target, interpolateVec2, options, finiteVec2);
  }

  cancel(name: string): boolean {
    const animation = this.animations.get(name);
    if (animation === undefined) return false;
    this.animations.delete(name);
    const errors: unknown[] = [];
    try {
      animation.cancel("cancelled");
    } catch (error) {
      errors.push(error);
    }
    this.stopFrameWhenIdle(errors);
    throwAnimationErrors(errors);
    return true;
  }

  cancelAll(): void {
    const snapshot = [...this.animations.entries()];
    for (const [name, animation] of snapshot) {
      if (this.animations.get(name) === animation) this.animations.delete(name);
    }
    const errors: unknown[] = [];
    for (const [, animation] of snapshot) {
      try {
        animation.cancel("cancelled");
      } catch (error) {
        errors.push(error);
      }
    }
    this.stopFrameWhenIdle(errors);
    this.ensureFrame();
    throwAnimationErrors(errors);
  }

  tick(timestamp: number): void {
    finiteScalar(timestamp, "Animation timestamp");
    const errors: unknown[] = [];
    for (const [name, animation] of [...this.animations]) {
      if (this.animations.get(name) !== animation) continue;
      let completed = false;
      try {
        completed = animation.sample(timestamp);
      } catch (error) {
        errors.push(error);
        this.animations.delete(name);
        animation.abandon();
        continue;
      }
      if (!completed || this.animations.get(name) !== animation) continue;
      this.animations.delete(name);
      try {
        animation.settle();
      } catch (error) {
        errors.push(error);
      }
    }
    this.stopFrameWhenIdle(errors);
    throwAnimationErrors(errors);
  }

  private animate<Value>(
    name: string,
    binding: AnimationBinding<Value>,
    targetInput: Value,
    interpolate: (from: Value, to: Value, amount: number) => Value,
    options: AnimationOptions,
    cloneFinite: (value: Value, label: string) => Value,
  ): void {
    if (name.length === 0) throw new Error("Animation name cannot be empty.");
    const durationMilliseconds = finiteScalar(
      options.durationMilliseconds,
      "Animation durationMilliseconds",
    );
    if (durationMilliseconds < 0)
      throw new RangeError("Animation durationMilliseconds must be >= 0.");
    const easing = requireFunction(options.easing ?? linearEasing, "Animation easing");
    const onSettle = optionalFunction(options.onSettle, "Animation onSettle");
    const onCancel = optionalFunction(options.onCancel, "Animation onCancel");
    const read = requireFunction(binding.read, "Animation binding.read").bind(binding);
    const write = requireFunction(binding.write, "Animation binding.write").bind(binding);
    const now = finiteScalar(this.clock.now(), "Animation clock time");
    const target = cloneFinite(targetInput, "Animation target");
    const errors: unknown[] = [];
    this.retireForRetarget(name, now, errors);
    throwAnimationErrors(errors);
    if (this.animations.has(name)) return;
    const from = cloneFinite(read(), "Animation source");
    if (durationMilliseconds === 0) {
      write(cloneFinite(target, "Animation target"));
      onSettle?.();
      const idleErrors: unknown[] = [];
      this.stopFrameWhenIdle(idleErrors);
      throwAnimationErrors(idleErrors);
      return;
    }
    let terminal = false;
    let sampling = false;
    const animation: ActiveAnimation = {
      sample: (timestamp) => {
        if (terminal) return true;
        if (sampling) return false;
        const rawAmount = (timestamp - now) / durationMilliseconds;
        if (rawAmount >= 1) return true;
        const amount = Math.max(0, rawAmount);
        sampling = true;
        try {
          const eased = finiteScalar(easing(amount), "Animation easing result");
          if (terminal) return true;
          write(cloneFinite(interpolate(from, target, eased), "Animation sample"));
        } finally {
          sampling = false;
        }
        return false;
      },
      settle: () => {
        if (terminal) return;
        terminal = true;
        write(cloneFinite(target, "Animation target"));
        onSettle?.();
      },
      cancel: (reason) => {
        if (terminal) return;
        terminal = true;
        onCancel?.(reason);
      },
      abandon: () => {
        terminal = true;
      },
    };
    this.animations.set(name, Object.freeze(animation));
    this.ensureFrame();
  }

  private retireForRetarget(name: string, timestamp: number, errors: unknown[]): void {
    const previous = this.animations.get(name);
    if (previous === undefined) return;
    this.animations.delete(name);
    let completed = false;
    try {
      completed = previous.sample(timestamp);
    } catch (error) {
      errors.push(error);
      previous.abandon();
      return;
    }
    try {
      if (completed) previous.settle();
      else previous.cancel("retargeted");
    } catch (error) {
      errors.push(error);
    }
  }

  private ensureFrame(): void {
    if (this.frameHandle !== undefined || this.animations.size === 0) return;
    try {
      this.frameHandle = this.clock.requestFrame((timestamp) => {
        this.frameHandle = undefined;
        try {
          this.tick(timestamp);
        } finally {
          this.ensureFrame();
        }
      });
    } catch (error) {
      const snapshot = [...this.animations.values()];
      this.animations.clear();
      for (const animation of snapshot) animation.abandon();
      throw error;
    }
  }

  private stopFrameWhenIdle(errors: unknown[]): void {
    if (this.animations.size !== 0 || this.frameHandle === undefined) return;
    const handle = this.frameHandle;
    this.frameHandle = undefined;
    try {
      this.clock.cancelFrame(handle);
    } catch (error) {
      errors.push(error);
    }
  }
}
