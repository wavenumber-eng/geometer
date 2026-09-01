import * as THREE from "three";
import { TrackballControls } from "three/addons/controls/TrackballControls.js";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";
import {
  type MeshIllustrationInput,
  type MeshIllustrationMaterial,
  type MeshIllustrationMesh,
  type MeshIllustrationScene,
  type MeshIllustrationStyle,
  type MeshIllustrationView,
  prepareMeshIllustration,
  type Rgb,
  renderMeshIllustrationCanvas,
  renderMeshIllustrationSvg,
  type Vec3,
} from "./mesh_illustration.js";

interface DemoModel {
  name: string;
  step: string;
  glb: string;
  stepBytes: number;
  glbBytes: number;
  cacheKey?: string;
  local?: boolean;
  glbBuffer?: ArrayBuffer;
}

interface EmbeddedDemoData {
  models: DemoModel[];
  workerSource: string;
}

declare global {
  interface Window {
    GeometerIllustrationEmbedded?: EmbeddedDemoData;
  }
}

type ViewId = "top" | "front" | "right" | "iso" | "camera";
type OutputId = "svg" | "canvas";

const DEMO_MODEL_ORDER = [
  "SOT-23.STEP",
  "SOIC-8-W.step",
  "sot223.stp",
  "TSOT-23-5.STEP",
  "BGA90-8X13mm.step",
] as const;

function required<T extends HTMLElement>(id: string): T {
  const value = document.getElementById(id);
  if (!(value instanceof HTMLElement)) throw new Error(`Missing required element #${id}.`);
  return value as T;
}

const els = {
  app: required<HTMLDivElement>("illustrationApp"),
  stepInput: required<HTMLInputElement>("illustrationStepInput"),
  openButton: required<HTMLButtonElement>("illustrationOpenButton"),
  modelSelect: required<HTMLSelectElement>("illustrationModelSelect"),
  viewButtons: required<HTMLSpanElement>("illustrationViewButtons"),
  outputButtons: required<HTMLSpanElement>("illustrationOutputButtons"),
  downloadSvg: required<HTMLButtonElement>("illustrationDownloadSvg"),
  downloadScene: required<HTMLButtonElement>("illustrationDownloadScene"),
  downloadStyle: required<HTMLButtonElement>("illustrationDownloadStyle"),
  modelPane: required<HTMLElement>("illustrationModelPane"),
  outputPane: required<HTMLElement>("illustrationOutputPane"),
  modelCanvas: required<HTMLCanvasElement>("illustrationModelCanvas"),
  illustrationCanvas: required<HTMLCanvasElement>("illustrationCanvas"),
  svgHost: required<HTMLDivElement>("illustrationSvgHost"),
  outputLabel: required<HTMLSpanElement>("illustrationOutputLabel"),
  busy: required<HTMLDivElement>("illustrationBusy"),
  busyText: required<HTMLDivElement>("illustrationBusyText"),
  status: required<HTMLSpanElement>("illustrationStatus"),
  timing: required<HTMLSpanElement>("illustrationTiming"),
  counts: required<HTMLParagraphElement>("illustrationCounts"),
  validation: required<HTMLPreElement>("illustrationValidation"),
  shading: required<HTMLSelectElement>("illustrationShading"),
  bands: required<HTMLInputElement>("illustrationBands"),
  bandsValue: required<HTMLOutputElement>("illustrationBandsValue"),
  ambient: required<HTMLInputElement>("illustrationAmbient"),
  ambientValue: required<HTMLOutputElement>("illustrationAmbientValue"),
  key: required<HTMLInputElement>("illustrationKey"),
  keyValue: required<HTMLOutputElement>("illustrationKeyValue"),
  rim: required<HTMLInputElement>("illustrationRim"),
  rimValue: required<HTMLOutputElement>("illustrationRimValue"),
  sourceColors: required<HTMLInputElement>("illustrationSourceColors"),
  fallback: required<HTMLInputElement>("illustrationFallback"),
  outlines: required<HTMLInputElement>("illustrationOutlines"),
  creases: required<HTMLInputElement>("illustrationCreases"),
  creaseAngle: required<HTMLInputElement>("illustrationCreaseAngle"),
  creaseAngleValue: required<HTMLOutputElement>("illustrationCreaseAngleValue"),
  outlineColor: required<HTMLInputElement>("illustrationOutlineColor"),
  outlineWidth: required<HTMLInputElement>("illustrationOutlineWidth"),
  outlineWidthValue: required<HTMLOutputElement>("illustrationOutlineWidthValue"),
  doubleSided: required<HTMLInputElement>("illustrationDoubleSided"),
  background: required<HTMLInputElement>("illustrationBackground"),
  transparent: required<HTMLInputElement>("illustrationTransparent"),
};

