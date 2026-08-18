import { createGeometerWorkerClient } from "@wavenumber/geometer/worker";
import { resolveReflectedCanvasArc } from "./analytic_canvas_arc.js";
import { Camera2D, CommandHistory, CommandRegistry, normalizePointerInput, normalizeWheelInput, resolvePointerIntent, ToolController, } from "./demo-tooling/index.js";
import { CLEARANCE_NM, footprintPads, initialPcbDemoState, makePcbPolygonPourRequest, moveBoardVertex, moveVia, nearestHit, NM_PER_MM, snapPointTo45, TRACE_WIDTH_NM, VIA_COPPER_RADIUS_NM, addTrace, addVia, classifyPcbLayerJob, } from "./pcb_polygon_pour_model.js";
const SOLVE_TIMEOUT_MS = 15_000;
const canvas = requireElement("pcb-canvas");
const shell = requireElement("pcb-shell");
const context = requireCanvasContext(canvas);
const copperCanvas = document.createElement("canvas");
const copperContext = requireCanvasContext(copperCanvas);
const history = new CommandHistory(initialPcbDemoState());
const camera = new Camera2D({ width: 1, height: 1 }, {
    center: { x: 13, y: 7.5 },
    pixelsPerWorldUnit: 40,
    minPixelsPerWorldUnit: 8,
    maxPixelsPerWorldUnit: 400,
});
const commands = new CommandRegistry();
let activeClient;
let latestResults = [];
let latestDigest = "";
let pendingState;
let solving = false;
let completedSolves = 0;
let replacedSolves = 0;
let terminalFailure = false;
let tool = "select";
let drag;
let cursorWorld = { x: 13, y: 7.5 };
let routePoints = [];
let routePreview;
let cameraFitted = false;
const embeddedRuntime = readEmbeddedRuntime();
if (GEOMETER_STANDALONE_BUILD && embeddedRuntime === undefined)
    throw new Error("Standalone Geometer runtime carriers are missing.");
const wasmBinaryPromise = GEOMETER_STANDALONE_BUILD
    ? Promise.resolve(decodeBase64(embeddedRuntime.wasmBase64).buffer)
    : fetch("./geometer.wasm").then((response) => {
        if (!response.ok)
            throw new Error(`geometer.wasm returned HTTP ${response.status}.`);
        return response.arrayBuffer();
    });
