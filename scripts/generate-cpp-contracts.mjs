// @ts-check

import { spawnSync } from "node:child_process";
import { createHash } from "node:crypto";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import { applyProjectionDeferrals } from "./contract-projection-deferral.mjs";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const catalogPath = join(root, "contracts/geometer/generated/wn_geometer_contract_catalog.a0.json");
const catalogText = await readFile(catalogPath, "utf8");
const contractCatalog = JSON.parse(catalogText);
const projectionCatalog = await applyProjectionDeferrals(contractCatalog, "cpp", {
  retainRuntimeDispatch: true,
});
const packedContracts = new Set(
  projectionCatalog.operations
    .filter((operation) => operation.runtime_dispatch === "packed_attachment")
    .flatMap((operation) => [operation.request_contract, operation.result_contract]),
);
const jsonCodecRoots = projectionCatalog.roots.filter(
  (rootRecord) => !packedContracts.has(rootRecord.contract_identity),
);
const catalogSha256 = createHash("sha256").update(catalogText).digest("hex");
const output = join(root, contractCatalog.output_roots.cpp);
const checkOnly = process.argv.includes("--check");
if (process.argv.some((value, index) => index > 1 && value !== "--check")) {
  throw new Error("Usage: node scripts/generate-cpp-contracts.mjs [--check]");
}

const declarations = new Map(projectionCatalog.declarations.map((item) => [item.name, item]));
const shortNames = new Map();
for (const declaration of projectionCatalog.declarations) {
  const name = shortName(declaration.name);
  if (shortNames.has(name)) throw new Error(`Duplicate C++ declaration name ${name}.`);
  shortNames.set(name, declaration.name);
}

const ordered = topologicalOrder(projectionCatalog.declarations);
const codecOrdered = topologicalOrder(reachableDeclarationItems(jsonCodecRoots));
const header = formatCpp(generateHeader(), "contracts.h");
const source = formatCpp(generateSource(), "contracts_json.cpp");
const operationCatalogSource = formatCpp(generateOperationCatalogSource(), "operation_catalog.cpp");
await mkdir(output, { recursive: true });
await emit("contracts.h", header);
await emit("contracts_json.cpp", source);
await emit("operation_catalog.cpp", operationCatalogSource);
process.stdout.write(
  checkOnly
    ? "Generated C++ contracts are current.\n"
    : "Generated C++ contract DTOs and codecs.\n",
);

async function emit(name, content) {
  const path = join(output, name);
  const normalized = `${content.trimEnd()}\n`;
  if (checkOnly) {
    let existing = "";
    try {
      existing = await readFile(path, "utf8");
    } catch {}
    if (existing !== normalized) throw new Error(`Generated C++ contract file is stale: ${path}`);
  } else {
    await writeFile(path, normalized, "utf8");
  }
}

function generateHeader() {
  const lines = [
    "// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.",
    "#pragma once",
    "",
    "#include <cstddef>",
    "#include <cstdint>",
    "#include <optional>",
    "#include <string>",
    "#include <variant>",
    "#include <vector>",
    "",
    "namespace geometer::contracts",
    "{",
    "",
    "struct ContractError",
    "{",
    "    std::string code;",
    "    std::string path;",
    "    std::string message;",
    "};",
    "",
  ];
  for (const declaration of ordered) lines.push(...headerDeclaration(declaration), "");
  for (const rootRecord of jsonCodecRoots) {
    const type = shortName(rootRecord.name);
    lines.push(
      `bool decode_json(const unsigned char* data, std::size_t size, ${type}* value,`,
      "                 ContractError* error = nullptr);",
      `bool encode_json(const ${type}& value, std::string* json, ContractError* error = nullptr);`,
      "",
    );
  }
  lines.push("} // namespace geometer::contracts");
  return lines.join("\n");
}

function headerDeclaration(item) {
  const name = shortName(item.name);
  if (item.kind === "enum") {
    return [
      `enum class ${name}`,
      "{",
      ...item.members.map((member) => `    ${member.name},`),
      "};",
    ];
  }
  if (item.kind === "union") {
    return [
      `using ${name} = std::variant<${item.variants.map((variant) => cppType(variant.type)).join(", ")}>;`,
    ];
  }
  if (item.kind === "scalar") return [`using ${name} = ${cppType(item.base)};`];
  if (item.kind !== "model") throw new Error(`Unsupported declaration kind ${item.kind}.`);
  if (item.model_kind === "array")
    return [`using ${name} = std::vector<${cppType(item.index_value)}>;`];
  if (item.model_kind !== "object") throw new Error(`Unsupported model kind ${item.model_kind}.`);
  return [
    `struct ${name}`,
    "{",
    ...item.properties.map((property) => {
      const type = cppType(property.type);
      const wrapped = property.optional ? `std::optional<${type}>` : type;
      return `    ${wrapped} ${property.name}${property.optional ? "{}" : valueInitializer(property.type)};`;
    }),
    "};",
  ];
}

