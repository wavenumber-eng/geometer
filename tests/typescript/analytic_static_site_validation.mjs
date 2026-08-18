import { createHash } from "node:crypto";
import { readFile, stat } from "node:fs/promises";
import { dirname, join, posix, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "../..");
const site = join(root, "dist", "wasm", "demos", "analytic-polygon-pour");
const manifest = JSON.parse(await readFile(join(site, "asset-manifest.json"), "utf8"));
if (manifest.schema !== "wn.geometer.static_site.a0" || !Array.isArray(manifest.files))
  throw new Error("Analytic static-site manifest is malformed.");

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
if (closure !== manifest.sha256) throw new Error("Analytic static-site closure digest drifted.");

const manifestPaths = new Set(manifest.files.map((item) => item.path));
if (!manifestPaths.has("JetBrainsMono-OFL.txt"))
  throw new Error("Analytic static site omits the JetBrains Mono OFL notice.");
const fontLicense = await readFile(join(site, "JetBrainsMono-OFL.txt"), "utf8");
const governedFontLicense = await readFile(
  join(root, "docs", "design", "assets", "fonts", "JetBrainsMono", "OFL.txt"),
  "utf8",
);
if (fontLicense !== governedFontLicense) throw new Error("Analytic static-site font license drifted.");
const indexText = await readFile(join(site, "index.html"), "utf8");
if ([...indexText.matchAll(/<link\b[^>]*rel="stylesheet"/gu)].length !== 1)
  throw new Error("Analytic demo must consume exactly one stylesheet.");
if (/\sstyle=/u.test(indexText)) throw new Error("Analytic demo contains inline style values.");
const importMapMatch = indexText.match(/<script type="importmap">([\s\S]*?)<\/script>/u);
if (importMapMatch === null) throw new Error("Analytic static site has no import map.");
const importMap = JSON.parse(importMapMatch[1]).imports;
const pending = ["index.html"];
const visited = new Set();
const resolveReference = (from, reference) => {
  if (/^(?:data:|#)/u.test(reference)) return undefined;
  const mapped = importMap[reference] ?? reference;
  if (mapped.startsWith("/")) throw new Error(`${from} contains root reference ${mapped}.`);
  if (!mapped.startsWith(".")) throw new Error(`${from} contains unmapped import ${mapped}.`);
  const target = posix.normalize(posix.join(posix.dirname(from), mapped));
  if (target.startsWith("../")) throw new Error(`${from} reference escapes the site: ${mapped}.`);
  if (!manifestPaths.has(target)) throw new Error(`${from} references missing asset ${target}.`);
  return target;
};
while (pending.length > 0) {
  const filename = pending.pop();
  if (filename === undefined || visited.has(filename)) continue;
  visited.add(filename);
  const text = await readFile(join(site, ...filename.split("/")), "utf8");
  const references = [];
  if (filename.endsWith(".html")) {
    for (const match of text.matchAll(/(?:src|href)="([^"]+)"/gu)) references.push(match[1]);
    for (const mapped of Object.values(importMap)) references.push(mapped);
  } else if (filename.endsWith(".css")) {
    for (const match of text.matchAll(/url\((?:"|')?([^"')]+)(?:"|')?\)/gu))
      references.push(match[1]);
  } else if (filename.endsWith(".js") && filename !== "geometer.js") {
    for (const pattern of [
      /(?:import|export)\s+(?:[^"']*?\s+from\s+)?["']([^"']+)["']/gu,
      /(?:import|importScripts|fetch)\(\s*["']([^"']+)["']/gu,
      /new Worker\(\s*["']([^"']+)["']/gu,
    ])
      for (const match of text.matchAll(pattern)) references.push(match[1]);
  }
  for (const reference of references) {
    const target = resolveReference(filename, reference);
    if (target !== undefined && /\.(?:css|html|js)$/u.test(target)) pending.push(target);
  }
}

for (const filename of [
  "index.html",
  "geometer_demo.css",
  "analytic_polygon_pour_demo.js",
  "analytic_polygon_pour_fixture.js",
  "analytic_polygon_pour_worker.js",
]) {
  const text = await readFile(join(site, filename), "utf8");
  if (/https?:\/\//u.test(text) || /["'(]\/dist\//u.test(text) || /["'(]\/examples\//u.test(text))
    throw new Error(`${filename} is not deploy-unchanged/offline.`);
}
const headers = await readFile(join(site, "_headers"), "utf8");
for (const directive of [
  "Content-Security-Policy",
  "X-Content-Type-Options",
  "Cache-Control: no-cache",
  "application/wasm",
])
  if (!headers.includes(directive)) throw new Error(`Static headers omit ${directive}.`);
if (/immutable/u.test(headers)) throw new Error("Stable demo asset names must revalidate.");

console.log(
  JSON.stringify({
    files: manifest.files.length + 1,
    reachable: visited.size,
    sha256: manifest.sha256,
  }),
);
