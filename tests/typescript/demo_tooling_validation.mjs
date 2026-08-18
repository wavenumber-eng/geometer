import assert from "node:assert/strict";
import {
  AnimationScheduler,
  Camera2D,
  CommandHistory,
  CommandRegistry,
  normalizePointerInput,
  normalizeWheelInput,
  resolvePointerIntent,
  scalarBinding,
  ToolController,
  vec2Binding,
} from "../../dist/wasm/demos/demo-tooling/index.js";

function pointer(overrides = {}) {
  return {
    pointerId: 7,
    pointerType: "mouse",
    position: { x: 20, y: 30 },
    button: 0,
    buttons: 1,
    modifiers: { alt: false, control: false, meta: false, shift: false },
    pressure: 0.5,
    ...overrides,
  };
}

{
  const input = pointer();
  const intent = resolvePointerIntent(input, [
    { intent: "primary", priority: 10, matches: () => true },
    { intent: "pan", priority: 20, matches: () => true },
    { intent: "cancel", priority: 20, matches: () => true },
  ]);
  assert.equal(
    intent,
    "pan",
    "higher priority wins and equal priorities retain registration order",
  );
  assert.equal(
    resolvePointerIntent(pointer({ button: 2, modifiers: { ...input.modifiers, alt: true } })),
    "cancel",
  );
  assert.equal(
    resolvePointerIntent(pointer({ modifiers: { ...input.modifiers, shift: true } })),
    "pan",
  );
}

{
  const surface = {
    clientWidth: 600,
    clientHeight: 400,
    getBoundingClientRect: () => ({ left: 10, top: 20 }),
  };
  const event = {
    pointerId: 3,
    pointerType: "pen",
    clientX: 25,
    clientY: 45,
    button: 0,
    buttons: 1,
    pressure: 0.75,
    altKey: false,
    ctrlKey: true,
    metaKey: false,
    shiftKey: false,
  };
  assert.deepEqual(normalizePointerInput(event, surface), {
    pointerId: 3,
    pointerType: "pen",
    position: { x: 15, y: 25 },
    button: 0,
    buttons: 1,
    pressure: 0.75,
    modifiers: { alt: false, control: true, meta: false, shift: false },
  });
  const wheel = (deltaMode) =>
    normalizeWheelInput(
      {
        clientX: 10,
        clientY: 20,
        deltaX: 2,
        deltaY: -3,
        deltaMode,
        altKey: false,
        ctrlKey: false,
        metaKey: false,
        shiftKey: false,
      },
      surface,
    );
  assert.deepEqual([wheel(0).deltaY, wheel(1).deltaY, wheel(2).deltaY], [-3, -48, -1200]);
  assert.throws(() => wheel(3), /deltaMode/);
}

{
  const center = { x: 10, y: -5 };
  const camera = new Camera2D(
    { width: 800, height: 600 },
    { center, pixelsPerWorldUnit: 4, minPixelsPerWorldUnit: 0.25, maxPixelsPerWorldUnit: 16 },
  );
  center.x = 999;
  const publishedCenter = camera.center;
  publishedCenter.x = 888;
  assert.deepEqual(camera.center, { x: 10, y: -5 }, "camera owns ingress and getter values");
  const anchor = { x: 135, y: 472 };
  const before = camera.screenToWorld(anchor);
  camera.zoomAt(anchor, -120);
  const after = camera.screenToWorld(anchor);
  assert.ok(Math.abs(before.x - after.x) < 1e-12);
  assert.ok(Math.abs(before.y - after.y) < 1e-12);
  camera.zoomAt(anchor, -Number.MAX_VALUE);
  assert.equal(camera.pixelsPerWorldUnit, 16, "overflowing wheel zoom clamps to the maximum");
  const afterClamp = camera.screenToWorld(anchor);
  assert.ok(Math.abs(before.x - afterClamp.x) < 1e-12);
  assert.ok(Math.abs(before.y - afterClamp.y) < 1e-12);
  camera.panByScreen({ x: 20, y: -10 });

  const fitted = new Camera2D({ width: 800, height: 600 });
  fitted.fit({ min: { x: 0, y: 0 }, max: { x: 100, y: 50 } }, 100);
  assert.equal(fitted.pixelsPerWorldUnit, 6);
  assert.deepEqual(fitted.center, { x: 50, y: 25 });
  assert.deepEqual(fitted.gridScale(20, 5), {
    minorWorldSpacing: 5,
    majorWorldSpacing: 25,
    minorPixelSpacing: 30,
    majorEvery: 5,
  });
  assert.throws(() => fitted.gridScale(Number.MAX_VALUE, 100), /finite numeric range/);
}

