import { createHash } from "node:crypto";
import { readFile, stat } from "node:fs/promises";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "../..");
const site = join(root, "dist", "wasm", "demos", "illustration");
const manifest = JSON.parse(await readFile(join(site, "asset-manifest.json"), "utf8"));
if (
  manifest.schema !== "wn.geometer.single_html_site.a0" ||
  manifest.entrypoint !== "index.html" ||
  JSON.stringify(manifest.runtime_files) !== JSON.stringify(["index.html"])
)
  throw new Error("Illustration site does not declare exactly one runtime file.");

for (const item of manifest.files) {
  const path = resolve(site, item.path);
  if (path !== site && !path.startsWith(`${site}\\`) && !path.startsWith(`${site}/`))
    throw new Error(`Illustration static-site path escaped its root: ${item.path}`);
  const bytes = await readFile(path);
  const digest = createHash("sha256").update(bytes).digest("hex");
  if ((await stat(path)).size !== item.bytes || digest !== item.sha256)
    throw new Error(`Illustration static-site asset drifted: ${item.path}`);
}

const htmlBytes = await readFile(join(site, "index.html"));
const html = htmlBytes.toString("utf8");
if (/<script\b[^>]*\bsrc=|<link\b[^>]*rel="stylesheet"/u.test(html))
  throw new Error("Illustration HTML retains an external script or stylesheet.");
if (/<script\b[^>]*type="(?:module|importmap)"/u.test(html))
  throw new Error("Illustration HTML retains an ESM/import-map dependency.");
for (const required of [
  "GeometerIllustrationEmbedded",
  "GEOMETER_JS_B64",
  "GEOMETER_WASM_B64",
  "geometry.mesh_illustration.prototype.a0",
  'id="illustrationStepInput"',
  'id="illustrationDownloadSvg"',
  'id="illustrationDownloadScene"',
  'id="illustrationDownloadStyle"',
  'data-output="svg"',
  'data-output="canvas"',
  'data-view="camera"',
  'id="illustrationHlrOutline"',
  'id="illustrationHlrDetail"',
  'id="illustrationMeshQuality"',
  'id="illustrationLinearDeflection"',
  'id="illustrationAngularDeflection"',
  'id="illustrationHlrDeflection"',
  'id="illustrationHlrAngularDeflection"',
  'id="illustrationTriangleLimit"',
  'option value="lambert"',
  'id="illustrationBands" type="range" min="2" max="32"',
  "data:model/gltf-binary;base64,",
  "data:application/step;base64,",
  "data:font/woff2;base64,",
  "data:image/svg+xml;base64,",
])
  if (!html.includes(required)) throw new Error(`Illustration HTML omits ${required}.`);
if (html.includes("https://cdn.jsdelivr.net"))
  throw new Error("Illustration HTML retains its development CDN import map.");

const license = await readFile(join(root, "node_modules", "three", "LICENSE"), "utf8");
const licenseMatch = html.match(
  /<script id="three-license" type="text\/plain" data-sha256="([0-9a-f]{64})">([\s\S]*?)<\/script>/u,
);
if (licenseMatch === null || licenseMatch[2] !== license)
  throw new Error("Illustration HTML omits the exact Three.js license.");
if (createHash("sha256").update(licenseMatch[2]).digest("hex") !== licenseMatch[1])
  throw new Error("Illustration Three.js license digest drifted.");

console.log(JSON.stringify({ htmlBytes: htmlBytes.byteLength, files: manifest.files.length + 1 }));
