import { createHash } from "node:crypto";
import { readFile } from "node:fs/promises";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "../..");
const htmlPath = join(root, "dist", "wasm", "demos", "analytic_polygon_pour_demo.html");
const htmlBytes = await readFile(htmlPath);
const html = htmlBytes.toString("utf8");
const licensePath = join(root, "docs", "design", "assets", "fonts", "JetBrainsMono", "OFL.txt");
const governedLicense = await readFile(licensePath, "utf8");

const exactly = (pattern, label) => {
  const matches = [...html.matchAll(pattern)];
  if (matches.length !== 1) throw new Error(`Expected one ${label}, found ${matches.length}.`);
  return matches[0];
};

exactly(/<style>[\s\S]*?<\/style>/gu, "inline stylesheet");
exactly(/<body data-wn-watermark="true">/gu, "canonical watermark opt-in");
const licenseMatch = exactly(
  /<script id="jetbrains-mono-license" type="text\/plain" data-sha256="([0-9a-f]{64})">([\s\S]*?)<\/script>/gu,
  "JetBrains Mono license",
);
if (licenseMatch[2] !== governedLicense) throw new Error("Embedded JetBrains Mono license drifted.");
if (createHash("sha256").update(licenseMatch[2]).digest("hex") !== licenseMatch[1])
  throw new Error("Embedded JetBrains Mono license digest drifted.");
const executableHtml = html.replace(licenseMatch[0], "");
if (/<link\b[^>]*rel="stylesheet"/u.test(html))
  throw new Error("Standalone demo retains an external stylesheet.");
if (/<script\b[^>]*\bsrc=/u.test(html)) throw new Error("Standalone demo retains a script src.");
if (/<script\b[^>]*type="(?:module|importmap)"/u.test(html))
  throw new Error("Standalone demo retains an ESM/import-map dependency.");
for (const forbidden of [
  'src="/',
  'href="/',
  "/dist/wasm/",
  "/examples/wasm/",
  "/docs/design/",
  "http://",
  "https://",
])
  if (executableHtml.includes(forbidden)) throw new Error(`Standalone demo contains ${forbidden}.`);
for (const required of [
  "data:font/woff2;base64,",
  "data:image/svg+xml;base64,",
  "JetBrains Mono",
  "--wn-accent:",
  "border-radius: 0 !important",
])
  if (!html.includes(required)) throw new Error(`Standalone theme omits ${required}.`);

const carrier = (id) => {
  const openingPattern = new RegExp(
    `<script id="${id}" type="application/octet-stream" data-encoding="base64" data-bytes="(\\d+)" data-sha256="([0-9a-f]{64})">`,
    "u",
  );
  const match = openingPattern.exec(html);
  if (match === null) throw new Error(`Standalone carrier ${id} is missing or malformed.`);
  const contentStart = match.index + match[0].length;
  const contentEnd = html.indexOf("</script>", contentStart);
  if (contentEnd < 0) throw new Error(`Standalone carrier ${id} is unterminated.`);
  const encoded = html.slice(contentStart, contentEnd);
  const bytes = Buffer.from(encoded, "base64");
  if (bytes.toString("base64") !== encoded)
    throw new Error(`Standalone carrier ${id} is not canonical base64.`);
  const digest = createHash("sha256").update(bytes).digest("hex");
  if (bytes.byteLength !== Number(match[1]) || digest !== match[2])
    throw new Error(`Standalone carrier ${id} metadata does not match its bytes.`);
  return bytes;
};

const wasm = carrier("geometer-analytic-wasm");
const governedWasm = await readFile(join(root, "dist", "wasm", "browser", "geometer.wasm"));
if (!wasm.equals(governedWasm)) throw new Error("Standalone WASM differs from browser distribution.");
const worker = carrier("geometer-analytic-worker").toString("utf8");
for (const required of [
  "createGeometerModule",
  "importScripts",
  "wn.geometer.worker_bootstrap.a0",
  "wn.geometer.wasm_worker.a0",
])
  if (!worker.includes(required)) throw new Error(`Standalone Worker omits ${required}.`);
if (/\b(?:import|export)\s/u.test(worker)) throw new Error("Standalone Worker is not a classic IIFE.");

console.log(
  JSON.stringify({
    bytes: htmlBytes.byteLength,
    sha256: createHash("sha256").update(htmlBytes).digest("hex"),
    wasmBytes: wasm.byteLength,
    workerBytes: Buffer.byteLength(worker),
  }),
);