const clientPromise = createClient();
const editorTool = {
    pointerDown(input, intent, toolContext) {
        cursorWorld = camera.screenToWorld(input.position);
        if (intent === "pan") {
            drag = { kind: "pan", lastScreen: input.position };
            toolContext.capturePointer();
            return;
        }
        if (intent !== "primary")
            return;
        if (tool === "route") {
            const next = routePoints.length === 0
                ? snapGrid(cursorWorld)
                : snapPointTo45(routePoints.at(-1), cursorWorld);
            if (!samePoint(routePoints.at(-1), next))
                routePoints.push(next);
            routePreview = next;
            render();
            return;
        }
        const state = history.state;
        const tolerance = 12 / camera.pixelsPerWorldUnit;
        const viaIndex = nearestHit(state.vias, cursorWorld, tolerance);
        if (viaIndex !== undefined) {
            const via = state.vias[viaIndex];
            if (via === undefined)
                return;
            drag = { kind: "via", id: via.id };
            history.beginTransaction("Move via");
            toolContext.capturePointer();
            return;
        }
        const vertexIndex = nearestHit(state.board, cursorWorld, tolerance);
        if (vertexIndex !== undefined) {
            drag = { kind: "vertex", index: vertexIndex };
            history.beginTransaction("Move polygon vertex");
            toolContext.capturePointer();
        }
    },
    pointerMove(input) {
        cursorWorld = camera.screenToWorld(input.position);
        if (drag?.kind === "pan") {
            camera.panByScreen({
                x: input.position.x - drag.lastScreen.x,
                y: input.position.y - drag.lastScreen.y,
            });
            drag = { kind: "pan", lastScreen: input.position };
            render();
            return;
        }
        if (drag?.kind === "via") {
            const viaId = drag.id;
            const point = snapGrid(cursorWorld);
            history.execute(replaceState("Move via", (state) => moveVia(state, viaId, point)));
            scheduleSolve(history.state);
            render();
            return;
        }
        if (drag?.kind === "vertex") {
            const vertexIndex = drag.index;
            const point = snapGrid(cursorWorld);
            history.execute(replaceState("Move polygon vertex", (state) => moveBoardVertex(state, vertexIndex, point)));
            scheduleSolve(history.state);
            render();
            return;
        }
        if (tool === "route" && routePoints.length > 0) {
            routePreview = snapPointTo45(routePoints.at(-1), cursorWorld);
            render();
        }
    },
    pointerUp() {
        if (drag?.kind === "via" || drag?.kind === "vertex") {
            history.commitTransaction();
            updateHistoryButtons();
            scheduleSolve(history.state);
        }
        drag = undefined;
    },
    wheel(input) {
        camera.zoomAt(input.position, input.deltaY);
        render();
    },
    cancel() {
        if (history.inTransaction) {
            history.rollbackTransaction();
            scheduleSolve(history.state);
        }
        drag = undefined;
        render();
    },
};
const controller = new ToolController(editorTool, canvas, {
    requestFrame: (callback) => requestAnimationFrame(callback),
    cancelFrame: (handle) => cancelAnimationFrame(handle),
}, render);
registerCommands();
wireEvents();
new ResizeObserver(resize).observe(canvas.parentElement ?? canvas);
window.addEventListener("pagehide", () => {
    controller.dispose();
    activeClient?.terminate();
}, { once: true });
window.__GEOMETER_PCB_DEMO__ = {
    snapshot: () => ({
        state: history.state,
        digest: latestDigest,
        completed: completedSolves,
        replaced: replacedSolves,
        routePointCount: routePoints.length,
        tool,
        camera: { center: camera.center, pixelsPerWorldUnit: camera.pixelsPerWorldUnit },
        layerRecords: sortedLayerRecords(latestResults),
    }),
    screenPoint: (point) => camera.worldToScreen(point),
};
resize();
scheduleSolve(history.state);
function registerCommands() {
    commands.register({
        id: "tool.select",
        title: "Select",
        help: "Select and drag vias or polygon vertices.",
        shortcut: { key: "Escape" },
        execute: () => {
            if (routePoints.length > 0) {
                routePoints = [];
                routePreview = undefined;
                render();
            }
            else
                setTool("select");
        },
    });
    commands.register({
        id: "tool.route",
        title: "Route",
        help: "Start a deterministic 45-degree route.",
        shortcut: { key: "r" },
        execute: () => setTool("route"),
    });
    commands.register({
        id: "route.commit",
        title: "Commit route",
        help: "Commit the active route.",
        shortcut: { key: "Enter" },
        enabled: () => routePoints.length >= 2,
        execute: commitRoute,
    });
    commands.register({
        id: "via.add",
        title: "Add via",
        help: "Add a same-net thermal via at the pointer.",
        shortcut: { key: "v" },
        execute: addViaAtCursor,
    });
    commands.register({
        id: "history.undo",
        title: "Undo",
        help: "Undo the last edit.",
        shortcut: { key: "z", control: true },
        enabled: () => history.canUndo,
        execute: undo,
    });
    commands.register({
        id: "history.redo",
        title: "Redo",
        help: "Redo the last undone edit.",
        shortcut: { key: "y", control: true },
        enabled: () => history.canRedo,
        execute: redo,
    });
}
function wireEvents() {
    canvas.addEventListener("contextmenu", (event) => event.preventDefault());
    canvas.addEventListener("pointerdown", (event) => {
        const input = normalizePointerInput(event, canvas);
        controller.pointerDown(input, resolvePointerIntent(input));
    });
    canvas.addEventListener("pointermove", (event) => controller.pointerMove(normalizePointerInput(event, canvas)));
    canvas.addEventListener("pointerup", (event) => controller.pointerUp(normalizePointerInput(event, canvas)));
    canvas.addEventListener("pointercancel", (event) => controller.pointerCancel(event.pointerId));
    canvas.addEventListener("lostpointercapture", (event) => controller.pointerCaptureLost(event.pointerId));
    canvas.addEventListener("wheel", (event) => {
        event.preventDefault();
        controller.wheel(normalizeWheelInput(event, canvas));
    }, { passive: false });
    window.addEventListener("keydown", (event) => {
        commands.dispatchShortcut(event, undefined);
    });
    requireElement("select-tool").addEventListener("click", () => setTool("select"));
    requireElement("route-tool").addEventListener("click", () => setTool("route"));
    requireElement("add-via").addEventListener("click", addViaAtCursor);
    requireElement("undo").addEventListener("click", undo);
    requireElement("redo").addEventListener("click", redo);
}
function setTool(next) {
    controller.cancel();
    tool = next;
    shell.dataset.tool = next;
    requireElement("select-tool").dataset.active = String(next === "select");
    requireElement("route-tool").dataset.active = String(next === "route");
    setText("tool-status", next === "route"
        ? "Route: click vertices, move for 45° preview, Enter commits, Escape cancels."
        : "Select: drag vias or white polygon vertices.");
    render();
}
function addViaAtCursor() {
    const point = snapGrid(cursorWorld);
    history.execute(replaceState("Add via", (state) => addVia(state, point)));
    updateHistoryButtons();
    scheduleSolve(history.state);
    render();
}
function commitRoute() {
    if (routePoints.length < 2)
        return;
    history.execute(replaceState("Route trace", (state) => addTrace(state, routePoints)));
    routePoints = [];
    routePreview = undefined;
    updateHistoryButtons();
    scheduleSolve(history.state);
    render();
}
function undo() {
    if (!history.undo())
        return;
    updateHistoryButtons();
    scheduleSolve(history.state);
    render();
}
function redo() {
    if (!history.redo())
        return;
    updateHistoryButtons();
    scheduleSolve(history.state);
    render();
}
function updateHistoryButtons() {
    requireElement("undo").disabled = !history.canUndo;
    requireElement("redo").disabled = !history.canRedo;
}
async function createClient() {
    const wasmBinary = await wasmBinaryPromise;
    const workerUrl = GEOMETER_STANDALONE_BUILD
        ? URL.createObjectURL(new Blob([new TextDecoder().decode(decodeBase64(embeddedRuntime.workerBase64))], { type: "text/javascript" }))
        : "./pcb_polygon_pour_worker.js";
    const worker = new Worker(workerUrl, { name: "geometer-pcb-polygon-pour-a0" });
    if (GEOMETER_STANDALONE_BUILD)
        URL.revokeObjectURL(workerUrl);
    try {
        await waitForWorkerBootstrap(worker);
        return await createGeometerWorkerClient(worker, { wasmBinary });
    }
    catch (error) {
        worker.terminate();
        throw error;
    }
}
function scheduleSolve(state) {
    if (terminalFailure)
        return;
    if (solving && pendingState !== undefined)
        replacedSolves += 1;
    pendingState = state;
    updateCounters();
    if (!solving)
        void drainLatest();
}
async function drainLatest() {
    solving = true;
    shell.dataset.state = "solving";
    try {
        const client = await clientPromise;
        activeClient = client;
        setText("runtime-label", `Worker ready · ${client.capabilities.releaseVersion}`);
        while (pendingState !== undefined) {
            const requestedState = pendingState;
            pendingState = undefined;
            const started = performance.now();
            const result = await withSolveDeadline(client.analyticPlanarBooleanBatch(makePcbPolygonPourRequest(requestedState)), client);
            const elapsed = performance.now() - started;
            const failures = result.job_results.filter((candidate) => candidate.status !== "success");
            if (failures.length > 0)
                throw new Error(failures.flatMap((failure) => failure.diagnostics.map((diagnostic) => diagnostic.code)).join(", "));
            const jobs = result.job_results.filter((candidate) => candidate.status === "success");
            const job = jobs.find((candidate) => candidate.job_id === 31n);
            if (job === undefined)
                throw new Error("Solver returned no PCB pour job.");
            if (pendingState !== undefined)
                continue;
            const compositeDigest = await digestLayerClosure(jobs);
            if (pendingState !== undefined)
                continue;
            latestResults = jobs;
            latestDigest = compositeDigest;
            completedSolves += 1;
            setText("latency", `${elapsed.toFixed(1)} ms`);
            setText("regions", String(job.result_regions.length));
            const fragments = jobs.reduce((sum, candidate) => sum + candidate.directed_fragments.length, 0);
            const regions = jobs.reduce((sum, candidate) => sum + candidate.result_regions.length, 0);
            setText("fragments", String(fragments));
            setText("digest", latestDigest);
            setText("status", `${elapsed.toFixed(1)} ms · ${regions} exact layer regions · ${fragments} exact fragments`);
            updateCounters();
            render();
        }
        shell.dataset.state = "ready";
    }
    catch (error) {
        terminalFailure = true;
        pendingState = undefined;
        activeClient?.terminate();
        activeClient = undefined;
        shell.dataset.state = "error";
        setText("runtime-label", "Kernel error");
        setText("status", error instanceof Error ? error.message : String(error));
    }
    finally {
        solving = false;
        if (!terminalFailure && pendingState !== undefined)
            void drainLatest();
    }
}
function render() {
    const ratio = Math.min(window.devicePixelRatio || 1, 2);
    const width = canvas.clientWidth;
    const height = canvas.clientHeight;
    if (canvas.width !== Math.round(width * ratio) || canvas.height !== Math.round(height * ratio)) {
        canvas.width = Math.round(width * ratio);
        canvas.height = Math.round(height * ratio);
    }
    context.setTransform(ratio, 0, 0, ratio, 0, 0);
    context.clearRect(0, 0, width, height);
    drawGrid(width, height, ratio);
    drawComposedCopper(width, height, ratio);
    context.save();
    const scale = camera.pixelsPerWorldUnit;
    const center = camera.center;
    context.setTransform(scale * ratio, 0, 0, -scale * ratio, (width / 2 - center.x * scale) * ratio, (height / 2 + center.y * scale) * ratio);
    context.lineCap = "round";
    context.lineJoin = "round";
    drawFootprints(scale);
    drawTraces(history.state);
    drawVias(history.state, scale);
    drawBoardHandles(history.state, scale);
    drawRoutePreview(scale);
    context.restore();
}
function drawComposedCopper(width, height, ratio) {
    const pixelWidth = Math.round(width * ratio);
    const pixelHeight = Math.round(height * ratio);
    if (copperCanvas.width !== pixelWidth || copperCanvas.height !== pixelHeight) {
        copperCanvas.width = pixelWidth;
        copperCanvas.height = pixelHeight;
    }
    copperContext.setTransform(1, 0, 0, 1, 0, 0);
    copperContext.clearRect(0, 0, pixelWidth, pixelHeight);
    const scale = camera.pixelsPerWorldUnit;
    const center = camera.center;
    copperContext.setTransform(scale * ratio, 0, 0, -scale * ratio, (width / 2 - center.x * scale) * ratio, (height / 2 + center.y * scale) * ratio);
    const base = latestResults.find((candidate) => classifyPcbLayerJob(candidate.job_id) === "base");
    if (base !== undefined)
        drawSolvedCopperFill(copperContext, base, themeColor("--pcb-copper-fill"));
    copperContext.globalCompositeOperation = "destination-out";
    for (const result of latestResults.filter((candidate) => classifyPcbLayerJob(candidate.job_id) === "clearance"))
        drawSolvedCopperFill(copperContext, result, themeColor("--pcb-composite-mask"));
    copperContext.globalCompositeOperation = "source-over";
    for (const result of latestResults.filter((candidate) => classifyPcbLayerJob(candidate.job_id) === "thermal"))
        drawSolvedCopperFill(copperContext, result, themeColor("--pcb-thermal-fill"));
    copperContext.globalCompositeOperation = "destination-in";
    if (base !== undefined)
        drawSolvedCopperFill(copperContext, base, themeColor("--pcb-composite-mask"));
    copperContext.globalCompositeOperation = "source-over";
    context.save();
    context.setTransform(ratio, 0, 0, ratio, 0, 0);
    context.drawImage(copperCanvas, 0, 0, width, height);
    context.restore();
}
function drawSolvedCopperFill(target, job, fillStyle) {
    const vertices = new Map(job.vertices.map((vertex) => [vertex.vertex_id, vertex]));
    const fragments = new Map(job.directed_fragments.map((fragment) => [fragment.fragment_id, fragment]));
    target.beginPath();
    for (const ring of job.rings)
        appendRing(target, ring.fragment_ids, vertices, fragments);
    target.fillStyle = fillStyle;
    target.fill("evenodd");
}
function drawFootprints(scale) {
    context.strokeStyle = themeColor("--pcb-footprint");
    context.fillStyle = themeColor("--pcb-pad-fill");
    context.lineWidth = 1 / scale;
    for (const [index, point] of footprintPads().entries()) {
        context.beginPath();
        context.arc(point.x, point.y, 0.32, 0, Math.PI * 2);
        context.fill();
        context.stroke();
        if (index % 3 === 0) {
            context.strokeRect(point.x - 0.75, point.y - 0.65, 1.5, 3.7);
        }
    }
}
function drawTraces(state) {
    context.strokeStyle = themeColor("--pcb-clearance");
    context.lineWidth = (TRACE_WIDTH_NM + CLEARANCE_NM * 2) / NM_PER_MM;
    for (const trace of state.traces) {
        context.beginPath();
        trace.points.forEach((point, index) => index === 0 ? context.moveTo(point.x, point.y) : context.lineTo(point.x, point.y));
        context.stroke();
    }
    context.strokeStyle = themeColor("--pcb-trace");
    context.lineWidth = TRACE_WIDTH_NM / NM_PER_MM;
    for (const trace of state.traces) {
        context.beginPath();
        trace.points.forEach((point, index) => index === 0 ? context.moveTo(point.x, point.y) : context.lineTo(point.x, point.y));
        context.stroke();
    }
}
function drawVias(state, scale) {
    for (const via of state.vias) {
        context.beginPath();
        context.arc(via.x, via.y, VIA_COPPER_RADIUS_NM / NM_PER_MM, 0, Math.PI * 2);
        context.fillStyle = themeColor("--pcb-via-fill");
        context.fill();
        context.strokeStyle = themeColor("--pcb-via-edge");
        context.lineWidth = 1.5 / scale;
        context.stroke();
        context.beginPath();
        context.arc(via.x, via.y, 0.14, 0, Math.PI * 2);
        context.fillStyle = themeColor("--pcb-drill");
        context.fill();
    }
}
function drawBoardHandles(state, scale) {
    context.fillStyle = themeColor("--pcb-handle");
    context.strokeStyle = themeColor("--pcb-handle-edge");
    context.lineWidth = 1 / scale;
    for (const point of state.board) {
        context.beginPath();
        context.rect(point.x - 4 / scale, point.y - 4 / scale, 8 / scale, 8 / scale);
        context.fill();
        context.stroke();
    }
}
function drawRoutePreview(scale) {
    if (routePoints.length === 0)
        return;
    const points = routePreview === undefined ? routePoints : [...routePoints, routePreview];
    context.beginPath();
    points.forEach((point, index) => index === 0 ? context.moveTo(point.x, point.y) : context.lineTo(point.x, point.y));
    context.strokeStyle = themeColor("--pcb-clearance");
    context.lineWidth = (TRACE_WIDTH_NM + CLEARANCE_NM * 2) / NM_PER_MM;
    context.stroke();
    context.beginPath();
    points.forEach((point, index) => index === 0 ? context.moveTo(point.x, point.y) : context.lineTo(point.x, point.y));
    context.strokeStyle = themeColor("--pcb-route-preview");
    context.lineWidth = TRACE_WIDTH_NM / NM_PER_MM;
    context.stroke();
    context.fillStyle = themeColor("--pcb-route-preview");
    for (const point of routePoints) {
        context.beginPath();
        context.arc(point.x, point.y, 3.5 / scale, 0, Math.PI * 2);
        context.fill();
    }
}
function appendRing(target, fragmentIds, vertices, fragments) {
    const first = fragmentIds[0] === undefined ? undefined : fragments.get(fragmentIds[0]);
    if (first === undefined)
        return;
    const start = vertices.get(first.start_vertex_id);
    if (start === undefined)
        return;
    target.moveTo(Number(start.point.x) / NM_PER_MM, Number(start.point.y) / NM_PER_MM);
    for (const fragmentId of fragmentIds) {
        const fragment = fragments.get(fragmentId);
        if (fragment !== undefined)
            appendFragment(target, fragment, vertices);
    }
    target.closePath();
}
function appendFragment(target, fragment, vertices) {
    const start = vertices.get(fragment.start_vertex_id);
    const end = vertices.get(fragment.end_vertex_id);
    if (start === undefined || end === undefined)
        return;
    const sx = Number(start.point.x) / NM_PER_MM;
    const sy = Number(start.point.y) / NM_PER_MM;
    const ex = Number(end.point.x) / NM_PER_MM;
    const ey = Number(end.point.y) / NM_PER_MM;
    if (fragment.kind === "line")
        target.lineTo(ex, ey);
    else
        appendArc(target, fragment, sx, sy, ex, ey);
}
function appendArc(target, fragment, sx, sy, ex, ey) {
    const arc = resolveReflectedCanvasArc(sx, sy, ex, ey, Number(fragment.radius_nm) / NM_PER_MM, fragment.direction, fragment.major_arc);
    if (arc === undefined)
        target.lineTo(ex, ey);
    else
        target.arc(arc.centerX, arc.centerY, arc.radius, arc.startAngle, arc.endAngle, arc.counterclockwise);
}
function drawGrid(width, height, ratio) {
    const spacing = camera.gridScale(24, 5);
    const topLeft = camera.screenToWorld({ x: 0, y: 0 });
    const bottomRight = camera.screenToWorld({ x: width, y: height });
    context.save();
    context.setTransform(ratio, 0, 0, ratio, 0, 0);
    for (let x = Math.floor(topLeft.x / spacing.minorWorldSpacing) * spacing.minorWorldSpacing; x <= bottomRight.x; x += spacing.minorWorldSpacing) {
        const screen = camera.worldToScreen({ x, y: 0 });
        const major = Math.round(x / spacing.minorWorldSpacing) % spacing.majorEvery === 0;
        context.strokeStyle = themeColor(major ? "--pcb-grid-major" : "--pcb-grid-minor");
        context.beginPath();
        context.moveTo(screen.x, 0);
        context.lineTo(screen.x, height);
        context.stroke();
    }
    for (let y = Math.floor(bottomRight.y / spacing.minorWorldSpacing) * spacing.minorWorldSpacing; y <= topLeft.y; y += spacing.minorWorldSpacing) {
        const screen = camera.worldToScreen({ x: 0, y });
        const major = Math.round(y / spacing.minorWorldSpacing) % spacing.majorEvery === 0;
        context.strokeStyle = themeColor(major ? "--pcb-grid-major" : "--pcb-grid-minor");
        context.beginPath();
        context.moveTo(0, screen.y);
        context.lineTo(width, screen.y);
        context.stroke();
    }
    context.restore();
}
function updateCounters() {
    const state = history.state;
    setText("object-count", `${state.vias.length} / ${state.traces.length}`);
    setText("solve-count", `${completedSolves} / ${replacedSolves}`);
}
async function digestLayerClosure(jobs) {
    const closure = sortedLayerRecords(jobs)
        .map((job) => `${job.jobId}\0${job.digest}\n`)
        .join("");
    const digest = await crypto.subtle.digest("SHA-256", new TextEncoder().encode(closure));
    return [...new Uint8Array(digest)].map((value) => value.toString(16).padStart(2, "0")).join("");
}
function sortedLayerRecords(jobs) {
    return [...jobs]
        .sort((left, right) => left.job_id < right.job_id ? -1 : left.job_id > right.job_id ? 1 : 0)
        .map((job) => ({ digest: job.digest_sha256, jobId: job.job_id.toString() }));
}
async function withSolveDeadline(operation, client) {
    let timer;
    const timeout = new Promise((_, reject) => {
        timer = window.setTimeout(() => {
            reject(new Error("Solver exceeded the 15 second deadline; the Worker was terminated."));
            client.terminate();
        }, SOLVE_TIMEOUT_MS);
    });
    try {
        return await Promise.race([operation, timeout]);
    }
    finally {
        if (timer !== undefined)
            window.clearTimeout(timer);
    }
}
function resize() {
    const width = Math.max(canvas.clientWidth, 1);
    const height = Math.max(canvas.clientHeight, 1);
    camera.setViewport({ width, height });
    if (!cameraFitted) {
        camera.fit({ min: { x: -1, y: -1 }, max: { x: 27, y: 16 } }, 50);
        cameraFitted = true;
    }
    render();
}
function snapGrid(point) {
    const snapped = camera.snapWorldToGrid(point, 0.05);
    return { x: snapped.x, y: snapped.y };
}
function samePoint(left, right) {
    return left !== undefined && left.x === right.x && left.y === right.y;
}
function replaceState(label, reducer) {
    return { label, apply: reducer, revert: (state) => state };
}
function themeColor(name) {
    const value = getComputedStyle(shell).getPropertyValue(name).trim();
    if (!value)
        throw new Error(`The shared demo theme omits ${name}.`);
    return value;
}
function waitForWorkerBootstrap(worker) {
    return new Promise((resolve, reject) => {
        const timeout = window.setTimeout(() => {
            cleanup();
            reject(new Error("Worker bootstrap timed out."));
        }, 30_000);
        const onMessage = (event) => {
            if (!isRecord(event.data) || event.data.protocol !== "wn.geometer.worker_bootstrap.a0")
                return;
            cleanup();
            if (event.data.kind === "ready")
                resolve();
            else
                reject(new Error(String(event.data.message || "Worker bootstrap failed.")));
        };
        const onError = (event) => {
            cleanup();
            reject(new Error(event.message || "Worker bootstrap failed."));
        };
        const cleanup = () => {
            window.clearTimeout(timeout);
            worker.removeEventListener("message", onMessage);
            worker.removeEventListener("error", onError);
        };
        worker.addEventListener("message", onMessage);
        worker.addEventListener("error", onError);
    });
}
function requireElement(id) {
    const element = document.getElementById(id);
    if (element === null)
        throw new Error(`Missing #${id}.`);
    return element;
}
function requireCanvasContext(element) {
    const result = element.getContext("2d");
    if (result === null)
        throw new Error("Canvas 2D is unavailable.");
    return result;
}
function setText(id, value) {
    requireElement(id).textContent = value;
}
function isRecord(value) {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}
function readEmbeddedRuntime() {
    const wasm = document.getElementById("geometer-pcb-wasm");
    const worker = document.getElementById("geometer-pcb-worker");
    if (wasm === null && worker === null)
        return undefined;
    if (wasm?.dataset.encoding !== "base64" || worker?.dataset.encoding !== "base64")
        throw new Error("Embedded Geometer runtime carriers are malformed.");
    const wasmBase64 = wasm.textContent?.trim();
    const workerBase64 = worker.textContent?.trim();
    if (!wasmBase64 || !workerBase64)
        throw new Error("Embedded Geometer runtime carriers are empty.");
    return { wasmBase64, workerBase64 };
}
function decodeBase64(value) {
    const binary = atob(value);
    const bytes = new Uint8Array(binary.length);
    for (let index = 0; index < binary.length; index += 1)
        bytes[index] = binary.charCodeAt(index);
    return bytes;
}
