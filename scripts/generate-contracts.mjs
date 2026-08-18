// @ts-check

import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { mkdir, readdir, readFile, rename, rm, writeFile } from "node:fs/promises";
import { dirname, join, relative, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";

import Ajv from "ajv";

const repositoryRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const manifestPath = join(repositoryRoot, "docs", "contracts", "promotion-manifest.toml");
const catalogSchemaPath = join(repositoryRoot, "contracts", "geometer", "catalog-schema.a0.json");
const generatedPath = join(repositoryRoot, "contracts", "geometer", "generated");
const stagingPath = join(
  repositoryRoot,
  "contracts",
  "geometer",
  `.generated-stage-${process.pid}`,
);
const backupPath = join(
  repositoryRoot,
  "contracts",
  "geometer",
  `.generated-backup-${process.pid}`,
);
const checkOnly = process.argv.slice(2).includes("--check");

if (process.argv.slice(2).some((argument) => argument !== "--check")) {
  throw new Error("Usage: node scripts/generate-contracts.mjs [--check]");
}

await rm(stagingPath, { recursive: true, force: true });
await rm(backupPath, { recursive: true, force: true });
await mkdir(stagingPath, { recursive: true });

try {
  compileTypeSpec();
  await normalizeGeneratedJson();
  const manifest = parsePromotionManifest(await readFile(manifestPath, "utf8"));
  const catalogPath = join(stagingPath, "wn_geometer_contract_catalog.a0.json");
  const catalog = JSON.parse(await readFile(catalogPath, "utf8"));
  const catalogSchema = JSON.parse(await readFile(catalogSchemaPath, "utf8"));
  validateCatalog(catalogSchema, catalog);
  await validateGeneratedState(manifest, catalog);

  if (checkOnly) {
    const differences = await compareDirectories(stagingPath, generatedPath);
    if (differences.length > 0) {
      throw new Error(
        `Generated contracts are stale:\n${differences.map((item) => `  - ${item}`).join("\n")}\nRun npm run generate:contracts.`,
      );
    }
    process.stdout.write("Geometer generated contracts are current.\n");
  } else {
    await installGeneratedState();
    process.stdout.write("Generated Geometer catalog and JSON Schemas.\n");
  }
} finally {
  await rm(stagingPath, { recursive: true, force: true });
  await rm(backupPath, { recursive: true, force: true });
}

function compileTypeSpec() {
  const compiler = join(repositoryRoot, "node_modules", "@typespec", "compiler", "cmd", "tsp.js");
  const result = spawnSync(
    process.execPath,
    [
      compiler,
      "compile",
      "src/tsp/geometer/main.tsp",
      "--config",
      "tspconfig.yaml",
      "--output-dir",
      stagingPath,
      "--warn-as-error",
    ],
    { cwd: repositoryRoot, encoding: "utf8", stdio: "pipe" },
  );
  if (result.error) {
    throw result.error;
  }
  if (result.status !== 0) {
    throw new Error(
      `TypeSpec compilation failed.\n${result.stdout ?? ""}${result.stderr ?? ""}`.trimEnd(),
    );
  }
}

function validateCatalog(schema, catalog) {
  const ajv = new Ajv({ allErrors: true, strict: true });
  const validate = ajv.compile(schema);
  if (!validate(catalog)) {
    throw new Error(
      `Normalized catalog is invalid:\n${ajv.errorsText(validate.errors, { separator: "\n" })}`,
    );
  }
}

async function normalizeGeneratedJson() {
  for (const path of await listFiles(stagingPath)) {
    if (!path.endsWith(".json")) {
      throw new Error(`TypeSpec emitted an unexpected non-JSON file: ${path}`);
    }
    const absolute = join(stagingPath, path);
    const value = JSON.parse(await readFile(absolute, "utf8"));
    await writeFile(absolute, `${JSON.stringify(value, null, 2)}\n`, "utf8");
  }
}

async function validateGeneratedState(manifest, catalog) {
  const toolchain = manifest.toolchain;
  assertEqual(catalog.catalog_identity, toolchain.catalog_identity, "catalog identity");
  assertEqual(catalog.generation, toolchain.catalog_generation, "catalog generation");

  const expectedOutputRoots = {
    cpp: "src/cpp/lib/geometer/generated/contracts",
    typescript: "src/ts/geometer/generated",
    rust: "src/rust/geometer-client/src/generated",
    python: "python/geometer/_generated/contracts",
    html: "docs/generated/contracts",
  };
  assertEqual(catalog.output_roots, expectedOutputRoots, "catalog output roots");

  const expectedContracts = manifest.contracts
    .filter((contract) => contract.pilot_required === true)
    .map((contract) => contract.id)
    .sort();
  const catalogContracts = catalog.roots.map((root) => root.contract_identity).sort();
  assertEqual(catalogContracts, expectedContracts, "pilot contract identities");

  const expectedOperations = [
    ...manifest.operations.filter((operation) =>
      ["pilot_candidate", "promoted"].includes(operation.status),
    ),
    ...manifest.candidateOperations.filter((operation) => operation.status === "contract_frozen"),
  ].sort((left, right) => left.id.localeCompare(right.id));
  assertEqual(
    catalog.operations.map((operation) => operation.identity).sort(),
    expectedOperations.map((operation) => operation.id),
    "generated operation identities",
  );
  for (const expected of expectedOperations) {
    const actual = catalog.operations.find((operation) => operation.identity === expected.id);
    if (!actual) {
      throw new Error(`Missing operation ${expected.id}.`);
    }
    assertEqual(
      actual.request_contract,
      expected.request_contract,
      `${expected.id} request contract`,
    );
    assertEqual(actual.result_contract, expected.result_contract, `${expected.id} result contract`);
    assertEqual(
      actual.input_attachments.map((attachment) => attachment.name).sort(),
      [...expected.input_attachments].sort(),
      `${expected.id} input attachments`,
    );
    assertEqual(
      actual.output_attachments.map((attachment) => attachment.name).sort(),
      [...expected.output_attachments].sort(),
      `${expected.id} output attachments`,
    );
  }

  const schemaFiles = (await listFiles(join(stagingPath, "schema"))).sort();
  const expectedSchemaFiles = catalog.roots
    .map((root) => `${root.name.slice(root.name.lastIndexOf(".") + 1)}.json`)
    .sort();
  assertEqual(schemaFiles, expectedSchemaFiles, "generated JSON Schema files");
  for (const root of catalog.roots) {
    const filename = `${root.name.slice(root.name.lastIndexOf(".") + 1)}.json`;
    const schema = JSON.parse(await readFile(join(stagingPath, "schema", filename), "utf8"));
    assertEqual(schema.$id, root.schema_id, `${filename} schema identity`);
  }

  for (const path of await listFiles(stagingPath)) {
    const bytes = await readFile(join(stagingPath, path));
    const text = bytes.toString("utf8");
    if (Buffer.from(text, "utf8").compare(bytes) !== 0) {
      throw new Error(`Generated file is not valid UTF-8: ${path}`);
    }
    if (text.includes("\r")) {
      throw new Error(`Generated file contains non-LF line endings: ${path}`);
    }
    if (!text.endsWith("\n") || text.endsWith("\n\n")) {
      throw new Error(`Generated file must end with exactly one newline: ${path}`);
    }
    const forbidden = [repositoryRoot, repositoryRoot.replaceAll("\\", "/"), "agent-worktrees"];
    if (forbidden.some((value) => value && text.toLowerCase().includes(value.toLowerCase()))) {
      throw new Error(`Generated file contains a host or sibling-workspace path: ${path}`);
    }
  }
}

async function installGeneratedState() {
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
  if (!existsSync(expectedRoot)) {
    return [`missing directory ${repositoryRelative(expectedRoot)}`];
  }
  const actualFiles = await listFiles(actualRoot);
  const expectedFiles = await listFiles(expectedRoot);
  const differences = [];
  const actualSet = new Set(actualFiles);
  const expectedSet = new Set(expectedFiles);
  for (const path of actualFiles) {
    if (!expectedSet.has(path)) {
      differences.push(`missing ${repositoryRelative(join(expectedRoot, path))}`);
    } else {
      const [actual, expected] = await Promise.all([
        readFile(join(actualRoot, path)),
        readFile(join(expectedRoot, path)),
      ]);
      if (actual.compare(expected) !== 0) {
        differences.push(`stale ${repositoryRelative(join(expectedRoot, path))}`);
      }
    }
  }
  for (const path of expectedFiles) {
    if (!actualSet.has(path)) {
      differences.push(`unexpected ${repositoryRelative(join(expectedRoot, path))}`);
    }
  }
  return differences.sort();
}

async function listFiles(root) {
  if (!existsSync(root)) {
    return [];
  }
  const output = [];
  async function visit(directory) {
    const entries = await readdir(directory, { withFileTypes: true });
    entries.sort((left, right) => left.name.localeCompare(right.name));
    for (const entry of entries) {
      const absolute = join(directory, entry.name);
      if (entry.isDirectory()) {
        await visit(absolute);
      } else if (entry.isFile()) {
        output.push(relative(root, absolute).split(sep).join("/"));
      } else {
        throw new Error(`Generated state contains unsupported entry ${absolute}.`);
      }
    }
  }
  await visit(root);
  return output;
}

function parsePromotionManifest(text) {
  const manifest = { toolchain: {}, contracts: [], operations: [], candidateOperations: [] };
  let target = null;
  for (const rawLine of text.split(/\r?\n/u)) {
    const line = rawLine.trim();
    if (!line || line.startsWith("#")) {
      continue;
    }
    if (line === "[toolchain]") {
      target = manifest.toolchain;
      continue;
    }
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
    if (line.startsWith("[") && line.endsWith("]")) {
      target = null;
      continue;
    }
    if (!target) {
      continue;
    }
    const match = /^([A-Za-z0-9_]+)\s*=\s*(.+)$/u.exec(line);
    if (match) {
      target[match[1]] = parseTomlValue(match[2]);
    }
  }
  return manifest;
}

function parseTomlValue(source) {
  if (source === "true") return true;
  if (source === "false") return false;
  if (/^-?[0-9]+$/u.test(source)) return Number(source);
  if (source.startsWith('"') && source.endsWith('"')) return JSON.parse(source);
  if (source.startsWith("[") && source.endsWith("]")) {
    return JSON.parse(source);
  }
  throw new Error(`Unsupported promotion-manifest value: ${source}`);
}

function assertEqual(actual, expected, label) {
  if (JSON.stringify(actual) !== JSON.stringify(expected)) {
    throw new Error(
      `${label} mismatch:\n  expected ${JSON.stringify(expected)}\n  actual   ${JSON.stringify(actual)}`,
    );
  }
}

function repositoryRelative(path) {
  return relative(repositoryRoot, path).split(sep).join("/");
}
