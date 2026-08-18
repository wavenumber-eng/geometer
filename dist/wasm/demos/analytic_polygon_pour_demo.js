import { createGeometerWorkerClient } from "@wavenumber/geometer/worker";
import { resolveReflectedCanvasArc } from "./analytic_canvas_arc.js";
import { makeAnalyticPolygonPourRequest } from "./analytic_polygon_pour_fixture.js";
const NM_PER_MM = 1_000_000;
const canvas = requireElement("pour-canvas");
const shell = requireElement("pour-shell");
const status = requireElement("status");
const runtime = requireElement("runtime-label");
const slotSlider = requireElement("slot-position");
const slotValue = requireElement("slot-value");
const resetButton = requireElement("reset-demo");
const context = requireCanvasContext(canvas);
let activeClient;
let latestResult;
let running = false;
let pendingPosition;
let completedSolves = 0;
let replacedSolves = 0;
const embeddedRuntime = readEmbeddedRuntime();
if (GEOMETER_STANDALONE_BUILD && embeddedRuntime === undefined)
    throw new Error("Standalone Geometer runtime carriers are missing.");
const wasmBinaryPromise = GEOMETER_STANDALONE_BUILD
    ? Promise.resolve(decodeBase64(embeddedRuntime.wasmBase64).buffer)
    : fetch("/dist/wasm/browser/geometer.wasm").then((response) => {
        if (!response.ok)
            throw new Error(`geometer.wasm returned HTTP ${response.status}.`);
        return response.arrayBuffer();
    });
