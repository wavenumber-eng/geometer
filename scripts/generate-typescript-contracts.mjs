// @ts-check

import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { mkdir, readFile, rename, rm, writeFile } from "node:fs/promises";
import { dirname, join, relative, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const catalogPath = join(
  root,
  "contracts",
  "geometer",
  "generated",
  "wn_geometer_contract_catalog.a0.json",
);
const outputPath = join(root, "src", "ts", "geometer", "generated");
const stagingPath = join(root, "src", "ts", "geometer", `generated-stage-${process.pid}`);
const backupPath = join(root, "src", "ts", "geometer", `generated-backup-${process.pid}`);
const checkOnly = process.argv.slice(2).includes("--check");

if (process.argv.slice(2).some((argument) => argument !== "--check")) {
  throw new Error("Usage: node scripts/generate-typescript-contracts.mjs [--check]");
}

const catalog = JSON.parse(await readFile(catalogPath, "utf8"));
const declarations = new Map(catalog.declarations.map((item) => [item.name, item]));
assertUniqueShortNames(catalog.declarations);

await rm(stagingPath, { recursive: true, force: true });
await rm(backupPath, { recursive: true, force: true });
await mkdir(stagingPath, { recursive: true });

try {
  await writeGenerated("contracts.ts", generateContracts());
  await writeGenerated("codecs.ts", generateCodecs());
  await writeGenerated("operations.ts", generateOperations());
  await writeGenerated("index.ts", generateIndex());
  formatGeneratedState();
  if (checkOnly) {
    const differences = await compareGeneratedFiles();
    if (differences.length > 0) {
      throw new Error(
        `Generated TypeScript contracts are stale:\n${differences.map((item) => `  - ${item}`).join("\n")}\nRun npm run generate:contracts.`,
      );
    }
    process.stdout.write("Generated TypeScript contracts are current.\n");
  } else {
    await installGeneratedState();
    process.stdout.write("Generated TypeScript contracts and strict codecs.\n");
  }
} finally {
  await rm(stagingPath, { recursive: true, force: true });
  await rm(backupPath, { recursive: true, force: true });
}

function generateContracts() {
  const lines = [generatedHeader()];
  for (const item of topologicalOrder(catalog.declarations)) {
    if (item.doc) lines.push(docComment(item.doc));
    if (item.kind === "enum") {
      lines.push(
        `export type ${shortName(item.name)} = ${item.members.map((member) => JSON.stringify(member.value)).join(" | ")};`,
        "",
      );
      continue;
    }
    if (item.kind === "union") {
      lines.push(
        `export type ${shortName(item.name)} = ${item.variants.map((variant) => tsType(variant.type)).join(" | ")};`,
        "",
      );
      continue;
    }
    if (item.kind !== "model") unsupported(item);
    if (item.model_kind === "array") {
      lines.push(
        `export type ${shortName(item.name)} = ${arrayType(item.index_value, item.constraints)};`,
        "",
      );
      continue;
    }
    if (item.model_kind !== "object" || item.base !== null) unsupported(item);
    lines.push(`export interface ${shortName(item.name)} {`);
    for (const property of item.properties) {
      if (property.doc) lines.push(indentDoc(property.doc));
      lines.push(
        `  readonly ${safeProperty(property.name)}${property.optional ? "?" : ""}: ${tsType(property.type)};`,
      );
    }
    lines.push("}", "");
  }
  return `${lines.join("\n").trimEnd()}\n`;
}

function generateCodecs() {
  const roots = catalog.roots.map((rootItem) => ({
    ...rootItem,
    typeName: shortName(rootItem.name),
  }));
  const lines = [
    generatedHeader(),
    'import { decodeContractJson, encodeContractJson } from "../codec-runtime.js";',
    'import type { ContractDescriptorMap } from "../codec-runtime.js";',
    `import type { ${roots.map((rootItem) => rootItem.typeName).join(", ")} } from "./contracts.js";`,
    "",
    "const declarations: ContractDescriptorMap = {",
  ];
  for (const item of catalog.declarations) {
    lines.push(`  ${JSON.stringify(item.name)}: ${descriptor(item)},`);
  }
  lines.push("};", "");
  for (const rootItem of roots) {
    lines.push(
      `export function decode${rootItem.typeName}Json(data: string | Uint8Array): ${rootItem.typeName} {`,
      `  return decodeContractJson(data, { kind: "reference", target: ${JSON.stringify(rootItem.name)} }, declarations) as ${rootItem.typeName};`,
      "}",
      "",
      `export function encode${rootItem.typeName}Json(value: ${rootItem.typeName}): string {`,
      `  return encodeContractJson(value, { kind: "reference", target: ${JSON.stringify(rootItem.name)} }, declarations);`,
      "}",
      "",
    );
  }
  return `${lines.join("\n").trimEnd()}\n`;
}

function generateOperations() {
  const lines = [generatedHeader(), "export const operationCatalog = {"];
  for (const operation of catalog.operations) {
    lines.push(`  ${JSON.stringify(operation.identity)}: {`);
    lines.push(`    identity: ${JSON.stringify(operation.identity)},`);
    lines.push(`    requestContract: ${JSON.stringify(operation.request_contract)},`);
    lines.push(`    resultContract: ${JSON.stringify(operation.result_contract)},`);
    lines.push(`    inputAttachments: ${JSON.stringify(operation.input_attachments)},`);
    lines.push(`    outputAttachments: ${JSON.stringify(operation.output_attachments)},`);
    lines.push(`    documentation: ${JSON.stringify(operation.doc)},`);
    lines.push("  },");
  }
  lines.push(
    "} as const;",
    "",
    "export type OperationIdentity = keyof typeof operationCatalog;",
    "export type ModelBoundsInputMediaType =",
    '  (typeof operationCatalog)["geometry.model_bounds.a0"]["inputAttachments"][0]["media_types"][number];',
  );
  return `${lines.join("\n").trimEnd()}\n`;
}

function generateIndex() {
  return `${generatedHeader()}export * from "./contracts.js";\nexport * from "./codecs.js";\nexport * from "./operations.js";\n`;
}

function descriptor(item) {
  if (item.kind === "enum") {
    return `{ kind: "enum", values: ${JSON.stringify(item.members.map((member) => member.value))} }`;
  }
  if (item.kind === "union") {
    return `{ kind: "union", variants: ${JSON.stringify(item.variants.map((variant) => typeDescriptor(variant.type)))} }`;
  }
  if (item.kind !== "model") unsupported(item);
  if (item.model_kind === "array") {
    return `{ kind: "array", element: ${JSON.stringify(typeDescriptor(item.index_value))}, constraints: ${JSON.stringify(item.constraints)} }`;
  }
  if (item.model_kind !== "object" || item.base !== null) unsupported(item);
  const properties = Object.fromEntries(
    item.properties.map((property) => [
      property.name,
      {
        type: typeDescriptor(property.type),
        optional: property.optional,
        constraints: property.constraints,
      },
    ]),
  );
  return `{ kind: "object", properties: ${JSON.stringify(properties)} }`;
}

function typeDescriptor(type) {
  if (["reference", "primitive", "literal"].includes(type.kind)) return type;
  if (type.kind === "array") return { kind: "array", element: typeDescriptor(type.element) };
  unsupported(type);
}

function tsType(type) {
  if (type.kind === "reference") return shortName(type.target);
  if (type.kind === "primitive") {
    const mapped = { string: "string", boolean: "boolean", float64: "number" }[type.name];
    if (!mapped) unsupported(type);
    return mapped;
  }
  if (type.kind === "literal") return JSON.stringify(type.value);
  if (type.kind === "array") return `readonly ${tsType(type.element)}[]`;
  unsupported(type);
}

function arrayType(element, constraints) {
  if (
    constraints.min_items !== undefined &&
    constraints.min_items === constraints.max_items &&
    constraints.min_items <= 32
  ) {
    return `readonly [${Array.from({ length: constraints.min_items }, () => tsType(element)).join(", ")}]`;
  }
  return `readonly ${tsType(element)}[]`;
}

function topologicalOrder(items) {
  const result = [];
  const visited = new Set();
  const visiting = new Set();
  function visit(item) {
    if (visited.has(item.name)) return;
    if (visiting.has(item.name))
      throw new Error(`Cyclic TypeScript DTO dependency at ${item.name}.`);
    visiting.add(item.name);
    for (const dependency of dependencies(item)) {
      const declaration = declarations.get(dependency);
      if (declaration) visit(declaration);
    }
    visiting.delete(item.name);
    visited.add(item.name);
    result.push(item);
  }
  for (const item of items) visit(item);
  return result;
}

function dependencies(item) {
  const found = new Set();
  function scan(type) {
    if (!type) return;
    if (type.kind === "reference") found.add(type.target);
    if (type.kind === "array") scan(type.element);
  }
  scan(item.base);
  scan(item.index_value);
  for (const property of item.properties ?? []) scan(property.type);
  for (const variant of item.variants ?? []) scan(variant.type);
  return found;
}

function assertUniqueShortNames(items) {
  const seen = new Map();
  for (const item of items) {
    const short = shortName(item.name);
    if (seen.has(short)) {
      throw new Error(`TypeScript short-name collision: ${seen.get(short)} and ${item.name}.`);
    }
    seen.set(short, item.name);
  }
}

async function writeGenerated(filename, content) {
  if (content.includes("\r") || !content.endsWith("\n") || content.endsWith("\n\n")) {
    throw new Error(`${filename} is not normalized UTF-8/LF text.`);
  }
  await writeFile(join(stagingPath, filename), content, "utf8");
}

async function compareGeneratedFiles() {
  const filenames = ["codecs.ts", "contracts.ts", "index.ts", "operations.ts"];
  const differences = [];
  for (const filename of filenames) {
    const actual = join(stagingPath, filename);
    const expected = join(outputPath, filename);
    if (!existsSync(expected)) differences.push(`missing ${repositoryRelative(expected)}`);
    else if ((await readFile(actual)).compare(await readFile(expected)) !== 0)
      differences.push(`stale ${repositoryRelative(expected)}`);
  }
  return differences;
}

async function installGeneratedState() {
  let movedExisting = false;
  try {
    if (existsSync(outputPath)) {
      await rename(outputPath, backupPath);
      movedExisting = true;
    }
    await rename(stagingPath, outputPath);
    await rm(backupPath, { recursive: true, force: true });
  } catch (error) {
    if (!existsSync(outputPath) && movedExisting && existsSync(backupPath)) {
      await rename(backupPath, outputPath);
    }
    throw error;
  }
}

function generatedHeader() {
  return "// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.\n";
}
function formatGeneratedState() {
  const biome = join(root, "node_modules", "@biomejs", "biome", "bin", "biome");
  const result = spawnSync(process.execPath, [biome, "check", "--write", stagingPath], {
    cwd: root,
    encoding: "utf8",
  });
  if (result.error) throw result.error;
  if (result.status !== 0) {
    throw new Error(
      `Biome failed to normalize generated TypeScript.\n${result.stdout}${result.stderr}`,
    );
  }
}
function docComment(value) {
  return `/** ${value.replaceAll("*/", "*\\/")} */`;
}
function indentDoc(value) {
  return `  /** ${value.replaceAll("*/", "*\\/")} */`;
}
function shortName(value) {
  return value.slice(value.lastIndexOf(".") + 1);
}
function safeProperty(value) {
  return /^[A-Za-z_$][A-Za-z0-9_$]*$/u.test(value) ? value : JSON.stringify(value);
}
function repositoryRelative(path) {
  return relative(root, path).split(sep).join("/");
}
function unsupported(value) {
  throw new Error(`Unsupported TypeScript catalog construct ${JSON.stringify(value)}`);
}
