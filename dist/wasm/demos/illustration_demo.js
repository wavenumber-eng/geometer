import { prepareMeshIllustration, renderMeshIllustrationCanvas, renderMeshIllustrationSvg, toMeshIllustrationStyleA0, } from "@wavenumber/geometer/mesh-illustration";
import * as THREE from "three";
import { TrackballControls } from "three/addons/controls/TrackballControls.js";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";
const MESH_QUALITY_PRESETS = {
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
const DEMO_MODEL_ORDER = [
    "SOT-23.STEP",
    "SOIC-8-W.step",
    "sot223.stp",
    "Cap_SMT_Aluminum_F.STEP",
    "BGA90-8X13mm.step",
];
function required(id) {
    const value = document.getElementById(id);
    if (!(value instanceof HTMLElement))
        throw new Error(`Missing required element #${id}.`);
    return value;
}
const els = {
    app: required("illustrationApp"),
    stepInput: required("illustrationStepInput"),
    openButton: required("illustrationOpenButton"),
    modelSelect: required("illustrationModelSelect"),
    viewButtons: required("illustrationViewButtons"),
    outputButtons: required("illustrationOutputButtons"),
    downloadSvg: required("illustrationDownloadSvg"),
    downloadStyle: required("illustrationDownloadStyle"),
    modelPane: required("illustrationModelPane"),
    outputPane: required("illustrationOutputPane"),
    modelCanvas: required("illustrationModelCanvas"),
    illustrationCanvas: required("illustrationCanvas"),
    svgHost: required("illustrationSvgHost"),
    outputLabel: required("illustrationOutputLabel"),
    busy: required("illustrationBusy"),
    busyText: required("illustrationBusyText"),
    status: required("illustrationStatus"),
    timing: required("illustrationTiming"),
    counts: required("illustrationCounts"),
    validation: required("illustrationValidation"),
    meshQuality: required("illustrationMeshQuality"),
    linearDeflection: required("illustrationLinearDeflection"),
    angularDeflection: required("illustrationAngularDeflection"),
    hlrDeflection: required("illustrationHlrDeflection"),
    hlrAngularDeflection: required("illustrationHlrAngularDeflection"),
    meshInfo: required("illustrationMeshInfo"),
    shading: required("illustrationShading"),
    bands: required("illustrationBands"),
    bandsValue: required("illustrationBandsValue"),
    ambient: required("illustrationAmbient"),
    ambientValue: required("illustrationAmbientValue"),
    key: required("illustrationKey"),
    keyValue: required("illustrationKeyValue"),
    rim: required("illustrationRim"),
    rimValue: required("illustrationRimValue"),
    sourceColors: required("illustrationSourceColors"),
    fuseSurfaces: required("illustrationFuseSurfaces"),
    layerCoplanar: required("illustrationLayerCoplanar"),
    fallback: required("illustrationFallback"),
    hlrOutline: required("illustrationHlrOutline"),
    hlrDetail: required("illustrationHlrDetail"),
    outlineColor: required("illustrationOutlineColor"),
    outlineWidth: required("illustrationOutlineWidth"),
    outlineWidthValue: required("illustrationOutlineWidthValue"),
    doubleSided: required("illustrationDoubleSided"),
    fastCrease: required("illustrationFastCrease"),
    fastCreaseValue: required("illustrationFastCreaseValue"),
    fastCoplanarSeams: required("illustrationFastCoplanarSeams"),
    fastSeamAngle: required("illustrationFastSeamAngle"),
    fastSeamAngleValue: required("illustrationFastSeamAngleValue"),
    fastSeamDepth: required("illustrationFastSeamDepth"),
    fastSeamDepthValue: required("illustrationFastSeamDepthValue"),
    background: required("illustrationBackground"),
    transparent: required("illustrationTransparent"),
};
const state = {
    models: [],
    model: null,
    root: null,
    scene: null,
    style: null,
    svg: "",
    view: "iso",
    output: "svg",
    activeLoad: 0,
    renderTimer: 0,
    worker: null,
    workerUrl: "",
    workerRequest: 0,
    pendingWorker: new Map(),
    uploadedUrls: [],
    projectionCache: new Map(),
    validation: new URLSearchParams(window.location.search).get("validation") === "1",
    suppressCamera: false,
    prepareGeneration: 0,
    prepareRequest: 0,
    lineworkRequest: 0,
    lineworkBusyRequest: null,
    showHlrOutline: true,
    showHlrDetail: true,
};
const renderer = new THREE.WebGLRenderer({
    canvas: els.modelCanvas,
    antialias: true,
    preserveDrawingBuffer: true,
});
renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.setClearColor(0xffffff, 1);
let fastCreaseLineworkTimer = 0;
const threeScene = new THREE.Scene();
threeScene.background = new THREE.Color(0xffffff);
const camera = new THREE.OrthographicCamera(-1, 1, 1, -1, 0.001, 100_000);
const controls = new TrackballControls(camera, renderer.domElement);
controls.rotateSpeed = 1;
controls.zoomSpeed = 1.2;
controls.panSpeed = 0.3;
// Camera inertia can continue changing the pose while an asynchronous STEP
// remesh captures and restores it. The illustration lab favors deterministic
// engineering views: the camera stops when pointer input stops.
controls.staticMoving = true;
threeScene.add(new THREE.HemisphereLight(0xffffff, 0xaeb7c0, 1.2));
const threeKey = new THREE.DirectionalLight(0xffffff, 1.7);
const previewLightRight = new THREE.Vector3();
const previewLightUp = new THREE.Vector3();
const previewLightTowardViewer = new THREE.Vector3();
threeScene.add(threeKey, threeKey.target);
const modelGroup = new THREE.Group();
threeScene.add(modelGroup);
const loader = new GLTFLoader();
function setBusy(message, presentation = "overlay") {
    els.busy.hidden = message === null;
    if (message) {
        els.busy.dataset.presentation = presentation;
        els.busyText.textContent = message;
    }
    els.app.dataset.state = message ? "loading" : "ready";
}
function setStatus(message) {
    els.status.textContent = message;
}
function formatBytes(value) {
    return value >= 1024 * 1024
        ? `${(value / (1024 * 1024)).toFixed(2)} MiB`
        : `${Math.max(1, Math.round(value / 1024))} KiB`;
}
function formatMs(value) {
    return value >= 1000 ? `${(value / 1000).toFixed(2)} s` : `${Math.max(1, Math.round(value))} ms`;
}
function colorFromHex(value) {
    const normalized = value.replace(/^#/u, "");
    const numeric = Number.parseInt(normalized, 16);
    return [((numeric >> 16) & 255) / 255, ((numeric >> 8) & 255) / 255, (numeric & 255) / 255];
}
function numberInput(input, output, digits) {
    const value = Number.parseFloat(input.value);
    output.value = Number.isFinite(value) ? value.toFixed(digits) : "0";
    return Number.isFinite(value) ? value : 0;
}
function boundedNumberInput(input, minimum, maximum, fallback) {
    const requested = Number.parseFloat(input.value);
    return Math.max(minimum, Math.min(maximum, Number.isFinite(requested) ? requested : fallback));
}
function currentMeshSettings() {
    return {
        linearDeflectionMm: boundedNumberInput(els.linearDeflection, 0.001, 10, 0.1),
        angularDeflectionDegrees: boundedNumberInput(els.angularDeflection, 1, 60, (0.5 * 180) / Math.PI),
        hlrDeflectionCoefficient: boundedNumberInput(els.hlrDeflection, 0.0001, 0.05, 0.004),
        hlrAngularDeflectionDegrees: boundedNumberInput(els.hlrAngularDeflection, 1, 60, (0.5 * 180) / Math.PI),
    };
}
function surfaceMeshKey(settings) {
    const angularRadians = (settings.angularDeflectionDegrees * Math.PI) / 180;
    return `${settings.linearDeflectionMm.toPrecision(12)}|${angularRadians.toPrecision(12)}`;
}
const DEFAULT_SURFACE_MESH_KEY = surfaceMeshKey(MESH_QUALITY_PRESETS.balanced);
function applyMeshSettings(settings) {
    els.linearDeflection.value = String(settings.linearDeflectionMm);
    els.angularDeflection.value = String(settings.angularDeflectionDegrees);
    els.hlrDeflection.value = String(settings.hlrDeflectionCoefficient);
    els.hlrAngularDeflection.value = String(settings.hlrAngularDeflectionDegrees);
}
function updateMeshInfo(prefix = "Mesh controls ready") {
    const settings = currentMeshSettings();
    els.meshInfo.textContent = `${prefix}: STEP ${settings.linearDeflectionMm} mm / ${settings.angularDeflectionDegrees.toFixed(2)} deg; HLR ${settings.hlrDeflectionCoefficient} bbox / ${settings.hlrAngularDeflectionDegrees.toFixed(2)} deg; no triangle-count cap.`;
}
function currentStyle() {
    const bands = Math.round(numberInput(els.bands, els.bandsValue, 0));
    const ambient = numberInput(els.ambient, els.ambientValue, 2);
    const keyIntensity = numberInput(els.key, els.keyValue, 2);
    const rimAmount = numberInput(els.rim, els.rimValue, 2);
    const outlineWidth = numberInput(els.outlineWidth, els.outlineWidthValue, 3);
    return {
        shading: els.shading.value,
        ambient,
        keyIntensity,
        lightDirection: [0.35, 0.8, 0.48],
        bands,
        sourceColors: els.sourceColors.checked,
        fallbackColor: colorFromHex(els.fallback.value),
        background: els.background.value,
        transparentBackground: els.transparent.checked,
        fuseSurfaces: els.fuseSurfaces.checked,
        layerCoplanarMaterials: els.layerCoplanar.checked,
        showHlrOutline: state.showHlrOutline,
        showHlrDetail: state.showHlrDetail,
        showOutlines: false,
        showCreases: false,
        creaseAngleDegrees: 42,
        outlineColor: els.outlineColor.value,
        creaseColor: els.outlineColor.value,
        outlineWidth,
        creaseWidth: outlineWidth * 0.55,
        doubleSided: els.doubleSided.checked,
        rimAmount,
    };
}
function scheduleFastCreaseUpdate() {
    const crease = boundedNumberInput(els.fastCrease, 1, 80, 25);
    els.fastCreaseValue.value = `${crease.toFixed(0)} deg`;
    scheduleFastVectorLineworkUpdate();
}
function scheduleFastVectorLineworkUpdate() {
    const seamAngle = boundedNumberInput(els.fastSeamAngle, 0, 10, 1);
    const seamDepth = boundedNumberInput(els.fastSeamDepth, 0, 0.01, 0.001);
    els.fastSeamAngleValue.value = `${seamAngle.toFixed(1)} deg`;
    els.fastSeamDepthValue.value = `${seamDepth.toFixed(4)} mm`;
    cancelLazyHlrLinework();
    window.clearTimeout(fastCreaseLineworkTimer);
    fastCreaseLineworkTimer = window.setTimeout(() => {
        fastCreaseLineworkTimer = 0;
        if (!state.scene || !state.showHlrDetail)
            return;
        delete state.scene.detailSegments;
        updateHlrVisibility().catch(showError);
    }, 100);
}
function activateButtons(container, key, value) {
    for (const button of Array.from(container.querySelectorAll("button")))
        button.classList.toggle("active", button.dataset[key] === value);
}
function assetUrl(path) {
    if (path.startsWith("data:") || path.startsWith("blob:"))
        return path;
    return `/${path.split("/").map(encodeURIComponent).join("/")}`;
}
async function fetchArrayBuffer(path) {
    const response = await fetch(assetUrl(path));
    if (!response.ok)
        throw new Error(`Asset fetch failed (${response.status}): ${path}`);
    return response.arrayBuffer();
}
function clearModel() {
    while (modelGroup.children.length > 0) {
        const child = modelGroup.children.pop();
        child?.traverse((node) => {
            if (node instanceof THREE.Mesh) {
                node.geometry.dispose();
                const materials = Array.isArray(node.material) ? node.material : [node.material];
                for (const material of materials)
                    material.dispose();
            }
        });
    }
    state.root = null;
}
function resize() {
    const modelRect = els.modelPane.getBoundingClientRect();
    const modelWidth = Math.max(1, Math.round(modelRect.width));
    const modelHeight = Math.max(1, Math.round(modelRect.height));
    renderer.setSize(modelWidth, modelHeight, false);
    const aspect = modelWidth / modelHeight;
    const halfHeight = Number(camera.userData.halfHeight ?? 1);
    camera.left = -halfHeight * aspect;
    camera.right = halfHeight * aspect;
    camera.top = halfHeight;
    camera.bottom = -halfHeight;
    camera.updateProjectionMatrix();
    const outputRect = els.outputPane.getBoundingClientRect();
    const ratio = Math.min(window.devicePixelRatio || 1, 2);
    els.illustrationCanvas.width = Math.max(1, Math.round(outputRect.width * ratio));
    els.illustrationCanvas.height = Math.max(1, Math.round(outputRect.height * ratio));
    redrawStyle();
}
function captureCameraView() {
    return {
        position: camera.position.clone(),
        quaternion: camera.quaternion.clone(),
        up: camera.up.clone(),
        target: controls.target.clone(),
        zoom: camera.zoom,
        halfHeight: Number(camera.userData.halfHeight ?? 1),
    };
}
function fitCamera(root, preservedView) {
    root.updateMatrixWorld(true);
    let bounds = new THREE.Box3().setFromObject(root);
    const center = bounds.getCenter(new THREE.Vector3());
    root.position.sub(center);
    root.updateMatrixWorld(true);
    bounds = new THREE.Box3().setFromObject(root);
    const sphere = bounds.getBoundingSphere(new THREE.Sphere());
    const radius = Math.max(sphere.radius, 0.001);
    camera.userData.halfHeight = preservedView?.halfHeight ?? radius * 1.12;
    camera.near = Math.max(radius / 1000, 0.0001);
    camera.far = Math.max(radius * 20, 10);
    controls.minDistance = radius * 0.05;
    controls.maxDistance = radius * 20;
    if (preservedView) {
        camera.position.copy(preservedView.position);
        camera.quaternion.copy(preservedView.quaternion);
        camera.up.copy(preservedView.up);
        camera.zoom = preservedView.zoom;
        controls.target.copy(preservedView.target);
        camera.updateMatrixWorld(true);
    }
    else {
        controls.target.set(0, 0, 0);
        moveCameraToView(state.view === "camera" ? "iso" : state.view);
    }
    resize();
}
function namedView(view) {
    if (view === "top")
        return { direction: [0, 1, 0], up: [0, 0, -1] };
    if (view === "front")
        return { direction: [0, 0, 1], up: [0, 1, 0] };
    if (view === "right")
        return { direction: [1, 0, 0], up: [0, 1, 0] };
    return { direction: normalizeTuple([1, 1, 1]), up: [0, 1, 0] };
}
function normalizeTuple(value) {
    const length = Math.hypot(value[0], value[1], value[2]) || 1;
    return [value[0] / length, value[1] / length, value[2] / length];
}
function currentView() {
    if (state.view !== "camera")
        return namedView(state.view);
    const direction = new THREE.Vector3().subVectors(camera.position, controls.target).normalize();
    const up = camera.up.clone().normalize();
    return { direction: [direction.x, direction.y, direction.z], up: [up.x, up.y, up.z] };
}
function currentModelTransform(root) {
    root.updateMatrixWorld(true);
    const value = root.matrixWorld.elements;
    const at = (index) => value[index] ?? (index % 5 === 0 ? 1 : 0);
    // Three stores column-major matrices; Geometer's HLR option is row-major.
    // Keep this transform in STEP millimetres so geometry.projection.b0 retains
    // its documented units; convert the resulting segments at the adapter edge.
    return [
        at(0),
        at(4),
        at(8),
        at(12) * 1000,
        at(1),
        at(5),
        at(9),
        at(13) * 1000,
        at(2),
        at(6),
        at(10),
        at(14) * 1000,
        at(3),
        at(7),
        at(11),
        at(15),
    ];
}
function moveCameraToView(viewId) {
    const view = namedView(viewId);
    const distance = Math.max(Number(camera.userData.halfHeight ?? 1) * 4, 1);
    state.suppressCamera = true;
    camera.position.set(view.direction[0] * distance, view.direction[1] * distance, view.direction[2] * distance);
    camera.up.set(view.up[0], view.up[1], view.up[2]);
    controls.target.set(0, 0, 0);
    camera.lookAt(controls.target);
    controls.update();
    state.suppressCamera = false;
}
function materialColor(material) {
    const candidate = material;
    const color = candidate.color ?? new THREE.Color(0xb8bdc7);
    const toSrgb = (value) => value <= 0.0031308 ? value * 12.92 : 1.055 * value ** (1 / 2.4) - 0.055;
    return {
        ...(material.name ? { name: material.name } : {}),
        // Three stores material colors in linear-sRGB; the generic illustration
        // contract accepts sRGB so that file adapters and UI colors agree.
        color: [toSrgb(color.r), toSrgb(color.g), toSrgb(color.b)],
        opacity: Number.isFinite(candidate.opacity) ? candidate.opacity : 1,
    };
}
function meshInput(root) {
    const meshes = [];
    root.updateMatrixWorld(true);
    let meshIndex = 0;
    root.traverse((node) => {
        if (!(node instanceof THREE.Mesh) || !node.visible)
            return;
        const position = node.geometry.getAttribute("position");
        if (!position || position.itemSize < 3)
            return;
        const positions = new Float32Array(position.count * 3);
        for (let index = 0; index < position.count; index += 1) {
            positions[index * 3] = position.getX(index);
            positions[index * 3 + 1] = position.getY(index);
            positions[index * 3 + 2] = position.getZ(index);
        }
        const sourceNormal = node.geometry.getAttribute("normal");
        let normals;
        if (sourceNormal && sourceNormal.itemSize >= 3 && sourceNormal.count === position.count) {
            normals = new Float32Array(sourceNormal.count * 3);
            for (let index = 0; index < sourceNormal.count; index += 1) {
                normals[index * 3] = sourceNormal.getX(index);
                normals[index * 3 + 1] = sourceNormal.getY(index);
                normals[index * 3 + 2] = sourceNormal.getZ(index);
            }
        }
        const sourceIndex = node.geometry.getIndex();
        let indices;
        if (sourceIndex) {
            indices = new Uint32Array(sourceIndex.count);
            for (let index = 0; index < sourceIndex.count; index += 1)
                indices[index] = sourceIndex.getX(index);
        }
        const triangleCount = (indices?.length ?? positions.length / 3) / 3;
        const triangleMaterialIndices = new Uint32Array(triangleCount);
        for (const group of node.geometry.groups) {
            const firstTriangle = Math.floor(group.start / 3);
            const lastTriangle = Math.min(triangleCount, Math.ceil((group.start + group.count) / 3));
            for (let index = firstTriangle; index < lastTriangle; index += 1)
                triangleMaterialIndices[index] = group.materialIndex ?? 0;
        }
        const materialSlots = Array.isArray(node.material) ? node.material : [node.material];
        meshes.push({
            id: `${node.name || "mesh"}-${meshIndex++}`,
            positions,
            ...(normals ? { normals } : {}),
            ...(indices ? { indices } : {}),
            matrix: node.matrixWorld.elements.slice(),
            materials: materialSlots.map(materialColor),
            triangleMaterialIndices,
            doubleSided: materialSlots.some((material) => material.side === THREE.DoubleSide),
        });
    });
    if (meshes.length === 0)
        throw new Error("The loaded model contains no triangle meshes.");
    return { meshes };
}
function drawSvg(svg) {
    const parsed = new DOMParser().parseFromString(svg, "image/svg+xml");
    const error = parsed.querySelector("parsererror");
    if (error)
        throw new Error(`Generated SVG failed to parse: ${error.textContent ?? "unknown error"}`);
    els.svgHost.replaceChildren(document.importNode(parsed.documentElement, true));
}
function fastVectorLineworkLabel() {
    const layers = [];
    if (state.showHlrOutline)
        layers.push("FAST MESH-SHADOW");
    if (state.showHlrDetail)
        layers.push("FAST DETAIL");
    return layers.length > 0 ? layers.join(" + ") : "LINEWORK OFF";
}
function redrawStyle() {
    if (!state.scene)
        return;
    const style = currentStyle();
    state.style = style;
    const rendered = renderMeshIllustrationSvg(state.scene, style, `${state.model?.name ?? "model"} ${state.view} illustration`);
    state.svg = rendered.svg;
    const svgBytes = new TextEncoder().encode(rendered.svg).byteLength;
    drawSvg(rendered.svg);
    const context = els.illustrationCanvas.getContext("2d");
    if (!context)
        throw new Error("Canvas2D is unavailable.");
    const canvasStats = renderMeshIllustrationCanvas(context, state.scene, style);
    els.counts.textContent = `${rendered.stats.triangles.toLocaleString()} front-facing triangles → ${rendered.stats.surfaceDraws.toLocaleString()} polygon regions/draws / ${rendered.stats.layeredSurfaces.toLocaleString()} coplanar layers / SVG ${formatBytes(svgBytes)} / ${rendered.stats.details.toLocaleString()} HLR detail / ${rendered.stats.outlines.toLocaleString()} HLR outline segments / ${state.scene.warnings.length} warnings`;
    els.downloadSvg.disabled = false;
    els.downloadStyle.disabled = false;
    els.svgHost.classList.toggle("hidden", state.output !== "svg");
    els.illustrationCanvas.classList.toggle("hidden", state.output !== "canvas");
    els.outputPane.dataset.engine = "fast-vector";
    els.outputLabel.textContent =
        state.output === "svg"
            ? `FAST VECTOR / C++ WASM CPU / SVG / ${fastVectorLineworkLabel()} / ${formatBytes(svgBytes)}`
            : `FAST VECTOR / C++ WASM CPU / CANVAS / ${fastVectorLineworkLabel()} / ${canvasStats.commands.toLocaleString()} DRAWS`;
    els.outputPane.dataset.output = state.output;
    els.outputPane.dataset.shading = style.shading;
    els.outputPane.dataset.canvasOutlines = String(canvasStats.outlines + canvasStats.details);
    els.outputPane.dataset.canvasDetails = String(canvasStats.details);
    els.outputPane.dataset.visibleTriangles = String(canvasStats.triangles);
    els.outputPane.dataset.surfaceDraws = String(canvasStats.surfaceDraws);
    els.outputPane.dataset.layeredSurfaces = String(canvasStats.layeredSurfaces);
    els.outputPane.dataset.svgBytes = String(svgBytes);
    threeScene.background = new THREE.Color(style.background);
    renderer.setClearColor(style.background, 1);
}
function cancelLazyHlrLinework() {
    state.lineworkRequest += 1;
    if (state.lineworkBusyRequest === null)
        return;
    state.lineworkBusyRequest = null;
    setBusy(null);
}
function syncHlrControls() {
    els.hlrOutline.checked = state.showHlrOutline;
    els.hlrDetail.checked = state.showHlrDetail;
}
async function prepareIllustration(reason) {
    const root = state.root;
    const model = state.model;
    if (!root)
        return;
    const requestId = ++state.prepareRequest;
    cancelLazyHlrLinework();
    syncHlrControls();
    const started = performance.now();
    setBusy(`Preparing ${reason} illustration...`, "indicator");
    await new Promise((resolve) => requestAnimationFrame(() => resolve()));
    try {
        const input = meshInput(root);
        const view = currentView();
        const meshSettings = currentMeshSettings();
        let prepared = prepareMeshIllustration(input, view, { weldTolerance: 1e-5 });
        let hlrMs = 0;
        if ((state.showHlrOutline || state.showHlrDetail) &&
            model &&
            (model.step || model.stepBuffer)) {
            setBusy(`Tracing ${model.name} with Geometer HLR...`, "indicator");
            const linework = await loadHlrLinework(model, view, currentModelTransform(root));
            hlrMs = linework.hlrMs;
            prepared = {
                ...prepared,
                outlineSegments: linework.outlineSegments,
                detailSegments: linework.detailSegments,
            };
        }
        if (requestId !== state.prepareRequest || state.root !== root || state.model !== model)
            return;
        cancelLazyHlrLinework();
        state.scene = prepared;
        state.prepareGeneration += 1;
        els.outputPane.dataset.view = state.view;
        els.outputPane.dataset.direction = state.scene.view.direction
            .map((value) => value.toFixed(6))
            .join(",");
        els.outputPane.dataset.triangles = String(state.scene.stats.projectedTriangles);
        els.outputPane.dataset.meshKey = surfaceMeshKey(meshSettings);
        els.outputPane.dataset.cameraPosition = camera.position
            .toArray()
            .map((value) => value.toFixed(9))
            .join(",");
        els.outputPane.dataset.cameraTarget = controls.target
            .toArray()
            .map((value) => value.toFixed(9))
            .join(",");
        els.outputPane.dataset.cameraZoom = camera.zoom.toFixed(9);
        els.outputPane.dataset.cameraHalfHeight = Number(camera.userData.halfHeight ?? 1).toFixed(9);
        els.outputPane.dataset.prepareGeneration = String(state.prepareGeneration);
        if (state.showHlrDetail) {
            els.outputPane.dataset.fastVectorCrease = els.fastCrease.value;
            els.outputPane.dataset.fastVectorCoplanarSeams = String(els.fastCoplanarSeams.checked);
        }
        redrawStyle();
        const elapsed = performance.now() - started;
        els.timing.textContent = `prepare ${formatMs(elapsed)} / HLR ${formatMs(hlrMs)} / ${state.scene.stats.sourceTriangles.toLocaleString()} triangles`;
        setStatus(`${state.model?.name ?? "Model"} / ${state.view} / ${state.output.toUpperCase()} ready`);
        await validateIfRequested();
    }
    catch (error) {
        if (requestId !== state.prepareRequest || state.root !== root || state.model !== model)
            return;
        throw error;
    }
    finally {
        if (requestId === state.prepareRequest)
            setBusy(null);
    }
}
async function updateHlrVisibility() {
    const scene = state.scene;
    const model = state.model;
    const root = state.root;
    if (!scene)
        return;
    if (!state.showHlrOutline && !state.showHlrDetail) {
        cancelLazyHlrLinework();
        redrawStyle();
        return;
    }
    if (scene.outlineSegments !== undefined && scene.detailSegments !== undefined) {
        cancelLazyHlrLinework();
        redrawStyle();
        return;
    }
    if (!model || !root || (!model.step && !model.stepBuffer)) {
        cancelLazyHlrLinework();
        redrawStyle();
        return;
    }
    cancelLazyHlrLinework();
    const requestId = state.lineworkRequest;
    state.lineworkBusyRequest = requestId;
    setBusy(`Tracing ${model.name} with Geometer HLR...`, "indicator");
    try {
        const view = {
            direction: scene.view.direction,
            up: scene.view.up,
            mirrorX: scene.view.mirrorX,
        };
        const linework = await loadHlrLinework(model, view, currentModelTransform(root));
        if (requestId !== state.lineworkRequest ||
            state.scene !== scene ||
            state.model !== model ||
            state.root !== root)
            return;
        scene.outlineSegments = linework.outlineSegments;
        scene.detailSegments = linework.detailSegments;
        if (state.showHlrDetail) {
            els.outputPane.dataset.fastVectorCrease = els.fastCrease.value;
            els.outputPane.dataset.fastVectorCoplanarSeams = String(els.fastCoplanarSeams.checked);
        }
        redrawStyle();
    }
    catch (error) {
        if (requestId !== state.lineworkRequest ||
            state.scene !== scene ||
            state.model !== model ||
            state.root !== root)
            return;
        throw error;
    }
    finally {
        if (state.lineworkBusyRequest === requestId) {
            state.lineworkBusyRequest = null;
            setBusy(null);
        }
    }
}
function scheduleCameraIllustration() {
    if (state.suppressCamera || !state.root)
        return;
    state.view = "camera";
    activateButtons(els.viewButtons, "view", state.view);
    window.clearTimeout(state.renderTimer);
    state.renderTimer = window.setTimeout(() => {
        prepareIllustration("camera").catch(showError);
    }, 180);
}
function updatePreviewLighting() {
    const scale = Math.max(Number(camera.userData.halfHeight ?? 1), 0.001);
    previewLightRight.set(1, 0, 0).applyQuaternion(camera.quaternion);
    previewLightUp.set(0, 1, 0).applyQuaternion(camera.quaternion);
    previewLightTowardViewer.subVectors(camera.position, controls.target).normalize();
    threeKey.position
        .copy(controls.target)
        .addScaledVector(previewLightTowardViewer, scale * 2.6)
        .addScaledVector(previewLightUp, scale * 1.1)
        .addScaledVector(previewLightRight, scale * 0.65);
    threeKey.target.position.copy(controls.target);
}
function renderLoop() {
    requestAnimationFrame(renderLoop);
    controls.update();
    updatePreviewLighting();
    renderer.render(threeScene, camera);
}
function loadGlb(buffer) {
    return new Promise((resolve, reject) => {
        loader.parse(buffer, "", (gltf) => resolve(gltf.scene), reject);
    });
}
async function modelGlbBuffer(model) {
    const settings = currentMeshSettings();
    const meshKey = surfaceMeshKey(settings);
    const stepBacked = Boolean(model.step || model.stepBuffer);
    if (!stepBacked) {
        const buffer = model.glbBuffer ?? (await fetchArrayBuffer(model.glb));
        model.glbBuffer = buffer;
        model.glbMeshKey = "fixed-glb";
        return { buffer, meshMs: 0, source: "fixed-glb" };
    }
    if (model.glbBuffer && model.glbMeshKey === meshKey)
        return { buffer: model.glbBuffer, meshMs: 0, source: "cached" };
    if (!model.local && model.glb && meshKey === DEFAULT_SURFACE_MESH_KEY) {
        const buffer = await fetchArrayBuffer(model.glb);
        model.glbBuffer = buffer;
        model.glbMeshKey = meshKey;
        model.glbBytes = buffer.byteLength;
        return { buffer, meshMs: 0, source: "prebuilt" };
    }
    const source = await modelStepBuffer(model);
    const converted = await convertStep(source.slice(0), settings);
    model.glbBuffer = converted.buffer;
    model.glbMeshKey = meshKey;
    model.glbBytes = converted.buffer.byteLength;
    return { buffer: converted.buffer, meshMs: converted.glbMs, source: "remeshed" };
}
async function selectModel(model, options = {}) {
    const preservedView = options.preserveCamera && state.root ? captureCameraView() : undefined;
    const loadId = ++state.activeLoad;
    state.prepareRequest += 1;
    cancelLazyHlrLinework();
    window.clearTimeout(state.renderTimer);
    state.model = model;
    state.scene = null;
    const stepBacked = Boolean(model.step || model.stepBuffer);
    for (const control of [
        els.meshQuality,
        els.linearDeflection,
        els.angularDeflection,
        els.hlrDeflection,
        els.hlrAngularDeflection,
    ])
        control.disabled = !stepBacked;
    els.downloadSvg.disabled = true;
    els.modelSelect.value = model.cacheKey ?? model.name;
    setBusy(`Loading ${model.name}...`);
    setStatus(`Loading ${model.name}`);
    const started = performance.now();
    try {
        const meshResult = await modelGlbBuffer(model);
        const { buffer } = meshResult;
        if (loadId !== state.activeLoad)
            return;
        const root = await loadGlb(buffer.slice(0));
        if (loadId !== state.activeLoad)
            return;
        clearModel();
        modelGroup.add(root);
        state.root = root;
        fitCamera(root, preservedView);
        await prepareIllustration("mesh");
        const elapsed = performance.now() - started;
        const sourceLabel = meshResult.source === "fixed-glb"
            ? "fixed GLB"
            : meshResult.source === "prebuilt"
                ? "balanced GLB"
                : meshResult.source === "cached"
                    ? "cached mesh"
                    : `STEP mesh ${formatMs(meshResult.meshMs)}`;
        els.timing.textContent = `${sourceLabel} / ${formatBytes(buffer.byteLength)} / total ${formatMs(elapsed)}`;
        updateMeshInfo(meshResult.source === "fixed-glb" ? "Source GLB is fixed" : "STEP mesh active");
    }
    catch (error) {
        showError(error);
    }
    finally {
        if (loadId === state.activeLoad)
            setBusy(null);
    }
}
function workerUrl() {
    const embedded = window.GeometerIllustrationEmbedded;
    if (!embedded)
        return "/examples/wasm/illustration_step_worker.js";
    if (!state.workerUrl)
        state.workerUrl = URL.createObjectURL(new Blob([embedded.workerSource], { type: "text/javascript" }));
    return state.workerUrl;
}
function stepWorker() {
    if (state.worker)
        return state.worker;
    const worker = new Worker(workerUrl());
    worker.onmessage = (event) => {
        const data = event.data;
        const pending = state.pendingWorker.get(data.id);
        if (!pending)
            return;
        state.pendingWorker.delete(data.id);
        if (data.ok)
            pending.resolve(data);
        else
            pending.reject(new Error(data.error ?? "STEP adapter Worker failed."));
    };
    const failWorker = (error) => {
        for (const pending of state.pendingWorker.values())
            pending.reject(error);
        state.pendingWorker.clear();
        worker.terminate();
        if (state.worker === worker)
            state.worker = null;
        showError(error);
    };
    worker.onerror = (event) => {
        event.preventDefault();
        failWorker(new Error(event.message || "STEP conversion Worker error."));
    };
    worker.onmessageerror = () => failWorker(new Error("STEP conversion Worker message error."));
    state.worker = worker;
    return worker;
}
function runStepWorker(message, stepBuffer) {
    const id = ++state.workerRequest;
    return new Promise((resolve, reject) => {
        state.pendingWorker.set(id, { resolve, reject });
        try {
            stepWorker().postMessage({ id, ...message, stepBuffer }, [stepBuffer]);
        }
        catch (error) {
            state.pendingWorker.delete(id);
            reject(error instanceof Error ? error : new Error(String(error)));
        }
    });
}
async function convertStep(stepBuffer, settings = currentMeshSettings()) {
    const result = await runStepWorker({
        operation: "step-to-glb",
        meshOptions: {
            linearDeflectionMm: settings.linearDeflectionMm,
            angularDeflectionRad: (settings.angularDeflectionDegrees * Math.PI) / 180,
        },
    }, stepBuffer);
    if (!result.glbBuffer)
        throw new Error("STEP conversion Worker returned no GLB data.");
    return { buffer: result.glbBuffer, glbMs: result.timings?.glbMs ?? 0 };
}
async function modelStepBuffer(model) {
    if (model.stepBuffer)
        return model.stepBuffer;
    if (!model.step)
        throw new Error(`${model.name} has no STEP source for HLR linework.`);
    model.stepBuffer = await fetchArrayBuffer(model.step);
    return model.stepBuffer;
}
function projectionCacheKey(model, view, modelTransform) {
    const settings = currentMeshSettings();
    const values = [
        ...view.direction,
        ...view.up,
        view.mirrorX ? 1 : 0,
        ...modelTransform,
        (settings.hlrAngularDeflectionDegrees * Math.PI) / 180,
        settings.hlrDeflectionCoefficient,
        Number.parseFloat(els.fastCrease.value),
        els.fastCoplanarSeams.checked ? 1 : 0,
        Number.parseFloat(els.fastSeamAngle.value),
        Number.parseFloat(els.fastSeamDepth.value),
        state.showHlrOutline ? 1 : 0,
        state.showHlrDetail ? 1 : 0,
    ];
    return `${model.cacheKey ?? model.name}|${values.map((value) => value.toFixed(7)).join(",")}`;
}
async function loadHlrLinework(model, view, modelTransform) {
    const cacheKey = projectionCacheKey(model, view, modelTransform);
    let projection = state.projectionCache.get(cacheKey);
    if (projection) {
        state.projectionCache.delete(cacheKey);
        state.projectionCache.set(cacheKey, projection);
    }
    let hlrMs = 0;
    if (!projection) {
        const source = await modelStepBuffer(model);
        const settings = currentMeshSettings();
        const result = await runStepWorker({
            operation: "mesh-shadow",
            view,
            modelTransform,
            hlrOptions: {
                angularDeflectionRad: (settings.hlrAngularDeflectionDegrees * Math.PI) / 180,
                deflectionCoefficient: settings.hlrDeflectionCoefficient,
                creaseAngleRad: (Number.parseFloat(els.fastCrease.value) * Math.PI) / 180,
                suppressCoplanarSeams: els.fastCoplanarSeams.checked,
                coplanarSeamAngleRad: (Number.parseFloat(els.fastSeamAngle.value) * Math.PI) / 180,
                coplanarSeamDepthTolerance: Number.parseFloat(els.fastSeamDepth.value),
                outputOutline: state.showHlrOutline,
                outputDetail: state.showHlrDetail,
            },
        }, source.slice(0));
        if (!result.projection)
            throw new Error("HLR Worker returned no projection data.");
        projection = result.projection;
        hlrMs = result.timings?.hlrMs ?? 0;
        state.projectionCache.set(cacheKey, projection);
        while (state.projectionCache.size > 8) {
            const oldest = state.projectionCache.keys().next().value;
            if (oldest === undefined)
                break;
            state.projectionCache.delete(oldest);
        }
    }
    const mirrorX = view.mirrorX === true ? -1 : 1;
    const millimetresToMetres = 0.001;
    const convertSegments = (segments) => segments
        .filter((segment) => segment.length === 4 && segment.every(Number.isFinite))
        .map(([x1, y1, x2, y2]) => ({
        points: [
            [mirrorX * x1 * millimetresToMetres, y1 * millimetresToMetres],
            [mirrorX * x2 * millimetresToMetres, y2 * millimetresToMetres],
        ],
    }));
    return {
        outlineSegments: convertSegments(projection.views?.[0]?.modes?.outline?.segments ?? []),
        detailSegments: convertSegments(projection.views?.[0]?.modes?.detail?.segments ?? []),
        hlrMs,
    };
}
function appendModel(model) {
    const option = document.createElement("option");
    option.value = model.cacheKey ?? model.name;
    option.textContent = model.local ? `${model.name} (local)` : model.name;
    els.modelSelect.append(option);
}
async function openStep(file) {
    if (!/\.(?:step|stp)$/iu.test(file.name))
        throw new Error("Choose a .step or .stp model.");
    if (file.size > 256 * 1024 * 1024)
        throw new Error("STEP files larger than 256 MiB are not supported.");
    els.openButton.disabled = true;
    setBusy(`Converting ${file.name} to a mesh in Geometer WASM...`);
    try {
        const stepBuffer = await file.arrayBuffer();
        const converted = await convertStep(stepBuffer.slice(0));
        const settings = currentMeshSettings();
        const model = {
            name: file.name,
            cacheKey: `local:${Date.now()}:${file.name}`,
            step: "",
            glb: "",
            stepBytes: file.size,
            glbBytes: converted.buffer.byteLength,
            glbBuffer: converted.buffer,
            glbMeshKey: surfaceMeshKey(settings),
            stepBuffer,
            local: true,
        };
        state.models.push(model);
        appendModel(model);
        await selectModel(model);
        els.timing.textContent = `STEP mesh ${formatMs(converted.glbMs)} / ${formatBytes(model.glbBytes)} / HLR ready`;
    }
    finally {
        els.openButton.disabled = false;
        els.stepInput.value = "";
        setBusy(null);
    }
}
function download(name, content, type) {
    const url = URL.createObjectURL(new Blob([content], { type }));
    const link = document.createElement("a");
    link.href = url;
    link.download = name;
    link.click();
    window.setTimeout(() => URL.revokeObjectURL(url), 0);
}
function fileStem() {
    return (state.model?.name ?? "model")
        .replace(/\.(?:step|stp|glb|gltf)$/iu, "")
        .replace(/[^a-z0-9._-]+/giu, "-");
}
function styleJson() {
    return `${JSON.stringify(toMeshIllustrationStyleA0(currentStyle()), null, 2)}\n`;
}
function canvasHasContent(canvas) {
    const sample = document.createElement("canvas");
    sample.width = 16;
    sample.height = 16;
    const context = sample.getContext("2d", { willReadFrequently: true });
    if (!context)
        return false;
    context.drawImage(canvas, 0, 0, 16, 16);
    const pixels = context.getImageData(0, 0, 16, 16).data;
    let first = null;
    for (let index = 0; index < pixels.length; index += 4) {
        const current = `${pixels[index]},${pixels[index + 1]},${pixels[index + 2]},${pixels[index + 3]}`;
        if (first === null)
            first = current;
        else if (current !== first)
            return true;
    }
    return false;
}
async function validateIfRequested() {
    if (!state.validation || !state.scene)
        return;
    await new Promise((resolve) => requestAnimationFrame(() => resolve()));
    await new Promise((resolve) => requestAnimationFrame(() => resolve()));
    const svgTriangles = els.svgHost.querySelectorAll("polygon").length;
    const pass = state.scene.stats.projectedTriangles > 0 &&
        svgTriangles > 0 &&
        canvasHasContent(els.modelCanvas) &&
        canvasHasContent(els.illustrationCanvas);
    const result = pass
        ? `PASS triangles=${state.scene.stats.projectedTriangles} svg=${svgTriangles} view=${state.view}`
        : "FAIL nonblank illustration validation";
    els.validation.textContent = result;
    document.title = pass ? result : `FAIL ${result}`;
}
function showError(error) {
    const message = error instanceof Error ? error.message : String(error);
    console.error(error);
    els.app.dataset.state = "error";
    setStatus(`Error: ${message}`);
    setBusy(null);
    if (state.validation) {
        els.validation.textContent = `FAIL ${message}`;
        document.title = `FAIL ${message}`;
    }
}
function markMeshQualityCustom() {
    els.meshQuality.value = "custom";
    applyMeshSettings(currentMeshSettings());
}
async function applyMeshQualityPreset() {
    const quality = els.meshQuality.value;
    if (quality === "custom")
        return;
    applyMeshSettings(MESH_QUALITY_PRESETS[quality]);
    updateMeshInfo(`${els.meshQuality.selectedOptions[0]?.textContent ?? quality} requested`);
    if (state.model)
        await selectModel(state.model, { preserveCamera: true });
}
async function rebuildSurfaceMesh() {
    markMeshQualityCustom();
    updateMeshInfo("Custom STEP mesh requested");
    if (state.model)
        await selectModel(state.model, { preserveCamera: true });
}
async function rebuildHlrLinework() {
    markMeshQualityCustom();
    updateMeshInfo("Custom HLR mesh requested");
    if (state.root)
        await prepareIllustration("HLR mesh");
}
function wireEvents() {
    new ResizeObserver(resize).observe(els.modelPane);
    new ResizeObserver(resize).observe(els.outputPane);
    controls.addEventListener("change", scheduleCameraIllustration);
    els.openButton.addEventListener("click", () => els.stepInput.click());
    els.stepInput.addEventListener("change", () => {
        const file = els.stepInput.files?.[0];
        if (file)
            openStep(file).catch(showError);
    });
    els.modelSelect.addEventListener("change", () => {
        const selected = state.models.find((model) => (model.cacheKey ?? model.name) === els.modelSelect.value);
        if (selected)
            selectModel(selected).catch(showError);
    });
    els.meshQuality.addEventListener("change", () => {
        applyMeshQualityPreset().catch(showError);
    });
    for (const control of [els.linearDeflection, els.angularDeflection]) {
        control.addEventListener("change", () => {
            rebuildSurfaceMesh().catch(showError);
        });
    }
    for (const control of [els.hlrDeflection, els.hlrAngularDeflection])
        control.addEventListener("change", () => {
            rebuildHlrLinework().catch(showError);
        });
    els.viewButtons.addEventListener("click", (event) => {
        const button = event.target?.closest("button[data-view]");
        if (!button)
            return;
        state.view = button.dataset.view;
        activateButtons(els.viewButtons, "view", state.view);
        if (state.view !== "camera")
            moveCameraToView(state.view);
        prepareIllustration(state.view).catch(showError);
    });
    els.outputButtons.addEventListener("click", (event) => {
        const button = event.target?.closest("button[data-output]");
        if (!button)
            return;
        state.output = button.dataset.output;
        activateButtons(els.outputButtons, "output", state.output);
        window.clearTimeout(state.renderTimer);
        if (!state.scene || state.view === "camera") {
            prepareIllustration("camera").catch(showError);
        }
        else {
            redrawStyle();
        }
    });
    for (const control of [
        els.shading,
        els.sourceColors,
        els.fuseSurfaces,
        els.layerCoplanar,
        els.fallback,
        els.outlineColor,
        els.doubleSided,
        els.background,
        els.transparent,
    ]) {
        control.addEventListener("change", redrawStyle);
    }
    els.hlrOutline.addEventListener("change", () => {
        state.showHlrOutline = els.hlrOutline.checked;
        updateHlrVisibility().catch(showError);
    });
    els.hlrDetail.addEventListener("change", () => {
        state.showHlrDetail = els.hlrDetail.checked;
        updateHlrVisibility().catch(showError);
    });
    for (const control of [els.bands, els.ambient, els.key, els.rim, els.outlineWidth]) {
        control.addEventListener("input", redrawStyle);
    }
    els.fastCrease.addEventListener("input", scheduleFastCreaseUpdate);
    els.fastCoplanarSeams.addEventListener("change", scheduleFastVectorLineworkUpdate);
    els.fastSeamAngle.addEventListener("input", scheduleFastVectorLineworkUpdate);
    els.fastSeamDepth.addEventListener("input", scheduleFastVectorLineworkUpdate);
    els.downloadSvg.addEventListener("click", () => download(`${fileStem()}-${state.view}.svg`, state.svg, "image/svg+xml"));
    els.downloadStyle.addEventListener("click", () => download(`${fileStem()}.illustration-style.json`, styleJson(), "application/json"));
    window.addEventListener("beforeunload", () => {
        state.worker?.terminate();
        if (state.workerUrl)
            URL.revokeObjectURL(state.workerUrl);
        for (const url of state.uploadedUrls)
            URL.revokeObjectURL(url);
        window.clearTimeout(fastCreaseLineworkTimer);
    });
}
async function loadModels() {
    if (window.GeometerIllustrationEmbedded)
        return window.GeometerIllustrationEmbedded.models;
    const response = await fetch("/tests/fixtures/embedded_models_manifest.json");
    if (!response.ok)
        throw new Error(`Model manifest fetch failed: ${response.status}`);
    const models = (await response.json());
    const modelByName = new Map(models.map((model) => [model.name, model]));
    const selected = DEMO_MODEL_ORDER.map((name) => modelByName.get(name)).filter((model) => model !== undefined);
    return selected.length > 0 ? selected : models.filter((model) => Boolean(model.glb)).slice(0, 5);
}
async function init() {
    wireEvents();
    resize();
    renderLoop();
    setBusy("Loading bundled models...");
    state.models = await loadModels();
    if (state.models.length === 0)
        throw new Error("No bundled mesh fixtures are available.");
    for (const model of state.models)
        appendModel(model);
    await selectModel(state.models[0]);
}
init().catch(showError);
