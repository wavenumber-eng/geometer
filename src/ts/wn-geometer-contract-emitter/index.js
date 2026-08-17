// @ts-check

import {
  emitFile,
  getDoc,
  getMaxItems,
  getMaxLength,
  getMaxValue,
  getMinItems,
  getMinLength,
  getMinValue,
  getNamespaceFullName,
  getPattern,
  isArrayModelType,
  isRecordModelType,
  navigateProgram,
  serializeValueAsJson,
} from "@typespec/compiler";
import {
  getExtensions,
  getId,
  getJsonSchemaTypes,
  getUniqueItems,
  isOneOf,
} from "@typespec/json-schema";

export const namespace = "Wavenumber.Geometer.ContractMetadata";

const OWNED_NAMESPACE = "Wavenumber.Geometer.Contracts";
const CONTRACT_IDENTITY = Symbol.for("wavenumber.geometer.contractIdentity");
const OPTION_PATCH = Symbol.for("wavenumber.geometer.optionPatch");
const OPERATION_IDENTITY = Symbol.for("wavenumber.geometer.operationIdentity");
const INPUT_ATTACHMENTS = Symbol.for("wavenumber.geometer.inputAttachments");
const OUTPUT_ATTACHMENTS = Symbol.for("wavenumber.geometer.outputAttachments");
const REQUEST_PACKED_PROJECTION = Symbol.for("wavenumber.geometer.requestPackedProjection");
const RESULT_PACKED_PROJECTION = Symbol.for("wavenumber.geometer.resultPackedProjection");

export function $contractIdentity(context, target, identity) {
  setUnique(context.program, CONTRACT_IDENTITY, target, identity, "contract identity");
}

export function $optionPatch(context, target) {
  setUnique(context.program, OPTION_PATCH, target, true, "option-patch marker");
}

export function $operationIdentity(context, target, identity) {
  setUnique(context.program, OPERATION_IDENTITY, target, identity, "operation identity");
}

export function $inputAttachment(context, target, name, required, mediaTypes, maxBytes) {
  addAttachment(context.program, INPUT_ATTACHMENTS, target, {
    name,
    required,
    media_types: parseMediaTypes(mediaTypes),
    max_bytes: numericValue(maxBytes),
  });
}

export function $outputAttachment(context, target, name, required, mediaTypes, maxBytes) {
  addAttachment(context.program, OUTPUT_ATTACHMENTS, target, {
    name,
    required,
    media_types: parseMediaTypes(mediaTypes),
    max_bytes: numericValue(maxBytes),
  });
}

export function $requestPackedProjection(context, target, attachmentName, format) {
  setPackedProjection(
    context.program,
    REQUEST_PACKED_PROJECTION,
    target,
    attachmentName,
    format,
    "request",
  );
}

export function $resultPackedProjection(context, target, attachmentName, format) {
  setPackedProjection(
    context.program,
    RESULT_PACKED_PROJECTION,
    target,
    attachmentName,
    format,
    "result",
  );
}

