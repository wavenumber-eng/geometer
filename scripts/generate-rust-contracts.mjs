import { spawnSync } from "node:child_process";
import { createHash } from "node:crypto";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const catalogText = await readFile(
  join(root, "contracts/geometer/generated/wn_geometer_contract_catalog.a0.json"),
  "utf8",
);
const catalog = JSON.parse(catalogText);
const catalogSha256 = createHash("sha256").update(catalogText).digest("hex");
const output = join(root, catalog.output_roots.rust);
const check = process.argv.slice(2).includes("--check");
if (process.argv.length > (check ? 3 : 2)) {
  throw new Error("Usage: node scripts/generate-rust-contracts.mjs [--check]");
}

const declarations = new Map(catalog.declarations.map((item) => [item.name, item]));
const roots = new Set(catalog.roots.map((item) => item.name));
const ordered = topologicalOrder(catalog.declarations);
const source = formatRust(
  `${generateHeader()}\n${ordered.map(generateDeclaration).join("\n\n")}\n\n${generateRootCodecs()}\n`,
);

await emit("contracts.rs", source);
await emit(
  "mod.rs",
  formatRust(
    "// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.\n\npub mod contracts;\n",
  ),
);

async function emit(name, content) {
  const path = join(output, name);
  if (check) {
    let current;
    try {
      current = await readFile(path, "utf8");
    } catch {
      throw new Error(`Generated Rust contract file is missing: ${path}`);
    }
    if (current !== content) throw new Error(`Generated Rust contract file is stale: ${path}`);
    return;
  }
  await mkdir(dirname(path), { recursive: true });
  await writeFile(path, content, "utf8");
}

function generateHeader() {
  return `// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

use serde::{de::DeserializeOwned, Deserialize, Serialize};

pub const NORMALIZED_CATALOG_SHA256: &str = "${catalogSha256}";

#[derive(Debug, thiserror::Error)]
pub enum ContractError {
    #[error("invalid JSON contract: {0}")]
    Json(#[from] serde_json::Error),
    #[error("contract validation failed at {path}: {message}")]
    Validation { path: String, message: String },
}

pub trait Validate {
    fn validate_at(&self, path: &str) -> Result<(), ContractError>;
}

pub fn decode_json<T: DeserializeOwned + Validate>(data: &[u8]) -> Result<T, ContractError> {
    let mut deserializer = serde_json::Deserializer::from_slice(data);
    let value = T::deserialize(&mut deserializer)?;
    deserializer.end()?;
    value.validate_at("")?;
    Ok(value)
}

pub fn encode_json<T: Serialize + Validate>(value: &T) -> Result<Vec<u8>, ContractError> {
    value.validate_at("")?;
    Ok(serde_json::to_vec(value)?)
}

fn child_path(path: &str, token: &str) -> String {
    format!("{path}/{}", token.replace('~', "~0").replace('/', "~1"))
}

fn invalid(path: &str, message: &str) -> ContractError {
    ContractError::Validation { path: path.to_owned(), message: message.to_owned() }
}

fn deserialize_optional_non_null<'de, D, T>(deserializer: D) -> Result<Option<T>, D::Error>
where
    D: serde::Deserializer<'de>,
    T: Deserialize<'de>,
{
    T::deserialize(deserializer).map(Some)
}`;
}

function generateDeclaration(item) {
  if (item.kind === "enum") return generateEnum(item);
  if (item.kind === "union") return generateUnion(item);
  if (item.kind === "model" && item.model_kind === "array") return generateArrayModel(item);
  if (item.kind === "model" && item.model_kind === "object") return generateObject(item);
  throw new Error(`Unsupported Rust declaration ${JSON.stringify(item)}`);
}

function generateEnum(item) {
  const variants = item.members
    .map(
      (member) =>
        `    #[serde(rename = ${JSON.stringify(member.value)})]\n    ${pascal(member.name)},`,
    )
    .join("\n");
  return `#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum ${shortName(item.name)} {
${variants}
}

impl Validate for ${shortName(item.name)} {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> { Ok(()) }
}`;
}

