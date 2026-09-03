import * as THREE from "three";
import { TrackballControls } from "three/addons/controls/TrackballControls.js";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";
import { PanelManager } from "/dist/wasm/demos/demo-tooling/panels.js";

const manifestUrl = "/tests/fixtures/embedded_models_manifest.json";
// A preset frame is explicit: Top and Front are viewer directions in model
// coordinates. Right is derived as Top x Front to keep the frame right-handed.
// Defaults use the demo convention requested for STEP models: Top +Y, Front +Z.
const AXIS_VECTORS = {
  "+x": [1, 0, 0],
  "-x": [-1, 0, 0],
  "+y": [0, 1, 0],
  "-y": [0, -1, 0],
  "+z": [0, 0, 1],
  "-z": [0, 0, -1],
};
let topAxisId = "+y";
let frontAxisId = "+z";

function negateVector(value) {
  return value.map((component) => -component);
}

function crossVector(a, b) {
  return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
}

function normalizedSum(...vectors) {
  const result = [0, 0, 0];
  for (const vector of vectors)
    for (let index = 0; index < 3; index += 1) result[index] += vector[index];
  const length = Math.hypot(...result) || 1;
  return result.map((component) => component / length);
}

function buildProjectionViews() {
  const top = AXIS_VECTORS[topAxisId];
  const front = AXIS_VECTORS[frontAxisId];
  const right = crossVector(top, front);
  return [
    { id: "top", label: "Top", direction: top, up: negateVector(front) },
    { id: "bottom", label: "Bottom", direction: negateVector(top), up: negateVector(front) },
    { id: "front", label: "Front", direction: front, up: top },
    { id: "back", label: "Back", direction: negateVector(front), up: top, mirrorX: true },
    { id: "left", label: "Left", direction: negateVector(right), up: top, mirrorX: true },
    { id: "right", label: "Right", direction: right, up: top },
    {
      id: "isoTop",
      label: "ISO Top",
      direction: normalizedSum(right, negateVector(front), top),
      up: negateVector(front),
    },
    {
      id: "isoBottom",
      label: "ISO Bot",
      direction: normalizedSum(right, front, negateVector(top)),
      up: front,
    },
    { id: "isoFront", label: "ISO Front", direction: normalizedSum(right, front, top), up: top },
    {
      id: "isoBack",
      label: "ISO Back",
      direction: normalizedSum(right, negateVector(front), negateVector(top)),
      up: negateVector(top),
    },
  ];
}

let projectionViews = buildProjectionViews();

const state = {
  models: [],
  selectedModel: null,
  viewId: "camera",
  mode: "detail",
  backend: "current",
  projectionWorker: null,
  workerBackend: null,
  projectionRequests: new Map(),
  nextProjectionRequestId: 1,
  projectionCache: new Map(),
  displayedProjection: null,
  displayedViewId: null,
  stepCache: new Map(),
  uploadedObjectUrls: [],
  uploadCounter: 0,
  exportReady: false,
  activeLoad: 0,
  activeProjection: 0,
  cameraReprojectTimer: 0,
  settingsReprojectTimer: 0,
  suppressCameraChange: false,
  cameraLens: "orthographic",
  viewportAspect: 1,
  orthoHalfHeight: 1,
  validation: new URLSearchParams(window.location.search).get("validation") === "1",
};

const els = {
  modelSelect: document.getElementById("modelSelect"),
  topAxisSelect: document.getElementById("topAxisSelect"),
  frontAxisSelect: document.getElementById("frontAxisSelect"),
  stepFileInput: document.getElementById("stepFileInput"),
  openStepButton: document.getElementById("openStepButton"),
  exportSvgButton: document.getElementById("exportSvgButton"),
  viewButtons: document.getElementById("viewButtons"),
  modeButtons: document.getElementById("modeButtons"),
  status: document.getElementById("status"),
  canvas: document.getElementById("modelCanvas"),
  projectionSvg: document.getElementById("projectionSvg"),
  modelMetric: document.getElementById("modelMetric"),
  meshMetric: document.getElementById("meshMetric"),
  projectionMetric: document.getElementById("projectionMetric"),
  sourceMetric: document.getElementById("sourceMetric"),
  workOverlay: document.getElementById("workOverlay"),
  workTitle: document.getElementById("workTitle"),
  workSubtitle: document.getElementById("workSubtitle"),
  validationResult: document.getElementById("validationResult"),
  cameraLensSelect: document.getElementById("cameraLensSelect"),
  fastBackendInput: document.getElementById("fastBackendInput"),
  algoSelect: document.getElementById("algoSelect"),
  outlineAlgoSelect: document.getElementById("outlineAlgoSelect"),
  meshDeflectionModeSelect: document.getElementById("meshDeflectionModeSelect"),
  linDeflInput: document.getElementById("linDeflInput"),
  angDeflInput: document.getElementById("angDeflInput"),
  hlrTolInput: document.getElementById("hlrTolInput"),
  deflCoeffInput: document.getElementById("deflCoeffInput"),
  detailColorInput: document.getElementById("detailColorInput"),
  detailWidthInput: document.getElementById("detailWidthInput"),
  detailStyleSelect: document.getElementById("detailStyleSelect"),
  outlineColorInput: document.getElementById("outlineColorInput"),
  outlineWidthInput: document.getElementById("outlineWidthInput"),
  outlineStyleSelect: document.getElementById("outlineStyleSelect"),
  bboxColorInput: document.getElementById("bboxColorInput"),
  bboxWidthInput: document.getElementById("bboxWidthInput"),
  bboxStyleSelect: document.getElementById("bboxStyleSelect"),
  bboxToggleInput: document.getElementById("bboxToggleInput"),
  edgePresetSelect: document.getElementById("edgePresetSelect"),
  edgeRow: document.getElementById("edgeRow"),
  occtSettings: document.getElementById("occtSettings"),
  fastSettings: document.getElementById("fastSettings"),
  fastEdgeRow: document.getElementById("fastEdgeRow"),
  fastBoundariesInput: document.getElementById("fastBoundariesInput"),
  fastCreasesInput: document.getElementById("fastCreasesInput"),
  fastSilhouettesInput: document.getElementById("fastSilhouettesInput"),
  fastHiddenInput: document.getElementById("fastHiddenInput"),
  fastCoplanarSeamsInput: document.getElementById("fastCoplanarSeamsInput"),
  fastCreaseAngleInput: document.getElementById("fastCreaseAngleInput"),
  fastWeldToleranceInput: document.getElementById("fastWeldToleranceInput"),
  fastProjectedToleranceInput: document.getElementById("fastProjectedToleranceInput"),
  fastDepthToleranceInput: document.getElementById("fastDepthToleranceInput"),
  resetGeometryButton: document.getElementById("resetGeometryButton"),
  workspace: document.getElementById("workspace"),
  settingsPanelContent: document.getElementById("settingsPanelContent"),
  threePanelContent: document.getElementById("threePanelContent"),
  materialModeSelect: document.getElementById("materialModeSelect"),
  shadingModeSelect: document.getElementById("shadingModeSelect"),
  sidednessSelect: document.getElementById("sidednessSelect"),
  wireframeInput: document.getElementById("wireframeInput"),
  ambientLightInput: document.getElementById("ambientLightInput"),
  ambientLightValue: document.getElementById("ambientLightValue"),
  keyLightInput: document.getElementById("keyLightInput"),
  keyLightValue: document.getElementById("keyLightValue"),
  cameraLightInput: document.getElementById("cameraLightInput"),
  cameraLightValue: document.getElementById("cameraLightValue"),
  backgroundColorInput: document.getElementById("backgroundColorInput"),
  toneMappingSelect: document.getElementById("toneMappingSelect"),
  exposureInput: document.getElementById("exposureInput"),
  exposureValue: document.getElementById("exposureValue"),
  resetThreeButton: document.getElementById("resetThreeButton"),
};

