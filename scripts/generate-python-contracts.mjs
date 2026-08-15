// @ts-check

import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { mkdir, readFile, rename, rm, writeFile } from "node:fs/promises";
import { dirname, join, relative, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";

import { applyProjectionDeferrals } from "./contract-projection-deferral.mjs";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const catalog = await applyProjectionDeferrals(
  JSON.parse(
    await readFile(
      join(root, "contracts/geometer/generated/wn_geometer_contract_catalog.a0.json"),
      "utf8",
    ),
  ),
  "python",
);
const outputPath = join(root, catalog.output_roots.python);
const stagingPath = join(dirname(outputPath), `contracts-stage-${process.pid}`);
const backupPath = join(dirname(outputPath), `contracts-backup-${process.pid}`);
const checkOnly = process.argv.slice(2).includes("--check");
if (process.argv.slice(2).some((argument) => argument !== "--check")) {
  throw new Error("Usage: node scripts/generate-python-contracts.mjs [--check]");
}

const declarations = new Map(catalog.declarations.map((item) => [item.name, item]));
assertUniqueShortNames(catalog.declarations);
await rm(stagingPath, { recursive: true, force: true });
await rm(backupPath, { recursive: true, force: true });
await mkdir(stagingPath, { recursive: true });

try {
  await emit("models.py", generateModels());
  await emit("codecs.py", generateCodecs());
  await emit("__init__.py", generateIndex());
  formatGeneratedState();
  if (checkOnly) {
    const differences = await compareGeneratedFiles();
    if (differences.length) {
      throw new Error(
        `Generated Python contracts are stale:\n${differences.map((item) => `  - ${item}`).join("\n")}\nRun npm run generate:contracts.`,
      );
    }
    process.stdout.write("Generated Python contracts are current.\n");
  } else {
    await installGeneratedState();
    process.stdout.write("Generated Python contract models and strict codecs.\n");
  }
} finally {
  await rm(stagingPath, { recursive: true, force: true });
  await rm(backupPath, { recursive: true, force: true });
}

function generateModels() {
  const lines = [
    generatedHeader(),
    "from dataclasses import dataclass",
    "from enum import Enum",
    "from typing import Literal, TypeAlias",
    "",
  ];
  for (const item of topologicalOrder(catalog.declarations)) {
    if (item.doc) lines.push(docComment(item.doc));
    if (item.kind === "enum") {
      lines.push(`class ${shortName(item.name)}(str, Enum):`);
      for (const member of item.members) {
        lines.push(
          `    ${pythonIdentifier(member.name).toUpperCase()} = ${pythonLiteral(member.value)}`,
        );
      }
      lines.push("");
      continue;
    }
    if (item.kind === "union") {
      lines.push(
        `${shortName(item.name)}: TypeAlias = ${item.variants.map((variant) => pythonType(variant.type)).join(" | ")}`,
        "",
      );
      continue;
    }
    if (item.kind !== "model") unsupported(item);
    if (item.model_kind === "array") {
      lines.push(
        `${shortName(item.name)}: TypeAlias = ${arrayType(item.index_value, item.constraints)}`,
        "",
      );
      continue;
    }
    if (item.model_kind !== "object" || item.base !== null) unsupported(item);
    lines.push(
      "@dataclass(frozen=True, slots=True, kw_only=True)",
      `class ${shortName(item.name)}:`,
    );
    if (!item.properties.length) lines.push("    pass");
    for (const property of item.properties) {
      if (property.doc) lines.push(indentDoc(property.doc));
      lines.push(
        `    ${pythonIdentifier(property.name)}: ${pythonType(property.type)}${property.optional ? " | None = None" : ""}`,
      );
    }
    lines.push("");
  }
  lines.push("MODEL_TYPES = {");
  for (const item of catalog.declarations.filter(
    (value) => value.kind === "model" && value.model_kind === "object",
  )) {
    lines.push(`    ${pythonLiteral(item.name)}: ${shortName(item.name)},`);
  }
  lines.push("}", "", "ENUM_TYPES = {");
  for (const item of catalog.declarations.filter((value) => value.kind === "enum")) {
    lines.push(`    ${pythonLiteral(item.name)}: ${shortName(item.name)},`);
  }
  lines.push("}", "");
  return `${lines.join("\n").trimEnd()}\n`;
}

function generateCodecs() {
  const roots = catalog.roots.map((item) => ({ ...item, typeName: shortName(item.name) }));
  const rootTypes = [...new Set(roots.map((item) => item.typeName))];
  const descriptorMap = Object.fromEntries(
    catalog.declarations.map((item) => [item.name, descriptor(item)]),
  );
  const lines = [
    generatedHeader(),
    "from collections.abc import Callable",
    "from typing import Any, cast",
    "",
    "from ..._contract_runtime import decode_contract_json, encode_contract_json",
    `from .models import ENUM_TYPES, MODEL_TYPES, ${rootTypes.join(", ")}`,
    "",
    `DECLARATIONS: dict[str, dict[str, Any]] = ${pythonLiteral(descriptorMap)}`,
    "",
  ];
  for (const rootItem of roots) {
    const snake = pythonIdentifier(rootItem.typeName);
    lines.push(
      `def decode_${snake}_json(data: str | bytes | bytearray | memoryview) -> ${rootItem.typeName}:`,
      `    return cast(${rootItem.typeName}, decode_contract_json(data, ${pythonLiteral(rootItem.name)}, DECLARATIONS, MODEL_TYPES, ENUM_TYPES))`,
      "",
      `def encode_${snake}_json(value: ${rootItem.typeName}) -> bytes:`,
      `    return encode_contract_json(value, ${pythonLiteral(rootItem.name)}, DECLARATIONS, MODEL_TYPES, ENUM_TYPES)`,
      "",
    );
  }
  lines.push("ROOT_DECODERS: dict[str, Callable[[str | bytes | bytearray | memoryview], Any]] = {");
  for (const rootItem of roots) {
    lines.push(
      `    ${pythonLiteral(rootItem.contract_identity)}: decode_${pythonIdentifier(rootItem.typeName)}_json,`,
    );
  }
  lines.push("}", "");
  return `${lines.join("\n").trimEnd()}\n`;
}

function generateIndex() {
  return `${generatedHeader()}from .codecs import *  # noqa: F403\nfrom .models import *  # noqa: F403\n`;
}

function descriptor(item) {
  if (item.kind === "enum") {
    return { kind: "enum", values: item.members.map((member) => member.value) };
  }
  if (item.kind === "union") {
    return {
      kind: "union",
      variants: item.variants.map((variant) => typeDescriptor(variant.type)),
    };
  }
  if (item.kind !== "model") unsupported(item);
  if (item.model_kind === "array") {
    return {
      kind: "array",
      element: typeDescriptor(item.index_value),
      constraints: item.constraints,
    };
  }
  if (item.model_kind !== "object" || item.base !== null) unsupported(item);
  return {
    kind: "object",
    properties: Object.fromEntries(
      item.properties.map((property) => [
        property.name,
        {
          type: typeDescriptor(property.type),
          optional: property.optional,
          constraints: property.constraints,
          field: pythonIdentifier(property.name),
        },
      ]),
    ),
  };
}

function typeDescriptor(type) {
  if (["reference", "primitive", "literal"].includes(type.kind)) return type;
  if (type.kind === "array") return { kind: "array", element: typeDescriptor(type.element) };
  unsupported(type);
}

function pythonType(type) {
  if (type.kind === "reference") return shortName(type.target);
  if (type.kind === "primitive") {
    const mapped = {
      string: "str",
      boolean: "bool",
      float64: "float",
      uint32: "int",
      uint64: "int",
    }[type.name];
    if (!mapped) unsupported(type);
    return mapped;
  }
  if (type.kind === "literal") return `Literal[${pythonLiteral(type.value)}]`;
  if (type.kind === "array") return `tuple[${pythonType(type.element)}, ...]`;
  unsupported(type);
}

function arrayType(element, constraints) {
  if (
    constraints.min_items !== undefined &&
    constraints.min_items === constraints.max_items &&
    constraints.min_items <= 32
  ) {
    return `tuple[${Array.from({ length: constraints.min_items }, () => pythonType(element)).join(", ")}]`;
  }
  return `tuple[${pythonType(element)}, ...]`;
}

function topologicalOrder(items) {
  const result = [];
  const visited = new Set();
  const visiting = new Set();
  function visit(item) {
    if (visited.has(item.name)) return;
    if (visiting.has(item.name)) throw new Error(`Cyclic Python DTO dependency at ${item.name}.`);
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

function pythonLiteral(value, level = 0) {
  if (value === null) return "None";
  if (value === true) return "True";
  if (value === false) return "False";
  if (typeof value === "string") return JSON.stringify(value);
  if (typeof value === "number") return String(value);
  if (Array.isArray(value))
    return `[${value.map((item) => pythonLiteral(item, level + 1)).join(", ")}]`;
  if (typeof value === "object") {
    const entries = Object.entries(value);
    if (!entries.length) return "{}";
    const indent = "    ".repeat(level + 1);
    const closing = "    ".repeat(level);
    return `{\n${entries.map(([key, item]) => `${indent}${pythonLiteral(key)}: ${pythonLiteral(item, level + 1)},`).join("\n")}\n${closing}}`;
  }
  unsupported(value);
}

function pythonIdentifier(value) {
  const snake = value
    .replace(/([a-z0-9])([A-Z])/gu, "$1_$2")
    .replace(/[^A-Za-z0-9_]/gu, "_")
    .toLowerCase();
  return /^\d/u.test(snake) ? `field_${snake}` : snake;
}

function assertUniqueShortNames(items) {
  const seen = new Map();
  for (const item of items) {
    const short = shortName(item.name);
    if (seen.has(short))
      throw new Error(`Python short-name collision: ${seen.get(short)} and ${item.name}.`);
    seen.set(short, item.name);
  }
}

async function emit(filename, content) {
  await writeFile(join(stagingPath, filename), content, "utf8");
}

function formatGeneratedState() {
  const command = process.platform === "win32" ? "uv.exe" : "uv";
  const result = spawnSync(command, ["run", "ruff", "format", stagingPath], {
    cwd: root,
    encoding: "utf8",
  });
  if (result.error) throw result.error;
  if (result.status !== 0)
    throw new Error(`Ruff failed to normalize generated Python.\n${result.stdout}${result.stderr}`);
}

async function compareGeneratedFiles() {
  const differences = [];
  for (const filename of ["__init__.py", "codecs.py", "models.py"]) {
    const actual = join(stagingPath, filename);
    const expected = join(outputPath, filename);
    if (!existsSync(expected)) differences.push(`missing ${repositoryRelative(expected)}`);
    else if ((await readFile(actual)).compare(await readFile(expected)) !== 0) {
      differences.push(`stale ${repositoryRelative(expected)}`);
    }
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
    if (!existsSync(outputPath) && movedExisting && existsSync(backupPath))
      await rename(backupPath, outputPath);
    throw error;
  }
}

function generatedHeader() {
  return "# Generated from wn_geometer_contract_catalog.a0.json. Do not edit.\n";
}
function docComment(value) {
  return `# ${value.replaceAll("\n", " ")}`;
}
function indentDoc(value) {
  return `    # ${value.replaceAll("\n", " ")}`;
}
function shortName(value) {
  return value.slice(value.lastIndexOf(".") + 1);
}
function repositoryRelative(path) {
  return relative(root, path).split(sep).join("/");
}
function unsupported(value) {
  throw new Error(`Unsupported Python catalog construct ${JSON.stringify(value)}`);
}