function generateUnion(item) {
  const variants = item.variants
    .map((variant) => `    ${pascal(variant.name)}(${rustType(variant.type)}),`)
    .join("\n");
  const validation = item.variants
    .map(
      (variant) => `            Self::${pascal(variant.name)}(value) => value.validate_at(path),`,
    )
    .join("\n");
  return `#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(untagged)]
pub enum ${shortName(item.name)} {
${variants}
}

impl Validate for ${shortName(item.name)} {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
${validation}
        }
    }
}`;
}

function generateArrayModel(item) {
  const name = shortName(item.name);
  const size = item.constraints.min_items;
  if (size === undefined || size !== item.constraints.max_items) {
    throw new Error(`Rust array root ${item.name} must have an exact size.`);
  }
  const itemValidation = validateValue(item.index_value, "item", "item_path", {});
  return `pub type ${name} = [${rustType(item.index_value)}; ${size}];

impl Validate for ${name} {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        for (index, item) in self.iter().enumerate() {
            let item_path = child_path(path, &index.to_string());
${itemValidation.map((line) => `            ${line}`).join("\n")}
        }
        Ok(())
    }
}`;
}

function generateObject(item) {
  const name = shortName(item.name);
  const fields = item.properties
    .map((property) => {
      const fieldName = snakeCase(property.name);
      const serdeOptions = [];
      if (fieldName !== property.name)
        serdeOptions.push(`rename = ${JSON.stringify(property.name)}`);
      if (property.optional)
        serdeOptions.push(
          "default",
          'deserialize_with = "deserialize_optional_non_null"',
          'skip_serializing_if = "Option::is_none"',
        );
      const attributes = serdeOptions.length ? `    #[serde(${serdeOptions.join(", ")})]\n` : "";
      const type = property.optional
        ? `Option<${rustType(property.type)}>`
        : rustType(property.type);
      return `${attributes}    pub ${fieldName}: ${type},`;
    })
    .join("\n");
  const checks = item.properties.flatMap((property) => validationLines(property));
  return `#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ${name} {
${fields}
}

impl Validate for ${name} {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
${checks.length ? checks.map((line) => `        ${line}`).join("\n") : "        let _ = path;"}
        Ok(())
    }
}`;
}

function validationLines(property) {
  const access = `self.${snakeCase(property.name)}`;
  const path = `child_path(path, ${JSON.stringify(property.name)})`;
  const body = validateValue(property.type, "value", "field_path", property.constraints ?? {});
  const literal = property.type.kind === "literal";
  if (body.length === 0 && !literal) return [];
  const lines = [`let field_path = ${path};`];
  if (property.optional) {
    lines.push(`if let Some(value) = &${access} {`, ...body.map((line) => `    ${line}`), "}");
  } else {
    lines.push(`let value = &${access};`, ...body);
  }
  if (literal) {
    const comparison =
      property.type.value_type === "string"
        ? `value != ${JSON.stringify(property.type.value)}`
        : property.type.value_type === "boolean"
          ? property.type.value
            ? "!*value"
            : "*value"
          : `*value != ${String(property.type.value)}`;
    lines.push(
      `if ${comparison} { return Err(invalid(&field_path, "literal value does not match the contract")); }`,
    );
  }
  return lines;
}

function formatRust(content) {
  const result = spawnSync("rustfmt", ["--edition", "2024"], {
    cwd: root,
    encoding: "utf8",
    input: content,
  });
  if (result.error)
    throw new Error(`rustfmt is required for Rust contract generation: ${result.error.message}`);
  if (result.status !== 0) throw new Error(`rustfmt failed:\n${result.stderr}`);
  return result.stdout;
}

