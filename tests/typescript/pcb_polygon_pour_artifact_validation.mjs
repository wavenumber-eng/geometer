import { createHash } from "node:crypto";
import { readFile, stat } from "node:fs/promises";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "../..");
const sourceRoot = join(root, "examples", "wasm");
const distRoot = join(root, "dist", "wasm", "demos");
const sourceHtml = await readFile(join(sourceRoot, "pcb_polygon_pour_demo.html"), "utf8");
const sourceTs = await readFile(join(sourceRoot, "pcb_polygon_pour_demo.ts"), "utf8");
if ([...sourceHtml.matchAll(/<link\b[^>]*rel="stylesheet"/gu)].length !== 1 || !sourceHtml.includes("geometer_demo.css"))
  throw new Error("PCB source page must use geometer_demo.css as its sole stylesheet.");
if (/<style\b|\sstyle\s*=/iu.test(sourceHtml)) throw new Error("PCB source page contains inline visual styles.");
if (/#[\da-f]{3,8}\b|\brgb\(/iu.test(sourceTs)) throw new Error("PCB TypeScript contains a hard-coded theme color.");
for (const required of ["createGeometerWorkerClient", "CommandHistory", "CommandRegistry", "ToolController", "Camera2D", "scheduleSolve", "pendingState"])
  if (!sourceTs.includes(required)) throw new Error(`PCB editor omits ${required}.`);
for (const required of [
  'globalCompositeOperation = "destination-out"',
  'globalCompositeOperation = "source-over"',
  'globalCompositeOperation = "destination-in"',
  "drawSolvedCopperFill(copperContext, base,",
  'themeColor("--pcb-composite-mask")',
  "SOLVE_TIMEOUT_MS = 15_000",
  "client.terminate()",
  "sortedLayerRecords",
  'themeColor("--pcb-thermal-fill")',
  'themeColor("--pcb-clearance")',
]) if (!sourceTs.includes(required)) throw new Error(`PCB exact compositor omits ${required}.`);
const routePreviewFunction = /function drawRoutePreview[\s\S]*?\n\}/u.exec(sourceTs)?.[0];
if (
  routePreviewFunction === undefined ||
  !routePreviewFunction.includes('themeColor("--pcb-clearance")') ||
  !routePreviewFunction.includes('themeColor("--pcb-route-preview")') ||
  routePreviewFunction.indexOf('themeColor("--pcb-clearance")') > routePreviewFunction.indexOf('themeColor("--pcb-route-preview")') ||
  !routePreviewFunction.includes("TRACE_WIDTH_NM + CLEARANCE_NM * 2")
) throw new Error("PCB route preview must draw its full white clearance envelope before the trace.");
const fillFunction = /function drawSolvedCopperFill[\s\S]*?\n\}/u.exec(sourceTs)?.[0];
if (fillFunction === undefined || !fillFunction.includes('target.fill("evenodd")') || fillFunction.includes("stroke("))
  throw new Error("Exact PCB layer passes must be fill-only.");
const subtractIndex = sourceTs.indexOf('globalCompositeOperation = "destination-out"');
const addIndex = sourceTs.indexOf('globalCompositeOperation = "source-over"', subtractIndex);
const maskIndex = sourceTs.indexOf('globalCompositeOperation = "destination-in"', addIndex);
const restoreIndex = sourceTs.indexOf('globalCompositeOperation = "source-over"', maskIndex);
if (!(subtractIndex >= 0 && subtractIndex < addIndex && addIndex < maskIndex && maskIndex < restoreIndex))
  throw new Error("PCB exact layer composition order drifted.");

const htmlPath = join(distRoot, "pcb_polygon_pour_demo.html");
const htmlBytes = await readFile(htmlPath);
const html = htmlBytes.toString("utf8");
if ([...html.matchAll(/<style>[\s\S]*?<\/style>/gu)].length !== 1)
  throw new Error("Standalone PCB demo must embed the one shared stylesheet exactly once.");
