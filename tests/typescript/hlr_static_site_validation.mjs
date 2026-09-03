import { createHash } from "node:crypto";
import { readFile, stat } from "node:fs/promises";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "../..");
const site = join(root, "dist", "wasm", "demos", "hlr");
const manifest = JSON.parse(await readFile(join(site, "asset-manifest.json"), "utf8"));
if (
  manifest.schema !== "wn.geometer.single_html_site.a0" ||
  manifest.entrypoint !== "index.html" ||
  JSON.stringify(manifest.runtime_files) !== JSON.stringify(["index.html"])
)
  throw new Error("HLR package does not declare exactly one runtime file.");

for (const item of manifest.files) {
  const path = resolve(site, item.path);
  if (path !== site && !path.startsWith(`${site}\\`) && !path.startsWith(`${site}/`))
    throw new Error(`Static-site path escaped its root: ${item.path}`);
  const bytes = await readFile(path);
  const actual = createHash("sha256").update(bytes).digest("hex");
  if ((await stat(path)).size !== item.bytes || actual !== item.sha256)
    throw new Error(`Static-site asset drifted: ${item.path}`);
}
const closure = createHash("sha256")
  .update(manifest.files.map((item) => `${item.path}\0${item.sha256}\n`).join(""))
  .digest("hex");
if (closure !== manifest.sha256) throw new Error("HLR static-site closure digest drifted.");
if (
  JSON.stringify(manifest.files.map((item) => item.path)) !==
  JSON.stringify(["_headers", "index.html"])
)
  throw new Error("HLR package contains a runtime companion asset.");

const htmlBytes = await readFile(join(site, "index.html"));
const html = htmlBytes.toString("utf8");
if (/<script\b[^>]*\bsrc=|<link\b[^>]*rel="stylesheet"/u.test(html))
  throw new Error("HLR single HTML retains an external script or stylesheet.");
if (/<script\b[^>]*type="(?:module|importmap)"/u.test(html))
  throw new Error("HLR single HTML retains an ESM/import-map dependency.");
for (const required of [
  "GeometerHlrDemoDeps",
  "GeometerDemoPanels",
  "TrackballControls",
  "GEOMETER_JS_B64",
  "GEOMETER_WASM_B64",
  "projectionWorkerSource",
  "data:application/step;base64,",
  "data:model/gltf-binary;base64,",
  "data:image/svg+xml;base64,",
  'id="stepFileInput"',
  'id="exportSvgButton"',
  'id="cameraLensSelect"',
  'id="topAxisSelect"',
  'id="frontAxisSelect"',
  'id="detailWidthInput"',
  'id="detailStyleSelect"',
  'id="outlineWidthInput"',
  'id="outlineStyleSelect"',
  'id="bboxWidthInput"',
  'id="bboxStyleSelect"',
  'id="meshDeflectionModeSelect"',
  'id="edgePresetSelect"',
  '<option value="fast">Fast vector (evaluation)</option>',
  '<option value="fast-mesh-shadow">Fast mesh shadow (evaluation)</option>',
  'id="resetGeometryButton"',
  'id="settingsPanelContent"',
  'id="threePanelContent"',
  'id="materialModeSelect"',
  'id="ambientLightInput"',
  'id="toneMappingSelect"',
  'id="resetThreeButton"',
  "gdm-panel-dock--right",
  "Reset geometry defaults",
  "Orthographic (matches HLR)",
  'http-equiv="Content-Security-Policy"',
])
  if (!html.includes(required)) throw new Error(`HLR single HTML omits ${required}.`);
if (html.includes("OrbitControls")) throw new Error("HLR single HTML retained OrbitControls.");
if (!html.includes("THREE.SRGBColorSpace") || !html.includes("THREE.MeshLambertMaterial"))
  throw new Error("HLR single HTML omits its Viz-style color/material path.");
if (!html.includes("new THREE.AmbientLight(0xffffff, 0.2)"))
  throw new Error("HLR single HTML omits the Viz-style default light rig.");
if (!html.includes('data-view="camera" class="active"') || !html.includes('viewId: "camera"'))
  throw new Error("HLR single HTML does not default to the fitted live camera view.");
if (!html.includes('cameraLens: "orthographic"'))
  throw new Error("HLR single HTML does not default the 3D lens to orthographic.");
if (!html.includes('let topAxisId = "+y"') || !html.includes('let frontAxisId = "+z"'))
  throw new Error("HLR single HTML omits the explicit preset-axis defaults.");
if (html.includes("detail muted") || html.includes(".muted"))
  throw new Error("Both mode still overrides the configured Detail appearance.");
if (html.includes('mesh_deflection_mode: "bbox-relative"'))
  throw new Error("HLR UI still hard-codes bbox-relative tessellation.");
if (html.includes('id="reprojectButton"'))
  throw new Error("HLR UI retained the manual Re-project button.");

const license = await readFile(join(root, "node_modules", "three", "LICENSE"), "utf8");
const licenseMatch = html.match(
  /<script id="three-license" type="text\/plain" data-sha256="([0-9a-f]{64})">([\s\S]*?)<\/script>/u,
);
if (licenseMatch === null || licenseMatch[2] !== license)
  throw new Error("HLR single HTML omits the exact Three.js license.");
if (createHash("sha256").update(licenseMatch[2]).digest("hex") !== licenseMatch[1])
  throw new Error("Embedded Three.js license digest drifted.");

const headers = await readFile(join(site, "_headers"), "utf8");
for (const directive of [
  "Content-Security-Policy",
  "X-Content-Type-Options",
  "Cache-Control: no-cache",
  "worker-src blob:",
  "frame-ancestors 'none'",
])
  if (!headers.includes(directive)) throw new Error(`HLR static headers omit ${directive}.`);
if (/https?:|immutable/u.test(headers))
  throw new Error("HLR static headers allow an external or stale asset.");

console.log(
  JSON.stringify({
    files: manifest.files.length + 1,
    runtimeFiles: manifest.runtime_files.length,
    htmlBytes: htmlBytes.byteLength,
    sha256: manifest.sha256,
  }),
);
