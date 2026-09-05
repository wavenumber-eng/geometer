import { spawnSync } from "node:child_process";
import { createHash } from "node:crypto";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import { applyProjectionDeferrals } from "./contract-projection-deferral.mjs";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const catalogText = await readFile(
  join(root, "contracts/geometer/generated/wn_geometer_contract_catalog.a0.json"),
  "utf8",
);
const contractCatalog = JSON.parse(catalogText);
const modelCatalog = await applyProjectionDeferrals(contractCatalog, "rust", {
  retainLogicalDtos: true,
});
const codecCatalog = await applyProjectionDeferrals(contractCatalog, "rust");
const packedContracts = new Set(
  contractCatalog.operations
    .filter((operation) => operation.runtime_dispatch === "packed_attachment")
    .flatMap((operation) => [operation.request_contract, operation.result_contract]),
);
const codecRoots = codecCatalog.roots.filter(
  (rootRecord) => !packedContracts.has(rootRecord.contract_identity),
);
const catalogSha256 = createHash("sha256").update(catalogText).digest("hex");
const output = join(root, contractCatalog.output_roots.rust);
const check = process.argv.slice(2).includes("--check");
if (process.argv.length > (check ? 3 : 2)) {
  throw new Error("Usage: node scripts/generate-rust-contracts.mjs [--check]");
}

const declarations = new Map(modelCatalog.declarations.map((item) => [item.name, item]));
const codecDeclarations = reachableNames(codecRoots);
const roots = new Set(codecRoots.map((item) => item.name));
const ordered = topologicalOrder(modelCatalog.declarations);
const emittedArrayValidation = new Set();
const source = formatRust(
  `${generateHeader()}\n${ordered.map((item) => generateDeclaration(item, codecDeclarations.has(item.name))).join("\n\n")}\n\n${generateRootCodecs()}\n`,
);