function generateSource() {
  const lines = [
    "// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.",
    '#include "geometer/generated/contracts/contracts.h"',
    "",
    "#include <cmath>",
    "#include <cstring>",
    "#include <limits>",
    "#include <rapidjson/document.h>",
    "#include <rapidjson/error/en.h>",
    "#include <rapidjson/stringbuffer.h>",
    "#include <rapidjson/writer.h>",
    "#include <type_traits>",
    "",
    "namespace geometer::contracts",
    "{",
    "namespace",
    "{",
    "",
    ...codecOrdered.flatMap((item) => [
      `bool decode_${shortName(item.name)}(const rapidjson::Value&, ${shortName(item.name)}*, const std::string&, ContractError*);`,
      `bool write_${shortName(item.name)}(rapidjson::Writer<rapidjson::StringBuffer>&, const ${shortName(item.name)}&, ContractError*);`,
    ]),
    "",
    "constexpr std::size_t kMaxJsonBytes = 8U * 1024U * 1024U;",
    "",
    "bool fail(ContractError* error, const char* code, const std::string& path, const std::string& message)",
    "{",
    "    if (error != nullptr) *error = {code, path, message};",
    "    return false;",
    "}",
    "",
    "bool valid_utf8(const char* data, std::size_t size)",
    "{",
    "    const auto* bytes = reinterpret_cast<const unsigned char*>(data); std::size_t index = 0;",
    "    while (index < size) {",
    "        const unsigned char first = bytes[index++]; if (first <= 0x7fU) continue;",
    "        unsigned int remaining = 0; unsigned int code_point = 0;",
    "        if (first >= 0xc2U && first <= 0xdfU) { remaining = 1; code_point = first & 0x1fU; }",
    "        else if (first >= 0xe0U && first <= 0xefU) { remaining = 2; code_point = first & 0x0fU; }",
    "        else if (first >= 0xf0U && first <= 0xf4U) { remaining = 3; code_point = first & 0x07U; }",
    "        else return false;",
    "        if (index + remaining > size) return false;",
    "        for (unsigned int offset = 0; offset < remaining; ++offset) { const unsigned char next = bytes[index++]; if ((next & 0xc0U) != 0x80U) return false; code_point = (code_point << 6U) | (next & 0x3fU); }",
    "        if ((remaining == 2 && code_point < 0x800U) || (remaining == 3 && code_point < 0x10000U) || code_point > 0x10ffffU || (code_point >= 0xd800U && code_point <= 0xdfffU)) return false;",
    "    }",
    "    return true;",
    "}",
    "",
    "std::string child_path(const std::string& parent, const char* name, std::size_t size)",
    "{",
    "    std::string escaped;",
    "    for (std::size_t index = 0; index < size; ++index) { const char c = name[index]; if (c == '~') escaped += \"~0\"; else if (c == '/') escaped += \"~1\"; else escaped += c; }",
    '    return parent + "/" + escaped;',
    "}",
    "",
    "std::string child_path(const std::string& parent, const char* name)",
    "{",
    "    return child_path(parent, name, std::strlen(name));",
    "}",
    "",
    "bool validate_object(const rapidjson::Value& value, const char* const* names, std::size_t count, const std::string& path, ContractError* error)",
    "{",
    '    if (!value.IsObject()) return fail(error, "geometer.contract.type_mismatch", path, "Expected an object.");',
    "    for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it) {",
    '        if (!it->name.IsString()) return fail(error, "geometer.contract.invalid_member", path, "Object member name is invalid.");',
    "        bool known = false; for (std::size_t i = 0; i < count; ++i) if (it->name.GetStringLength() == std::strlen(names[i]) && std::memcmp(it->name.GetString(), names[i], it->name.GetStringLength()) == 0) known = true;",
    '        if (!known) return fail(error, "geometer.contract.unknown_field", child_path(path, it->name.GetString(), it->name.GetStringLength()), "Unknown field.");',
    "        for (auto jt = value.MemberBegin(); jt != it; ++jt) if (jt->name.GetStringLength() == it->name.GetStringLength() && std::memcmp(jt->name.GetString(), it->name.GetString(), it->name.GetStringLength()) == 0)",
    '            return fail(error, "geometer.contract.duplicate_field", child_path(path, it->name.GetString(), it->name.GetStringLength()), "Duplicate field.");',
    "    }",
    "    return true;",
    "}",
    "",
    "bool decode_string(const rapidjson::Value& value, std::string* out, const std::string& path, ContractError* error, std::size_t minimum, std::size_t maximum)",
    "{",
    '    if (!value.IsString()) return fail(error, "geometer.contract.type_mismatch", path, "Expected a string.");',
    '    const std::size_t size = value.GetStringLength(); if (size < minimum || size > maximum) return fail(error, "geometer.contract.string_length", path, "String length is outside its contract bounds.");',
    "    out->assign(value.GetString(), size); return true;",
    "}",
    "",
    "bool decode_boolean(const rapidjson::Value& value, bool* out, const std::string& path, ContractError* error)",
    "{",
    '    if (!value.IsBool()) return fail(error, "geometer.contract.type_mismatch", path, "Expected a boolean."); *out = value.GetBool(); return true;',
    "}",
    "",
    "bool decode_uint32(const rapidjson::Value& value, std::uint32_t* out, const std::string& path, ContractError* error, std::uint64_t minimum, std::uint64_t maximum)",
    "{",
    '    if (!value.IsUint64() || value.GetUint64() < minimum || value.GetUint64() > maximum || value.GetUint64() > std::numeric_limits<std::uint32_t>::max()) return fail(error, "geometer.contract.number_range", path, "Expected an unsigned 32-bit integer within its contract bounds."); *out = static_cast<std::uint32_t>(value.GetUint64()); return true;',
    "}",
    "",
    "bool decode_uint64(const rapidjson::Value& value, std::uint64_t* out, const std::string& path, ContractError* error, std::uint64_t minimum, std::uint64_t maximum)",
    "{",
    '    if (!value.IsUint64() || value.GetUint64() < minimum || value.GetUint64() > maximum) return fail(error, "geometer.contract.number_range", path, "Expected an unsigned 64-bit integer within its contract bounds."); *out = value.GetUint64(); return true;',
    "}",
    "",
    "bool decode_double(const rapidjson::Value& value, double* out, const std::string& path, ContractError* error, double minimum, double maximum, bool minimum_exclusive, bool maximum_exclusive)",
    "{",
    '    if (!value.IsNumber() || !std::isfinite(value.GetDouble())) return fail(error, "geometer.contract.type_mismatch", path, "Expected a finite number.");',
    '    const double number = value.GetDouble(); if (number < minimum || number > maximum || (minimum_exclusive && number == minimum) || (maximum_exclusive && number == maximum)) return fail(error, "geometer.contract.number_range", path, "Number is outside its contract bounds."); *out = number; return true;',
    "}",
    "",
    "bool decode_literal_string(const rapidjson::Value& value, std::string* out, const std::string& path, ContractError* error, const char* expected)",
    "{",
    '    if (!value.IsString() || value.GetStringLength() != std::strlen(expected) || std::memcmp(value.GetString(), expected, value.GetStringLength()) != 0) return fail(error, "geometer.contract.literal_mismatch", path, "String literal does not match."); *out = expected; return true;',
    "}",
    "",
    "bool decode_literal_boolean(const rapidjson::Value& value, bool* out, const std::string& path, ContractError* error, bool expected)",
    "{",
    '    if (!value.IsBool() || value.GetBool() != expected) return fail(error, "geometer.contract.literal_mismatch", path, "Boolean literal does not match."); *out = expected; return true;',
    "}",
    "",
    "template <typename T>",
    "bool decode_array(const rapidjson::Value& value, std::vector<T>* out, const std::string& path, ContractError* error, std::size_t minimum, std::size_t maximum, bool (*decode_item)(const rapidjson::Value&, T*, const std::string&, ContractError*))",
    "{",
    '    if (!value.IsArray() || value.Size() < minimum || value.Size() > maximum) return fail(error, "geometer.contract.array_size", path, "Array length is outside its contract bounds.");',
    '    out->clear(); out->reserve(value.Size()); for (rapidjson::SizeType i = 0; i < value.Size(); ++i) { T item{}; if (!decode_item(value[i], &item, path + "/" + std::to_string(i), error)) return false; out->push_back(std::move(item)); } return true;',
    "}",
    "",
    "bool write_double(rapidjson::Writer<rapidjson::StringBuffer>& writer, double value, ContractError* error, double minimum, double maximum, bool minimum_exclusive, bool maximum_exclusive)",
    "{",
    '    if (!std::isfinite(value) || value < minimum || value > maximum || (minimum_exclusive && value == minimum) || (maximum_exclusive && value == maximum)) return fail(error, "geometer.contract.number_range", "", "Number is outside its contract bounds."); writer.Double(value); return true;',
    "}",
    "",
    "bool write_uint32(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::uint32_t value, ContractError* error, std::uint64_t minimum, std::uint64_t maximum)",
    "{",
    '    if (value < minimum || value > maximum) return fail(error, "geometer.contract.number_range", "", "Unsigned integer is outside its contract bounds."); writer.Uint(value); return true;',
    "}",
    "",
    "bool write_uint64(rapidjson::Writer<rapidjson::StringBuffer>& writer, std::uint64_t value, ContractError* error, std::uint64_t minimum, std::uint64_t maximum)",
    "{",
    '    if (value < minimum || value > maximum) return fail(error, "geometer.contract.number_range", "", "Unsigned integer is outside its contract bounds."); writer.Uint64(value); return true;',
    "}",
    "",
    "bool write_string(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::string& value, ContractError* error, std::size_t minimum, std::size_t maximum)",
    "{",
    '    if (value.size() < minimum || value.size() > maximum) return fail(error, "geometer.contract.string_length", "", "String length is outside its contract bounds.");',
    '    if (!valid_utf8(value.data(), value.size())) return fail(error, "geometer.contract.invalid_utf8", "", "String is not valid UTF-8.");',
    "    writer.String(value.data(), static_cast<rapidjson::SizeType>(value.size())); return true;",
    "}",
    "",
    "bool write_literal_string(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::string& value, ContractError* error, const char* expected)",
    "{",
    '    if (value != expected) return fail(error, "geometer.contract.literal_mismatch", "", "String literal does not match."); writer.String(expected); return true;',
    "}",
    "",
    "bool write_literal_boolean(rapidjson::Writer<rapidjson::StringBuffer>& writer, bool value, ContractError* error, bool expected)",
    "{",
    '    if (value != expected) return fail(error, "geometer.contract.literal_mismatch", "", "Boolean literal does not match."); writer.Bool(expected); return true;',
    "}",
    "",
    "bool decode_string_item(const rapidjson::Value& value, std::string* out, const std::string& path, ContractError* error)",
    "{",
    "    return decode_string(value, out, path, error, 0U, std::numeric_limits<std::size_t>::max());",
    "}",
    "",
    "bool write_string_item(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::string& value, ContractError* error)",
    "{",
    "    return write_string(writer, value, error, 0U, std::numeric_limits<std::size_t>::max());",
    "}",
    "",
    "bool decode_uint32_item(const rapidjson::Value& value, std::uint32_t* out, const std::string& path, ContractError* error)",
    "{",
    "    return decode_uint32(value, out, path, error, 0U, std::numeric_limits<std::uint32_t>::max());",
    "}",
    "",
    "bool write_uint32_item(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::uint32_t& value, ContractError* error)",
    "{",
    "    return write_uint32(writer, value, error, 0U, std::numeric_limits<std::uint32_t>::max());",
    "}",
    "",
    "bool decode_double_item(const rapidjson::Value& value, double* out, const std::string& path, ContractError* error)",
    "{",
    "    return decode_double(value, out, path, error, -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), false, false);",
    "}",
    "",
    "bool write_double_item(rapidjson::Writer<rapidjson::StringBuffer>& writer, const double& value, ContractError* error)",
    "{",
    "    return write_double(writer, value, error, -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), false, false);",
    "}",
    "",
    "template <typename T>",
    "bool write_array(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::vector<T>& value, ContractError* error, std::size_t minimum, std::size_t maximum, bool (*write_item)(rapidjson::Writer<rapidjson::StringBuffer>&, const T&, ContractError*))",
    "{",
    '    if (value.size() < minimum || value.size() > maximum) return fail(error, "geometer.contract.array_size", "", "Array length is outside its contract bounds."); writer.StartArray(); for (const auto& item : value) if (!write_item(writer, item, error)) return false; writer.EndArray(); return true;',
    "}",
    "",
  ];
  for (const item of codecOrdered) lines.push(...decoder(item), "", ...writer(item), "");
  lines.push(
    "bool parse_document(const unsigned char* data, std::size_t size, rapidjson::Document* document, ContractError* error)",
    "{",
    '    if (document == nullptr || (data == nullptr && size != 0)) return fail(error, "geometer.contract.invalid_argument", "", "Invalid JSON buffer.");',
    '    if (size > kMaxJsonBytes) return fail(error, "geometer.contract.limit_exceeded", "", "JSON exceeds the 8 MiB contract limit.");',
    "    document->Parse<rapidjson::kParseValidateEncodingFlag>(reinterpret_cast<const char*>(data), size);",
    '    if (document->HasParseError()) return fail(error, "geometer.contract.invalid_json", "", rapidjson::GetParseError_En(document->GetParseError()));',
    "    return true;",
    "}",
    "",
    "template <typename T>",
    "bool encode_root(const T& value, bool (*write)(rapidjson::Writer<rapidjson::StringBuffer>&, const T&, ContractError*), std::string* json, ContractError* error)",
    "{",
    '    if (json == nullptr) return fail(error, "geometer.contract.invalid_argument", "", "Output JSON pointer is null.");',
    "    rapidjson::StringBuffer buffer; rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);",
    "    if (!write(writer, value, error)) return false;",
    "    json->assign(buffer.GetString(), buffer.GetSize()); return true;",
    "}",
    "",
    "} // namespace",
    "",
  );
  for (const rootRecord of jsonCodecRoots) {
    const type = shortName(rootRecord.name);
    lines.push(
      `bool decode_json(const unsigned char* data, std::size_t size, ${type}* value, ContractError* error)`,
      "{",
      '    if (value == nullptr) return fail(error, "geometer.contract.invalid_argument", "", "Output value pointer is null.");',
      "    rapidjson::Document document; if (!parse_document(data, size, &document, error)) return false;",
      `    ${type} decoded{}; if (!decode_${type}(document, &decoded, "", error)) return false;`,
      "    *value = std::move(decoded); return true;",
      "}",
      "",
      `bool encode_json(const ${type}& value, std::string* json, ContractError* error)`,
      "{",
      `    return encode_root<${type}>(value, write_${type}, json, error);`,
      "}",
      "",
    );
  }
  lines.push("} // namespace geometer::contracts");
  return lines.join("\n");
}