const clientPromise = createClient();
new ResizeObserver(resize).observe(canvas.parentElement ?? canvas);
slotSlider.addEventListener("input", () => {
    const position = Number(slotSlider.value);
    slotValue.textContent = `${position.toFixed(2)} mm`;
    scheduleSolve(position);
});
resetButton.addEventListener("click", () => {
    slotSlider.value = "10";
    slotValue.textContent = "10.00 mm";
    scheduleSolve(10);
});
window.addEventListener("pagehide", () => activeClient?.terminate(), { once: true });
resize();
scheduleSolve(Number(slotSlider.value));
async function createClient() {
    const wasmBinary = await wasmBinaryPromise;
    const workerUrl = GEOMETER_STANDALONE_BUILD
        ? URL.createObjectURL(new Blob([new TextDecoder().decode(decodeBase64(embeddedRuntime.workerBase64))], {
            type: "text/javascript",
        }))
        : "/dist/wasm/demos/analytic_polygon_pour_worker.js";
    const worker = new Worker(workerUrl, {
        name: "geometer-analytic-polygon-pour-a0",
    });
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
function scheduleSolve(positionMm) {
    if (running && pendingPosition !== undefined)
        replacedSolves += 1;
    pendingPosition = positionMm;
    if (running) {
        updateCounters();
        return;
    }
    void drainLatest();
}
async function drainLatest() {
    running = true;
    shell.dataset.state = "solving";
    try {
        const client = await clientPromise;
        activeClient = client;
        runtime.textContent = `Worker ready · ${client.capabilities.releaseVersion}`;
        while (pendingPosition !== undefined) {
            const position = pendingPosition;
            pendingPosition = undefined;
            const started = performance.now();
            const result = await client.analyticPlanarBooleanBatch(makeAnalyticPolygonPourRequest(position));
            const elapsed = performance.now() - started;
            const job = result.job_results.find((item) => item.job_id === 7n);
            const failureProbe = result.job_results.find((item) => item.job_id === 8n);
            if (job === undefined || job.status !== "success") {
                throw new Error(job?.diagnostics.map((item) => item.code).join(", ") || "Solver returned no job.");
            }
            if (failureProbe?.status !== "failure" ||
                !failureProbe.diagnostics.some((diagnostic) => diagnostic.code === "geometer.operation.analytic_planar_boolean.invalid_topology"))
                throw new Error("The governed self-crossing path probe did not return invalid_topology.");
            if (pendingPosition !== undefined)
                continue;
            latestResult = job;
            completedSolves += 1;
            render(job);
            updateMetrics(job, failureProbe, elapsed);
            status.textContent =
                `${elapsed.toFixed(1)} ms · ${job.result_regions.length} regions · ` +
                    `${job.directed_fragments.length} exact boundary fragments`;
        }
        shell.dataset.state = "ready";
    }
    catch (error) {
        shell.dataset.state = "error";
        const message = error instanceof Error ? error.message : String(error);
        runtime.textContent = "Kernel error";
        status.textContent = message;
    }
    finally {
        running = false;
        if (pendingPosition !== undefined)
            void drainLatest();
    }
}
function render(job) {
    const ratio = Math.min(window.devicePixelRatio || 1, 2);
    const width = canvas.clientWidth;
    const height = canvas.clientHeight;
    canvas.width = Math.round(width * ratio);
    canvas.height = Math.round(height * ratio);
    context.setTransform(ratio, 0, 0, ratio, 0, 0);
    context.clearRect(0, 0, width, height);
    const margin = 46;
    const scale = Math.min((width - margin * 2) / 22_000_000, (height - margin * 2) / 12_000_000);
    const originX = (width - 20_000_000 * scale) / 2;
    const originY = (height + 10_000_000 * scale) / 2;
    drawGrid(width, height, scale, originX, originY);
    context.save();
    context.setTransform(scale * ratio, 0, 0, -scale * ratio, originX * ratio, originY * ratio);
    context.lineJoin = "round";
    context.lineCap = "round";
    const vertices = new Map(job.vertices.map((vertex) => [vertex.vertex_id, vertex]));
    const fragments = new Map(job.directed_fragments.map((fragment) => [fragment.fragment_id, fragment]));
    context.beginPath();
    for (const ring of job.rings)
        appendRing(ring.fragment_ids, vertices, fragments);
    context.fillStyle = themeColor("--wn-geometry-result-fill");
    context.fill("evenodd");
    for (const fragment of job.directed_fragments) {
        context.beginPath();
        appendFragment(fragment, vertices, true);
        const subtractive = fragment.surviving_subtraction_sources.sources.length > 0;
        context.strokeStyle = themeColor(subtractive ? "--wn-geometry-subtract" : "--wn-geometry-result");
        context.lineWidth = (subtractive ? 2.8 : 1.7) / scale;
        context.stroke();
    }
    context.restore();
}
function appendRing(fragmentIds, vertices, fragments) {
    const first = fragmentIds[0] === undefined ? undefined : fragments.get(fragmentIds[0]);
    if (first === undefined)
        return;
    const start = vertices.get(first.start_vertex_id);
    if (start === undefined)
        return;
    context.moveTo(Number(start.point.x), Number(start.point.y));
    for (const fragmentId of fragmentIds) {
        const fragment = fragments.get(fragmentId);
        if (fragment !== undefined)
            appendFragment(fragment, vertices, false);
    }
    context.closePath();
}
function appendFragment(fragment, vertices, move) {
    const start = vertices.get(fragment.start_vertex_id);
    const end = vertices.get(fragment.end_vertex_id);
    if (start === undefined || end === undefined)
        return;
    const sx = Number(start.point.x);
    const sy = Number(start.point.y);
    const ex = Number(end.point.x);
    const ey = Number(end.point.y);
    if (move)
        context.moveTo(sx, sy);
    if (fragment.kind === "line")
        context.lineTo(ex, ey);
    else
        appendArc(fragment, sx, sy, ex, ey);
}
function appendArc(fragment, sx, sy, ex, ey) {
    const arc = resolveReflectedCanvasArc(sx, sy, ex, ey, Number(fragment.radius_nm), fragment.direction, fragment.major_arc);
    if (arc === undefined) {
        context.lineTo(ex, ey);
        return;
    }
    context.arc(arc.centerX, arc.centerY, arc.radius, arc.startAngle, arc.endAngle, arc.counterclockwise);
}
function drawGrid(width, height, scale, originX, originY) {
    context.save();
    context.strokeStyle = themeColor("--wn-geometry-grid");
    context.lineWidth = 1;
    for (let mm = -2; mm <= 22; mm += 1) {
        const x = originX + mm * NM_PER_MM * scale;
        context.beginPath();
        context.moveTo(x, 0);
        context.lineTo(x, height);
        context.stroke();
    }
    for (let mm = -1; mm <= 11; mm += 1) {
        const y = originY - mm * NM_PER_MM * scale;
        context.beginPath();
        context.moveTo(0, y);
        context.lineTo(width, y);
        context.stroke();
    }
    context.restore();
}
function updateMetrics(job, failureProbe, elapsed) {
    setText("latency", `${elapsed.toFixed(1)} ms`);
    setText("regions", String(job.result_regions.length));
    setText("rings", String(job.rings.length));
    setText("fragments", String(job.directed_fragments.length));
    setText("digest", `${job.digest_sha256.slice(0, 16)}…`);
    updateCounters();
    const subtractionEdges = job.directed_fragments.filter((fragment) => fragment.surviving_subtraction_sources.sources.length > 0).length;
    setText("subtraction-edges", String(subtractionEdges));
    const sourceKeys = new Set();
    for (const fragment of job.directed_fragments)
        for (const source of [
            ...fragment.coincident_positive_sources.sources,
            ...fragment.surviving_subtraction_sources.sources,
        ])
            sourceKeys.add(`${source.kind}/${source.role}/${source.operand_id}/${source.primary_id}/${source.secondary_id}`);
    setText("source-incidence", `${sourceKeys.size} canonical boundary source identities`);
    const lineage = requireElement("lineage-events");
    lineage.replaceChildren(...job.operand_outcomes.slice(0, 6).map((event) => {
        const item = document.createElement("li");
        item.textContent = `operand ${event.operand_id} · ${event.kind.replaceAll("_", " ")}`;
        return item;
    }));
    const diagnostic = failureProbe?.status === "failure" ? failureProbe.diagnostics[0] : undefined;
    setText("failure-code", diagnostic?.code ?? "failure probe invalid");
    setText("failure-path", diagnostic?.path_identity ?? "job-local diagnostic");
}
function updateCounters() {
    setText("completed", String(completedSolves));
    setText("replaced", String(replacedSolves));
}
function resize() {
    if (latestResult !== undefined)
        render(latestResult);
}
function setText(id, value) {
    requireElement(id).textContent = value;
}
function themeColor(name) {
    const value = getComputedStyle(document.documentElement).getPropertyValue(name).trim();
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
function isRecord(value) {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}
function readEmbeddedRuntime() {
    const wasm = document.getElementById("geometer-analytic-wasm");
    const worker = document.getElementById("geometer-analytic-worker");
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