export async function $onEmit(context) {
  const roots = getJsonSchemaTypes(context.program)
    .filter((type) => type.kind !== "Namespace" && getId(context.program, type))
    .sort((left, right) => qualifiedName(left).localeCompare(qualifiedName(right)));
  if (roots.length === 0) {
    throw new Error(
      "Geometer catalog emitter found no @jsonSchema roots with explicit @id values.",
    );
  }

  const declarations = new Map();
  const visiting = new Set();
  for (const root of roots) {
    if (!getState(context.program, CONTRACT_IDENTITY, root)) {
      throw new Error(`Root ${qualifiedName(root)} is missing @contractIdentity.`);
    }
    collectOwnedDeclarations(root, declarations, visiting);
  }

  const operations = [];
  const ownedDeclarations = [];
  navigateProgram(context.program, {
    model(type) {
      if (isOwnedDeclaration(type)) ownedDeclarations.push(type);
    },
    scalar(type) {
      if (isOwnedDeclaration(type)) ownedDeclarations.push(type);
    },
    enum(type) {
      if (isOwnedDeclaration(type)) ownedDeclarations.push(type);
    },
    union(type) {
      if (isOwnedDeclaration(type)) ownedDeclarations.push(type);
    },
    operation(operation) {
      const identity = getState(context.program, OPERATION_IDENTITY, operation);
      if (!identity) {
        return;
      }
      operations.push(operationRecord(context.program, operation, identity));
    },
  });
  operations.sort((left, right) => left.identity.localeCompare(right.identity));

  for (const declaration of ownedDeclarations) {
    if (!declarations.has(qualifiedName(declaration))) {
      throw new Error(
        `Owned declaration ${qualifiedName(declaration)} is not reachable from a contract root.`,
      );
    }
  }

  assertUnique(
    roots.map((root) => getState(context.program, CONTRACT_IDENTITY, root)),
    "contract",
  );
  assertUnique(
    roots.map((root) => requiredSchemaId(context.program, root)),
    "schema",
  );
  assertUnique(
    operations.map((operation) => operation.identity),
    "operation",
  );

  const catalog = {
    catalog_identity: "wn.geometer.contract_catalog",
    generation: "a0",
    output_roots: {
      cpp: "src/cpp/lib/geometer/generated/contracts",
      typescript: "src/ts/geometer/generated",
      rust: "src/rust/geometer-client/src/generated",
      python: "python/geometer/_generated/contracts",
      html: "docs/generated/contracts",
    },
    roots: roots.map((root) => ({
      name: qualifiedName(root),
      contract_identity: getState(context.program, CONTRACT_IDENTITY, root),
      schema_id: requiredSchemaId(context.program, root),
      declaration_kind: root.kind.toLowerCase(),
      option_patch: Boolean(getState(context.program, OPTION_PATCH, root)),
    })),
    declarations: [...declarations.entries()]
      .sort(([left], [right]) => left.localeCompare(right))
      .map(([, type]) => declarationRecord(context.program, type)),
    operations,
  };

  await emitFile(context.program, {
    path: `${context.emitterOutputDir}/wn_geometer_contract_catalog.a0.json`,
    content: `${JSON.stringify(catalog, null, 2)}\n`,
  });
}

function operationRecord(program, operation, identity) {
  const parameters = [...operation.parameters.properties.values()];
  if (parameters.length !== 1 || parameters[0].name !== "request") {
    throw new Error(
      `Operation ${qualifiedName(operation)} must have exactly one request parameter.`,
    );
  }
  const request = requiredContractReference(program, parameters[0].type, operation, "request");
  const result = requiredContractReference(program, operation.returnType, operation, "result");
  const inputAttachments = attachmentRecords(program, INPUT_ATTACHMENTS, operation);
  const outputAttachments = attachmentRecords(program, OUTPUT_ATTACHMENTS, operation);
  const requestProjection = packedProjectionRecord(
    program,
    REQUEST_PACKED_PROJECTION,
    operation,
    inputAttachments,
    "request",
  );
  const resultProjection = packedProjectionRecord(
    program,
    RESULT_PACKED_PROJECTION,
    operation,
    outputAttachments,
    "result",
  );
  const runtimeDispatch =
    requestProjection?.kind === "packed_attachment" &&
    resultProjection?.kind === "packed_attachment"
      ? "packed_attachment"
      : "logical_dto";
  return {
    name: qualifiedName(operation),
    identity,
    request_contract: request,
    result_contract: result,
    input_attachments: inputAttachments,
    output_attachments: outputAttachments,
    runtime_dispatch: runtimeDispatch,
    ...(requestProjection ? { request_projection: requestProjection } : {}),
    ...(resultProjection ? { result_projection: resultProjection } : {}),
    doc: getDoc(program, operation) ?? "",
  };
}

function packedProjectionRecord(program, key, operation, attachments, role) {
  const projection = getState(program, key, operation);
  if (!projection) return null;
  if (!attachments.some((attachment) => attachment.name === projection.attachment_name)) {
    throw new Error(
      `Operation ${qualifiedName(operation)} ${role} packed projection names undeclared attachment ${projection.attachment_name}.`,
    );
  }
  return projection;
}

function requiredContractReference(program, type, operation, role) {
  const identity = getState(program, CONTRACT_IDENTITY, type);
  if (!identity) {
    throw new Error(
      `Operation ${qualifiedName(operation)} ${role} ${qualifiedName(type)} lacks @contractIdentity.`,
    );
  }
  return identity;
}

function attachmentRecords(program, key, operation) {
  return [...(program.stateMap(key).get(operation) ?? [])].sort((left, right) =>
    left.name.localeCompare(right.name),
  );
}