if (/<link\b[^>]*rel="stylesheet"|<script\b[^>]*\bsrc=|<script\b[^>]*type="(?:module|importmap)"/u.test(html))
  throw new Error("Standalone PCB demo retains an external or module dependency.");
const license = await readFile(join(root, "docs", "design", "assets", "fonts", "JetBrainsMono", "OFL.txt"), "utf8");
if (!html.includes(license) || !html.includes("data:font/woff2;base64,") || !html.includes("data:image/svg+xml;base64,"))
  throw new Error("Standalone PCB demo omits its governed font, license, or watermark.");
const carrier = (id) => {
  const opening = new RegExp(`<script id="${id}" type="application/octet-stream" data-encoding="base64" data-bytes="(\\d+)" data-sha256="([0-9a-f]{64})">`, "u").exec(html);
  if (opening === null) throw new Error(`Standalone PCB demo omits ${id}.`);
  const start = opening.index + opening[0].length;
  const end = html.indexOf("</script>", start);
  const encoded = html.slice(start, end);
  const bytes = Buffer.from(encoded, "base64");
  if (bytes.toString("base64") !== encoded || bytes.byteLength !== Number(opening[1]) || createHash("sha256").update(bytes).digest("hex") !== opening[2])
    throw new Error(`Standalone PCB carrier ${id} metadata drifted.`);
  return bytes;
};
const wasm = carrier("geometer-pcb-wasm");
const governedWasm = await readFile(join(root, "dist", "wasm", "browser", "geometer.wasm"));
if (!wasm.equals(governedWasm)) throw new Error("Standalone PCB WASM differs from the browser distribution.");
const worker = carrier("geometer-pcb-worker").toString("utf8");
for (const required of ["createGeometerModule", "importScripts", "wn.geometer.worker_bootstrap.a0", "wn.geometer.wasm_worker.a0"])
  if (!worker.includes(required)) throw new Error(`Standalone PCB Worker omits ${required}.`);
if (/\b(?:import|export)\s/u.test(worker)) throw new Error("Standalone PCB Worker is not a classic IIFE.");
for (const forbidden of ["http://", "https://", 'src="/', 'href="/', "/dist/wasm/", "/examples/wasm/"])
  if (html.replace(license, "").includes(forbidden)) throw new Error(`Standalone PCB demo contains ${forbidden}.`);

const site = join(distRoot, "pcb-polygon-pour");
const manifest = JSON.parse(await readFile(join(site, "asset-manifest.json"), "utf8"));
if (manifest.schema !== "wn.geometer.static_site.a0" || !Array.isArray(manifest.files))
  throw new Error("PCB hosted-site manifest is malformed.");
for (const item of manifest.files) {
  const path = resolve(site, item.path);
  if (path !== site && !path.startsWith(`${site}\\`) && !path.startsWith(`${site}/`))
    throw new Error(`PCB hosted-site path escaped: ${item.path}`);
  const bytes = await readFile(path);
  if ((await stat(path)).size !== item.bytes || createHash("sha256").update(bytes).digest("hex") !== item.sha256)
    throw new Error(`PCB hosted-site asset drifted: ${item.path}`);
}
const closure = createHash("sha256").update(manifest.files.map((item) => `${item.path}\0${item.sha256}\n`).join("")).digest("hex");
if (closure !== manifest.sha256) throw new Error("PCB hosted-site closure digest drifted.");
const hostedIndex = await readFile(join(site, "index.html"), "utf8");
if (/<style\b|\sstyle\s*=/iu.test(hostedIndex)) throw new Error("PCB hosted page contains inline visual styles.");
if ([...hostedIndex.matchAll(/<link\b[^>]*rel="stylesheet"/gu)].length !== 1)
  throw new Error("PCB hosted page does not retain exactly one stylesheet.");

console.log(JSON.stringify({ bytes: htmlBytes.byteLength, files: manifest.files.length + 1, sha256: manifest.sha256 }));