{
  const captured = new Set();
  const captureEvents = [];
  const frames = new Map();
  const cancelledFrames = [];
  let nextFrame = 1;
  let previews = 0;
  let moves = 0;
  let cancellations = 0;
  const capturePort = {
    hasPointerCapture: (id) => captured.has(id),
    setPointerCapture: (id) => {
      captured.add(id);
      captureEvents.push(`capture:${id}`);
    },
    releasePointerCapture: (id) => {
      captured.delete(id);
      captureEvents.push(`release:${id}`);
    },
  };
  const framePort = {
    requestFrame: (callback) => {
      const handle = nextFrame++;
      frames.set(handle, callback);
      return handle;
    },
    cancelFrame: (handle) => {
      cancelledFrames.push(handle);
      frames.delete(handle);
    },
  };
  const tool = {
    pointerDown: (_input, _intent, context) => {
      context.capturePointer();
      context.invalidatePreview();
      context.invalidatePreview();
    },
    pointerMove: (_input, context) => {
      moves += 1;
      context.invalidatePreview();
    },
    pointerUp: (_input, context) => context.invalidatePreview(),
    cancel: () => {
      cancellations += 1;
    },
  };
  const controller = new ToolController(tool, capturePort, framePort, () => {
    previews += 1;
  });
  controller.pointerDown(pointer(), "primary");
  assert.deepEqual(captureEvents, ["capture:7"]);
  assert.equal(frames.size, 1, "preview invalidations coalesce into one frame");
  controller.pointerMove(pointer({ pointerId: 8 }));
  assert.equal(moves, 0, "non-captured pointer is ignored");
  const callback = frames.values().next().value;
  assert.equal(typeof callback, "function");
  frames.clear();
  callback();
  assert.equal(previews, 1);
  controller.pointerUp(pointer());
  assert.deepEqual(captureEvents, ["capture:7", "release:7"]);
  controller.cancel();
  assert.equal(cancelledFrames.length, 1, "cancel removes the queued post-up preview");

  controller.pointerDown(pointer(), "primary");
  captured.delete(7);
  controller.pointerCaptureLost(7);
  controller.pointerCaptureLost(7);
  assert.equal(cancellations, 1, "lost capture cancels exactly once");
  controller.pointerMove(pointer({ pointerId: 8 }));
  assert.equal(moves, 1, "lost capture cancels the gesture and permits a new pointer");

  controller.pointerDown(pointer({ pointerId: 8 }), "primary");
  controller.pointerCancel(7);
  assert.equal(cancellations, 1, "pointer cancellation is pointer-id aware");
  controller.pointerCancel(8);
  controller.pointerCancel(8);
  assert.equal(cancellations, 2, "pointer cancellation is terminal and exact once");

  controller.pointerDown(pointer(), "primary");
  controller.dispose();
  controller.dispose();
  assert.equal(cancellations, 3, "dispose cancels an active tool exactly once");
  assert.throws(() => controller.pointerMove(pointer()), /disposed/);
}

{
  const captured = new Set();
  let releaseThrows = true;
  let cancelThrows = true;
  let cancellations = 0;
  const controller = new ToolController(
    {
      pointerDown: (_input, _intent, context) => context.capturePointer(),
      cancel: () => {
        cancellations += 1;
        if (cancelThrows) throw new Error("cancel failed");
      },
    },
    {
      hasPointerCapture: (id) => captured.has(id),
      setPointerCapture: (id) => captured.add(id),
      releasePointerCapture: (id) => {
        captured.delete(id);
        if (releaseThrows) throw new Error("release failed");
      },
    },
    { requestFrame: () => 1, cancelFrame: () => undefined },
    () => undefined,
  );
  controller.pointerDown(pointer(), "primary");
  assert.throws(
    () => controller.pointerCancel(7),
    (error) =>
      error instanceof AggregateError &&
      error.errors.some((item) => item.message === "cancel failed") &&
      error.errors.some((item) => item.message === "release failed"),
  );
  assert.equal(cancellations, 1);
  releaseThrows = false;
  cancelThrows = false;
  controller.pointerDown(pointer({ pointerId: 8 }), "primary");
  controller.pointerCancel(8);
  assert.equal(cancellations, 2, "cleanup failure leaves the controller reusable");
}