function generateOperationCatalogSource() {
  const runtimeOperations = projectionCatalog.operations.filter(
    (operation) => operation.runtime_available,
  );
  const nativeRuntimeOperations = projectionCatalog.operations.filter(
    (operation) => operation.runtime_available || operation.native_runtime_available,
  );
  const runtimeCatalogFor = (operations) => ({
    catalog: "wn.geometer.operation_catalog.a0",
    generic_abi: "a0",
    release_version: "__WN_RELEASE_VERSION__",
    c_abi_generation: "__WN_C_ABI_GENERATION__",
    operations: operations.map((operation) => ({
      identity: operation.identity,
      request_contract: operation.request_contract,
      result_contract: operation.result_contract,
      input_attachments: operation.input_attachments,
      output_attachments: operation.output_attachments,
      runtime_dispatch: operation.runtime_dispatch,
      ...(operation.request_projection ? { request_projection: operation.request_projection } : {}),
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
  });
  const catalogFragments = (operations) => {
    const serialized = JSON.stringify(runtimeCatalogFor(operations));
    const [beforeRelease, afterReleaseMarker] = serialized.split("__WN_RELEASE_VERSION__");
    const [beforeAbi, afterAbi] = afterReleaseMarker.split('"__WN_C_ABI_GENERATION__"');
    if (afterAbi === undefined) throw new Error("Could not place runtime catalog version markers.");
    return { beforeRelease, beforeAbi, afterAbi };
  };
  const portableCatalog = catalogFragments(runtimeOperations);
  const nativeCatalog = catalogFragments(nativeRuntimeOperations);
  const attachmentChecks = [];
  const inputAttachmentChecks = [];
  const inputAttachmentMaximums = [];
  const inputAttachmentPrimaryMediaTypes = [];
  const outputAttachmentMaximums = [];
  const outputAttachmentPrimaryMediaTypes = [];
  const requestContracts = [];
  const requestProjections = [];
  const resultContracts = [];
  const resultProjections = [];
  const requiredAttachmentCounts = [];
  const requiredAttachmentNames = [];
  const logicalResultChecks = [];
  const structuralRequestChecks = [];
  const structuralResultChecks = [];
  const rootsByContract = new Map(
    projectionCatalog.roots.map((rootRecord) => [rootRecord.contract_identity, rootRecord]),
  );
  for (const operation of projectionCatalog.operations) {
    const requestRoot = rootsByContract.get(operation.request_contract);
    const resultRoot = rootsByContract.get(operation.result_contract);
    if (requestRoot === undefined || resultRoot === undefined) {
      throw new Error(`Operation ${operation.identity} has an ungenerated request or result root.`);
    }
    const requestType = operation.request_projection
      ? "PackedAttachmentProjectionA0"
      : shortName(requestRoot.name);
    const resultType = operation.result_projection
      ? "PackedAttachmentProjectionA0"
      : shortName(resultRoot.name);
    structuralRequestChecks.push(
      `    if (operation_id == ${JSON.stringify(operation.identity)}) return std::holds_alternative<contracts::${requestType}>(request);`,
    );
    structuralResultChecks.push(
      `    if (operation_id == ${JSON.stringify(operation.identity)}) return std::holds_alternative<contracts::${resultType}>(result);`,
    );
  }
  for (const operation of nativeRuntimeOperations) {
    requestContracts.push(
      `    if (operation_id == ${JSON.stringify(operation.identity)}) return ${JSON.stringify(operation.request_contract)};`,
    );
    if (operation.request_projection) {
      requestProjections.push(
        `    if (operation_id == ${JSON.stringify(operation.identity)}) { *attachment_name = ${JSON.stringify(operation.request_projection.attachment_name)}; *format = ${JSON.stringify(operation.request_projection.format)}; return true; }`,
      );
    }
    resultContracts.push(
      `    if (operation_id == ${JSON.stringify(operation.identity)}) return ${JSON.stringify(operation.result_contract)};`,
    );
    if (operation.result_projection) {
      resultProjections.push(
        `    if (operation_id == ${JSON.stringify(operation.identity)}) { *attachment_name = ${JSON.stringify(operation.result_projection.attachment_name)}; *format = ${JSON.stringify(operation.result_projection.format)}; return true; }`,
      );
    }
    if (operation.runtime_dispatch === "logical_dto") {
      const resultRoot = rootsByContract.get(operation.result_contract);
      if (resultRoot === undefined)
        throw new Error(`Logical operation ${operation.identity} has no generated result root.`);
      logicalResultChecks.push(
        `    if (operation_id == ${JSON.stringify(operation.identity)}) return std::holds_alternative<contracts::${shortName(resultRoot.name)}>(result);`,
      );
    }
    const required = operation.output_attachments.filter((attachment) => attachment.required);
    requiredAttachmentCounts.push(
      `    if (operation_id == ${JSON.stringify(operation.identity)}) return ${required.length}U;`,
    );
    for (const [index, attachment] of required.entries()) {
      requiredAttachmentNames.push(
        `    if (operation_id == ${JSON.stringify(operation.identity)} && index == ${index}U) return ${JSON.stringify(attachment.name)};`,
      );
    }
    for (const attachment of operation.output_attachments) {
      outputAttachmentMaximums.push(
        `    if (operation_id == ${JSON.stringify(operation.identity)} && attachment_name == ${JSON.stringify(attachment.name)}) return ${attachment.max_bytes}U;`,
      );
      outputAttachmentPrimaryMediaTypes.push(
        `    if (operation_id == ${JSON.stringify(operation.identity)} && attachment_name == ${JSON.stringify(attachment.name)}) return ${JSON.stringify(attachment.media_types[0])};`,
      );
      for (const mediaType of attachment.media_types) {
        attachmentChecks.push(
          `    if (operation_id == ${JSON.stringify(operation.identity)} && attachment_name == ${JSON.stringify(attachment.name)} && media_type == ${JSON.stringify(mediaType)}) return true;`,
        );
      }
    }
    for (const attachment of operation.input_attachments) {
      inputAttachmentMaximums.push(
        `    if (operation_id == ${JSON.stringify(operation.identity)} && attachment_name == ${JSON.stringify(attachment.name)}) return ${attachment.max_bytes}U;`,
      );
      inputAttachmentPrimaryMediaTypes.push(
        `    if (operation_id == ${JSON.stringify(operation.identity)} && attachment_name == ${JSON.stringify(attachment.name)}) return ${JSON.stringify(attachment.media_types[0])};`,
      );
      for (const mediaType of attachment.media_types) {
        inputAttachmentChecks.push(
          `    if (operation_id == ${JSON.stringify(operation.identity)} && attachment_name == ${JSON.stringify(attachment.name)} && media_type == ${JSON.stringify(mediaType)}) return true;`,
        );
      }
    }
  }
  return [
    "// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.",
    '#include "geometer/operation_registry.h"',
    '#include "geometer/version.h"',
    "",
    "#include <string>",
    "",
    "namespace geometer",
    "{",
    "",
    "const char* operation_catalog_json()",
    "{",
    `    static const std::string catalog = std::string(${JSON.stringify(portableCatalog.beforeRelease)}) +`,
    `                                       version_string() + ${JSON.stringify(portableCatalog.beforeAbi)} +`,
    `                                       std::to_string(abi_version()) + ${JSON.stringify(portableCatalog.afterAbi)};`,
    "    return catalog.c_str();",
    "}",
    "",
    "const char* native_operation_catalog_json()",
    "{",
    `    static const std::string catalog = std::string(${JSON.stringify(nativeCatalog.beforeRelease)}) +`,
    `                                       version_string() + ${JSON.stringify(nativeCatalog.beforeAbi)} +`,
    `                                       std::to_string(abi_version()) + ${JSON.stringify(nativeCatalog.afterAbi)};`,
    "    return catalog.c_str();",
    "}",
    "",
    "const char* normalized_contract_catalog_sha256()",
    "{",
    `    return ${JSON.stringify(catalogSha256)};`,
    "}",
    "",
    "bool operation_output_attachment_declared(const std::string& operation_id,",
    "                                          const std::string& attachment_name,",
    "                                          const std::string& media_type)",
    "{",
    ...attachmentChecks,
    "    (void)operation_id;",
    "    (void)attachment_name;",
    "    (void)media_type;",
    "    return false;",
    "}",
    "",
    "bool operation_input_attachment_declared(const std::string& operation_id,",
    "                                         const std::string& attachment_name,",
    "                                         const std::string& media_type)",
    "{",
    ...inputAttachmentChecks,
    "    (void)operation_id;",
    "    (void)attachment_name;",
    "    (void)media_type;",
    "    return false;",
    "}",
    "",
    "std::size_t operation_input_attachment_max_bytes(const std::string& operation_id,",
    "                                                 const std::string& attachment_name)",
    "{",
    ...inputAttachmentMaximums,
    "    (void)operation_id;",
    "    (void)attachment_name;",
    "    return 0U;",
    "}",
    "",
    "const char* operation_input_attachment_primary_media_type(const std::string& operation_id,",
    "                                                          const std::string& attachment_name)",
    "{",
    ...inputAttachmentPrimaryMediaTypes,
    "    (void)operation_id;",
    "    (void)attachment_name;",
    "    return nullptr;",
    "}",
    "",
    "std::size_t operation_output_attachment_max_bytes(const std::string& operation_id,",
    "                                                  const std::string& attachment_name)",
    "{",
    ...outputAttachmentMaximums,
    "    (void)operation_id;",
    "    (void)attachment_name;",
    "    return 0U;",
    "}",
    "",
    "const char* operation_output_attachment_primary_media_type(const std::string& operation_id,",
    "                                                           const std::string& attachment_name)",
    "{",
    ...outputAttachmentPrimaryMediaTypes,
    "    (void)operation_id;",
    "    (void)attachment_name;",
    "    return nullptr;",
    "}",
    "",
    "const char* operation_request_contract(const std::string& operation_id)",
    "{",
    ...requestContracts,
    "    return nullptr;",
    "}",
    "",
    "bool operation_request_projection(const std::string& operation_id, const char** attachment_name,",
    "                                  const char** format)",
    "{",
    "    if (attachment_name == nullptr || format == nullptr) return false;",
    ...requestProjections,
    "    *attachment_name = nullptr;",
    "    *format = nullptr;",
    "    return false;",
    "}",
    "",
    "const char* operation_result_contract(const std::string& operation_id)",
    "{",
    ...resultContracts,
    "    return nullptr;",
    "}",
    "",
    "bool operation_result_projection(const std::string& operation_id, const char** attachment_name,",
    "                                 const char** format)",
    "{",
    "    if (attachment_name == nullptr || format == nullptr) return false;",
    ...resultProjections,
    "    *attachment_name = nullptr;",
    "    *format = nullptr;",
    "    return false;",
    "}",
    "",
    "bool operation_logical_result_matches(const std::string& operation_id,",
    "                                      const contracts::OperationResultValueA0& result)",
    "{",
    ...logicalResultChecks,
    "    (void)operation_id;",
    "    (void)result;",
    "    return false;",
    "}",
    "",
    "bool operation_request_value_matches(const std::string& operation_id,",
    "                                     const contracts::IpcRequestValueA0& request)",
    "{",
    ...structuralRequestChecks,
    "    (void)operation_id;",
    "    (void)request;",
    "    return false;",
    "}",
    "",
    "bool operation_result_value_matches(const std::string& operation_id,",
    "                                    const contracts::OperationResultValueA0& result)",
    "{",
    ...structuralResultChecks,
    "    (void)operation_id;",
    "    (void)result;",
    "    return false;",
    "}",
    "",
    "std::size_t operation_required_output_attachment_count(const std::string& operation_id)",
    "{",
    ...requiredAttachmentCounts,
    "    return 0;",
    "}",
    "",
    "const char* operation_required_output_attachment_name(const std::string& operation_id,",
    "                                                      std::size_t index)",
    "{",
    ...requiredAttachmentNames,
    "    (void)operation_id;",
    "    (void)index;",
    "    return nullptr;",
    "}",
    "",
    "} // namespace geometer",
  ].join("\n");
}

function decoder(item) {
  const name = shortName(item.name);
  if (item.kind === "enum") {
    const checks = item.members.map(
      (member) =>
        `    if (text == ${JSON.stringify(String(member.value))}) { *out = ${name}::${member.name}; return true; }`,
    );
    return [
      `bool decode_${name}(const rapidjson::Value& value, ${name}* out, const std::string& path, ContractError* error)`,
      "{",
      '    if (!value.IsString()) return fail(error, "geometer.contract.type_mismatch", path, "Expected a string enum.");',
      "    const std::string text(value.GetString(), value.GetStringLength());",
      ...checks,
      '    return fail(error, "geometer.contract.enum_mismatch", path, "Unknown enum value.");',
      "}",
    ];
  }
  if (item.kind === "union") {
    const oneOf = item.composition === "one_of";
    const body = [];
    item.variants.forEach((variant, index) => {
      const type = cppType(variant.type);
      body.push(
        oneOf
          ? `    { ${type} candidate{}; ContractError ignored; if (${decodeCall(variant.type, "value", "&candidate", "path", "&ignored")}) { ++matches; selected = ${name}(std::in_place_index<${index}>, std::move(candidate)); } }`
          : `    { ${type} candidate{}; ContractError ignored; if (${decodeCall(variant.type, "value", "&candidate", "path", "&ignored")}) { *out = ${name}(std::in_place_index<${index}>, std::move(candidate)); return true; } }`,
      );
    });
    return [
      `bool decode_${name}(const rapidjson::Value& value, ${name}* out, const std::string& path, ContractError* error)`,
      "{",
      ...(oneOf ? [`    int matches = 0; ${name} selected{};`] : []),
      ...body,
      ...(oneOf
        ? [
            '    if (matches != 1) return fail(error, "geometer.contract.union_mismatch", path, "Expected exactly one union variant.");',
            "    *out = std::move(selected); return true;",
          ]
        : [
            '    return fail(error, "geometer.contract.union_mismatch", path, "Value does not match a union variant.");',
          ]),
      "}",
    ];
  }
  if (item.kind !== "model") throw new Error(`Unsupported ${item.kind}`);
  if (item.model_kind === "array") {
    return [
      `bool decode_${name}(const rapidjson::Value& value, ${name}* out, const std::string& path, ContractError* error)`,
      "{",
      `    if (!value.IsArray() || value.Size() < ${item.constraints.min_items ?? 0}U${item.constraints.max_items !== undefined ? ` || value.Size() > ${item.constraints.max_items}U` : ""}) return fail(error, "geometer.contract.array_size", path, "Array length is outside its contract bounds.");`,
      "    out->clear(); out->reserve(value.Size());",
      `    for (rapidjson::SizeType i = 0; i < value.Size(); ++i) { ${cppType(item.index_value)} item_value{}; if (!${decodeCall(item.index_value, "value[i]", "&item_value", 'path + "/" + std::to_string(i)', "error")}) return false; out->push_back(std::move(item_value)); }`,
      "    return true;",
      "}",
    ];
  }
  const names = item.properties.map((property) => JSON.stringify(property.name)).join(", ");
  const body = [
    `    static const char* const names[] = {${names}};`,
    `    if (!validate_object(value, names, ${item.properties.length}U, path, error)) return false;`,
  ];
  for (const property of item.properties) {
    body.push(`    { const auto member = value.FindMember(${JSON.stringify(property.name)});`);
    if (property.optional) {
      const type = cppType(property.type);
      body.push(
        `      if (member != value.MemberEnd()) { ${type} decoded{}; if (!${decodeCall(property.type, "member->value", "&decoded", `child_path(path, ${JSON.stringify(property.name)})`, "error", property.constraints)}) return false; out->${property.name} = std::move(decoded); } else out->${property.name}.reset();`,
      );
    } else {
      body.push(
        `      if (member == value.MemberEnd()) return fail(error, "geometer.contract.missing_field", child_path(path, ${JSON.stringify(property.name)}), "Required field is missing.");`,
      );
      body.push(
        `      if (!${decodeCall(property.type, "member->value", `&out->${property.name}`, `child_path(path, ${JSON.stringify(property.name)})`, "error", property.constraints)}) return false;`,
      );
    }
    body.push("    }");
  }
  return [
    `bool decode_${name}(const rapidjson::Value& value, ${name}* out, const std::string& path, ContractError* error)`,
    "{",
    ...body,
    "    return true;",
    "}",
  ];
}

function writer(item) {
  const name = shortName(item.name);
  const signature = `bool write_${name}(rapidjson::Writer<rapidjson::StringBuffer>& writer, const ${name}& value, ContractError* error)`;
  if (item.kind === "enum") {
    return [
      signature,
      "{",
      "    switch (value) {",
      ...item.members.map(
        (member) =>
          `    case ${name}::${member.name}: writer.String(${JSON.stringify(String(member.value))}); return true;`,
      ),
      "    }",
      '    return fail(error, "geometer.contract.enum_mismatch", "", "Unknown enum value.");',
      "}",
    ];
  }
  if (item.kind === "union") {
    const cases = item.variants.map(
      (variant, index) =>
        `    case ${index}: return ${writeCall(variant.type, `std::get<${index}>(value)`, "error")};`,
    );
    return [
      signature,
      "{",
      "    switch (value.index()) {",
      ...cases,
      '    default: return fail(error, "geometer.contract.union_mismatch", "", "Unknown union variant.");',
      "    }",
      "}",
    ];
  }
  if (item.kind !== "model") throw new Error(`Unsupported ${item.kind}`);
  if (item.model_kind === "array")
    return [
      signature,
      "{",
      `    if (value.size() < ${item.constraints.min_items ?? 0}U${item.constraints.max_items !== undefined ? ` || value.size() > ${item.constraints.max_items}U` : ""}) return fail(error, "geometer.contract.array_size", "", "Array length is outside its contract bounds.");`,
      "    writer.StartArray();",
      `    for (const auto& item_value : value) if (!${writeCall(item.index_value, "item_value", "error")}) return false;`,
      "    writer.EndArray(); return true;",
      "}",
    ];
  const body = ["    writer.StartObject();"];
  for (const property of item.properties) {
    if (property.optional)
      body.push(
        `    if (value.${property.name}.has_value()) { writer.Key(${JSON.stringify(property.name)}); if (!${writeCall(property.type, `*value.${property.name}`, "error", property.constraints)}) return false; }`,
      );
    else
      body.push(
        `    writer.Key(${JSON.stringify(property.name)}); if (!${writeCall(property.type, `value.${property.name}`, "error", property.constraints)}) return false;`,
      );
  }
  body.push("    writer.EndObject(); return true;");
  return [signature, "{", ...body, "}"];
}

function decodeCall(type, value, out, path, error, constraints = {}) {
  if (type.kind === "reference")
    return `decode_${shortName(type.target)}(${value}, ${out}, ${path}, ${error})`;
  if (type.kind === "primitive") {
    if (type.name === "string")
      return `decode_string(${value}, ${out}, ${path}, ${error}, ${sizeConstant(constraints.min_length, "0U")}, ${sizeConstant(constraints.max_length)})`;
    if (type.name === "boolean") return `decode_boolean(${value}, ${out}, ${path}, ${error})`;
    if (type.name === "uint32")
      return `decode_uint32(${value}, ${out}, ${path}, ${error}, ${integerMinimum(constraints)}, ${integerMaximum(constraints, "std::numeric_limits<std::uint32_t>::max()")})`;
    if (type.name === "uint64")
      return `decode_uint64(${value}, ${out}, ${path}, ${error}, ${integerMinimum(constraints)}, ${integerMaximum(constraints, "std::numeric_limits<std::uint64_t>::max()")})`;
    if (type.name === "float64")
      return `decode_double(${value}, ${out}, ${path}, ${error}, ${constraints.min_value_exclusive ?? constraints.min_value ?? "-std::numeric_limits<double>::infinity()"}, ${constraints.max_value_exclusive ?? constraints.max_value ?? "std::numeric_limits<double>::infinity()"}, ${constraints.min_value_exclusive !== undefined}, ${constraints.max_value_exclusive !== undefined})`;
  }
  if (type.kind === "literal")
    return `decode_literal_${type.value_type}(${value}, ${out}, ${path}, ${error}, ${JSON.stringify(type.value)})`;
  if (type.kind === "array") {
    const decoder =
      type.element.kind === "reference"
        ? `decode_${shortName(type.element.target)}`
        : type.element.kind === "primitive"
          ? ({
              string: "decode_string_item",
              uint32: "decode_uint32_item",
              float64: "decode_double_item",
            }[type.element.name] ?? unsupported(type))
          : unsupported(type);
    return `decode_array(${value}, ${out}, ${path}, ${error}, ${sizeConstant(constraints.min_items, "0U")}, ${sizeConstant(constraints.max_items)}, ${decoder})`;
  }
  throw new Error(`Unsupported decode type ${JSON.stringify(type)}`);
}

function writeCall(type, value, error, constraints = {}) {
  if (type.kind === "reference")
    return `write_${shortName(type.target)}(writer, ${value}, ${error})`;
  if (type.kind === "primitive") {
    if (type.name === "string")
      return `write_string(writer, ${value}, ${error}, ${sizeConstant(constraints.min_length, "0U")}, ${sizeConstant(constraints.max_length)})`;
    if (type.name === "boolean") return `(writer.Bool(${value}), true)`;
    if (type.name === "uint32")
      return `write_uint32(writer, ${value}, ${error}, ${integerMinimum(constraints)}, ${integerMaximum(constraints, "std::numeric_limits<std::uint32_t>::max()")})`;
    if (type.name === "uint64")
      return `write_uint64(writer, ${value}, ${error}, ${integerMinimum(constraints)}, ${integerMaximum(constraints, "std::numeric_limits<std::uint64_t>::max()")})`;
    if (type.name === "float64")
      return `write_double(writer, ${value}, ${error}, ${constraints.min_value_exclusive ?? constraints.min_value ?? "-std::numeric_limits<double>::infinity()"}, ${constraints.max_value_exclusive ?? constraints.max_value ?? "std::numeric_limits<double>::infinity()"}, ${constraints.min_value_exclusive !== undefined}, ${constraints.max_value_exclusive !== undefined})`;
  }
  if (type.kind === "literal")
    return `write_literal_${type.value_type}(writer, ${value}, ${error}, ${JSON.stringify(type.value)})`;
  if (type.kind === "array") {
    const writer =
      type.element.kind === "reference"
        ? `write_${shortName(type.element.target)}`
        : type.element.kind === "primitive"
          ? ({
              string: "write_string_item",
              uint32: "write_uint32_item",
              float64: "write_double_item",
            }[type.element.name] ?? unsupported(type))
          : unsupported(type);
    return `write_array(writer, ${value}, ${error}, ${sizeConstant(constraints.min_items, "0U")}, ${sizeConstant(constraints.max_items)}, ${writer})`;
  }
  throw new Error(`Unsupported write type ${JSON.stringify(type)}`);
}

function cppType(type) {
  if (type.kind === "reference") return shortName(type.target);
  if (type.kind === "primitive")
    return (
      {
        string: "std::string",
        boolean: "bool",
        float64: "double",
        int64: "std::int64_t",
        uint32: "std::uint32_t",
        uint64: "std::uint64_t",
      }[type.name] ?? unsupported(type)
    );
  if (type.kind === "literal")
    return (
      { string: "std::string", boolean: "bool", number: "double" }[type.value_type] ??
      unsupported(type)
    );
  if (type.kind === "array") return `std::vector<${cppType(type.element)}>`;
  return unsupported(type);
}

function valueInitializer(type) {
  if (type.kind !== "literal") return "{}";
  if (type.value_type === "string") return ` = ${JSON.stringify(type.value)}`;
  return ` = ${String(type.value)}`;
}

function shortName(name) {
  return name.slice(name.lastIndexOf(".") + 1);
}
function formatCpp(content, filename) {
  const result = spawnSync("clang-format", ["--style=file", `--assume-filename=${filename}`], {
    cwd: root,
    encoding: "utf8",
    input: content,
  });
  if (result.error)
    throw new Error(
      `clang-format is required for C++ contract generation: ${result.error.message}`,
    );
  if (result.status !== 0) throw new Error(`clang-format failed: ${result.stderr}`);
  return result.stdout;
}
function sizeConstant(value, fallback = "std::numeric_limits<std::size_t>::max()") {
  return value === undefined ? fallback : `${value}U`;
}
function integerMaximum(constraints, fallback) {
  return constraints.max_value === undefined ? fallback : `${constraints.max_value}ULL`;
}
function integerMinimum(constraints) {
  return constraints.min_value === undefined ? "0ULL" : `${constraints.min_value}ULL`;
}
function unsupported(value) {
  throw new Error(`Unsupported C++ catalog type ${JSON.stringify(value)}`);
}

function topologicalOrder(items) {
  const result = [],
    visiting = new Set(),
    visited = new Set();
  function visit(item) {
    if (visited.has(item.name)) return;
    if (visiting.has(item.name)) throw new Error(`Cyclic DTO dependency at ${item.name}.`);
    visiting.add(item.name);
    for (const dependency of dependencies(item))
      if (declarations.has(dependency)) visit(declarations.get(dependency));
    visiting.delete(item.name);
    visited.add(item.name);
    result.push(item);
  }
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
  scan(item.base);
  scan(item.index_value);
  for (const property of item.properties ?? []) scan(property.type);
  for (const variant of item.variants ?? []) scan(variant.type);
  return found;
}

function reachableDeclarationItems(roots) {
  const reachable = new Set();
  function visit(name) {
    if (reachable.has(name)) return;
    const declaration = declarations.get(name);
    if (!declaration) throw new Error(`Catalog reference does not resolve: ${name}.`);
    reachable.add(name);
    for (const dependency of dependencies(declaration)) visit(dependency);
  }
  for (const rootRecord of roots) visit(rootRecord.name);
  return projectionCatalog.declarations.filter((declaration) => reachable.has(declaration.name));
}