function validateValue(type, value, path, constraints) {
  const lines = [];
  if (type.kind === "reference") lines.push(`${value}.validate_at(&${path})?;`);
  if (type.kind === "primitive" && type.name === "float64") {
    lines.push(
      `if !${value}.is_finite() { return Err(invalid(&${path}, "number must be finite")); }`,
    );
  }
  if (type.kind === "primitive" && ["uint32", "uint64"].includes(type.name)) {
    if (constraints.min_value !== undefined)
      lines.push(
        `if *${value} < ${constraints.min_value} { return Err(invalid(&${path}, "number is below its minimum")); }`,
      );
    if (constraints.max_value !== undefined)
      lines.push(
        `if *${value} > ${constraints.max_value} { return Err(invalid(&${path}, "number exceeds its maximum")); }`,
      );
  }
  if (type.kind === "primitive" && type.name === "string") {
    if (constraints.min_length !== undefined)
      lines.push(
        constraints.min_length === 1
          ? `if ${value}.is_empty() { return Err(invalid(&${path}, "string is shorter than its minimum")); }`
          : `if ${value}.len() < ${constraints.min_length} { return Err(invalid(&${path}, "string is shorter than its minimum")); }`,
      );
    if (constraints.max_length !== undefined)
      lines.push(
        `if ${value}.len() > ${constraints.max_length} { return Err(invalid(&${path}, "string exceeds its maximum")); }`,
      );
  }
  if (type.kind === "array") {
    if (constraints.min_items !== undefined)
      lines.push(
        constraints.min_items === 1
          ? `if ${value}.is_empty() { return Err(invalid(&${path}, "array is shorter than its minimum")); }`
          : `if ${value}.len() < ${constraints.min_items} { return Err(invalid(&${path}, "array is shorter than its minimum")); }`,
      );
    if (constraints.max_items !== undefined)
      lines.push(
        `if ${value}.len() > ${constraints.max_items} { return Err(invalid(&${path}, "array exceeds its maximum")); }`,
      );
    if (type.element.kind === "reference")
      lines.push(
        `for (index, item) in ${value}.iter().enumerate() { item.validate_at(&child_path(&${path}, &index.to_string()))?; }`,
      );
  }
  if (type.kind === "primitive" && type.name === "float64" && constraints.min_value !== undefined)
    lines.push(
      `if *${value} < ${constraints.min_value}_f64 { return Err(invalid(&${path}, "number is below its minimum")); }`,
    );
  if (type.kind === "primitive" && type.name === "float64" && constraints.max_value !== undefined)
    lines.push(
      `if *${value} > ${constraints.max_value}_f64 { return Err(invalid(&${path}, "number exceeds its maximum")); }`,
    );
  return lines;
}

function generateRootCodecs() {
  return [...roots]
    .map((rootName) => {
      const name = shortName(rootName);
      const snake = snakeCase(name);
      return `pub fn decode_${snake}_json(data: &[u8]) -> Result<${name}, ContractError> {
    decode_json(data)
}

pub fn encode_${snake}_json(value: &${name}) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}`;
    })
    .join("\n\n");
}

function rustType(type) {
  if (type.kind === "reference") return shortName(type.target);
  if (type.kind === "primitive") {
    const mapped = {
      string: "String",
      boolean: "bool",
      float64: "f64",
      uint32: "u32",
      uint64: "u64",
    }[type.name];
    if (mapped) return mapped;
  }
  if (type.kind === "literal") {
    const mapped = { string: "String", boolean: "bool", number: "f64" }[type.value_type];
    if (mapped) return mapped;
  }
  if (type.kind === "array") return `Vec<${rustType(type.element)}>`;
  throw new Error(`Unsupported Rust type ${JSON.stringify(type)}`);
}

function topologicalOrder(items) {
  const result = [];
  const visited = new Set();
  const visiting = new Set();
  const visit = (item) => {
    if (visited.has(item.name)) return;
    if (visiting.has(item.name)) throw new Error(`Cyclic Rust DTO dependency at ${item.name}.`);
    visiting.add(item.name);
    for (const dependency of dependencies(item)) {
      const declaration = declarations.get(dependency);
      if (declaration) visit(declaration);
    }
    visiting.delete(item.name);
    visited.add(item.name);
    result.push(item);
  };
  for (const item of items) visit(item);
  return result;
}

function dependencies(item) {
  const found = new Set();
  const scan = (type) => {
    if (!type) return;
    if (type.kind === "reference") found.add(type.target);
    if (type.kind === "array") scan(type.element);
  };
  if (item.index_value) scan(item.index_value);
  for (const property of item.properties ?? []) scan(property.type);
  for (const variant of item.variants ?? []) scan(variant.type);
  return found;
}

function shortName(name) {
  return name.slice(name.lastIndexOf(".") + 1);
}

function pascal(name) {
  return name
    .split(/[^A-Za-z0-9]+/u)
    .filter(Boolean)
    .map((part) => `${part[0].toUpperCase()}${part.slice(1)}`)
    .join("");
}

function snakeCase(name) {
  return name.replace(/([a-z0-9])([A-Z])/gu, "$1_$2").toLowerCase();
}