{
  const captured = new Set();
  let cancellations = 0;
  let releases = 0;
  const controller = new ToolController(
    {
      pointerDown: (_input, _intent, context) => {
        context.capturePointer();
        throw new Error("down failed");
      },
      cancel: () => {
        cancellations += 1;
        throw new Error("down cancel failed");
      },
    },
    {
      hasPointerCapture: (id) => captured.has(id),
      setPointerCapture: (id) => captured.add(id),
      releasePointerCapture: (id) => {
        releases += 1;
        captured.delete(id);
        throw new Error("down release failed");
      },
    },
    { requestFrame: () => 1, cancelFrame: () => undefined },
    () => undefined,
  );
  assert.throws(
    () => controller.pointerDown(pointer(), "primary"),
    (error) => error instanceof AggregateError && error.errors.length === 3,
  );
  assert.deepEqual({ cancellations, releases }, { cancellations: 1, releases: 1 });
  assert.throws(() => controller.pointerDown(pointer({ pointerId: 8 }), "primary"));
  assert.deepEqual(
    { cancellations, releases },
    { cancellations: 2, releases: 2 },
    "pointer-down failure clears terminal state before cleanup callbacks",
  );
}

{
  let time = 0;
  let nextHandle = 1;
  const frames = new Map();
  const cancelled = [];
  const clock = {
    now: () => time,
    requestFrame: (callback) => {
      const handle = nextHandle++;
      frames.set(handle, callback);
      return handle;
    },
    cancelFrame: (handle) => {
      cancelled.push(handle);
      frames.delete(handle);
    },
  };
  const fireFrame = (timestamp) => {
    time = timestamp;
    const entry = frames.entries().next().value;
    assert.notEqual(entry, undefined, "expected a scheduled animation frame");
    const [handle, callback] = entry;
    frames.delete(handle);
    callback(timestamp);
  };
  const scheduler = new AnimationScheduler(clock);
  let scalar = 0;
  const cancelReasons = [];
  scheduler.animateScalar(
    "camera",
    scalarBinding(
      () => scalar,
      (value) => (scalar = value),
    ),
    10,
    {
      durationMilliseconds: 100,
      onCancel: (reason) => cancelReasons.push(reason),
    },
  );
  fireFrame(25);
  assert.equal(scalar, 2.5);
  scheduler.animateScalar(
    "camera",
    scalarBinding(
      () => scalar,
      (value) => (scalar = value),
    ),
    20,
    {
      durationMilliseconds: 75,
      onCancel: (reason) => cancelReasons.push(reason),
    },
  );
  assert.deepEqual(cancelReasons, ["retargeted"]);
  fireFrame(62.5);
  assert.equal(scalar, 11.25, "retarget starts from the sampled current value");
  assert.equal(scheduler.cancel("camera"), true);
  assert.deepEqual(cancelReasons, ["retargeted", "cancelled"]);
  assert.equal(scheduler.isIdle, true);
  assert.ok(cancelled.length > 0, "idle scheduler cancels its outstanding RAF");

  let vector = { x: 0, y: 0 };
  const vectorTarget = { x: 4, y: 8 };
  let settled = 0;
  scheduler.animateVec2(
    "point",
    vec2Binding(
      () => vector,
      (value) => (vector = value),
    ),
    vectorTarget,
    {
      durationMilliseconds: 40,
      onSettle: () => {
        settled += 1;
      },
    },
  );
  vectorTarget.x = 400;
  vector = { x: 100, y: 100 };
  fireFrame(time + 20);
  assert.deepEqual(vector, { x: 2, y: 4 }, "vector source and target are captured by value");
  fireFrame(time + 20);
  assert.deepEqual(vector, { x: 4, y: 8 });
  assert.equal(settled, 1);
  assert.equal(scheduler.isIdle, true);
  assert.equal(scheduler.cancel("point"), false, "settlement is terminal and idempotent");

  let survivor = 0;
  scheduler.animateScalar(
    "throws",
    scalarBinding(
      () => 0,
      () => {
        throw new Error("binding failed");
      },
    ),
    1,
    { durationMilliseconds: 100 },
  );
  scheduler.animateScalar(
    "survivor",
    scalarBinding(
      () => survivor,
      (value) => (survivor = value),
    ),
    10,
    { durationMilliseconds: 100 },
  );
  assert.throws(() => fireFrame(time + 50), /binding failed/);
  assert.equal(survivor, 5, "one animation failure does not skip independent records");
  assert.equal(frames.size, 1, "RAF scheduling survives a tick callback failure");
  scheduler.cancelAll();
  let reentrant = 0;
  let retargetedFromWrite = false;
  scheduler.animateScalar(
    "reentrant",
    scalarBinding(
      () => reentrant,
      (value) => {
        reentrant = value;
        if (!retargetedFromWrite) {
          retargetedFromWrite = true;
          scheduler.animateScalar(
            "reentrant",
            scalarBinding(
              () => reentrant,
              (next) => (reentrant = next),
            ),
            20,
            { durationMilliseconds: 10 },
          );
        }
      },
    ),
    10,
    { durationMilliseconds: 100 },
  );
  fireFrame(time + 50);
  fireFrame(time + 10);
  assert.equal(reentrant, 20, "a binding may safely retarget its own named animation");
  let aliasedValue = 0;
  let originalSettles = 0;
  const aliasedBinding = {
    read: () => aliasedValue,
    write: (value) => (aliasedValue = value),
  };
  const aliasedOptions = {
    durationMilliseconds: 100,
    easing: (amount) => amount,
    onSettle: () => {
      originalSettles += 1;
    },
  };
  scheduler.animateScalar("aliased", aliasedBinding, 10, aliasedOptions);
  aliasedBinding.read = () => {
    throw new Error("mutated read");
  };
  aliasedBinding.write = () => {
    throw new Error("mutated write");
  };
  aliasedOptions.durationMilliseconds = 1;
  aliasedOptions.easing = () => Number.NaN;
  aliasedOptions.onSettle = () => {
    throw new Error("mutated settle");
  };
  fireFrame(time + 50);
  assert.equal(aliasedValue, 5);
  fireFrame(time + 50);
  assert.equal(aliasedValue, 10);
  assert.equal(originalSettles, 1, "animation records capture binding and option callbacks");
  let spawned = 0;
  scheduler.animateScalar(
    "parent",
    scalarBinding(
      () => 0,
      () => undefined,
    ),
    1,
    {
      durationMilliseconds: 10,
      onCancel: () => {
        scheduler.animateScalar(
          "spawned",
          scalarBinding(
            () => spawned,
            (value) => (spawned = value),
          ),
          1,
          { durationMilliseconds: 10 },
        );
      },
    },
  );
  scheduler.cancelAll();
  assert.equal(scheduler.isIdle, false, "cancelAll snapshots and preserves reentrant animations");
  scheduler.cancel("spawned");
  time = Number.NaN;
  assert.throws(
    () =>
      scheduler.animateScalar(
        "bad-clock",
        scalarBinding(
          () => 0,
          () => undefined,
        ),
        1,
        {
          durationMilliseconds: 1,
        },
      ),
    /clock time/,
  );
}

