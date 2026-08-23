// @ts-check

import { createHash } from "node:crypto";
import { existsSync } from "node:fs";
import { mkdir, readdir, readFile, rename, rm, writeFile } from "node:fs/promises";
import { dirname, join, relative, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";

const repositoryRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const catalogPath = join(
  repositoryRoot,
  "contracts",
  "geometer",
  "generated",
  "wn_geometer_contract_catalog.a0.json",
);
const manifestPath = join(repositoryRoot, "docs", "contracts", "promotion-manifest.toml");
const generatedParent = join(repositoryRoot, "docs", "generated");
const generatedPath = join(generatedParent, "contracts");
const stagingPath = join(generatedParent, `.contracts-stage-${process.pid}`);
const backupPath = join(generatedParent, `.contracts-backup-${process.pid}`);
const checkOnly = process.argv.slice(2).includes("--check");

if (process.argv.slice(2).some((argument) => argument !== "--check")) {
  throw new Error("Usage: node scripts/generate-contract-docs.mjs [--check]");
}

await rm(stagingPath, { recursive: true, force: true });
await rm(backupPath, { recursive: true, force: true });
await mkdir(stagingPath, { recursive: true });

try {
  const catalogBytes = await readFile(catalogPath);
  const catalog = JSON.parse(catalogBytes.toString("utf8"));
  const manifest = parsePromotionManifest(await readFile(manifestPath, "utf8"));
  const catalogDigest = sha256(catalogBytes);
  await verifyDocumentationAssets(manifest.documentationAssets);
  await generateSite(catalog, manifest, catalogDigest);
  await verifyGeneratedSite(catalog, catalogDigest);

  if (checkOnly) {
    const differences = await compareDirectories(stagingPath, generatedPath);
    if (differences.length > 0) {
      throw new Error(
        `Generated contract documentation is stale:\n${differences.map((item) => `  - ${item}`).join("\n")}\nRun npm run generate:docs.`,
      );
    }
    process.stdout.write("Generated contract documentation is current.\n");
  } else {
    await installGeneratedSite();
    process.stdout.write("Generated offline Geometer contract documentation.\n");
  }
} finally {
  await rm(stagingPath, { recursive: true, force: true });
  await rm(backupPath, { recursive: true, force: true });
}

async function generateSite(catalog, manifest, catalogDigest) {
  const contractById = new Map(manifest.contracts.map((contract) => [contract.id, contract]));
  const operationById = new Map(
    [...manifest.operations, ...manifest.candidateOperations].map((operation) => [
      operation.id,
      operation,
    ]),
  );
  const declarationByName = new Map(
    catalog.declarations.map((declaration) => [declaration.name, declaration]),
  );
  const contractPages = Object.fromEntries(
    catalog.roots.map((root) => [
      root.contract_identity,
      `contracts/${slug(root.contract_identity)}.html`,
    ]),
  );
  const operationPages = Object.fromEntries(
    catalog.operations.map((operation) => [
      operation.identity,
      `operations/${slug(operation.identity)}.html`,
    ]),
  );

  await mkdir(join(stagingPath, "contracts"), { recursive: true });
  await mkdir(join(stagingPath, "operations"), { recursive: true });

  for (const root of catalog.roots) {
    const lifecycle = contractById.get(root.contract_identity);
    if (!lifecycle) throw new Error(`Missing manifest lifecycle for ${root.contract_identity}.`);
    const declarations = reachableDeclarations(root.name, declarationByName);
    const body = renderContractPage(root, lifecycle, declarations, catalogDigest);
    await writeGenerated(contractPages[root.contract_identity], body);
  }

  for (const operation of catalog.operations) {
    const lifecycle = operationById.get(operation.identity);
    if (!lifecycle) throw new Error(`Missing manifest lifecycle for ${operation.identity}.`);
    const body = renderOperationPage(operation, lifecycle, contractPages, catalogDigest);
    await writeGenerated(operationPages[operation.identity], body);
  }

  await writeGenerated(
    "index.html",
    renderIndex(catalog, contractById, operationById, contractPages, operationPages, catalogDigest),
  );
  await writeGenerated(
    "site-manifest.a0.json",
    `${JSON.stringify(
      {
        generator_identity: "wn.geometer.contract_docs",
        generation: "a0",
        catalog_identity: catalog.catalog_identity,
        catalog_generation: catalog.generation,
        catalog_sha256: catalogDigest,
        contracts: contractPages,
        operations: operationPages,
      },
      null,
      2,
    )}\n`,
  );
}

function renderIndex(catalog, contracts, operations, contractPages, operationPages, digest) {
  const contractCards = catalog.roots
    .map((root) => {
      const lifecycle = contracts.get(root.contract_identity);
      return `<article class="panel">
  <h3><a href="${escapeAttribute(contractPages[root.contract_identity])}"><code>${escapeHtml(root.contract_identity)}</code></a></h3>
  <p>${escapeHtml(declarationDoc(catalog, root.name))}</p>
  <p><span class="tag">${escapeHtml(root.declaration_kind)}</span> <span class="status">${escapeHtml(lifecycle.status)}</span></p>
</article>`;
    })
    .join("\n");
  const operationRows = catalog.operations
    .map((operation) => {
      const lifecycle = operations.get(operation.identity);
      return `<tr><td><a href="${escapeAttribute(operationPages[operation.identity])}"><code>${escapeHtml(operation.identity)}</code></a></td><td>${escapeHtml(operation.request_contract)}</td><td>${escapeHtml(operation.result_contract)}</td><td>${escapeHtml(lifecycle.status)}</td></tr>`;
    })
    .join("\n");
  return htmlDocument({
    title: "Geometer Contract Reference",
    depth: 0,
    digest,
    bodyAttributes: 'data-page-kind="index"',
    content: `<header>
  <p class="page-type">Generated contract reference</p>
  <h1>Geometer Contracts</h1>
  <p class="lede">Typed operation structures for native, browser/WASM, executable IPC, and generated clients.</p>
</header>
<nav class="nav" aria-label="Reference navigation">
  <a href="../../design/typespec-toolchain.md">TypeSpec toolchain</a>
  <a href="../../design/contract-semantics.md">Contract semantics</a>
  <a href="../../design/generic-operation-c-abi.md">Generic C ABI</a>
  <a href="../../design/typescript-client.md">TypeScript client</a>
  <a href="../../design/executable-ipc-a0.md">Executable IPC A0</a>
</nav>
<aside class="callout"><strong>Generated reference.</strong> Authored TypeSpec owns promoted structure; this site is a deterministic navigation and review artifact.</aside>
<section>
  <h2>Contract roots <span class="tag">${catalog.roots.length}</span></h2>
  <div class="grid">${contractCards}</div>
</section>
<section>
  <h2>Operations <span class="tag">${catalog.operations.length}</span></h2>
  <table><thead><tr><th>Identity</th><th>Request</th><th>Result</th><th>Lifecycle</th></tr></thead><tbody>${operationRows}</tbody></table>
</section>
<section>
  <h2>Generated inputs</h2>
  <div class="panel"><p>Catalog <code>${escapeHtml(catalog.catalog_identity)}:${escapeHtml(catalog.generation)}</code></p><p>SHA-256 <code>${digest}</code></p><p><a href="../../../contracts/geometer/generated/wn_geometer_contract_catalog.a0.json">Normalized catalog JSON</a></p></div>
</section>`,
  });
}

function renderContractPage(root, lifecycle, declarations, digest) {
  const schemaName = `${root.name.slice(root.name.lastIndexOf(".") + 1)}.json`;
  const declarationSections = declarations.map(renderDeclaration).join("\n");
  const patchNote = root.option_patch
    ? '<aside class="callout"><strong>Presence-preserving patch.</strong> Absent fields do not materialize defaults and do not replace inherited values.</aside>'
    : "";
  const compatibilityLink = root.contract_identity.startsWith("geometry.model_bounds")
    ? '<a href="../../../design/model-bounds-contract-compatibility.md">Compatibility analysis</a>'
    : '<a href="../../../design/contract-semantics.md">Compatibility policy</a>';
  return htmlDocument({
    title: `${root.contract_identity} — Geometer`,
    depth: 1,
    digest,
    bodyAttributes: `data-page-kind="contract" data-contract-identity="${escapeAttribute(root.contract_identity)}" data-promotion-status="${escapeAttribute(lifecycle.status)}"`,
    content: `<header>
  <p class="page-type">Contract root</p>
  <h1><code>${escapeHtml(root.contract_identity)}</code></h1>
  <p class="lede">${escapeHtml(declarations[0]?.doc ?? "")}</p>
</header>
<nav class="nav" aria-label="Contract navigation"><a href="../index.html">All contracts</a><a href="../../../../contracts/geometer/generated/schema/${escapeAttribute(schemaName)}">JSON Schema</a><a href="../../../../src/tsp/geometer/main.tsp">TypeSpec entry point</a>${compatibilityLink}</nav>
<aside class="callout"><strong>Generated, not authored.</strong> Edit TypeSpec and regenerate. Lifecycle evidence remains in the promotion manifest.</aside>
${patchNote}
<section><h2>Identity</h2><table><tbody>
<tr><th>Contract</th><td><code>${escapeHtml(root.contract_identity)}</code></td></tr>
<tr><th>Schema</th><td><code>${escapeHtml(root.schema_id)}</code></td></tr>
<tr><th>Declaration</th><td><code>${escapeHtml(root.name)}</code></td></tr>
<tr><th>Lifecycle</th><td><span class="status">${escapeHtml(lifecycle.status)}</span></td></tr>
<tr><th>Current authority</th><td><code>${escapeHtml(lifecycle.current_authority)}</code></td></tr>
</tbody></table></section>
<section class="model-section"><h2>Declarations <span class="tag">${declarations.length}</span></h2>${declarationSections}</section>`,
  });
}

function renderDeclaration(declaration) {
  const anchor = `declaration-${slug(declaration.name)}`;
  let content = "";
  if (declaration.kind === "model") {
    if (declaration.model_kind === "object") {
      const rows = declaration.properties
        .map(
          (property) =>
            `<tr><td><code>${escapeHtml(property.name)}</code></td><td>${property.optional ? "optional" : "required"}</td><td><code>${renderType(property.type)}</code></td><td>${renderDefault(property)}</td><td>${renderConstraints(property.constraints)}</td><td>${escapeHtml(property.doc)}</td></tr>`,
        )
        .join("\n");
      content = `<table><thead><tr><th>Field</th><th>Presence</th><th>Type</th><th>Default intent</th><th>Constraints</th><th>Description</th></tr></thead><tbody>${rows}</tbody></table>`;
    } else {
      content = `<p><span class="tag">${escapeHtml(declaration.model_kind)}</span> element <code>${renderType(declaration.index_value)}</code>; ${renderConstraints(declaration.constraints)}</p>`;
    }
  } else if (declaration.kind === "enum") {
    content = `<table><thead><tr><th>Member</th><th>Wire value</th><th>Description</th></tr></thead><tbody>${declaration.members.map((member) => `<tr><td><code>${escapeHtml(member.name)}</code></td><td><code>${escapeHtml(String(member.value))}</code></td><td>${escapeHtml(member.doc)}</td></tr>`).join("\n")}</tbody></table>`;
  } else if (declaration.kind === "union") {
    content = `<ul>${declaration.variants.map((variant) => `<li><code>${escapeHtml(variant.name)}</code>: <code>${renderType(variant.type)}</code></li>`).join("")}</ul>`;
  } else {
    content = `<p>Base type <code>${renderType(declaration.base)}</code>; ${renderConstraints(declaration.constraints)}</p>`;
  }
  return `<details open id="${anchor}"><summary><h2><code>${escapeHtml(shortName(declaration.name))}</code> <span class="tag">${escapeHtml(declaration.kind)}</span></h2></summary><p>${escapeHtml(declaration.doc)}</p>${content}</details>`;
}

function renderOperationPage(operation, lifecycle, contractPages, digest) {
  const attachmentRows = [
    ...operation.input_attachments.map((attachment) => ["input", attachment]),
    ...operation.output_attachments.map((attachment) => ["output", attachment]),
  ]
    .map(
      ([direction, attachment]) =>
        `<tr><td>${direction}</td><td><code>${escapeHtml(attachment.name)}</code></td><td>${attachment.required ? "required" : "optional"}</td><td>${attachment.media_types.map((item) => `<code>${escapeHtml(item)}</code>`).join("<br>")}</td><td>${attachment.max_bytes.toLocaleString("en-US")}</td></tr>`,
    )
    .join("\n");
  const exampleSection =
    operation.identity === "geometry.model_bounds.a0"
      ? '<section><h2>TypeScript example</h2><div class="panel"><p><a href="../../../../examples/wasm/model_bounds_demo.html">Run the generated model-bounds browser client</a></p><p>The window and dedicated Worker sources use <code>@wavenumber/geometer/worker</code> and <code>@wavenumber/geometer/worker-host</code>; application code contains no direct pointer management.</p></div></section>'
      : "";
  const availabilityCallout = operation.runtime_available
    ? '<aside class="callout"><strong>Additive generic operation.</strong> Raw model bytes travel as named attachments; JSON contains only the typed request and result structures.</aside>'
    : '<aside class="callout"><strong>Structural research only.</strong> Generated DTOs and codecs exist, but this operation is not advertised or callable through a Geometer runtime transport.</aside>';
  const transportSection = operation.runtime_available
    ? '<section><h2>Supported transport design</h2><div class="grid"><article class="panel"><h3>Native and browser/WASM</h3><p>Additive generic C ABI with explicit ownership and attachment arrays.</p></article><article class="panel"><h3>Executable IPC</h3><p>Binary-safe framed stdio A0 with generated JSON envelopes and raw attachments.</p></article></div></section>'
    : '<section><h2>Transport status</h2><div class="panel"><p>Unavailable. A later adapter must implement and validate the native behavior before this operation may enter a runtime capability catalog.</p></div></section>';
  return htmlDocument({
    title: `${operation.identity} operation — Geometer`,
    depth: 1,
    digest,
    bodyAttributes: `data-page-kind="operation" data-operation-identity="${escapeAttribute(operation.identity)}" data-promotion-status="${escapeAttribute(lifecycle.status)}"`,
    content: `<header><p class="page-type">Operation</p><h1><code>${escapeHtml(operation.identity)}</code></h1><p class="lede">${escapeHtml(operation.doc)}</p></header>
<nav class="nav" aria-label="Operation navigation"><a href="../index.html">All contracts</a><a href="../${escapeAttribute(contractPages[operation.request_contract])}">Request contract</a><a href="../${escapeAttribute(contractPages[operation.result_contract])}">Result contract</a><a href="../../../design/generic-operation-c-abi.md">Generic C ABI</a><a href="../../../design/typescript-client.md">TypeScript client</a><a href="../../../design/executable-ipc-a0.md">Executable IPC A0</a></nav>
${availabilityCallout}
<section><h2>Registry</h2><table><tbody><tr><th>Operation identity</th><td><code>${escapeHtml(operation.identity)}</code></td></tr><tr><th>Lifecycle</th><td><span class="status">${escapeHtml(lifecycle.status)}</span></td></tr><tr><th>Runtime available</th><td><code>${operation.runtime_available ? "true" : "false"}</code></td></tr><tr><th>Request</th><td><code>${escapeHtml(operation.request_contract)}</code></td></tr><tr><th>Result</th><td><code>${escapeHtml(operation.result_contract)}</code></td></tr></tbody></table></section>
<section><h2>Attachments <span class="tag">${operation.input_attachments.length + operation.output_attachments.length}</span></h2><table><thead><tr><th>Direction</th><th>Name</th><th>Presence</th><th>Media types</th><th>Maximum bytes</th></tr></thead><tbody>${attachmentRows}</tbody></table></section>
${transportSection}${exampleSection}`,
  });
}

function htmlDocument({ title, depth, digest, bodyAttributes, content }) {
  const stylesheet = depth === 0 ? "../../design/styles.css" : "../../../design/styles.css";
  return `<!doctype html>
<html lang="en" data-generator="wn.geometer.contract_docs" data-generator-generation="a0">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="generator" content="wn.geometer.contract_docs:a0">
  <meta name="wn-catalog-sha256" content="${digest}">
  <title>${escapeHtml(title)}</title>
  <link rel="stylesheet" href="${stylesheet}">
</head>
<body class="generated-model-doc" data-doc-status="generated" data-generated="true" data-wn-watermark="true" data-source-catalog-sha256="${digest}" ${bodyAttributes}>
<main>
${content}
</main>
</body>
</html>
`;
}

function reachableDeclarations(rootName, declarationByName) {
  const found = new Map();
  function visitName(name) {
    if (found.has(name)) return;
    const declaration = declarationByName.get(name);
    if (!declaration) throw new Error(`Catalog reference does not resolve: ${name}.`);
    found.set(name, declaration);
    if (declaration.kind === "model") {
      visitType(declaration.base);
      visitType(declaration.index_value);
      for (const property of declaration.properties) visitType(property.type);
    } else if (declaration.kind === "scalar") visitType(declaration.base);
    else if (declaration.kind === "union") {
      for (const variant of declaration.variants) visitType(variant.type);
    }
  }
  function visitType(type) {
    if (!type) return;
    if (type.kind === "reference") visitName(type.target);
    else if (type.kind === "array") visitType(type.element);
    else if (type.kind === "record") visitType(type.value);
  }
  visitName(rootName);
  const [root, ...nested] = [...found.values()];
  nested.sort((left, right) => left.name.localeCompare(right.name));
  return [root, ...nested];
}

function renderType(type) {
  if (!type) return "none";
  if (type.kind === "reference") return escapeHtml(shortName(type.target));
  if (type.kind === "primitive") return escapeHtml(type.name);
  if (type.kind === "literal") return escapeHtml(JSON.stringify(type.value));
  if (type.kind === "array") return `${renderType(type.element)}[]`;
  if (type.kind === "record") return `record&lt;${renderType(type.value)}&gt;`;
  throw new Error(`Unsupported documentation type expression ${type.kind}.`);
}

function renderDefault(property) {
  if ("default" in property) {
    return `<code>${escapeHtml(JSON.stringify(property.default))}</code>`;
  }
  const intent = property.annotations["x-wn-default-intent"];
  return intent === undefined ? "—" : `<code>${escapeHtml(String(intent))}</code>`;
}

function renderConstraints(constraints) {
  const entries = Object.entries(constraints);
  if (entries.length === 0) return "none";
  return entries
    .map(([name, value]) => `<code>${escapeHtml(name)}=${escapeHtml(String(value))}</code>`)
    .join(" ");
}

async function verifyDocumentationAssets(assets) {
  if (assets.length !== 5)
    throw new Error(`Expected five documentation assets; found ${assets.length}.`);
  for (const asset of assets) {
    if (asset.status !== "vendored") throw new Error(`${asset.id}: asset is not vendored.`);
    const path = join(repositoryRoot, asset.destination);
    if (!existsSync(path)) throw new Error(`${asset.id}: missing ${asset.destination}.`);
    const digest = sha256(await readFile(path));
    if (digest !== asset.sha256) {
      throw new Error(`${asset.id}: digest ${digest} does not match ${asset.sha256}.`);
    }
  }
  const stylesheet = await readFile(join(repositoryRoot, "docs", "design", "styles.css"), "utf8");
  if (!stylesheet.includes('font-family: "Cousine"') || stylesheet.includes("Berkeley Mono")) {
    throw new Error("Vendored stylesheet must use Cousine and exclude Berkeley Mono.");
  }
}

async function verifyGeneratedSite(catalog, digest) {
  const files = await listFiles(stagingPath);
  const expected = [
    "index.html",
    "site-manifest.a0.json",
    ...catalog.roots.map((root) => `contracts/${slug(root.contract_identity)}.html`),
    ...catalog.operations.map((operation) => `operations/${slug(operation.identity)}.html`),
  ].sort();
  assertEqual(files, expected, "generated documentation file inventory");

  const linkedInternalPages = new Set();
  for (const path of files.filter((item) => item.endsWith(".html"))) {
    const absolute = join(stagingPath, path);
    const html = await readFile(absolute, "utf8");
    if (!html.endsWith("\n") || html.includes("\r")) throw new Error(`${path}: noncanonical text.`);
    for (const marker of [
      'data-doc-status="generated"',
      'data-generated="true"',
      'data-wn-watermark="true"',
      `data-source-catalog-sha256="${digest}"`,
    ]) {
      if (!html.includes(marker)) throw new Error(`${path}: missing ${marker}.`);
    }
    if (/\b(?:href|src)=["'](?:https?:|\/\/|file:)/iu.test(html)) {
      throw new Error(`${path}: generated HTML must not require external or absolute resources.`);
    }
    const references = [...html.matchAll(/\b(?:href|src)="([^"]+)"/gu)].map((match) => match[1]);
    for (const reference of references) {
      const [target] = reference.split("#", 1);
      if (!target) continue;
      const resolved = resolve(dirname(absolute), target);
      if (!existsSync(resolved)) throw new Error(`${path}: broken relative link ${reference}.`);
      const relativeTarget = relative(stagingPath, resolved).split(sep).join("/");
      if (!relativeTarget.startsWith("..") && relativeTarget.endsWith(".html")) {
        linkedInternalPages.add(relativeTarget);
      }
    }
  }
  for (const page of expected.filter((item) => item.endsWith(".html") && item !== "index.html")) {
    if (!linkedInternalPages.has(page)) throw new Error(`Generated page is not linked: ${page}.`);
  }
}

async function writeGenerated(path, content) {
  if (content.includes("\r")) throw new Error(`${path}: generated content contains CR.`);
  if (!content.endsWith("\n") || content.endsWith("\n\n")) {
    throw new Error(`${path}: generated content must end with exactly one newline.`);
  }
  const absolute = join(stagingPath, path);
  await mkdir(dirname(absolute), { recursive: true });
  await writeFile(absolute, content, "utf8");
}

async function installGeneratedSite() {
  let movedExisting = false;
  try {
    if (existsSync(generatedPath)) {
      await rename(generatedPath, backupPath);
      movedExisting = true;
    }
    await rename(stagingPath, generatedPath);
    await rm(backupPath, { recursive: true, force: true });
  } catch (error) {
    if (!existsSync(generatedPath) && movedExisting && existsSync(backupPath)) {
      await rename(backupPath, generatedPath);
    }
    throw error;
  }
}

async function compareDirectories(actualRoot, expectedRoot) {
  if (!existsSync(expectedRoot)) return [`missing directory ${repositoryRelative(expectedRoot)}`];
  const actualFiles = await listFiles(actualRoot);
  const expectedFiles = await listFiles(expectedRoot);
  const differences = [];
  const actualSet = new Set(actualFiles);
  const expectedSet = new Set(expectedFiles);
  for (const path of actualFiles) {
    if (!expectedSet.has(path))
      differences.push(`missing ${repositoryRelative(join(expectedRoot, path))}`);
    else {
      const [actual, expected] = await Promise.all([
        readFile(join(actualRoot, path)),
        readFile(join(expectedRoot, path)),
      ]);
      if (actual.compare(expected) !== 0)
        differences.push(`stale ${repositoryRelative(join(expectedRoot, path))}`);
    }
  }
  for (const path of expectedFiles) {
    if (!actualSet.has(path))
      differences.push(`unexpected ${repositoryRelative(join(expectedRoot, path))}`);
  }
  return differences.sort();
}

async function listFiles(root) {
  if (!existsSync(root)) return [];
  const output = [];
  async function visit(directory) {
    const entries = await readdir(directory, { withFileTypes: true });
    entries.sort((left, right) => left.name.localeCompare(right.name));
    for (const entry of entries) {
      const absolute = join(directory, entry.name);
      if (entry.isDirectory()) await visit(absolute);
      else if (entry.isFile()) output.push(relative(root, absolute).split(sep).join("/"));
      else throw new Error(`Generated docs contain unsupported entry ${absolute}.`);
    }
  }
  await visit(root);
  return output.sort();
}

function parsePromotionManifest(text) {
  const manifest = {
    contracts: [],
    operations: [],
    candidateOperations: [],
    documentationAssets: [],
  };
  let target = null;
  for (const rawLine of text.split(/\r?\n/u)) {
    const line = rawLine.trim();
    if (!line || line.startsWith("#")) continue;
    if (line === "[[contracts]]") {
      target = {};
      manifest.contracts.push(target);
      continue;
    }
    if (line === "[[operations]]") {
      target = {};
      manifest.operations.push(target);
      continue;
    }
    if (line === "[[candidate_operations]]") {
      target = {};
      manifest.candidateOperations.push(target);
      continue;
    }
    if (line === "[[documentation.assets]]") {
      target = {};
      manifest.documentationAssets.push(target);
      continue;
    }
    if (line.startsWith("[") && line.endsWith("]")) {
      target = null;
      continue;
    }
    if (!target) continue;
    const match = /^([A-Za-z0-9_]+)\s*=\s*(.+)$/u.exec(line);
    if (match) target[match[1]] = parseTomlValue(match[2]);
  }
  return manifest;
}

function parseTomlValue(source) {
  if (source === "true") return true;
  if (source === "false") return false;
  if (/^-?[0-9]+$/u.test(source)) return Number(source);
  if (source.startsWith('"') && source.endsWith('"')) return JSON.parse(source);
  if (source.startsWith("[") && source.endsWith("]")) return JSON.parse(source);
  throw new Error(`Unsupported promotion-manifest value: ${source}`);
}

function declarationDoc(catalog, name) {
  return catalog.declarations.find((declaration) => declaration.name === name)?.doc ?? "";
}

function slug(value) {
  return value.replaceAll("_", "-").replaceAll(".", "-").toLowerCase();
}

function shortName(value) {
  return value.slice(value.lastIndexOf(".") + 1);
}

function sha256(bytes) {
  return createHash("sha256").update(bytes).digest("hex");
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#39;");
}

function escapeAttribute(value) {
  return escapeHtml(value);
}

function assertEqual(actual, expected, label) {
  if (JSON.stringify(actual) !== JSON.stringify(expected)) {
    throw new Error(
      `${label} mismatch:\nexpected ${JSON.stringify(expected)}\nactual   ${JSON.stringify(actual)}`,
    );
  }
}

function repositoryRelative(path) {
  return relative(repositoryRoot, path).split(sep).join("/");
}