function collectOwnedDeclarations(type, declarations, visiting) {
  if (visiting.has(type)) {
    return;
  }
  visiting.add(type);
  try {
    if (isOwnedDeclaration(type)) {
      declarations.set(qualifiedName(type), type);
    }
    switch (type.kind) {
      case "Model":
        if (isArrayModelType(type) || isRecordModelType(type)) {
          collectOwnedDeclarations(type.indexer.value, declarations, visiting);
          return;
        }
        if (type.baseModel) {
          collectOwnedDeclarations(type.baseModel, declarations, visiting);
        }
        for (const property of type.properties.values()) {
          collectOwnedDeclarations(property.type, declarations, visiting);
        }
        return;
      case "Scalar":
        if (type.baseScalar) {
          collectOwnedDeclarations(type.baseScalar, declarations, visiting);
        }
        return;
      case "Enum":
      case "String":
      case "Number":
      case "Boolean":
        return;
      case "Union":
        for (const variant of type.variants.values()) {
          collectOwnedDeclarations(variant.type, declarations, visiting);
        }
        return;
      case "Intrinsic":
        if (type.name === "null") {
          return;
        }
        throw new Error(`Unsupported intrinsic type ${type.name} reached the Geometer catalog.`);
      default:
        throw new Error(`Unsupported TypeSpec kind ${type.kind} reached the Geometer catalog.`);
    }
  } finally {
    visiting.delete(type);
  }
}

function declarationRecord(program, type) {
  const common = {
    name: qualifiedName(type),
    kind: type.kind.toLowerCase(),
    doc: getDoc(program, type) ?? "",
    annotations: extensionRecord(program, type),
    constraints: constraintRecord(program, type),
  };
  switch (type.kind) {
    case "Model":
      return {
        ...common,
        model_kind: isArrayModelType(type)
          ? "array"
          : isRecordModelType(type)
            ? "record"
            : "object",
        base: type.baseModel ? typeExpression(type.baseModel) : null,
        index_value: type.indexer ? typeExpression(type.indexer.value) : null,
        properties: [...type.properties.values()].map((property) => {
          const record = {
            name: property.name,
            optional: property.optional,
            doc: getDoc(program, property) ?? "",
            type: typeExpression(property.type),
            annotations: extensionRecord(program, property),
            constraints: constraintRecord(program, property),
          };
          if (property.defaultValue !== undefined) {
            record.default = serializeValueAsJson(program, property.defaultValue, property);
          }
          return record;
        }),
      };
    case "Scalar":
      return { ...common, base: type.baseScalar ? typeExpression(type.baseScalar) : null };
    case "Enum":
      return {
        ...common,
        members: [...type.members.values()].map((member) => ({
          name: member.name,
          value: member.value ?? member.name,
          doc: getDoc(program, member) ?? "",
        })),
      };
    case "Union":
      return {
        ...common,
        ...(isOneOf(program, type) ? { composition: "one_of" } : {}),
        variants: [...type.variants.values()].map((variant) => ({
          name: String(variant.name),
          type: typeExpression(variant.type),
          doc: getDoc(program, variant) ?? "",
          annotations: extensionRecord(program, variant),
        })),
      };
    default:
      throw new Error(
        `Owned declaration ${qualifiedName(type)} has unsupported kind ${type.kind}.`,
      );
  }
}

function typeExpression(type) {
  if (isOwnedDeclaration(type)) {
    return { kind: "reference", target: qualifiedName(type) };
  }
  switch (type.kind) {
    case "Model":
      if (isArrayModelType(type)) {
        return { kind: "array", element: typeExpression(type.indexer.value) };
      }
      if (isRecordModelType(type)) {
        return { kind: "record", value: typeExpression(type.indexer.value) };
      }
      throw new Error(`Anonymous object model ${type.name || "<unnamed>"} is not supported.`);
    case "Scalar":
      return { kind: "primitive", name: type.name };
    case "String":
      return { kind: "literal", value_type: "string", value: type.value };
    case "Number":
      return { kind: "literal", value_type: "number", value: numericValue(type.value) };
    case "Boolean":
      return { kind: "literal", value_type: "boolean", value: type.value };
    case "Intrinsic":
      if (type.name === "null") {
        return { kind: "literal", value_type: "null", value: null };
      }
      throw new Error(`Unsupported intrinsic type ${type.name} reached a type expression.`);
    case "Union":
      throw new Error("Anonymous unions are not supported; declare and document the union.");
    default:
      throw new Error(`Unsupported TypeSpec kind ${type.kind} reached a type expression.`);
  }
}