{
  const registry = new CommandRegistry();
  const executed = [];
  registry.register({
    id: "global.delete",
    title: "Delete",
    help: "Delete selection",
    shortcut: { key: "Delete" },
    execute: () => executed.push("global"),
  });
  const unregisterRoute = registry.register({
    id: "route.delete",
    title: "Delete route vertex",
    help: "Delete the active route vertex",
    scope: "route",
    shortcut: { key: "Delete" },
    execute: () => executed.push("route"),
  });
  const key = (target, keyValue = "Delete") => {
    let prevented = false;
    return {
      event: {
        key: keyValue,
        altKey: false,
        ctrlKey: false,
        metaKey: false,
        shiftKey: false,
        target,
        preventDefault: () => {
          prevented = true;
        },
      },
      prevented: () => prevented,
    };
  };
  const scoped = key({ tagName: "canvas" });
  assert.equal(registry.dispatchShortcut(scoped.event, {}, new Set(["route"])), true);
  assert.deepEqual(executed, ["route"], "active scoped command takes precedence over global");
  assert.equal(scoped.prevented(), true);
  const editable = key({ tagName: "INPUT" });
  assert.equal(registry.dispatchShortcut(editable.event, {}, new Set(["route"])), false);
  assert.deepEqual(executed, ["route"], "editable targets are guarded by default");
  const editableAncestor = key({ tagName: "span" });
  editableAncestor.event.composedPath = () => [{ tagName: "span" }, { isContentEditable: true }];
  assert.equal(registry.dispatchShortcut(editableAncestor.event, {}, new Set(["route"])), false);
  assert.deepEqual(
    registry.help(new Set(["route"])).map((entry) => entry.shortcut),
    ["Delete", "Delete"],
  );
  unregisterRoute();
  const global = key({ tagName: "canvas" });
  assert.equal(registry.dispatchShortcut(global.event, {}, new Set(["route"])), true);
  assert.deepEqual(executed, ["route", "global"]);
  assert.equal(registry.help(new Set(["route"])).length, 1);

  const mutableShortcut = { key: "x" };
  const mutableCommand = {
    id: "mutable",
    title: "Original title",
    help: "Original help",
    shortcut: mutableShortcut,
    execute: () => executed.push("stable"),
  };
  const unregisterMutable = registry.register(mutableCommand);
  mutableCommand.id = "mutated";
  mutableCommand.title = "Mutated title";
  mutableShortcut.key = "y";
  mutableCommand.execute = () => executed.push("mutated");
  assert.equal(registry.dispatchShortcut(key({ tagName: "canvas" }, "x").event, {}), true);
  assert.equal(executed.at(-1), "stable");
  assert.equal(registry.help().at(-1).title, "Original title");
  unregisterMutable();
  registry.register({
    id: "mutable",
    title: "Replacement",
    help: "Replacement registration",
    shortcut: { key: "x" },
    execute: () => executed.push("replacement"),
  });
  unregisterMutable();
  assert.equal(registry.dispatchShortcut(key({ tagName: "canvas" }, "x").event, {}), true);
  assert.equal(executed.at(-1), "replacement", "unregister closure uses stable record identity");
}

