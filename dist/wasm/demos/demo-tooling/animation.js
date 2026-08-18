import { interpolateVec2 } from "./geometry.js";
export const linearEasing = (amount) => amount;
export function scalarBinding(read, write) {
    return { read, write };
}
export function vec2Binding(read, write) {
    return { read, write };
}
function throwAnimationErrors(errors) {
    if (errors.length === 1)
        throw errors[0];
    if (errors.length > 1)
        throw new AggregateError(errors, "Multiple animation callbacks failed.");
}
function finiteScalar(value, label) {
    if (!Number.isFinite(value))
        throw new RangeError(`${label} must be finite.`);
    return value;
}
function finiteVec2(value, label) {
    return {
        x: finiteScalar(value.x, `${label}.x`),
        y: finiteScalar(value.y, `${label}.y`),
    };
}
function requireFunction(value, label) {
    if (typeof value !== "function")
        throw new TypeError(`${label} must be a function.`);
    return value;
}
function optionalFunction(value, label) {
    return value === undefined ? undefined : requireFunction(value, label);
}
export class AnimationScheduler {
    clock;
    animations = new Map();
    frameHandle;
    constructor(clock) {
        this.clock = clock;
    }
    get isIdle() {
        return this.animations.size === 0;
    }
    animateScalar(name, binding, target, options) {
        this.animate(name, binding, target, (from, to, amount) => from + (to - from) * amount, options, finiteScalar);
    }
    animateVec2(name, binding, target, options) {
        this.animate(name, binding, target, interpolateVec2, options, finiteVec2);
    }
    cancel(name) {
        const animation = this.animations.get(name);
        if (animation === undefined)
            return false;
        this.animations.delete(name);
        const errors = [];
        try {
            animation.cancel("cancelled");
        }
        catch (error) {
            errors.push(error);
        }
        this.stopFrameWhenIdle(errors);
        throwAnimationErrors(errors);
        return true;
    }
    cancelAll() {
        const snapshot = [...this.animations.entries()];
        for (const [name, animation] of snapshot) {
            if (this.animations.get(name) === animation)
                this.animations.delete(name);
        }
        const errors = [];
        for (const [, animation] of snapshot) {
            try {
                animation.cancel("cancelled");
            }
            catch (error) {
                errors.push(error);
            }
        }
        this.stopFrameWhenIdle(errors);
        this.ensureFrame();
        throwAnimationErrors(errors);
    }
    tick(timestamp) {
        finiteScalar(timestamp, "Animation timestamp");
        const errors = [];
        for (const [name, animation] of [...this.animations]) {
            if (this.animations.get(name) !== animation)
                continue;
            let completed = false;
            try {
                completed = animation.sample(timestamp);
            }
            catch (error) {
                errors.push(error);
                this.animations.delete(name);
                animation.abandon();
                continue;
            }
            if (!completed || this.animations.get(name) !== animation)
                continue;
            this.animations.delete(name);
            try {
                animation.settle();
            }
            catch (error) {
                errors.push(error);
            }
        }
        this.stopFrameWhenIdle(errors);
        throwAnimationErrors(errors);
    }
    animate(name, binding, targetInput, interpolate, options, cloneFinite) {
        if (name.length === 0)
            throw new Error("Animation name cannot be empty.");
        const durationMilliseconds = finiteScalar(options.durationMilliseconds, "Animation durationMilliseconds");
        if (durationMilliseconds < 0)
            throw new RangeError("Animation durationMilliseconds must be >= 0.");
        const easing = requireFunction(options.easing ?? linearEasing, "Animation easing");
        const onSettle = optionalFunction(options.onSettle, "Animation onSettle");
        const onCancel = optionalFunction(options.onCancel, "Animation onCancel");
        const read = requireFunction(binding.read, "Animation binding.read").bind(binding);
        const write = requireFunction(binding.write, "Animation binding.write").bind(binding);
        const now = finiteScalar(this.clock.now(), "Animation clock time");
        const target = cloneFinite(targetInput, "Animation target");
        const errors = [];
        this.retireForRetarget(name, now, errors);
        throwAnimationErrors(errors);
        if (this.animations.has(name))
            return;
        const from = cloneFinite(read(), "Animation source");
        if (durationMilliseconds === 0) {
            write(cloneFinite(target, "Animation target"));
            onSettle?.();
            const idleErrors = [];
            this.stopFrameWhenIdle(idleErrors);
            throwAnimationErrors(idleErrors);
            return;
        }
        let terminal = false;
        let sampling = false;
        const animation = {
            sample: (timestamp) => {
                if (terminal)
                    return true;
                if (sampling)
                    return false;
                const rawAmount = (timestamp - now) / durationMilliseconds;
                if (rawAmount >= 1)
                    return true;
                const amount = Math.max(0, rawAmount);
                sampling = true;
                try {
                    const eased = finiteScalar(easing(amount), "Animation easing result");
                    if (terminal)
                        return true;
                    write(cloneFinite(interpolate(from, target, eased), "Animation sample"));
                }
                finally {
                    sampling = false;
                }
                return false;
            },
            settle: () => {
                if (terminal)
                    return;
                terminal = true;
                write(cloneFinite(target, "Animation target"));
                onSettle?.();
            },
            cancel: (reason) => {
                if (terminal)
                    return;
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
    retireForRetarget(name, timestamp, errors) {
        const previous = this.animations.get(name);
        if (previous === undefined)
            return;
        this.animations.delete(name);
        let completed = false;
        try {
            completed = previous.sample(timestamp);
        }
        catch (error) {
            errors.push(error);
            previous.abandon();
            return;
        }
        try {
            if (completed)
                previous.settle();
            else
                previous.cancel("retargeted");
        }
        catch (error) {
            errors.push(error);
        }
    }
    ensureFrame() {
        if (this.frameHandle !== undefined || this.animations.size === 0)
            return;
        try {
            this.frameHandle = this.clock.requestFrame((timestamp) => {
                this.frameHandle = undefined;
                try {
                    this.tick(timestamp);
                }
                finally {
                    this.ensureFrame();
                }
            });
        }
        catch (error) {
            const snapshot = [...this.animations.values()];
            this.animations.clear();
            for (const animation of snapshot)
                animation.abandon();
            throw error;
        }
    }
    stopFrameWhenIdle(errors) {
        if (this.animations.size !== 0 || this.frameHandle === undefined)
            return;
        const handle = this.frameHandle;
        this.frameHandle = undefined;
        try {
            this.clock.cancelFrame(handle);
        }
        catch (error) {
            errors.push(error);
        }
    }
}