await emit("contracts.rs", source);
await emit("operations.rs", formatRust(generateOperations()));
await emit("dispatch.rs", formatRust(generateDispatch()));
await emit(
  "mod.rs",
  formatRust(
    "// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.\n\npub mod contracts;\npub mod dispatch;\npub mod operations;\n",
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

// IPC unions and contract identities are authored in TypeSpec. Keep transport
// validation exhaustive as operations are added; never maintain a second list
// of supported logical DTOs in the handwritten client.
function generateDispatch() {
  const variantsFor = (unionName) => {
    const union = modelCatalog.declarations.find((item) => shortName(item.name) === unionName);
    if (!union || union.kind !== "union") throw new Error(`Missing IPC union ${unionName}`);
    return union.variants.map((variant) => {
      const rootRecord = codecRoots.find((item) => item.name === variant.type.target);
      if (!rootRecord && variant.type.target?.endsWith(".PackedAttachmentProjectionA0"))
        return { variant: pascal(variant.name), contract: null };
      if (!rootRecord) throw new Error(`IPC variant lacks a codec root: ${variant.name}`);
      return {
        variant: pascal(variant.name),
        contract: rootRecord.contract_identity,
        type: shortName(rootRecord.name),
      };
    });
  };
  const requests = variantsFor("IpcRequestValueA0");
  const results = variantsFor("OperationResultValueA0");
  const identityFunction = (name, type, variants) => `
pub fn ${name}(value: &contracts::${type}) -> Option<&'static str> {
    match value {
${variants.map((item) => `        contracts::${type}::${item.variant}(_) => ${item.contract ? `Some(${JSON.stringify(item.contract)})` : "None"},`).join("\n")}
    }
}`;
  return `// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

use super::contracts;

/// Decode using the negotiated operation's exact contract, not union trial order.
pub fn decode_logical_request(
    contract: &str,
    data: &[u8],
) -> Result<contracts::IpcRequestValueA0, contracts::ContractError> {
    match contract {
${requests
  .filter((item) => item.contract)
  .map(
    (item) =>
      `        ${JSON.stringify(item.contract)} => Ok(contracts::IpcRequestValueA0::${item.variant}(contracts::decode_json::<contracts::${item.type}>(data)?)),`,
  )
  .join("\n")}
        _ => Err(contracts::ContractError::Validation {
            path: "/request".to_owned(),
            message: format!("no generated logical request codec for {contract}"),
        }),
    }
}
${identityFunction("logical_request_contract", "IpcRequestValueA0", requests)}
${identityFunction("logical_result_contract", "OperationResultValueA0", results)}
`;
}

function generateHeader() {
  return `// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

#![allow(clippy::approx_constant, reason = "schema bounds retain their exact generated decimal form")]
#![allow(clippy::large_enum_variant, reason = "generated wire DTOs preserve their unboxed contract shape")]
#![allow(clippy::cognitive_complexity, reason = "generated closed-union decoders enumerate governed variants")]
#![allow(clippy::too_many_lines, reason = "generated validators enumerate every governed field")]

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

function generateOperations() {
  const runtimeCatalog = operationCatalogTemplate();
  const analytic = contractCatalog.operations.find(
    (operation) => operation.identity === "geometry.analytic_planar_boolean_batch.a0",
  );
  if (!analytic) throw new Error("Analytic operation is absent from the normalized catalog.");
  return `// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

use super::contracts::{self, IpcOperationCatalogA0};

pub const ANALYTIC_PLANAR_BOOLEAN_BATCH_A0_IDENTITY: &str = ${JSON.stringify(analytic.identity)};

const OPERATION_CATALOG_TEMPLATE: &str = r#"${JSON.stringify(runtimeCatalog)}"#;

pub fn expected_operation_catalog(
    release_version: &str,
    c_abi_generation: u32,
) -> IpcOperationCatalogA0 {
    let mut value: serde_json::Value = serde_json::from_str(OPERATION_CATALOG_TEMPLATE)
        .expect("generated operation catalog template must be valid JSON");
    value["release_version"] = serde_json::Value::String(release_version.to_owned());
    value["c_abi_generation"] = serde_json::Value::Number(c_abi_generation.into());
    let bytes = serde_json::to_vec(&value)
        .expect("generated operation catalog value must serialize");
    contracts::decode_ipc_operation_catalog_a0_json(&bytes)
        .expect("generated operation catalog must satisfy its contract")
}`;
}

function operationCatalogTemplate() {
  return {
    catalog: "wn.geometer.operation_catalog.a0",
    generic_abi: "a0",
    release_version: "",
    c_abi_generation: 0,
    operations: contractCatalog.operations
      .filter((operation) => operation.runtime_available || operation.native_runtime_available)
      .map((operation) => ({
        identity: operation.identity,
        request_contract: operation.request_contract,
        result_contract: operation.result_contract,
        input_attachments: operation.input_attachments,
        output_attachments: operation.output_attachments,
        runtime_dispatch: operation.runtime_dispatch,
        ...(operation.request_projection
          ? { request_projection: operation.request_projection }
          : {}),
        ...(operation.result_projection ? { result_projection: operation.result_projection } : {}),
      })),
    attachment_descriptor: {
      wasm32: {
        size: 36,
        offsets: {
          struct_size: 0,
          flags: 4,
          name: 8,
          name_size: 12,
          media_type: 16,
          media_type_size: 20,
          data: 24,
          data_size: 28,
          reserved0: 32,
        },
      },
      pointer64: {
        size: 56,
        offsets: {
          struct_size: 0,
          flags: 4,
          name: 8,
          name_size: 16,
          media_type: 24,
          media_type_size: 32,
          data: 40,
          data_size: 48,
          reserved0: 52,
        },
      },
    },
    limits: {
      operation_id_bytes: 128,
      request_json_bytes: 8 * 1024 * 1024,
      response_json_bytes: 8 * 1024 * 1024,
      attachment_count: 16,
      attachment_name_bytes: 128,
      attachment_media_type_bytes: 128,
      attachment_bytes: 256 * 1024 * 1024,
      aggregate_attachment_bytes_native: 512 * 1024 * 1024,
      aggregate_attachment_bytes_wasm: 256 * 1024 * 1024,
    },
  };
}

function generateDeclaration(item, jsonWire) {
  if (item.kind === "enum") return generateEnum(item, jsonWire);
  if (item.kind === "union") return generateUnion(item, jsonWire);
  if (item.kind === "scalar") return generateScalar(item, jsonWire);
  if (item.kind === "model" && item.model_kind === "array") return generateArrayModel(item);
  if (item.kind === "model" && item.model_kind === "object") return generateObject(item, jsonWire);
  throw new Error(`Unsupported Rust declaration ${JSON.stringify(item)}`);
}

function generateEnum(item, jsonWire) {
  const variants = item.members
    .map(
      (member) =>
        `${jsonWire ? `    #[serde(rename = ${JSON.stringify(member.value)})]\n` : ""}    ${pascal(member.name)},`,
    )
    .join("\n");
  return `#[derive(Clone, Debug, ${jsonWire ? "Deserialize, " : ""}PartialEq${jsonWire ? ", Serialize" : ""})]
pub enum ${shortName(item.name)} {
${variants}
}

impl Validate for ${shortName(item.name)} {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> { Ok(()) }
}`;
}

function generateUnion(item, jsonWire) {
  const name = shortName(item.name);
  const variants = item.variants
    .map((variant) => `    ${pascal(variant.name)}(${rustType(variant.type)}),`)
    .join("\n");
  const validation = item.variants
    .map(
      (variant) => `            Self::${pascal(variant.name)}(value) => value.validate_at(path),`,
    )
    .join("\n");
  const deserialization = jsonWire
    ? `

impl<'de> Deserialize<'de> for ${name} {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let raw = Box::<serde_json::value::RawValue>::deserialize(deserializer)?;
${item.variants
  .map(
    (
      variant,
    ) => `        if let Ok(value) = serde_json::from_str::<${rustType(variant.type)}>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::${pascal(variant.name)}(value));
            }
        }`,
  )
  .join("\n")}
        Err(serde::de::Error::custom("value does not match any ${name} variant"))
    }
}`
    : "";
  return `#[derive(Clone, Debug, PartialEq${jsonWire ? ", Serialize" : ""})]
${jsonWire ? "#[serde(untagged)]\n" : ""}pub enum ${name} {
${variants}
}${deserialization}

impl Validate for ${name} {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
${validation}
        }
    }
}`;
}

function generateScalar(item, jsonWire) {
  const name = shortName(item.name);
  if (
    jsonWire ||
    item.base?.kind !== "primitive" ||
    item.base.name !== "uint64" ||
    item.constraints?.min_value !== 1 ||
    item.constraints?.max_value !== undefined
  ) {
    throw new Error(`Unsupported Rust scalar declaration ${JSON.stringify(item)}`);
  }
  return `#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct ${name}(std::num::NonZeroU64);