const GEOMETRY_DEFAULTS = Object.freeze({
  fastBackend: false,
  algorithm: "poly",
  outlineAlgorithm: "mesh-shadow",
  meshDeflectionMode: "bbox-relative",
  deflectionCoefficient: "0.004",
  linearDeflection: "0.01",
  angularDeflection: "0.5",
  hlrAngleTolerance: "0.0174533",
  edgePreset: "detail",
  fastBoundaries: true,
  fastCreases: true,
  fastSilhouettes: true,
  fastHidden: false,
  fastCoplanarSeams: false,
  fastCreaseAngleDegrees: "30",
  fastWeldTolerance: "0.0000001",
  fastProjectedTolerance: "0.00000001",
  fastDepthTolerance: "0.0000001",
});

const THREE_DEFAULTS = Object.freeze({
  materialMode: "lambert",
  shadingMode: "smooth",
  sidedness: "source",
  wireframe: false,
  ambientLight: "0.2",
  keyLight: "0.5",
  cameraLight: "0.75",
  background: "#ffffff",
  toneMapping: "none",
  exposure: "1",
});

let panelManager = null;

// OCCT HLR edge categories. Order matches the C++ HlrProjectionOptions struct.
const EDGE_FLAGS = [
  "edge_v_sharp",
  "edge_v_outline",
  "edge_v_smooth",
  "edge_v_sewn",
  "edge_v_iso",
  "edge_h_sharp",
  "edge_h_outline",
  "edge_h_smooth",
  "edge_h_sewn",
  "edge_h_iso",
];

// Demo convenience presets for the raw OCCT categories feeding Detail.
const EDGE_PRESETS = {
  detail: ["edge_v_sharp", "edge_v_outline"],
  "all-visible": ["edge_v_sharp", "edge_v_outline", "edge_v_smooth", "edge_v_sewn"],
  "visible+hidden": ["edge_v_sharp", "edge_v_outline", "edge_h_sharp", "edge_h_outline"],
};

function edgeCheckbox(name) {
  return els.edgeRow.querySelector(`input[data-edge="${name}"]`);
}

function readEdgeFlags() {
  const flags = {};
  for (const name of EDGE_FLAGS) flags[name] = edgeCheckbox(name).checked;
  return flags;
}

function applyEdgePreset(presetId) {
  const enabled = EDGE_PRESETS[presetId];
  if (!enabled) return;
  for (const name of EDGE_FLAGS) edgeCheckbox(name).checked = enabled.includes(name);
}

function syncGeometryControls() {
  // Fast vector uses a separate provisional candidate contract. Poly HLR
  // supports only V/H Compound + OutLine; Exact exposes all OCCT categories.
  const algorithm = els.algoSelect.value;
  const poly = algorithm === "poly";
  const fast = els.fastBackendInput.checked;
  els.occtSettings.hidden = fast;
  els.fastSettings.hidden = !fast;
  for (const label of els.edgeRow.querySelectorAll("label.flag")) {
    const supported = !poly || label.dataset.algoPoly === "ok";
    label.classList.toggle("disabled", !supported);
    const cb = label.querySelector("input[type=checkbox]");
    cb.disabled = !supported;
  }
  els.edgeRow.dataset.algorithm = algorithm;
  els.hlrTolInput.disabled = !poly;
  const relative = els.meshDeflectionModeSelect.value === "bbox-relative";
  els.deflCoeffInput.disabled = !relative;
  els.linDeflInput.disabled = relative;
}

function resetGeometryDefaults({ reproject = true } = {}) {
  els.fastBackendInput.checked = GEOMETRY_DEFAULTS.fastBackend;
  els.algoSelect.value = GEOMETRY_DEFAULTS.algorithm;
  els.outlineAlgoSelect.value = GEOMETRY_DEFAULTS.outlineAlgorithm;
  els.meshDeflectionModeSelect.value = GEOMETRY_DEFAULTS.meshDeflectionMode;
  els.deflCoeffInput.value = GEOMETRY_DEFAULTS.deflectionCoefficient;
  els.linDeflInput.value = GEOMETRY_DEFAULTS.linearDeflection;
  els.angDeflInput.value = GEOMETRY_DEFAULTS.angularDeflection;
  els.hlrTolInput.value = GEOMETRY_DEFAULTS.hlrAngleTolerance;
  els.edgePresetSelect.value = GEOMETRY_DEFAULTS.edgePreset;
  els.fastBoundariesInput.checked = GEOMETRY_DEFAULTS.fastBoundaries;
  els.fastCreasesInput.checked = GEOMETRY_DEFAULTS.fastCreases;
  els.fastSilhouettesInput.checked = GEOMETRY_DEFAULTS.fastSilhouettes;
  els.fastHiddenInput.checked = GEOMETRY_DEFAULTS.fastHidden;
  els.fastCoplanarSeamsInput.checked = GEOMETRY_DEFAULTS.fastCoplanarSeams;
  els.fastCreaseAngleInput.value = GEOMETRY_DEFAULTS.fastCreaseAngleDegrees;
  els.fastWeldToleranceInput.value = GEOMETRY_DEFAULTS.fastWeldTolerance;
  els.fastProjectedToleranceInput.value = GEOMETRY_DEFAULTS.fastProjectedTolerance;
  els.fastDepthToleranceInput.value = GEOMETRY_DEFAULTS.fastDepthTolerance;
  applyEdgePreset(GEOMETRY_DEFAULTS.edgePreset);
  syncGeometryControls();
  if (reproject && state.selectedModel) reprojectCurrent({ force: true });
}

const renderer = new THREE.WebGLRenderer({
  canvas: els.canvas,
  antialias: true,
  preserveDrawingBuffer: true,
});
renderer.setClearColor(0xffffff, 1);
renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
renderer.outputColorSpace = THREE.SRGBColorSpace;

const scene = new THREE.Scene();
scene.background = new THREE.Color(0xffffff);
const perspectiveCamera = new THREE.PerspectiveCamera(45, 1, 0.1, 100000);
const orthographicCamera = new THREE.OrthographicCamera(-1, 1, 1, -1, 0.1, 100000);
let camera = orthographicCamera;
const controls = new TrackballControls(camera, renderer.domElement);
controls.rotateSpeed = 1.0;
controls.zoomSpeed = 1.2;
controls.panSpeed = 0.3;
controls.staticMoving = false;
controls.dynamicDampingFactor = 0.18;

const ambient = new THREE.AmbientLight(0xffffff, 0.2);
scene.add(ambient);
const key = new THREE.DirectionalLight(0xffffff, 0.5);
key.position.set(5, 10, 7.5);
scene.add(key);
const cameraLight = new THREE.DirectionalLight(0xffffff, 0.75);
scene.add(cameraLight);

const modelGroup = new THREE.Group();
scene.add(modelGroup);
const grid = new THREE.GridHelper(10, 10, 0xa7b0ba, 0xc6cdd4);
grid.visible = false;
scene.add(grid);

const loader = new GLTFLoader();
const resizeObserver = new ResizeObserver(resizeRenderer);
resizeObserver.observe(document.getElementById("modelPane"));

function assetUrl(path) {
  if (path.startsWith("data:") || path.startsWith("blob:")) return path;
  return `/${path.split("/").map(encodeURIComponent).join("/")}`;
}