const state: {
  models: DemoModel[];
  model: DemoModel | null;
  root: THREE.Object3D | null;
  scene: MeshIllustrationScene | null;
  style: MeshIllustrationStyle | null;
  svg: string;
  view: ViewId;
  output: OutputId;
  activeLoad: number;
  renderTimer: number;
  worker: Worker | null;
  workerUrl: string;
  workerRequest: number;
  pendingWorker: Map<
    number,
    {
      resolve: (value: { buffer: ArrayBuffer; glbMs: number }) => void;
      reject: (reason: Error) => void;
    }
  >;
  uploadedUrls: string[];
  validation: boolean;
  suppressCamera: boolean;
  prepareGeneration: number;
} = {
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
  validation: new URLSearchParams(window.location.search).get("validation") === "1",
  suppressCamera: false,
  prepareGeneration: 0,
};

const renderer = new THREE.WebGLRenderer({
  canvas: els.modelCanvas,
  antialias: true,
  preserveDrawingBuffer: true,
});
renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.setClearColor(0xf7f2df, 1);

const threeScene = new THREE.Scene();
threeScene.background = new THREE.Color(0xf7f2df);
const camera = new THREE.OrthographicCamera(-1, 1, 1, -1, 0.001, 100_000);
const controls = new TrackballControls(camera, renderer.domElement);
controls.rotateSpeed = 1;
controls.zoomSpeed = 1.2;
controls.panSpeed = 0.3;
controls.dynamicDampingFactor = 0.16;
controls.staticMoving = false;
threeScene.add(new THREE.HemisphereLight(0xffffff, 0x667078, 1.25));
const threeKey = new THREE.DirectionalLight(0xffffff, 1.7);
threeKey.position.set(4, 8, 6);
threeScene.add(threeKey);
const modelGroup = new THREE.Group();
threeScene.add(modelGroup);
const loader = new GLTFLoader();

function setBusy(message: string | null): void {
  els.busy.hidden = message === null;
  if (message) els.busyText.textContent = message;
  els.app.dataset.state = message ? "loading" : "ready";
}

function setStatus(message: string): void {
  els.status.textContent = message;
}

function formatBytes(value: number): string {
  return value >= 1024 * 1024
    ? `${(value / (1024 * 1024)).toFixed(2)} MiB`
    : `${Math.max(1, Math.round(value / 1024))} KiB`;
}

function formatMs(value: number): string {
  return value >= 1000 ? `${(value / 1000).toFixed(2)} s` : `${Math.max(1, Math.round(value))} ms`;
}