function extensionRecord(program, target) {
  return Object.fromEntries(
    getExtensions(program, target)
      .map(({ key, value }) => [key, normalizeExtensionValue(value)])
      .sort(([left], [right]) => left.localeCompare(right)),
  );
}

function normalizeExtensionValue(value) {
  if (value === null || ["string", "number", "boolean"].includes(typeof value)) {
    return value;
  }
  if (Array.isArray(value)) {
    return value.map(normalizeExtensionValue);
  }
  if (typeof value === "object" && value !== null) {
    if ("entityKind" in value) {
      throw new Error("Type-valued extensions are not supported by the Geometer catalog emitter.");
    }
    return Object.fromEntries(
      Object.entries(value)
        .sort(([left], [right]) => left.localeCompare(right))
        .map(([key, item]) => [key, normalizeExtensionValue(item)]),
    );
  }
  throw new Error(`Unsupported extension value type ${typeof value}.`);
}

function constraintRecord(program, target) {
  const entries = [
    ["pattern", getPattern(program, target)],
    ["min_length", getMinLength(program, target)],
    ["max_length", getMaxLength(program, target)],
    ["min_items", getMinItems(program, target)],
    ["max_items", getMaxItems(program, target)],
    ["min_value", getMinValue(program, target)],
    ["max_value", getMaxValue(program, target)],
    ["unique_items", getUniqueItems(program, target)],
  ].filter(([, value]) => value !== undefined);
  return Object.fromEntries(entries);
}

function isOwnedDeclaration(type) {
  if (!["Model", "Scalar", "Enum", "Union"].includes(type.kind)) {
    return false;
  }
  const namespaceName = type.namespace ? getNamespaceFullName(type.namespace) : "";
  return (
    Boolean(type.name) &&
    (namespaceName === OWNED_NAMESPACE || namespaceName.startsWith(`${OWNED_NAMESPACE}.`))
  );
}

function qualifiedName(type) {
  if (!("name" in type) || !type.name) {
    throw new Error(`TypeSpec ${type.kind} does not have a stable declaration name.`);
  }
  const namespaceName =
    "namespace" in type && type.namespace ? getNamespaceFullName(type.namespace) : "";
  return namespaceName ? `${namespaceName}.${String(type.name)}` : String(type.name);
}

function requiredSchemaId(program, root) {
  const id = getId(program, root);
  if (!id) {
    throw new Error(`Root ${qualifiedName(root)} is missing an explicit @id.`);
  }
  return id;
}

function setUnique(program, key, target, value, label) {
  const state = program.stateMap(key);
  if (state.has(target)) {
    throw new Error(`${qualifiedName(target)} has duplicate ${label} decorators.`);
  }
  state.set(target, value);
}

function addAttachment(program, key, target, attachment) {
  const state = program.stateMap(key);
  const existing = state.get(target) ?? [];
  if (existing.some((candidate) => candidate.name === attachment.name)) {
    throw new Error(`${qualifiedName(target)} has duplicate attachment ${attachment.name}.`);
  }
  state.set(target, [...existing, attachment]);
}

function setPackedProjection(program, key, target, attachmentName, format, role) {
  if (!attachmentName.trim() || !format.trim()) {
    throw new Error(`${role} packed projection attachment name and format must be nonempty.`);
  }
  setUnique(
    program,
    key,
    target,
    { kind: "packed_attachment", attachment_name: attachmentName, format },
    `${role} packed projection`,
  );
}

function getState(program, key, target) {
  return program.stateMap(key).get(target);
}

function parseMediaTypes(value) {
  const mediaTypes = value
    .split(",")
    .map((item) => item.trim())
    .filter(Boolean)
    .sort();
  if (mediaTypes.length === 0 || new Set(mediaTypes).size !== mediaTypes.length) {
    throw new Error("Attachment media types must be a nonempty unique comma-separated list.");
  }
  return mediaTypes;
}

function numericValue(value) {
  if (typeof value === "number") {
    return value;
  }
  if (value && typeof value.asNumber === "function") {
    return value.asNumber();
  }
  throw new Error("Expected a losslessly representable numeric value.");
}

function assertUnique(values, label) {
  const seen = new Set();
  for (const value of values) {
    if (!value || seen.has(value)) {
      throw new Error(`Missing or duplicate ${label} identity ${String(value)}.`);
    }
    seen.add(value);
  }
}