function formatBytes(bytes) {
  if (bytes >= 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
  return `${Math.max(1, Math.round(bytes / 1024))} KB`;
}

function formatMs(ms) {
  if (!Number.isFinite(ms)) return "0 ms";
  if (ms >= 1000) return `${(ms / 1000).toFixed(2)} s`;
  return `${Math.max(1, Math.round(ms))} ms`;
}

function projectionTimingText(timings) {
  if (timings.cached) return "HLR cache";
  const stepText = timings.cachedStep ? "STEP cache" : `STEP ${formatMs(timings.stepFetchMs)}`;
  const phases = [];
  if (Number.isFinite(timings.meshMs)) phases.push(`mesh ${formatMs(timings.meshMs)}`);
  if (Number.isFinite(timings.nativeHlrMs)) phases.push(`hlr ${formatMs(timings.nativeHlrMs)}`);
  if (Number.isFinite(timings.extractMs)) phases.push(`ext ${formatMs(timings.extractMs)}`);
  const phaseText = phases.length ? ` (${phases.join(" / ")})` : "";
  return `${stepText} - HLR ${formatMs(timings.hlrMs)} worker${phaseText}`;
}

function moduleTimingText(timings) {
  return Number.isFinite(timings.moduleMs) && timings.moduleMs > 1
    ? ` - WASM ${formatMs(timings.moduleMs)}`
    : "";
}

function setStatus(text, busy = false, detail = "") {
  els.status.textContent = text;
  document.body.classList.toggle("busy", busy);
  els.workOverlay.classList.toggle("visible", busy);
  els.workTitle.textContent = busy ? text : "Idle";
  els.workSubtitle.textContent = busy ? detail : "";
}

function setValidation(text, pass) {
  els.validationResult.textContent = text;
  if (state.validation)
    document.title = text.startsWith("RUNNING") ? "RUNNING" : pass ? "PASS" : "FAIL";
}

function cssNumber(value) {
  return String(Number(value.toFixed(3)));
}

function lineDasharray(style, width) {
  if (style === "dashed") return `${cssNumber(width * 4)} ${cssNumber(width * 2.5)}`;
  if (style === "dotted") return `${cssNumber(width * 0.2)} ${cssNumber(width * 2.5)}`;
  return "none";
}

function projectionLayerStyle(layer) {
  const colorInput = els[`${layer}ColorInput`];
  const widthInput = els[`${layer}WidthInput`];
  const styleSelect = els[`${layer}StyleSelect`];
  const width = Math.min(5, Math.max(0.25, Number.parseFloat(widthInput.value) || 1));
  widthInput.value = cssNumber(width);
  const dasharray = lineDasharray(styleSelect.value, width);
  return `stroke: ${colorInput.value}; stroke-width: ${width}px; stroke-dasharray: ${dasharray};`;
}

function projectionAppearanceCss() {
  return `
    .detail { ${projectionLayerStyle("detail")} }
    .outline { ${projectionLayerStyle("outline")} }
    .bbox { ${projectionLayerStyle("bbox")} }
    line, path, circle { fill: none; stroke-linecap: round; stroke-linejoin: round; vector-effect: non-scaling-stroke; }
  `;
}

function resizeRenderer() {
  const rect = els.canvas.parentElement.getBoundingClientRect();
  const width = Math.max(1, Math.floor(rect.width));
  const height = Math.max(1, Math.floor(rect.height));
  renderer.setSize(width, height, false);
  controls.handleResize();
  state.viewportAspect = width / height;
  perspectiveCamera.aspect = state.viewportAspect;
  perspectiveCamera.updateProjectionMatrix();
  orthographicCamera.left = -state.orthoHalfHeight * state.viewportAspect;
  orthographicCamera.right = state.orthoHalfHeight * state.viewportAspect;
  orthographicCamera.top = state.orthoHalfHeight;
  orthographicCamera.bottom = -state.orthoHalfHeight;
  orthographicCamera.updateProjectionMatrix();
  renderOnce();
}

function setCameraLens(lens) {
  const normalized = lens === "perspective" ? "perspective" : "orthographic";
  const nextCamera = normalized === "perspective" ? perspectiveCamera : orthographicCamera;
  els.cameraLensSelect.value = normalized;
  state.cameraLens = normalized;
  els.canvas.dataset.cameraLens = normalized;
  if (nextCamera === camera) return;

  const target = controls.target.clone();
  const direction = camera.position.clone().sub(target).normalize();
  const currentDistance = Math.max(camera.position.distanceTo(target), 0.0001);
  const visibleHalfHeight = camera.isPerspectiveCamera
    ? (currentDistance * Math.tan(THREE.MathUtils.degToRad(camera.fov) / 2)) /
      Math.max(camera.zoom, 0.0001)
    : state.orthoHalfHeight / Math.max(camera.zoom, 0.0001);
  const nextDistance = nextCamera.isPerspectiveCamera
    ? visibleHalfHeight / Math.tan(THREE.MathUtils.degToRad(nextCamera.fov) / 2)
    : currentDistance;

  nextCamera.position.copy(target).addScaledVector(direction, nextDistance);
  nextCamera.up.copy(camera.up);
  nextCamera.near = camera.near;
  nextCamera.far = camera.far;
  nextCamera.zoom = nextCamera.isOrthographicCamera
    ? state.orthoHalfHeight / Math.max(visibleHalfHeight, 0.0001)
    : 1;
  camera = nextCamera;
  controls.object = camera;
  state.suppressCameraChange = true;
  resizeRenderer();
  controls.update();
  state.suppressCameraChange = false;
}

function renderOnce() {
  controls.update();
  cameraLight.position.copy(camera.position);
  renderer.render(scene, camera);
}

function animate() {
  requestAnimationFrame(animate);
  renderOnce();
}

function materialSlots(material) {
  return (Array.isArray(material) ? material : [material]).filter(Boolean);
}

function commonMaterialParameters(source) {
  return {
    name: source.name ? `${source.name} demo` : "Geometer demo material",
    color: source.color?.clone() || new THREE.Color(0xd8d8d8),
    map: source.map || null,
    alphaMap: source.alphaMap || null,
    aoMap: source.aoMap || null,
    lightMap: source.lightMap || null,
    vertexColors: Boolean(source.vertexColors),
    opacity: Number.isFinite(source.opacity) ? source.opacity : 1,
    transparent: Boolean(source.transparent),
    alphaTest: Number.isFinite(source.alphaTest) ? source.alphaTest : 0,
    side: source.side,
    depthTest: source.depthTest !== false,
    depthWrite: source.depthWrite !== false,
  };
}

function convertedMaterial(source, mode) {
  const common = commonMaterialParameters(source);
  if (mode === "basic") return new THREE.MeshBasicMaterial(common);
  return new THREE.MeshLambertMaterial({
    ...common,
    emissive: source.emissive?.clone() || new THREE.Color(0x000000),
    emissiveMap: source.emissiveMap || null,
  });
}

function meshMaterialCache(mesh) {
  if (mesh.userData.geometerDemoMaterials) return mesh.userData.geometerDemoMaterials;
  const source = materialSlots(mesh.material);
  const cache = {
    source,
    sourceWasArray: Array.isArray(mesh.material),
    originalSides: source.map((material) => material.side),
    lambert: null,
    basic: null,
  };
  mesh.userData.geometerDemoMaterials = cache;
  return cache;
}

function applyThreeMaterials(root = modelGroup) {
  const mode = els.materialModeSelect.value;
  const flatShading = els.shadingModeSelect.value === "flat";
  const wireframe = els.wireframeInput.checked;
  const sidedness = els.sidednessSelect.value;
  root.traverse((node) => {
    if (!node.isMesh || !node.material) return;
    const cache = meshMaterialCache(node);
    if (mode !== "source" && !cache[mode]) {
      cache[mode] = cache.source.map((material) => convertedMaterial(material, mode));
    }
    const active = mode === "source" ? cache.source : cache[mode];
    active.forEach((material, index) => {
      if ("flatShading" in material) material.flatShading = flatShading;
      if ("wireframe" in material) material.wireframe = wireframe;
      material.side =
        sidedness === "double"
          ? THREE.DoubleSide
          : sidedness === "front"
            ? THREE.FrontSide
            : cache.originalSides[index];
      material.needsUpdate = true;
    });
    node.material = cache.sourceWasArray ? active : active[0];
  });
}

function rangeValue(input, output) {
  const value = Number.parseFloat(input.value) || 0;
  output.value = value.toFixed(2);
  return value;
}

function syncThreeSettings() {
  ambient.intensity = rangeValue(els.ambientLightInput, els.ambientLightValue);
  key.intensity = rangeValue(els.keyLightInput, els.keyLightValue);
  cameraLight.intensity = rangeValue(els.cameraLightInput, els.cameraLightValue);
  const exposure = rangeValue(els.exposureInput, els.exposureValue);
  const toneMapping = els.toneMappingSelect.value;
  els.exposureInput.disabled = toneMapping === "none";
  renderer.toneMapping = toneMapping === "aces" ? THREE.ACESFilmicToneMapping : THREE.NoToneMapping;
  renderer.toneMappingExposure = exposure;
  scene.background.set(els.backgroundColorInput.value);
  renderer.setClearColor(els.backgroundColorInput.value, 1);
  applyThreeMaterials();
  els.canvas.dataset.materialMode = els.materialModeSelect.value;
  els.canvas.dataset.shadingMode = els.shadingModeSelect.value;
  els.canvas.dataset.sidedness = els.sidednessSelect.value;
  els.canvas.dataset.wireframe = String(els.wireframeInput.checked);
  els.canvas.dataset.toneMapping = toneMapping;
  els.canvas.dataset.ambientLight = ambient.intensity.toFixed(2);
  els.canvas.dataset.keyLight = key.intensity.toFixed(2);
  els.canvas.dataset.cameraLight = cameraLight.intensity.toFixed(2);
  els.canvas.dataset.exposure = exposure.toFixed(2);
  els.canvas.dataset.background = els.backgroundColorInput.value;
  renderOnce();
}

function resetThreeDefaults() {
  els.materialModeSelect.value = THREE_DEFAULTS.materialMode;
  els.shadingModeSelect.value = THREE_DEFAULTS.shadingMode;
  els.sidednessSelect.value = THREE_DEFAULTS.sidedness;
  els.wireframeInput.checked = THREE_DEFAULTS.wireframe;
  els.ambientLightInput.value = THREE_DEFAULTS.ambientLight;
  els.keyLightInput.value = THREE_DEFAULTS.keyLight;
  els.cameraLightInput.value = THREE_DEFAULTS.cameraLight;
  els.backgroundColorInput.value = THREE_DEFAULTS.background;
  els.toneMappingSelect.value = THREE_DEFAULTS.toneMapping;
  els.exposureInput.value = THREE_DEFAULTS.exposure;
  syncThreeSettings();
}

function clearModel() {
  while (modelGroup.children.length) {
    const child = modelGroup.children.pop();
    child.traverse((node) => {
      if (node.geometry) node.geometry.dispose();
      const cache = node.userData?.geometerDemoMaterials;
      const materials = cache
        ? [...cache.source, ...(cache.lambert || []), ...(cache.basic || [])]
        : materialSlots(node.material);
      for (const material of new Set(materials)) material.dispose();
    });
  }
}

function meshBounds(root) {
  // Compute bounds only from Mesh nodes inside the GLB. Lights, cameras, empty
  // groups, or helper objects can have huge implicit positions that inflate
  // the bounding box and push the camera unnecessarily far back.
  const box = new THREE.Box3();
  let empty = true;
  root.updateMatrixWorld(true);
  root.traverse((node) => {
    if (!node.isMesh || !node.geometry) return;
    if (!node.geometry.boundingBox) node.geometry.computeBoundingBox();
    const local = node.geometry.boundingBox;
    if (!local || !Number.isFinite(local.min.x) || !Number.isFinite(local.max.x)) return;
    const world = local.clone().applyMatrix4(node.matrixWorld);
    box.union(world);
    empty = false;
  });
  if (empty) return new THREE.Box3().setFromObject(root);
  return box;
}

function fitCamera(root) {
  const bounds = meshBounds(root);
  const size = new THREE.Vector3();
  const center = new THREE.Vector3();
  bounds.getSize(size);
  bounds.getCenter(center);
  root.position.sub(center);

  const sphere = new THREE.Sphere();
  meshBounds(root).getBoundingSphere(sphere);
  const radius = Math.max(sphere.radius, Math.max(size.x, size.y, size.z) * 0.5, 0.001);
  // Aspect-aware bounding-sphere fit. Use whichever (vertical or horizontal)
  // FOV is the limiting dimension so the sphere fits regardless of pane shape.
  const halfFovV = (perspectiveCamera.fov * Math.PI) / 180 / 2;
  const halfFovH = Math.atan(Math.tan(halfFovV) * Math.max(state.viewportAspect, 0.01));
  const halfFov = Math.min(halfFovV, halfFovH);
  const distance = (radius / Math.sin(halfFov)) * 1.05;
  const viewDirection = new THREE.Vector3(0.72, 0.54, 1.0).normalize();
  const cameraPosition = viewDirection.multiplyScalar(distance);
  state.orthoHalfHeight = (radius * 1.05) / Math.min(state.viewportAspect, 1);
  for (const modelCamera of [perspectiveCamera, orthographicCamera]) {
    modelCamera.near = Math.max(radius / 1000, 0.0001);
    modelCamera.far = Math.max(distance * 8, radius * 8, 10);
    modelCamera.position.copy(cameraPosition);
    modelCamera.up.set(0, 1, 0);
    modelCamera.zoom = 1;
    modelCamera.updateProjectionMatrix();
  }
  controls.target.set(0, 0, 0);
  state.suppressCameraChange = true;
  resizeRenderer();
  controls.update();
  state.suppressCameraChange = false;
  controls.minDistance = Math.max(radius * 0.01, 0.0001);
  controls.maxDistance = Math.max(distance * 8, radius * 8);

  grid.scale.setScalar(Math.max(radius / 10, 1));
  grid.visible = false;

  // Fitting establishes the new model's scale and orbit target. A named
  // preset must then be restored because fitCamera's fallback isometric
  // direction is only appropriate for the live Camera view.
  if (state.viewId === "camera") els.canvas.dataset.cameraView = "camera";
  else moveCameraToView(projectionView(state.viewId));
}

function currentOptions() {
  const creaseAngleDegrees = Number.parseFloat(els.fastCreaseAngleInput.value);
  return {
    projection_algorithm: els.fastBackendInput.checked ? "fast" : els.algoSelect.value,
    outline_algorithm: els.outlineAlgoSelect.value,
    mesh_linear_deflection: Number.parseFloat(els.linDeflInput.value) || 0.01,
    mesh_angular_deflection: Number.parseFloat(els.angDeflInput.value) || 0.5,
    hlr_angle_tolerance: Number.parseFloat(els.hlrTolInput.value) || 0.0174533,
    mesh_relative: false,
    mesh_deflection_mode: els.meshDeflectionModeSelect.value,
    mesh_deflection_coefficient: Number.parseFloat(els.deflCoeffInput.value) || 0.004,
    fast: {
      include_boundaries: els.fastBoundariesInput.checked,
      include_creases: els.fastCreasesInput.checked,
      include_silhouettes: els.fastSilhouettesInput.checked,
      include_hidden: els.fastHiddenInput.checked,
      suppress_coplanar_seams: els.fastCoplanarSeamsInput.checked,
      crease_angle_rad:
        ((Number.isFinite(creaseAngleDegrees) ? creaseAngleDegrees : 30) * Math.PI) / 180,
      weld_tolerance: Number.parseFloat(els.fastWeldToleranceInput.value) || 0.0000001,
      projected_tolerance: Number.parseFloat(els.fastProjectedToleranceInput.value) || 0.00000001,
      depth_tolerance: Number.parseFloat(els.fastDepthToleranceInput.value) || 0.0000001,
    },
    ...readEdgeFlags(),
  };
}

function ensureProjectionWorker(backend) {
  if (state.projectionWorker && state.workerBackend === backend) return state.projectionWorker;
  if (state.projectionWorker) {
    state.projectionWorker.terminate();
    state.projectionRequests.clear();
  }
  state.projectionWorker = new Worker("/examples/wasm/hlr_projection_worker.js");
  state.workerBackend = backend;
  state.projectionWorker.onmessage = (event) => {
    const { id, ok, error, timings } = event.data;
    const request = state.projectionRequests.get(id);
    if (!request) return;
    state.projectionRequests.delete(id);
    if (ok) request.resolve(event.data);
    else {
      const failure = new Error(error || "HLR worker failed.");
      failure.timings = timings || {};
      request.reject(failure);
    }
  };
  const rejectPending = (error) => {
    const failure =
      error instanceof Error ? error : new Error(String(error || "HLR worker failed."));
    for (const request of state.projectionRequests.values()) request.reject(failure);
    state.projectionRequests.clear();
  };
  state.projectionWorker.onerror = (event) => {
    rejectPending(event.message || "HLR worker startup failed.");
  };
  state.projectionWorker.onmessageerror = () => {
    rejectPending("HLR worker returned an unreadable message.");
  };
  return state.projectionWorker;
}

function cameraViewSpec() {
  // Build a ProjectionViewSpec from the live three.js camera. OCCT's HLR
  // wants the outward plane normal (pointing from origin toward the viewer),
  // which is (camera.position - target). For the "up" we use camera.up â€” the
  // orthogonalization happens inside gp_Ax2::SetYDirection.
  const dir = new THREE.Vector3().subVectors(camera.position, controls.target).normalize();
  const up = camera.up.clone().normalize();
  if (!Number.isFinite(dir.x) || dir.length() < 1e-6) {
    return { id: "camera", direction: [0, 0, 1], up: [0, 1, 0], mirrorX: false };
  }
  return {
    id: "camera",
    direction: [dir.x, dir.y, dir.z],
    up: [up.x, up.y, up.z],
    mirrorX: false,
  };
}

function projectionView(viewId) {
  if (viewId === "camera") return cameraViewSpec();
  return projectionViews.find((view) => view.id === viewId) || projectionViews[0];
}

function axisVectorLabel(vector) {
  for (const [id, candidate] of Object.entries(AXIS_VECTORS)) {
    if (candidate.every((component, index) => component === vector[index])) return id.toUpperCase();
  }
  return `[${vector.map((component) => component.toFixed(3)).join(", ")}]`;
}

function axesAreParallel(firstId, secondId) {
  return firstId.slice(1) === secondId.slice(1);
}

function defaultFrontAxis(topId) {
  return topId.slice(1) === "z" ? "+x" : "+z";
}

function syncPresetAxes({ reproject = false } = {}) {
  topAxisId = els.topAxisSelect.value;
  for (const option of els.frontAxisSelect.options)
    option.disabled = axesAreParallel(topAxisId, option.value);
  if (axesAreParallel(topAxisId, els.frontAxisSelect.value)) {
    els.frontAxisSelect.value = defaultFrontAxis(topAxisId);
  }
  frontAxisId = els.frontAxisSelect.value;
  projectionViews = buildProjectionViews();
  els.viewButtons.dataset.topAxis = topAxisId;
  els.viewButtons.dataset.frontAxis = frontAxisId;
  for (const button of els.viewButtons.querySelectorAll("button[data-view]")) {
    if (button.dataset.view === "camera") continue;
    const view = projectionViews.find((candidate) => candidate.id === button.dataset.view);
    button.title = `${view.label}: viewer ${axisVectorLabel(view.direction)}; page up ${axisVectorLabel(view.up)}.`;
  }
  state.projectionCache.clear();
  if (reproject && state.selectedModel && state.viewId !== "camera") {
    moveCameraToView(projectionView(state.viewId));
    renderProjectionForCurrentView().catch(console.error);
  }
}

function moveCameraToView(viewSpec) {
  // Place the camera along viewSpec.direction at the same distance from the
  // orbit target as it currently is, so toggling Top/Front/etc snaps the 3D
  // view to match the 2D projection direction. Camera button is excluded â€”
  // it reads the live camera position instead of moving it.
  if (!viewSpec || viewSpec.id === "camera") return;
  const target = controls.target.clone();
  const currentDistance = camera.position.distanceTo(target);
  const dir = new THREE.Vector3(
    viewSpec.direction[0],
    viewSpec.direction[1],
    viewSpec.direction[2],
  ).normalize();
  if (dir.length() < 1e-6) return;
  camera.up.set(viewSpec.up[0], viewSpec.up[1], viewSpec.up[2]).normalize();
  camera.position.copy(target).addScaledVector(dir, currentDistance);
  camera.lookAt(target);
  state.suppressCameraChange = true;
  controls.update();
  state.suppressCameraChange = false;
  els.canvas.dataset.cameraView = viewSpec.id;
  renderOnce();
}

function optionsCacheTag(opts) {
  const base = [
    opts.projection_algorithm,
    opts.outline_algorithm,
    opts.mesh_linear_deflection,
    opts.mesh_angular_deflection,
    opts.hlr_angle_tolerance,
    opts.mesh_relative,
    opts.mesh_deflection_mode,
    opts.mesh_deflection_coefficient,
  ];
  const edges = EDGE_FLAGS.map((name) => (opts[name] ? 1 : 0)).join("");
  return [...base, edges].join(":");
}

function modelCacheKey(model) {
  return model.cacheKey || model.name;
}

function projectionCacheKey(model, viewId, opts, backend) {
  return `${backend}|${optionsCacheTag(opts)}|${modelCacheKey(model)}|${viewId}`;
}

async function loadStepBuffer(model) {
  const key = modelCacheKey(model);
  const cached = state.stepCache.get(key);
  if (cached) return { stepBuffer: cached, stepFetchMs: 0, cachedStep: true };

  const fetchStart = performance.now();
  const response = await fetch(assetUrl(model.step));
  if (!response.ok) throw new Error(`STEP fetch failed: ${response.status}`);
  const stepBuffer = await response.arrayBuffer();
  const stepFetchMs = performance.now() - fetchStart;
  state.stepCache.set(key, stepBuffer);
  return { stepBuffer, stepFetchMs, cachedStep: false };
}

function runWorkerRequest(message, transferBuffer, backend) {
  const id = state.nextProjectionRequestId++;
  const worker = ensureProjectionWorker(backend);
  return new Promise((resolve, reject) => {
    state.projectionRequests.set(id, { resolve, reject });
    worker.postMessage({ id, backend, ...message }, [transferBuffer]);
  });
}

function runProjectionInWorker(stepBuffer, views, backend, options) {
  return runWorkerRequest(
    { operation: "project", stepBuffer, views, options },
    stepBuffer,
    backend,
  );
}

function runStepToGlbInWorker(stepBuffer, backend) {
  return runWorkerRequest({ operation: "step-to-glb", stepBuffer }, stepBuffer, backend);
}

async function loadProjection(model, viewId, opts, backend) {
  // Camera view depends on live orbit state; never cache it.
  const cacheable = viewId !== "camera";
  const cacheKey = projectionCacheKey(model, viewId, opts, backend);
  if (cacheable) {
    const cached = state.projectionCache.get(cacheKey);
    if (cached) {
      return {
        projection: cached,
        timings: { cached: true, stepFetchMs: 0, moduleMs: 0, hlrMs: 0, jsonParseMs: 0 },
      };
    }
  }

  const step = await loadStepBuffer(model);
  const transferBuffer = step.stepBuffer.slice(0);
  const workerResult = await runProjectionInWorker(
    transferBuffer,
    [projectionView(viewId)],
    backend,
    opts,
  );
  const projection = workerResult.projection;
  if (cacheable) state.projectionCache.set(cacheKey, projection);
  return {
    projection,
    timings: {
      ...workerResult.timings,
      stepFetchMs: step.stepFetchMs,
      cachedStep: step.cachedStep,
      cached: false,
    },
  };
}

async function renderProjectionForCurrentView() {
  const model = state.selectedModel;
  if (!model) return null;

  const loadId = state.activeLoad;
  const projectionId = ++state.activeProjection;
  const viewId = state.viewId;
  const backend = state.backend;
  const opts = currentOptions();
  const started = performance.now();
  state.exportReady = false;
  els.exportSvgButton.disabled = true;
  setStatus(
    `Projecting ${model.name}: ${viewId}`,
    true,
    "HLR runs in a browser Worker; 3D view stays interactive.",
  );
  const projectionResult = await loadProjection(model, viewId, opts, backend);
  if (
    loadId !== state.activeLoad ||
    projectionId !== state.activeProjection ||
    viewId !== state.viewId
  )
    return null;

  const counts = drawProjection(projectionResult.projection, viewId);
  els.projectionSvg.dataset.projectionAlgorithm = opts.projection_algorithm;
  els.projectionSvg.dataset.outlineAlgorithm = opts.outline_algorithm;
  const timings = projectionResult.timings || {};
  const hlrText = projectionTimingText(timings);
  const totalMs = performance.now() - started;
  els.sourceMetric.textContent = `STEP ${formatBytes(model.stepBytes)} - ${hlrText}${moduleTimingText(timings)}`;
  setStatus(`${model.name} ${viewId} ready - ${hlrText} - ${formatMs(totalMs)}`, false);
  return { counts, projectionResult };
}

function collectPrimitives(viewGeometry, mode) {
  const result = [];
  const addMode = (name, className) => {
    const geometry = viewGeometry.modes?.[name];
    if (!geometry) return;
    for (const segment of geometry.segments || [])
      result.push({ type: "segment", className, value: segment });
    for (const arc of geometry.arcs || []) result.push({ type: "arc", className, value: arc });
  };
  if (mode === "both") {
    addMode("detail", "detail");
    addMode("outline", "outline");
  } else addMode(mode, mode);
  if (els.bboxToggleInput.checked && mode !== "bbox") addMode("bbox", "bbox");
  return result;
}

function transformedPoints(primitive, mirrorX = false) {
  const sx = mirrorX ? -1 : 1;
  if (primitive.type === "segment") {
    const [x1, y1, x2, y2] = primitive.value;
    return [
      [sx * x1, -y1],
      [sx * x2, -y2],
    ];
  }
  const arc = primitive.value;
  const center = [sx * arc.center[0], -arc.center[1]];
  if (arc.full_circle) {
    return [
      [center[0] - arc.radius, center[1] - arc.radius],
      [center[0] + arc.radius, center[1] + arc.radius],
    ];
  }
  return [
    [sx * arc.start[0], -arc.start[1]],
    [sx * arc.end[0], -arc.end[1]],
    [center[0] - arc.radius, center[1] - arc.radius],
    [center[0] + arc.radius, center[1] + arc.radius],
  ];
}

function projectionBounds(primitives, mirrorX = false) {
  const bounds = { minX: Infinity, minY: Infinity, maxX: -Infinity, maxY: -Infinity };
  for (const primitive of primitives) {
    for (const [x, y] of transformedPoints(primitive, mirrorX)) {
      bounds.minX = Math.min(bounds.minX, x);
      bounds.minY = Math.min(bounds.minY, y);
      bounds.maxX = Math.max(bounds.maxX, x);
      bounds.maxY = Math.max(bounds.maxY, y);
    }
  }
  if (!Number.isFinite(bounds.minX)) return { minX: -1, minY: -1, maxX: 1, maxY: 1 };
  return bounds;
}

function drawProjection(projection, viewId = state.viewId, mode = state.mode) {
  const viewGeometry = (projection.views || []).find((view) => view.id === viewId);
  if (!viewGeometry) throw new Error(`Projection view not found: ${viewId}`);

  // Altium convention: horizontally mirror Back and Left ortho views so all 6 sides read left-to-right consistently.
  const viewSpec = projectionView(viewId);
  const mirrorX = !!viewSpec?.mirrorX;
  const sx = mirrorX ? -1 : 1;

  const primitives = collectPrimitives(viewGeometry, mode);
  const bounds = projectionBounds(primitives, mirrorX);
  const width = Math.max(bounds.maxX - bounds.minX, 1);
  const height = Math.max(bounds.maxY - bounds.minY, 1);
  const pad = Math.max(width, height) * 0.08;

  const svg = els.projectionSvg;
  state.displayedProjection = projection;
  state.displayedViewId = viewId;
  svg.replaceChildren();
  svg.setAttribute(
    "viewBox",
    `${bounds.minX - pad} ${bounds.minY - pad} ${width + pad * 2} ${height + pad * 2}`,
  );

  const defs = document.createElementNS("http://www.w3.org/2000/svg", "defs");
  const style = document.createElementNS("http://www.w3.org/2000/svg", "style");
  style.textContent = projectionAppearanceCss();
  defs.append(style);
  svg.append(defs);

  for (const primitive of primitives) {
    if (primitive.type === "segment") {
      const [x1, y1, x2, y2] = primitive.value;
      const line = document.createElementNS("http://www.w3.org/2000/svg", "line");
      line.setAttribute("class", primitive.className);
      line.setAttribute("x1", sx * x1);
      line.setAttribute("y1", -y1);
      line.setAttribute("x2", sx * x2);
      line.setAttribute("y2", -y2);
      svg.append(line);
    } else {
      const arc = primitive.value;
      if (arc.full_circle) {
        const circle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
        circle.setAttribute("class", primitive.className);
        circle.setAttribute("cx", sx * arc.center[0]);
        circle.setAttribute("cy", -arc.center[1]);
        circle.setAttribute("r", arc.radius);
        svg.append(circle);
      } else {
        const path = document.createElementNS("http://www.w3.org/2000/svg", "path");
        const largeArc = Math.abs(arc.extent_rad) > Math.PI ? 1 : 0;
        // Mirroring flips arc orientation, so flip the SVG sweep flag too.
        const sweep = arc.ccw ? (mirrorX ? 1 : 0) : mirrorX ? 0 : 1;
        path.setAttribute("class", primitive.className);
        path.setAttribute(
          "d",
          `M ${sx * arc.start[0]} ${-arc.start[1]} A ${arc.radius} ${arc.radius} 0 ${largeArc} ${sweep} ${sx * arc.end[0]} ${-arc.end[1]}`,
        );
        svg.append(path);
      }
    }
  }

  const detail = viewGeometry.modes.detail || { segments: [], arcs: [] };
  const outline = viewGeometry.modes.outline || { segments: [], arcs: [] };
  const bbox = viewGeometry.modes.bbox || { segments: [], arcs: [] };
  const detailCount = (detail.segments || []).length + (detail.arcs || []).length;
  const outlineCount = (outline.segments || []).length + (outline.arcs || []).length;
  const bboxCount = (bbox.segments || []).length + (bbox.arcs || []).length;
  els.projectionMetric.textContent = `${viewId} detail ${detailCount} outline ${outlineCount} bbox ${bboxCount}`;
  state.exportReady = primitives.length > 0;
  els.exportSvgButton.disabled = !state.exportReady;
  return { detailCount, outlineCount, bboxCount, primitiveCount: primitives.length };
}

function exportCurrentSvg() {
  if (!state.exportReady || !state.selectedModel) return;
  const source = els.projectionSvg;
  const clone = source.cloneNode(true);
  const viewBox = (clone.getAttribute("viewBox") || "0 0 1 1").split(/\s+/).map(Number);
  clone.removeAttribute("id");
  clone.setAttribute("xmlns", "http://www.w3.org/2000/svg");
  clone.setAttribute("width", `${Math.max(viewBox[2] || 1, 1)}mm`);
  clone.setAttribute("height", `${Math.max(viewBox[3] || 1, 1)}mm`);
  const title = document.createElementNS("http://www.w3.org/2000/svg", "title");
  title.textContent = `${state.selectedModel.name} - ${state.viewId} ${state.mode}`;
  clone.prepend(title);
  const serialized = `<?xml version="1.0" encoding="UTF-8"?>\n${new XMLSerializer().serializeToString(clone)}\n`;
  const url = URL.createObjectURL(new Blob([serialized], { type: "image/svg+xml" }));
  const stem = state.selectedModel.name
    .replace(/\.(?:step|stp)$/iu, "")
    .replace(/[^a-z0-9._-]+/giu, "-");
  const link = document.createElement("a");
  link.href = url;
  link.download = `${stem || "model"}-${state.viewId}-${state.mode}.svg`;
  link.click();
  window.setTimeout(() => URL.revokeObjectURL(url), 0);
}

function activateSegmented(container, attribute, value) {
  for (const button of container.querySelectorAll("button")) {
    button.classList.toggle("active", button.dataset[attribute] === value);
  }
}

async function loadSelectedModel() {
  const model = state.selectedModel;
  if (!model) return;

  const loadId = ++state.activeLoad;
  const viewId = state.viewId;
  const backend = state.backend;
  const opts = currentOptions();
  const totalStart = performance.now();
  state.exportReady = false;
  els.exportSvgButton.disabled = true;
  setStatus(
    `Loading ${model.name}`,
    true,
    "Loading the 3D mesh. HLR continues in a browser Worker.",
  );
  els.sourceMetric.textContent = `STEP ${formatBytes(model.stepBytes)}`;
  els.meshMetric.textContent = `GLB ${formatBytes(model.glbBytes)}`;
  els.projectionMetric.textContent = `${viewId} pending`;
  setValidation("RUNNING", false);

  try {
    let projectionPromise = null;
    if (viewId !== "camera") {
      projectionPromise = loadProjection(model, viewId, opts, backend)
        .then((projectionResult) => ({ projectionResult }))
        .catch((projectionError) => ({ projectionError }));
    }

    const glbResult = await loadGlb(model, loadId);
    if (loadId !== state.activeLoad) return;
    setStatus(
      `${model.name} 3D ready - projecting ${viewId}`,
      true,
      "Use the 3D controls while HLR finishes.",
    );
    if (viewId === "camera") {
      projectionPromise = loadProjection(model, viewId, opts, backend)
        .then((projectionResult) => ({ projectionResult }))
        .catch((projectionError) => ({ projectionError }));
    }

    const projectionOutcome = await projectionPromise;
    if (loadId !== state.activeLoad) return;
    if (viewId !== state.viewId) return;
    if (projectionOutcome.projectionError) throw projectionOutcome.projectionError;

    const projectionResult = projectionOutcome.projectionResult;
    const projection = projectionResult.projection;
    const counts = drawProjection(projection, viewId);
    const totalMs = performance.now() - totalStart;
    const timings = projectionResult.timings || {};
    const hlrText = projectionTimingText(timings);
    const moduleText = moduleTimingText(timings);
    const glbText = `GLB ${formatMs(glbResult.glbMs)}`;
    els.sourceMetric.textContent = `STEP ${formatBytes(model.stepBytes)} - ${hlrText}${moduleText}`;
    els.meshMetric.textContent = `GLB ${formatBytes(model.glbBytes)} - ${formatMs(glbResult.glbMs)}`;
    setStatus(`${model.name} ready - ${glbText} - ${hlrText} - total ${formatMs(totalMs)}`, false);

    if (state.validation) {
      await new Promise((resolve) => requestAnimationFrame(resolve));
      await new Promise((resolve) => requestAnimationFrame(resolve));
      const nonBlank = canvasHasPixels();
      const pass = nonBlank && counts.detailCount > 0 && counts.outlineCount > 0;
      setValidation(
        pass
          ? `PASS model=${model.name} detail=${counts.detailCount} outline=${counts.outlineCount} hlr=${formatMs(timings.hlrMs)} glb=${formatMs(glbResult.glbMs)}`
          : "FAIL render check",
        pass,
      );
    }
  } catch (error) {
    const message = error?.stack ? error.stack : String(error);
    setStatus(`Error: ${error?.message ? error.message : error}`, false);
    setValidation(`FAIL ${message}`, false);
    throw error;
  }
}

function loadGlb(model, loadId) {
  const started = performance.now();
  return new Promise((resolve, reject) => {
    loader.load(
      assetUrl(model.glb),
      (gltf) => {
        const glbMs = performance.now() - started;
        if (loadId !== state.activeLoad) {
          resolve({ loaded: false, glbMs });
          return;
        }
        clearModel();
        modelGroup.add(gltf.scene);
        fitCamera(gltf.scene);
        applyThreeMaterials(gltf.scene);
        els.modelMetric.textContent = model.name;
        els.meshMetric.textContent = `GLB ${formatBytes(model.glbBytes)} - ${formatMs(glbMs)}`;
        renderOnce();
        resolve({ loaded: true, glbMs });
      },
      undefined,
      reject,
    );
  });
}

function canvasHasPixels() {
  const canvas = renderer.domElement;
  const sample = document.createElement("canvas");
  sample.width = 16;
  sample.height = 16;
  const context = sample.getContext("2d", { willReadFrequently: true });
  context.drawImage(canvas, 0, 0, sample.width, sample.height);
  const pixels = context.getImageData(0, 0, sample.width, sample.height).data;
  for (let i = 0; i < pixels.length; i += 4) {
    if (pixels[i] !== 223 || pixels[i + 1] !== 229 || pixels[i + 2] !== 234) return true;
  }
  return false;
}

function appendModelOption(model, local = false) {
  const option = document.createElement("option");
  option.value = modelCacheKey(model);
  option.textContent = local ? `${model.name} (local)` : model.name;
  els.modelSelect.append(option);
}

async function openStepFile(file) {
  if (!file) return;
  if (!/\.(?:step|stp)$/iu.test(file.name)) throw new Error("Choose a .step or .stp model.");
  if (file.size > 256 * 1024 * 1024)
    throw new Error("STEP files larger than 256 MB are not supported by this demo.");

  els.openStepButton.disabled = true;
  state.exportReady = false;
  els.exportSvgButton.disabled = true;
  setStatus(
    `Opening ${file.name}`,
    true,
    "Converting the local STEP model to a 3D preview in the browser.",
  );
  try {
    const stepBuffer = await file.arrayBuffer();
    const cacheKey = `local:${++state.uploadCounter}:${file.name}`;
    state.stepCache.set(cacheKey, stepBuffer);
    const converted = await runStepToGlbInWorker(stepBuffer.slice(0), state.backend);
    const glbUrl = URL.createObjectURL(
      new Blob([converted.glbBuffer], { type: "model/gltf-binary" }),
    );
    state.uploadedObjectUrls.push(glbUrl);
    const model = {
      name: file.name,
      cacheKey,
      step: "",
      glb: glbUrl,
      stepBytes: file.size,
      glbBytes: converted.glbBuffer.byteLength,
      local: true,
    };
    state.models.push(model);
    appendModelOption(model, true);
    await selectModelByName(cacheKey);
  } finally {
    els.openStepButton.disabled = false;
    els.stepFileInput.value = "";
  }
}

function selectModelByName(name) {
  const model =
    state.models.find((item) => modelCacheKey(item) === name || item.name === name) ||
    state.models[0];
  state.selectedModel = model;
  els.modelSelect.value = modelCacheKey(model);
  return loadSelectedModel();
}

function reprojectCurrent({ force = true } = {}) {
  if (!state.selectedModel) return;
  const opts = currentOptions();
  if (force) {
    state.projectionCache.delete(
      projectionCacheKey(state.selectedModel, state.viewId, opts, state.backend),
    );
  }
  renderProjectionForCurrentView().catch(console.error);
}

function scheduleSettingsReprojection({ immediate = false } = {}) {
  window.clearTimeout(state.settingsReprojectTimer);
  if (immediate) {
    reprojectCurrent({ force: true });
    return;
  }
  state.settingsReprojectTimer = window.setTimeout(() => reprojectCurrent({ force: true }), 250);
}

function redrawCurrentProjection() {
  if (!state.selectedModel) return;
  if (state.displayedProjection && state.displayedViewId === state.viewId) {
    drawProjection(state.displayedProjection);
    return;
  }
  const opts = currentOptions();
  const cached = state.projectionCache.get(
    projectionCacheKey(state.selectedModel, state.viewId, opts, state.backend),
  );
  if (cached) drawProjection(cached);
  else renderProjectionForCurrentView().catch(console.error);
}

function scheduleCameraProjection() {
  if (!state.selectedModel || state.suppressCameraChange) return;
  els.canvas.dataset.cameraView = "camera";
  if (state.viewId !== "camera") {
    state.viewId = "camera";
    activateSegmented(els.viewButtons, "view", state.viewId);
  }
  window.clearTimeout(state.cameraReprojectTimer);
  state.cameraReprojectTimer = window.setTimeout(() => reprojectCurrent({ force: true }), 160);
}

async function init() {
  setStatus("Loading manifest", true, "Reading embedded model fixture list.");
  const response = await fetch(manifestUrl);
  if (!response.ok) throw new Error(`Manifest fetch failed: ${response.status}`);
  state.models = await response.json();

  for (const model of state.models) appendModelOption(model);

  els.workspace.addEventListener("geometer-demo-panel-layout-change", resizeRenderer);
  panelManager = new PanelManager(els.workspace);
  panelManager.register(
    {
      id: "settings",
      title: "Settings",
      side: "right",
      mount: (container) => container.append(els.settingsPanelContent),
    },
    "open",
  );
  panelManager.register({
    id: "three",
    title: "3D",
    side: "right",
    mount: (container) => container.append(els.threePanelContent),
  });

  for (const control of [
    els.materialModeSelect,
    els.shadingModeSelect,
    els.sidednessSelect,
    els.wireframeInput,
    els.backgroundColorInput,
    els.toneMappingSelect,
  ]) {
    control.addEventListener("change", syncThreeSettings);
  }
  for (const control of [
    els.ambientLightInput,
    els.keyLightInput,
    els.cameraLightInput,
    els.exposureInput,
  ]) {
    control.addEventListener("input", syncThreeSettings);
  }
  els.resetThreeButton.addEventListener("click", resetThreeDefaults);
  resetThreeDefaults();

  els.modelSelect.addEventListener("change", () => {
    selectModelByName(els.modelSelect.value).catch(console.error);
  });

  els.topAxisSelect.addEventListener("change", () => syncPresetAxes({ reproject: true }));
  els.frontAxisSelect.addEventListener("change", () => syncPresetAxes({ reproject: true }));

  els.openStepButton.addEventListener("click", () => els.stepFileInput.click());
  els.stepFileInput.addEventListener("change", () => {
    openStepFile(els.stepFileInput.files?.[0]).catch((error) => {
      setStatus(`Error: ${error?.message ? error.message : error}`, false);
      console.error(error);
    });
  });
  els.exportSvgButton.addEventListener("click", exportCurrentSvg);
  els.cameraLensSelect.addEventListener("change", () => setCameraLens(els.cameraLensSelect.value));
  window.addEventListener("pagehide", () => {
    window.clearTimeout(state.settingsReprojectTimer);
    for (const url of state.uploadedObjectUrls) URL.revokeObjectURL(url);
    panelManager?.destroy();
  });

  controls.addEventListener("change", scheduleCameraProjection);

  els.viewButtons.addEventListener("click", (event) => {
    const button = event.target.closest("button[data-view]");
    if (!button) return;
    state.viewId = button.dataset.view;
    activateSegmented(els.viewButtons, "view", state.viewId);
    // Snap the 3D camera to the equivalent direction so the 3D pane mirrors
    // the chosen ortho/ISO view. The "camera" button skips this.
    moveCameraToView(projectionView(state.viewId));
    renderProjectionForCurrentView().catch(console.error);
  });

  els.modeButtons.addEventListener("click", (event) => {
    const button = event.target.closest("button[data-mode]");
    if (!button) return;
    state.mode = button.dataset.mode;
    activateSegmented(els.modeButtons, "mode", state.mode);
    redrawCurrentProjection();
  });

  els.fastBackendInput.addEventListener("change", () => {
    syncGeometryControls();
    reprojectCurrent({ force: true });
  });

  els.algoSelect.addEventListener("change", () => {
    syncGeometryControls();
    reprojectCurrent({ force: true });
  });

  els.meshDeflectionModeSelect.addEventListener("change", () => {
    syncGeometryControls();
    reprojectCurrent({ force: true });
  });

  els.outlineAlgoSelect.addEventListener("change", () =>
    scheduleSettingsReprojection({ immediate: true }),
  );
  for (const input of [els.linDeflInput, els.angDeflInput, els.hlrTolInput, els.deflCoeffInput]) {
    input.addEventListener("input", () => scheduleSettingsReprojection());
    input.addEventListener("change", () => scheduleSettingsReprojection({ immediate: true }));
  }
  for (const input of [
    els.fastCreaseAngleInput,
    els.fastWeldToleranceInput,
    els.fastProjectedToleranceInput,
    els.fastDepthToleranceInput,
  ]) {
    input.addEventListener("input", () => scheduleSettingsReprojection());
    input.addEventListener("change", () => scheduleSettingsReprojection({ immediate: true }));
  }
  els.fastEdgeRow.addEventListener("change", () => reprojectCurrent({ force: true }));

  for (const input of [
    els.detailColorInput,
    els.detailWidthInput,
    els.detailStyleSelect,
    els.outlineColorInput,
    els.outlineWidthInput,
    els.outlineStyleSelect,
    els.bboxColorInput,
    els.bboxWidthInput,
    els.bboxStyleSelect,
  ]) {
    input.addEventListener("input", redrawCurrentProjection);
    input.addEventListener("change", redrawCurrentProjection);
  }

  els.bboxToggleInput.addEventListener("change", redrawCurrentProjection);

  els.edgePresetSelect.addEventListener("change", () => {
    if (els.edgePresetSelect.value === "custom") return;
    applyEdgePreset(els.edgePresetSelect.value);
    reprojectCurrent({ force: true });
  });

  // Manually toggling any raw OCCT category flips the convenience preset to Custom.
  els.edgeRow.addEventListener("change", (event) => {
    const cb = event.target.closest("input[data-edge]");
    if (!cb) return;
    els.edgePresetSelect.value = "custom";
    reprojectCurrent({ force: true });
  });

  els.resetGeometryButton.addEventListener("click", () => resetGeometryDefaults());

  const params = new URLSearchParams(window.location.search);
  resetGeometryDefaults({ reproject: false });
  const requestedLens = params.get("lens");
  setCameraLens(requestedLens === "perspective" ? "perspective" : "orthographic");
  const requestedTopAxis = (params.get("topAxis") || "+y").toLowerCase();
  const requestedFrontAxis = (params.get("frontAxis") || "+z").toLowerCase();
  if (AXIS_VECTORS[requestedTopAxis]) els.topAxisSelect.value = requestedTopAxis;
  if (AXIS_VECTORS[requestedFrontAxis]) els.frontAxisSelect.value = requestedFrontAxis;
  syncPresetAxes();
  const requestedView = params.get("view");
  if (requestedView === "camera" || projectionViews.some((view) => view.id === requestedView)) {
    state.viewId = requestedView;
    activateSegmented(els.viewButtons, "view", state.viewId);
  }
  const requestedMode = params.get("mode");
  if (["detail", "outline", "both"].includes(requestedMode)) {
    state.mode = requestedMode;
    activateSegmented(els.modeButtons, "mode", state.mode);
  }
  const requestedAlgo = params.get("algo");
  if (requestedAlgo === "fast") els.fastBackendInput.checked = true;
  else if (["poly", "exact"].includes(requestedAlgo)) {
    els.fastBackendInput.checked = false;
    els.algoSelect.value = requestedAlgo;
  }
  const requestedOutlineAlgo = params.get("outlineAlgo") || params.get("outline_algorithm");
  if (["mesh-shadow", "fast-mesh-shadow", "hlr-close"].includes(requestedOutlineAlgo))
    els.outlineAlgoSelect.value = requestedOutlineAlgo;
  const requestedMeshMode = params.get("meshMode") || params.get("mesh_deflection_mode");
  if (["bbox-relative", "absolute"].includes(requestedMeshMode))
    els.meshDeflectionModeSelect.value = requestedMeshMode;
  const requestedEdgePreset = params.get("edges") || params.get("profile");
  if (requestedEdgePreset && EDGE_PRESETS[requestedEdgePreset]) {
    els.edgePresetSelect.value = requestedEdgePreset;
    applyEdgePreset(requestedEdgePreset);
  } else {
    applyEdgePreset("detail");
  }
  const lin = params.get("lin");
  if (lin) els.linDeflInput.value = lin;
  const ang = params.get("ang");
  if (ang) els.angDeflInput.value = ang;
  const coef = params.get("coef");
  if (coef) els.deflCoeffInput.value = coef;
  syncGeometryControls();

  const requested = params.get("model");
  const index = Number.parseInt(params.get("index") || "0", 10);
  const initial =
    requested ||
    (state.models[Math.max(0, Math.min(index, state.models.length - 1))] || state.models[0]).name;

  resizeRenderer();
  animate();
  await selectModelByName(initial);
}

init().catch((error) => {
  const message = error?.stack ? error.stack : String(error);
  setStatus(`Error: ${error?.message ? error.message : error}`, false);
  setValidation(`FAIL ${message}`, false);
  console.error(error);
});