impl ${name} {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 { self.0.get() }
}

impl TryFrom<u64> for ${name} {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<${name}> for u64 {
    fn from(value: ${name}) -> Self { value.get() }
}

impl Validate for ${name} {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> { Ok(()) }
}`;
}

function generateArrayModel(item) {
  const name = shortName(item.name);
  const size = item.constraints.min_items;
  if (size === undefined || size !== item.constraints.max_items) {
    throw new Error(`Rust array root ${item.name} must have an exact size.`);
  }
  const itemValidation = validateValue(item.index_value, "item", "item_path", {});
  const alias = `pub type ${name} = [${rustType(item.index_value)}; ${size}];`;
  const implementationKey = `${rustType(item.index_value)};${size}`;
  if (emittedArrayValidation.has(implementationKey)) return alias;
  emittedArrayValidation.add(implementationKey);
  return `${alias}

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

function generateObject(item, jsonWire) {
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
      const attributes =
        jsonWire && serdeOptions.length ? `    #[serde(${serdeOptions.join(", ")})]\n` : "";
      const type = property.optional
        ? `Option<${rustType(property.type)}>`
        : rustType(property.type);
      return `${attributes}    pub ${fieldName}: ${type},`;
    })
    .join("\n");
  const checks = item.properties.flatMap((property) => validationLines(property));
  return `#[derive(Clone, Debug, ${jsonWire ? "Deserialize, " : ""}PartialEq${jsonWire ? ", Serialize" : ""})]
${jsonWire ? "#[serde(deny_unknown_fields)]\n" : ""}pub struct ${name} {
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
    if (constraints.min_value !== undefined && constraints.min_value > 0)
      lines.push(
        `if *${value} < ${constraints.min_value} { return Err(invalid(&${path}, "number is below its minimum")); }`,
      );
    const typeMaximum = type.name === "uint32" ? 4294967295 : Number.POSITIVE_INFINITY;
    if (constraints.max_value !== undefined && constraints.max_value < typeMaximum)
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
  if (
    type.kind === "primitive" &&
    type.name === "float64" &&
    constraints.min_value_exclusive !== undefined
  )
    lines.push(
      `if *${value} <= ${constraints.min_value_exclusive}_f64 { return Err(invalid(&${path}, "number is not above its exclusive minimum")); }`,
    );
  if (type.kind === "primitive" && type.name === "float64" && constraints.max_value !== undefined)
    lines.push(
      `if *${value} > ${constraints.max_value}_f64 { return Err(invalid(&${path}, "number exceeds its maximum")); }`,
    );
  if (
    type.kind === "primitive" &&
    type.name === "float64" &&
    constraints.max_value_exclusive !== undefined
  )
    lines.push(
      `if *${value} >= ${constraints.max_value_exclusive}_f64 { return Err(invalid(&${path}, "number is not below its exclusive maximum")); }`,
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
      int64: "i64",
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

function reachableNames(rootRecords) {
  const found = new Set();
  const visit = (name) => {
    if (found.has(name)) return;
    const declaration = declarations.get(name);
    if (!declaration) throw new Error(`Catalog reference does not resolve: ${name}.`);
    found.add(name);
    for (const dependency of dependencies(declaration)) visit(dependency);
  };
  for (const rootRecord of rootRecords) visit(rootRecord.name);
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
