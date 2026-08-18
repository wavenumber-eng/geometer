import { createGeometerWorkerClient } from "@wavenumber/geometer/worker";
import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";
const fixture = {
    displayModel: "/tests/fixtures/glb/embedded_models/SOT-23.glb",
    stepModel: "/tests/fixtures/step/embedded_models/SOT-23.STEP",
};
const root = requireElement("three_d_viz-root");
const viewport = requireElement("viewport-shell");
const canvas = requireElement("model-canvas");
const runButton = requireElement("run-bounds");
const resetButton = requireElement("reset-camera");
const modelToggle = requireElement("show-model");
const boundsToggle = requireElement("show-bounds");
const rotateToggle = requireElement("auto-rotate");
const status = requireElement("status");
const runtimeLabel = requireElement("runtime-label");
const loadingDetail = requireElement("loading-detail");
const theme = {
    accent: themeColor("--wn-accent"),
    axisX: themeColor("--bounds-axis-x"),
    axisY: themeColor("--bounds-axis-y"),
    axisZ: themeColor("--bounds-axis-z"),
    fillLight: themeColor("--bounds-light-fill"),
    gridMajor: themeColor("--bounds-grid-major"),
    gridMinor: themeColor("--bounds-grid-minor"),
    groundLight: themeColor("--bounds-light-ground"),
    modelDark: themeColor("--bounds-model-dark"),
    modelLight: themeColor("--bounds-model-light"),
    paper: themeColor("--wn-paper"),
    skyLight: themeColor("--bounds-light-sky"),
    viewport: themeColor("--bounds-viewport-bg"),
};
const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: false });
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.setClearColor(theme.viewport, 1);
renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
const scene = new THREE.Scene();
scene.background = new THREE.Color(theme.viewport);
scene.fog = new THREE.FogExp2(theme.viewport, 0.035);
const camera = new THREE.PerspectiveCamera(38, 1, 0.01, 10000);
camera.up.set(0, 0, 1);
const controls = new OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.06;
controls.screenSpacePanning = true;
controls.autoRotateSpeed = 1.25;
scene.add(new THREE.HemisphereLight(theme.skyLight, theme.groundLight, 2.35));
const keyLight = new THREE.DirectionalLight(theme.paper, 3.4);
keyLight.position.set(3.5, -4.5, 6);
scene.add(keyLight);
const fillLight = new THREE.DirectionalLight(theme.fillLight, 1.25);
fillLight.position.set(-4, 2, 1.5);
scene.add(fillLight);
const rimLight = new THREE.DirectionalLight(theme.accent, 0.7);
rimLight.position.set(1, 4, 2);
scene.add(rimLight);
const modelGroup = new THREE.Group();
const boundsGroup = new THREE.Group();
scene.add(modelGroup, boundsGroup);
let boundsResult;
let cameraHome;
const wasmBinaryPromise = fetchBytes("/dist/wasm/browser/geometer.wasm", "geometer.wasm").then(copyToArrayBuffer);
const stepModelPromise = fetchBytes(fixture.stepModel, "SOT-23 STEP fixture");
const displayModelPromise = loadDisplayModel();
const clientPromise = createWorkerClient();
let activeClient;
new ResizeObserver(resizeRenderer).observe(viewport);
runButton.addEventListener("click", () => void run());
resetButton.addEventListener("click", resetCamera);
modelToggle.addEventListener("change", () => {
    modelGroup.visible = modelToggle.checked;
});
boundsToggle.addEventListener("change", () => {
    boundsGroup.visible = boundsToggle.checked;
    setDimensionVisibility(boundsToggle.checked);
});
rotateToggle.addEventListener("change", () => {
    controls.autoRotate =
        rotateToggle.checked && !window.matchMedia("(prefers-reduced-motion: reduce)").matches;
});
window.addEventListener("pagehide", () => activeClient?.terminate(), { once: true });
resizeRenderer();
renderer.setAnimationLoop(renderFrame);
void run();
async function run() {
    runButton.disabled = true;
    root.dataset.state = "loading";
    runtimeLabel.textContent = "Starting kernel";
    status.textContent = "Preparing generated client, STEP source, and display mesh…";
    loadingDetail.textContent = "Loading browser kernel and STEP fixture";
    try {
        const [client, model, displayModel] = await Promise.all([
            clientPromise,
            stepModelPromise,
            displayModelPromise,
        ]);
        activeClient = client;
        installDisplayModel(displayModel);
        loadingDetail.textContent = "Executing geometry.model_bounds.a0";
        status.textContent = `Computing in a Worker through ${client.capabilities.genericAbi.toUpperCase()} generic ABI…`;
        const result = await client.modelBounds({ model });
        renderBounds(result);
        runtimeLabel.textContent = `Ready · ${client.capabilities.releaseVersion}`;
        requireElement("kernel-version").textContent = client.capabilities.releaseVersion;
        status.textContent = `Complete · geometry.model_bounds.a0 · ${result.source.hash.slice(0, 12)}…`;
        root.dataset.state = "complete";
    }
    catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        runtimeLabel.textContent = "Geometry failed";
        status.textContent = message;
        loadingDetail.textContent = message;
        root.dataset.state = "error";
    }
    finally {
        runButton.disabled = false;
    }
}
async function createWorkerClient() {
    const wasmBinary = await wasmBinaryPromise;
    const worker = new Worker("/dist/wasm/demos/model_bounds_worker.js", {
        name: "geometer-model-bounds-a0",
    });
    try {
        await waitForWorkerBootstrap(worker);
        return await createGeometerWorkerClient(worker, { wasmBinary });
    }
    catch (error) {
        worker.terminate();
        throw error;
    }
}
function waitForWorkerBootstrap(worker) {
    return new Promise((resolve, reject) => {
        const timeout = window.setTimeout(() => {
            cleanup();
            reject(new Error("Geometer Worker bootstrap timed out."));
        }, 15_000);
        const onMessage = (event) => {
            if (!isRecord(event.data) || event.data.protocol !== "wn.geometer.worker_bootstrap.a0")
                return;
            cleanup();
            if (event.data.kind === "ready")
                resolve();
            else
                reject(new Error(String(event.data.message || "Geometer Worker bootstrap failed.")));
        };
        const onError = (event) => {
            cleanup();
            reject(new Error(event.message || "Geometer Worker bootstrap failed."));
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
function renderBounds(result) {
    boundsResult = result;
    const vectors = {
        min: result.bounds.min,
        max: result.bounds.max,
        center: result.bounds.center,
    };
    for (const [name, vector] of Object.entries(vectors)) {
        requireElement(`value-${name}`).textContent = formatVector(vector);
    }
    const axes = [
        {
            axis: "x",
            extent: result.bounds.size[0],
            minimum: result.bounds.min[0],
            maximum: result.bounds.max[0],
        },
        {
            axis: "y",
            extent: result.bounds.size[1],
            minimum: result.bounds.min[1],
            maximum: result.bounds.max[1],
        },
        {
            axis: "z",
            extent: result.bounds.size[2],
            minimum: result.bounds.min[2],
            maximum: result.bounds.max[2],
        },
    ];
    for (const { axis, extent, minimum, maximum } of axes) {
        requireElement(`axis-${axis}`).textContent = extent.toFixed(3);
        requireElement(`range-${axis}`).textContent =
            `${minimum.toFixed(3)} → ${maximum.toFixed(3)}`;
        requireElement(`dimension-${axis}`).textContent =
            `${axis.toUpperCase()}  ${extent.toFixed(3)} mm`;
    }
    requireElement("source-hash").textContent = result.source.hash;
    requireElement("timing").textContent =
        `${(result.timings.model_read_ms + result.timings.bounds_ms).toFixed(2)} ms`;
    alignDisplayModel(result);
    buildBoundsGraphics(result);
    fitCamera(result);
}
function alignDisplayModel(result) {
    modelGroup.position.set(0, 0, 0);
    modelGroup.scale.setScalar(1);
    modelGroup.updateMatrixWorld(true);
    const displayBox = new THREE.Box3().setFromObject(modelGroup);
    const displaySize = displayBox.getSize(new THREE.Vector3());
    const ratios = [
        result.bounds.size[0] / displaySize.x,
        result.bounds.size[1] / displaySize.y,
        result.bounds.size[2] / displaySize.z,
    ].filter((value) => Number.isFinite(value) && value > 0);
    ratios.sort((left, right) => left - right);
    const scale = ratios[Math.floor(ratios.length / 2)] ?? 1;
    modelGroup.scale.setScalar(scale);
    modelGroup.updateMatrixWorld(true);
    const scaledCenter = new THREE.Box3().setFromObject(modelGroup).getCenter(new THREE.Vector3());
    modelGroup.position.copy(new THREE.Vector3(...result.bounds.center).sub(scaledCenter));
    modelGroup.updateMatrixWorld(true);
}
function buildBoundsGraphics(result) {
    boundsGroup.clear();
    const minimum = new THREE.Vector3(...result.bounds.min);
    const maximum = new THREE.Vector3(...result.bounds.max);
    const size = new THREE.Vector3(...result.bounds.size);
    const center = new THREE.Vector3(...result.bounds.center);
    const geometry = new THREE.BoxGeometry(size.x, size.y, size.z);
    const volume = new THREE.Mesh(geometry, new THREE.MeshBasicMaterial({
        color: theme.accent,
        depthWrite: false,
        opacity: 0.055,
        side: THREE.DoubleSide,
        transparent: true,
    }));
    volume.position.copy(center);
    volume.renderOrder = 2;
    boundsGroup.add(volume);
    const edges = new THREE.LineSegments(new THREE.EdgesGeometry(geometry), new THREE.LineBasicMaterial({
        color: theme.accent,
        depthTest: false,
        transparent: true,
        opacity: 0.96,
    }));
    edges.position.copy(center);
    edges.renderOrder = 4;
    boundsGroup.add(edges);
    const span = Math.max(size.x, size.y, size.z);
    const offset = Math.max(span * 0.12, 0.18);
    addDimensionLine(new THREE.Vector3(minimum.x, minimum.y - offset, minimum.z), new THREE.Vector3(maximum.x, minimum.y - offset, minimum.z), theme.axisX, offset * 0.28);
    addDimensionLine(new THREE.Vector3(minimum.x - offset, minimum.y, minimum.z), new THREE.Vector3(minimum.x - offset, maximum.y, minimum.z), theme.axisY, offset * 0.28);
    addDimensionLine(new THREE.Vector3(maximum.x + offset, maximum.y, minimum.z), new THREE.Vector3(maximum.x + offset, maximum.y, maximum.z), theme.axisZ, offset * 0.28);
    const gridSize = Math.max(span * 2.6, 6);
    const grid = new THREE.GridHelper(gridSize, 16, theme.gridMajor, theme.gridMinor);
    grid.rotation.x = Math.PI / 2;
    grid.position.set(center.x, center.y, minimum.z - Math.max(span * 0.02, 0.025));
    grid.material.transparent = true;
    grid.material.opacity = 0.42;
    boundsGroup.add(grid);
    boundsGroup.visible = boundsToggle.checked;
}
function addDimensionLine(start, end, color, tickLength) {
    const direction = end.clone().sub(start).normalize();
    const reference = Math.abs(direction.z) > 0.8 ? new THREE.Vector3(1, 0, 0) : new THREE.Vector3(0, 0, 1);
    const tick = new THREE.Vector3()
        .crossVectors(direction, reference)
        .normalize()
        .multiplyScalar(tickLength);
    const points = [
        start,
        end,
        start.clone().sub(tick),
        start.clone().add(tick),
        end.clone().sub(tick),
        end.clone().add(tick),
    ];
    const line = new THREE.LineSegments(new THREE.BufferGeometry().setFromPoints(points), new THREE.LineBasicMaterial({ color, depthTest: false, transparent: true, opacity: 0.95 }));
    line.renderOrder = 5;
    boundsGroup.add(line);
}
async function loadDisplayModel() {
    const loader = new GLTFLoader();
    const gltf = await loader.loadAsync(fixture.displayModel);
    const displayMaterials = new Map();
    gltf.scene.traverse((child) => {
        if (!(child instanceof THREE.Mesh))
            return;
        child.castShadow = true;
        child.receiveShadow = true;
        const materials = Array.isArray(child.material) ? child.material : [child.material];
        const styled = materials.map((material) => {
            const existing = displayMaterials.get(material);
            if (existing)
                return existing;
            if (!(material instanceof THREE.MeshStandardMaterial))
                return material;
            const clone = material.clone();
            const isDarkSurface = material.name === "mat_1";
            clone.color.set(isDarkSurface ? theme.modelDark : theme.modelLight);
            clone.metalness = isDarkSurface ? 0.08 : 0.72;
            clone.roughness = isDarkSurface ? 0.74 : 0.34;
            displayMaterials.set(material, clone);
            return clone;
        });
        child.material = Array.isArray(child.material) ? styled : (styled[0] ?? child.material);
    });
    return gltf.scene;
}
function installDisplayModel(displayModel) {
    if (modelGroup.children.length === 0)
        modelGroup.add(displayModel);
    modelGroup.visible = modelToggle.checked;
}
function fitCamera(result) {
    const target = new THREE.Vector3(...result.bounds.center);
    const size = new THREE.Vector3(...result.bounds.size);
    const radius = Math.max(size.length() * 0.5, 0.5);
    const position = target
        .clone()
        .add(new THREE.Vector3(1.45, -1.9, 1.25).normalize().multiplyScalar(radius * 3.75));
    camera.near = Math.max(radius / 1000, 0.001);
    camera.far = Math.max(radius * 100, 100);
    camera.position.copy(position);
    camera.updateProjectionMatrix();
    controls.target.copy(target);
    controls.minDistance = radius * 0.35;
    controls.maxDistance = radius * 15;
    controls.update();
    cameraHome = { position: position.clone(), target: target.clone() };
}
function resetCamera() {
    if (!cameraHome)
        return;
    camera.position.copy(cameraHome.position);
    controls.target.copy(cameraHome.target);
    controls.update();
}
function renderFrame() {
    controls.update();
    updateDimensionLabels();
    renderer.render(scene, camera);
}
function updateDimensionLabels() {
    if (!boundsResult || !boundsGroup.visible)
        return;
    const minimum = new THREE.Vector3(...boundsResult.bounds.min);
    const maximum = new THREE.Vector3(...boundsResult.bounds.max);
    const span = Math.max(...boundsResult.bounds.size);
    const offset = Math.max(span * 0.12, 0.18);
    positionLabel("dimension-x", new THREE.Vector3((minimum.x + maximum.x) / 2, minimum.y - offset, minimum.z));
    positionLabel("dimension-y", new THREE.Vector3(minimum.x - offset, (minimum.y + maximum.y) / 2, minimum.z));
    positionLabel("dimension-z", new THREE.Vector3(maximum.x + offset, maximum.y, (minimum.z + maximum.z) / 2));
}
function positionLabel(id, point) {
    const projected = point.clone().project(camera);
    const element = requireElement(id);
    element.style.left = `${(projected.x * 0.5 + 0.5) * viewport.clientWidth}px`;
    element.style.top = `${(-projected.y * 0.5 + 0.5) * viewport.clientHeight}px`;
}
function setDimensionVisibility(visible) {
    for (const axis of ["x", "y", "z"]) {
        requireElement(`dimension-${axis}`).style.display = visible ? "block" : "none";
    }
}
function resizeRenderer() {
    const width = Math.max(viewport.clientWidth, 1);
    const height = Math.max(viewport.clientHeight, 1);
    renderer.setSize(width, height, false);
    camera.aspect = width / height;
    camera.updateProjectionMatrix();
}
async function fetchBytes(url, label) {
    const response = await fetch(url);
    if (!response.ok)
        throw new Error(`Unable to load ${label} (${response.status}).`);
    return new Uint8Array(await response.arrayBuffer());
}
function formatVector(vector) {
    return vector.map((value) => value.toFixed(3)).join("  ");
}
function copyToArrayBuffer(bytes) {
    const copy = new Uint8Array(bytes.byteLength);
    copy.set(bytes);
    return copy.buffer;
}
function isRecord(value) {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}
function themeColor(property) {
    const value = getComputedStyle(root).getPropertyValue(property).trim();
    if (!value)
        throw new Error(`Shared demo theme is missing ${property}.`);
    return value;
}
function requireElement(id) {
    const element = document.getElementById(id);
    if (!element)
        throw new Error(`Missing demo element #${id}.`);
    return element;
}