function colorFromHex(value: string): Rgb {
  const normalized = value.replace(/^#/u, "");
  const numeric = Number.parseInt(normalized, 16);
  return [((numeric >> 16) & 255) / 255, ((numeric >> 8) & 255) / 255, (numeric & 255) / 255];
}

function numberInput(input: HTMLInputElement, output: HTMLOutputElement, digits: number): number {
  const value = Number.parseFloat(input.value);
  output.value = Number.isFinite(value) ? value.toFixed(digits) : "0";
  return Number.isFinite(value) ? value : 0;
}

function currentStyle(): MeshIllustrationStyle {
  const bands = Math.round(numberInput(els.bands, els.bandsValue, 0));
  const ambient = numberInput(els.ambient, els.ambientValue, 2);
  const keyIntensity = numberInput(els.key, els.keyValue, 2);
  const rimAmount = numberInput(els.rim, els.rimValue, 2);
  const creaseAngleDegrees = numberInput(els.creaseAngle, els.creaseAngleValue, 0);
  const outlineWidth = numberInput(els.outlineWidth, els.outlineWidthValue, 3);
  return {
    shading: els.shading.value as MeshIllustrationStyle["shading"],
    ambient,
    keyIntensity,
    lightDirection: [0.35, 0.8, 0.48],
    bands,
    sourceColors: els.sourceColors.checked,
    fallbackColor: colorFromHex(els.fallback.value),
    background: els.background.value,
    transparentBackground: els.transparent.checked,
    showOutlines: els.outlines.checked,
    showCreases: els.creases.checked,
    creaseAngleDegrees,
    outlineColor: els.outlineColor.value,
    creaseColor: els.outlineColor.value,
    outlineWidth,
    creaseWidth: outlineWidth * 0.55,
    doubleSided: els.doubleSided.checked,
    rimAmount,
  };
}

function activateButtons(container: HTMLElement, key: "view" | "output", value: string): void {
  for (const button of Array.from(container.querySelectorAll<HTMLButtonElement>("button")))
    button.classList.toggle("active", button.dataset[key] === value);
}

function assetUrl(path: string): string {
  if (path.startsWith("data:") || path.startsWith("blob:")) return path;
  return `/${path.split("/").map(encodeURIComponent).join("/")}`;
}

async function fetchArrayBuffer(path: string): Promise<ArrayBuffer> {
  const response = await fetch(assetUrl(path));
  if (!response.ok) throw new Error(`Asset fetch failed (${response.status}): ${path}`);
  return response.arrayBuffer();
}

function clearModel(): void {
  while (modelGroup.children.length > 0) {
    const child = modelGroup.children.pop();
    child?.traverse((node) => {
      if (node instanceof THREE.Mesh) {
        node.geometry.dispose();
        const materials = Array.isArray(node.material) ? node.material : [node.material];
        for (const material of materials) material.dispose();
      }
    });
  }
  state.root = null;
}

function resize(): void {
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

function fitCamera(root: THREE.Object3D): void {
  root.updateMatrixWorld(true);
  let bounds = new THREE.Box3().setFromObject(root);
  const center = bounds.getCenter(new THREE.Vector3());
  root.position.sub(center);
  root.updateMatrixWorld(true);
  bounds = new THREE.Box3().setFromObject(root);
  const sphere = bounds.getBoundingSphere(new THREE.Sphere());
  const radius = Math.max(sphere.radius, 0.001);
  camera.userData.halfHeight = radius * 1.12;
  camera.near = Math.max(radius / 1000, 0.0001);
  camera.far = Math.max(radius * 20, 10);
  controls.target.set(0, 0, 0);
  controls.minDistance = radius * 0.05;
  controls.maxDistance = radius * 20;
  moveCameraToView(state.view === "camera" ? "iso" : state.view);
  resize();
}

function namedView(view: Exclude<ViewId, "camera">): MeshIllustrationView {
  if (view === "top") return { direction: [0, 1, 0], up: [0, 0, -1] };
  if (view === "front") return { direction: [0, 0, 1], up: [0, 1, 0] };
  if (view === "right") return { direction: [1, 0, 0], up: [0, 1, 0] };
  return { direction: normalizeTuple([1, 1, 1]), up: [0, 1, 0] };
}

function normalizeTuple(value: Vec3): Vec3 {
  const length = Math.hypot(value[0], value[1], value[2]) || 1;
  return [value[0] / length, value[1] / length, value[2] / length];
}

function currentView(): MeshIllustrationView {
  if (state.view !== "camera") return namedView(state.view);
  const direction = new THREE.Vector3().subVectors(camera.position, controls.target).normalize();
  const up = camera.up.clone().normalize();
  return { direction: [direction.x, direction.y, direction.z], up: [up.x, up.y, up.z] };
}

function moveCameraToView(viewId: Exclude<ViewId, "camera">): void {
  const view = namedView(viewId);
  const distance = Math.max(Number(camera.userData.halfHeight ?? 1) * 4, 1);
  state.suppressCamera = true;
  camera.position.set(
    view.direction[0] * distance,
    view.direction[1] * distance,
    view.direction[2] * distance,
  );
  camera.up.set(view.up[0], view.up[1], view.up[2]);
  controls.target.set(0, 0, 0);
  camera.lookAt(controls.target);
  controls.update();
  state.suppressCamera = false;
}

function materialColor(material: THREE.Material): MeshIllustrationMaterial {
  const candidate = material as THREE.Material & { color?: THREE.Color; opacity?: number };
  const color = candidate.color ?? new THREE.Color(0xb8bdc7);
  const toSrgb = (value: number): number =>
    value <= 0.0031308 ? value * 12.92 : 1.055 * value ** (1 / 2.4) - 0.055;
  return {
    ...(material.name ? { name: material.name } : {}),
    // Three stores material colors in linear-sRGB; the generic illustration
    // contract accepts sRGB so that file adapters and UI colors agree.
    color: [toSrgb(color.r), toSrgb(color.g), toSrgb(color.b)],
    opacity: Number.isFinite(candidate.opacity) ? candidate.opacity : 1,
  };
}

function meshInput(root: THREE.Object3D): MeshIllustrationInput {
  const meshes: MeshIllustrationMesh[] = [];
  root.updateMatrixWorld(true);
  let meshIndex = 0;
  root.traverse((node) => {
    if (!(node instanceof THREE.Mesh) || !node.visible) return;
    const position = node.geometry.getAttribute("position");
    if (!position || position.itemSize < 3) return;
    const positions = new Float32Array(position.count * 3);
    for (let index = 0; index < position.count; index += 1) {
      positions[index * 3] = position.getX(index);
      positions[index * 3 + 1] = position.getY(index);
      positions[index * 3 + 2] = position.getZ(index);
    }
    const sourceNormal = node.geometry.getAttribute("normal");
    let normals: Float32Array | undefined;
    if (sourceNormal && sourceNormal.itemSize >= 3 && sourceNormal.count === position.count) {
      normals = new Float32Array(sourceNormal.count * 3);
      for (let index = 0; index < sourceNormal.count; index += 1) {
        normals[index * 3] = sourceNormal.getX(index);
        normals[index * 3 + 1] = sourceNormal.getY(index);
        normals[index * 3 + 2] = sourceNormal.getZ(index);
      }
    }
    const sourceIndex = node.geometry.getIndex();
    let indices: Uint32Array | undefined;
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
  if (meshes.length === 0) throw new Error("The loaded model contains no triangle meshes.");
  return { meshes };
}

function drawSvg(svg: string): void {
  const parsed = new DOMParser().parseFromString(svg, "image/svg+xml");
  const error = parsed.querySelector("parsererror");
  if (error)
    throw new Error(`Generated SVG failed to parse: ${error.textContent ?? "unknown error"}`);
  els.svgHost.replaceChildren(document.importNode(parsed.documentElement, true));
}

function redrawStyle(): void {
  if (!state.scene) return;
  const style = currentStyle();
  state.style = style;
  const rendered = renderMeshIllustrationSvg(
    state.scene,
    style,
    `${state.model?.name ?? "model"} ${state.view} illustration`,
  );
  state.svg = rendered.svg;
  drawSvg(rendered.svg);
  const context = els.illustrationCanvas.getContext("2d");
  if (!context) throw new Error("Canvas2D is unavailable.");
  const canvasStats = renderMeshIllustrationCanvas(context, state.scene, style);
  els.counts.textContent = `${rendered.stats.triangles.toLocaleString()} visible triangles / ${rendered.stats.outlines.toLocaleString()} outlines / ${rendered.stats.creases.toLocaleString()} creases / ${state.scene.warnings.length} warnings`;
  els.downloadSvg.disabled = false;
  els.downloadScene.disabled = false;
  els.downloadStyle.disabled = false;
  els.svgHost.classList.toggle("hidden", state.output !== "svg");
  els.illustrationCanvas.classList.toggle("hidden", state.output !== "canvas");
  els.outputLabel.textContent = `2D ${state.output.toUpperCase()} / ${style.shading.toUpperCase()} / ${canvasStats.commands.toLocaleString()} DRAWS`;
  els.outputPane.dataset.output = state.output;
  els.outputPane.dataset.shading = style.shading;
  threeScene.background = new THREE.Color(style.background);
  renderer.setClearColor(style.background, 1);
}

async function prepareIllustration(reason: string): Promise<void> {
  if (!state.root) return;
  const started = performance.now();
  setBusy(`Preparing ${reason} illustration...`);
  await new Promise<void>((resolve) => requestAnimationFrame(() => resolve()));
  try {
    const input = meshInput(state.root);
    state.scene = prepareMeshIllustration(input, currentView(), {
      maxTriangles: 140_000,
      weldTolerance: 1e-5,
    });
    state.prepareGeneration += 1;
    els.outputPane.dataset.view = state.view;
    els.outputPane.dataset.direction = state.scene.view.direction
      .map((value) => value.toFixed(6))
      .join(",");
    els.outputPane.dataset.triangles = String(state.scene.stats.projectedTriangles);
    els.outputPane.dataset.prepareGeneration = String(state.prepareGeneration);
    redrawStyle();
    const elapsed = performance.now() - started;
    els.timing.textContent = `prepare ${formatMs(elapsed)} / ${state.scene.stats.sourceTriangles.toLocaleString()} triangles`;
    setStatus(
      `${state.model?.name ?? "Model"} / ${state.view} / ${state.output.toUpperCase()} ready`,
    );
    await validateIfRequested();
  } finally {
    setBusy(null);
  }
}

function scheduleCameraIllustration(): void {
  if (state.suppressCamera || !state.root) return;
  state.view = "camera";
  activateButtons(els.viewButtons, "view", state.view);
  window.clearTimeout(state.renderTimer);
  state.renderTimer = window.setTimeout(() => {
    prepareIllustration("camera").catch(showError);
  }, 180);
}

function renderLoop(): void {
  requestAnimationFrame(renderLoop);
  controls.update();
  renderer.render(threeScene, camera);
}

function loadGlb(buffer: ArrayBuffer): Promise<THREE.Object3D> {
  return new Promise((resolve, reject) => {
    loader.parse(buffer, "", (gltf) => resolve(gltf.scene), reject);
  });
}

async function selectModel(model: DemoModel): Promise<void> {
  const loadId = ++state.activeLoad;
  state.model = model;
  els.modelSelect.value = model.cacheKey ?? model.name;
  setBusy(`Loading ${model.name}...`);
  setStatus(`Loading ${model.name}`);
  const started = performance.now();
  try {
    const buffer = model.glbBuffer ?? (await fetchArrayBuffer(model.glb));
    const root = await loadGlb(buffer.slice(0));
    if (loadId !== state.activeLoad) return;
    clearModel();
    modelGroup.add(root);
    state.root = root;
    fitCamera(root);
    await prepareIllustration("mesh");
    const elapsed = performance.now() - started;
    els.timing.textContent = `GLB ${formatBytes(buffer.byteLength)} / total ${formatMs(elapsed)}`;
  } catch (error) {
    showError(error);
  } finally {
    if (loadId === state.activeLoad) setBusy(null);
  }
}

function workerUrl(): string {
  const embedded = window.GeometerIllustrationEmbedded;
  if (!embedded) return "/examples/wasm/illustration_step_worker.js";
  if (!state.workerUrl)
    state.workerUrl = URL.createObjectURL(
      new Blob([embedded.workerSource], { type: "text/javascript" }),
    );
  return state.workerUrl;
}

function stepWorker(): Worker {
  if (state.worker) return state.worker;
  const worker = new Worker(workerUrl());
  worker.onmessage = (event: MessageEvent) => {
    const data = event.data as {
      id: number;
      ok: boolean;
      glbBuffer?: ArrayBuffer;
      error?: string;
      timings?: { glbMs?: number };
    };
    const pending = state.pendingWorker.get(data.id);
    if (!pending) return;
    state.pendingWorker.delete(data.id);
    if (data.ok && data.glbBuffer)
      pending.resolve({ buffer: data.glbBuffer, glbMs: data.timings?.glbMs ?? 0 });
    else pending.reject(new Error(data.error ?? "STEP conversion Worker failed."));
  };
  const failWorker = (error: Error): void => {
    for (const pending of state.pendingWorker.values()) pending.reject(error);
    state.pendingWorker.clear();
    worker.terminate();
    if (state.worker === worker) state.worker = null;
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

function convertStep(stepBuffer: ArrayBuffer): Promise<{ buffer: ArrayBuffer; glbMs: number }> {
  const id = ++state.workerRequest;
  return new Promise((resolve, reject) => {
    state.pendingWorker.set(id, { resolve, reject });
    try {
      stepWorker().postMessage({ id, stepBuffer }, [stepBuffer]);
    } catch (error) {
      state.pendingWorker.delete(id);
      reject(error instanceof Error ? error : new Error(String(error)));
    }
  });
}

function appendModel(model: DemoModel): void {
  const option = document.createElement("option");
  option.value = model.cacheKey ?? model.name;
  option.textContent = model.local ? `${model.name} (local)` : model.name;
  els.modelSelect.append(option);
}

async function openStep(file: File): Promise<void> {
  if (!/\.(?:step|stp)$/iu.test(file.name)) throw new Error("Choose a .step or .stp model.");
  if (file.size > 256 * 1024 * 1024)
    throw new Error("STEP files larger than 256 MiB are not supported.");
  els.openButton.disabled = true;
  setBusy(`Converting ${file.name} to a mesh in Geometer WASM...`);
  try {
    const converted = await convertStep(await file.arrayBuffer());
    const model: DemoModel = {
      name: file.name,
      cacheKey: `local:${Date.now()}:${file.name}`,
      step: "",
      glb: "",
      stepBytes: file.size,
      glbBytes: converted.buffer.byteLength,
      glbBuffer: converted.buffer,
      local: true,
    };
    state.models.push(model);
    appendModel(model);
    await selectModel(model);
    els.timing.textContent = `STEP mesh ${formatMs(converted.glbMs)} / ${formatBytes(model.glbBytes)}`;
  } finally {
    els.openButton.disabled = false;
    els.stepInput.value = "";
    setBusy(null);
  }
}

function download(name: string, content: BlobPart, type: string): void {
  const url = URL.createObjectURL(new Blob([content], { type }));
  const link = document.createElement("a");
  link.href = url;
  link.download = name;
  link.click();
  window.setTimeout(() => URL.revokeObjectURL(url), 0);
}

function fileStem(): string {
  return (state.model?.name ?? "model")
    .replace(/\.(?:step|stp|glb|gltf)$/iu, "")
    .replace(/[^a-z0-9._-]+/giu, "-");
}

function sceneJson(): string {
  return `${JSON.stringify(state.scene, null, 2)}\n`;
}

function styleJson(): string {
  return `${JSON.stringify({ schema: "geometry.illustration_style.prototype.a0", ...state.style }, null, 2)}\n`;
}

function canvasHasContent(canvas: HTMLCanvasElement): boolean {
  const sample = document.createElement("canvas");
  sample.width = 16;
  sample.height = 16;
  const context = sample.getContext("2d", { willReadFrequently: true });
  if (!context) return false;
  context.drawImage(canvas, 0, 0, 16, 16);
  const pixels = context.getImageData(0, 0, 16, 16).data;
  let first: string | null = null;
  for (let index = 0; index < pixels.length; index += 4) {
    const current = `${pixels[index]},${pixels[index + 1]},${pixels[index + 2]},${pixels[index + 3]}`;
    if (first === null) first = current;
    else if (current !== first) return true;
  }
  return false;
}

async function validateIfRequested(): Promise<void> {
  if (!state.validation || !state.scene) return;
  await new Promise<void>((resolve) => requestAnimationFrame(() => resolve()));
  await new Promise<void>((resolve) => requestAnimationFrame(() => resolve()));
  const svgTriangles = els.svgHost.querySelectorAll("polygon").length;
  const pass =
    state.scene.stats.projectedTriangles > 0 &&
    svgTriangles > 0 &&
    canvasHasContent(els.modelCanvas) &&
    canvasHasContent(els.illustrationCanvas);
  const result = pass
    ? `PASS triangles=${state.scene.stats.projectedTriangles} svg=${svgTriangles} view=${state.view}`
    : "FAIL nonblank illustration validation";
  els.validation.textContent = result;
  document.title = pass ? result : `FAIL ${result}`;
}

function showError(error: unknown): void {
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

function wireEvents(): void {
  new ResizeObserver(resize).observe(els.modelPane);
  new ResizeObserver(resize).observe(els.outputPane);
  controls.addEventListener("change", scheduleCameraIllustration);
  els.openButton.addEventListener("click", () => els.stepInput.click());
  els.stepInput.addEventListener("change", () => {
    const file = els.stepInput.files?.[0];
    if (file) openStep(file).catch(showError);
  });
  els.modelSelect.addEventListener("change", () => {
    const selected = state.models.find(
      (model) => (model.cacheKey ?? model.name) === els.modelSelect.value,
    );
    if (selected) selectModel(selected).catch(showError);
  });
  els.viewButtons.addEventListener("click", (event) => {
    const button = (event.target as Element | null)?.closest<HTMLButtonElement>(
      "button[data-view]",
    );
    if (!button) return;
    state.view = button.dataset.view as ViewId;
    activateButtons(els.viewButtons, "view", state.view);
    if (state.view !== "camera") moveCameraToView(state.view);
    prepareIllustration(state.view).catch(showError);
  });
  els.outputButtons.addEventListener("click", (event) => {
    const button = (event.target as Element | null)?.closest<HTMLButtonElement>(
      "button[data-output]",
    );
    if (!button) return;
    state.output = button.dataset.output as OutputId;
    activateButtons(els.outputButtons, "output", state.output);
    redrawStyle();
  });
  for (const control of [
    els.shading,
    els.sourceColors,
    els.fallback,
    els.outlines,
    els.creases,
    els.outlineColor,
    els.doubleSided,
    els.background,
    els.transparent,
  ]) {
    control.addEventListener("change", redrawStyle);
  }
  for (const control of [
    els.bands,
    els.ambient,
    els.key,
    els.rim,
    els.creaseAngle,
    els.outlineWidth,
  ]) {
    control.addEventListener("input", redrawStyle);
  }
  els.downloadSvg.addEventListener("click", () =>
    download(`${fileStem()}-${state.view}.svg`, state.svg, "image/svg+xml"),
  );
  els.downloadScene.addEventListener("click", () =>
    download(`${fileStem()}-${state.view}.illustration.json`, sceneJson(), "application/json"),
  );
  els.downloadStyle.addEventListener("click", () =>
    download(`${fileStem()}.illustration-style.json`, styleJson(), "application/json"),
  );
  window.addEventListener("beforeunload", () => {
    state.worker?.terminate();
    if (state.workerUrl) URL.revokeObjectURL(state.workerUrl);
    for (const url of state.uploadedUrls) URL.revokeObjectURL(url);
  });
}

async function loadModels(): Promise<DemoModel[]> {
  if (window.GeometerIllustrationEmbedded) return window.GeometerIllustrationEmbedded.models;
  const response = await fetch("/tests/fixtures/embedded_models_manifest.json");
  if (!response.ok) throw new Error(`Model manifest fetch failed: ${response.status}`);
  const models = (await response.json()) as DemoModel[];
  const modelByName = new Map(models.map((model) => [model.name, model]));
  const selected = DEMO_MODEL_ORDER.map((name) => modelByName.get(name)).filter(
    (model): model is DemoModel => model !== undefined,
  );
  return selected.length > 0 ? selected : models.filter((model) => Boolean(model.glb)).slice(0, 5);
}

async function init(): Promise<void> {
  wireEvents();
  resize();
  renderLoop();
  setBusy("Loading bundled models...");
  state.models = await loadModels();
  if (state.models.length === 0) throw new Error("No bundled mesh fixtures are available.");
  for (const model of state.models) appendModel(model);
  await selectModel(state.models[0] as DemoModel);
}

init().catch(showError);