{
  const add = (amount, label = `Add ${amount}`) => ({
    label,
    apply: (state) => ({ ...state, value: state.value + amount }),
    revert: (state) => ({ ...state, value: state.value - amount }),
  });
  const initial = Object.freeze({ value: 1 });
  const history = new CommandHistory(initial);
  const published = history.state;
  published.value = 999;
  assert.deepEqual(history.state, { value: 1 }, "published state cannot mutate history state");
  history.beginTransaction("empty");
  assert.equal(history.commitTransaction(), false);
  assert.equal(history.canUndo, false);
  history.beginTransaction("gesture");
  history.execute(add(2));
  history.execute(add(3));
  assert.equal(history.state.value, 6);
  assert.equal(history.commitTransaction(), true);
  assert.equal(history.undo(), true);
  assert.deepEqual(history.state, { value: 1 });
  assert.equal(history.redo(), true);
  assert.deepEqual(history.state, { value: 6 });
  assert.notEqual(history.state, initial, "commands produce replacement state values");
  history.beginTransaction("preview rollback");
  history.execute(add(10));
  assert.equal(history.rollbackTransaction(), true);
  assert.deepEqual(history.state, { value: 6 });
  assert.throws(
    () =>
      history.execute({
        label: "mutating",
        apply: (state) => {
          state.value += 1;
          return state;
        },
        revert: (state) => ({ ...state, value: state.value - 1 }),
      }),
    /replacement state/,
  );
  assert.deepEqual(history.state, { value: 6 });
  assert.equal(history.undo(), true);
  assert.equal(history.canRedo, true);
  history.execute(add(7));
  assert.equal(history.canRedo, false, "a new command invalidates the redo branch");
  assert.throws(
    () =>
      history.execute({
        label: "throwing apply",
        apply: (state) => {
          state.value = 1234;
          throw new Error("apply failed");
        },
        revert: (state) => ({ ...state }),
      }),
    /apply failed/,
  );
  assert.deepEqual(history.state, { value: 8 }, "throwing apply is atomic");
  history.execute({
    label: "throwing revert",
    apply: (state) => ({ ...state, value: state.value + 1 }),
    revert: (state) => {
      state.value = -1;
      throw new Error("revert failed");
    },
  });
  assert.equal(history.undo(), true);
  assert.deepEqual(history.state, { value: 8 }, "undo restores its captured before snapshot");
  assert.equal(history.redo(), true);
  assert.deepEqual(history.state, { value: 9 }, "redo restores its captured after snapshot");

  const stableHistory = new CommandHistory({ value: 0 });
  const methodCommand = {
    label: "method amount",
    amount: 2,
    apply(state) {
      return { ...state, value: state.value + this.amount };
    },
    revert(state) {
      return { ...state, value: state.value - this.amount };
    },
  };
  stableHistory.execute(methodCommand);
  methodCommand.amount = 100;
  stableHistory.undo();
  stableHistory.redo();
  assert.deepEqual(
    stableHistory.state,
    { value: 2 },
    "history entries do not replay a command's mutable method receiver",
  );
  let closureAmount = 3;
  stableHistory.execute({
    label: "closure amount",
    apply: (state) => ({ ...state, value: state.value + closureAmount }),
    revert: (state) => ({ ...state, value: state.value - closureAmount }),
  });
  closureAmount = 300;
  stableHistory.undo();
  stableHistory.redo();
  assert.deepEqual(
    stableHistory.state,
    { value: 5 },
    "history entries do not replay mutable command closures",
  );
}

console.log(
  JSON.stringify({
    animation: true,
    camera: true,
    commands: true,
    history: true,
    inputPriority: true,
    toolCapture: true,
  }),
);
