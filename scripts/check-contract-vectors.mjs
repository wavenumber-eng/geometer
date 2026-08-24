// @ts-check

import { existsSync } from "node:fs";
import { readdir, readFile } from "node:fs/promises";
import { dirname, join, relative, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";

import Ajv2020 from "ajv/dist/2020.js";
import {
  decodeIpcRequestA0Json,
  decodeOperationOutcomeA0Json,
  decodeStepTopologyAnalyzeRecoveryRequestA0Json,
  decodeStepTopologyAnalyzeRecoveryResultA0Json,
  decodeStepTopologyApplyHierarchyRequestA0Json,
  decodeStepTopologyApplyHierarchyResultA0Json,
  decodeStepTopologyApplyLogicalGroupsRequestA0Json,
  decodeStepTopologyApplyLogicalGroupsResultA0Json,
  decodeStepTopologyApplyMetadataProbesRequestA0Json,
  decodeStepTopologyApplyMetadataProbesResultA0Json,
  decodeStepTopologyCheckpointEditJournalResultA0Json,
  decodeStepTopologyInspectResultA0Json,
  decodeStepTopologyRenderResultA0Json,
  decodeStepTopologyResolveHitRequestA0Json,
  decodeStepTopologyRestoreRequestA0Json,
  decodeStepTopologyRestoreResultA0Json,
  decodeStepTopologySaveRequestA0Json,
  decodeStepTopologySaveResultA0Json,
  operationCatalog,
  StepTopologyInspectionAccumulator,
  validateIpcOutcomeOperationPair,
  validateIpcRequestOperationPair,
  validateStepTopologyCheckpointAttachment,
  validateStepTopologyHierarchyCommands,
  validateStepTopologyHierarchyResult,
  validateStepTopologyInspection,
  validateStepTopologyLogicalGroupCommands,
  validateStepTopologyLogicalGroupResult,
  validateStepTopologyMetadataProbeCommands,
  validateStepTopologyRecoveryRequest,
  validateStepTopologyRecoveryResults,
  validateStepTopologyRenderAttachments,
  validateStepTopologyResolveHitContext,
  validateStepTopologyRestoreAttachments,
  validateStepTopologyRestoreResult,
  validateStepTopologySaveAttachments,
} from "../dist/wasm/npm/geometer/index.js";

const repositoryRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const vectorRoot = join(repositoryRoot, "tests", "contracts", "vectors");
const manifestPath = join(vectorRoot, "manifest.json");
const catalogPath = join(
  repositoryRoot,
  "contracts",
  "geometer",
  "generated",
  "wn_geometer_contract_catalog.a0.json",
);

async function main() {
  const manifest = JSON.parse(await readFile(manifestPath, "utf8"));
  const catalog = JSON.parse(await readFile(catalogPath, "utf8"));
  validateManifest(manifest);

  const schemas = new Map();
  // strictTypes stays off: the governed int64-as-string schema strategy keeps
  // numeric bounds (enforced by catalog constraints and generated codecs) that
  // Ajv's type lint would reject on string-typed scalars.
  const ajv = new Ajv2020({ allErrors: true, strict: true, strictTypes: false });
  ajv.addKeyword({ keyword: "x-wn-default-intent", schemaType: "string" });
  ajv.addKeyword({ keyword: "x-wn-production-wire", schemaType: "string" });
  ajv.addKeyword({ keyword: "x-wn-derived-field", schemaType: "string" });
  ajv.addKeyword({ keyword: "x-wn-packed-field", schemaType: "string" });
  ajv.addKeyword({
    keyword: "x-wn-max-logical-source-reference-expansions",
    schemaType: "number",
    metaSchema: { type: "integer", minimum: 1 },
  });
  const schemaDocuments = [];
  for (const root of catalog.roots) {
    const filename = `${root.name.slice(root.name.lastIndexOf(".") + 1)}.json`;
    const schemaPath = join(
      repositoryRoot,
      "contracts",
      "geometer",
      "generated",
      "schema",
      filename,
    );
    const document = JSON.parse(await readFile(schemaPath, "utf8"));
    schemaDocuments.push([root.contract_identity, document]);
    ajv.addSchema(document);
  }
  for (const [identity, document] of schemaDocuments) {
    const validate = ajv.getSchema(document.$id);
    if (!validate) throw new Error(`Could not compile generated schema ${document.$id}.`);
    schemas.set(identity, validate);
  }

  const referencedFiles = new Set();
  for (const vector of manifest.vectors) {
    const vectorPath = join(vectorRoot, vector.file);
    if (!existsSync(vectorPath)) {
      throw new Error(`${vector.id}: missing ${vector.file}`);
    }
    referencedFiles.add(vector.file);
    const storedBytes = await readFile(vectorPath);
    const bytes =
      vector.oracle === "strict_parser_hex" ? decodeHexVector(storedBytes, vector.id) : storedBytes;
    let value;
    let parseError = null;
    try {
      value = parseStrictJson(bytes);
    } catch (error) {
      parseError = error;
    }

    if (vector.lane === "strict_json") {
      assertOutcome(vector, parseError === null, parseError);
      continue;
    }
    if (parseError) {
      throw new Error(`${vector.id}: non-strict lane has invalid JSON: ${formatError(parseError)}`);
    }
    if (vector.lane === "schema") {
      const validate = schemas.get(vector.contract_identity);
      if (!validate) {
        throw new Error(`${vector.id}: unknown contract ${vector.contract_identity}`);
      }
      const valid = validate(value);
      assertOutcome(vector, valid, valid ? null : ajv.errorsText(validate.errors));
      continue;
    }
    if (vector.lane === "semantic" && vector.oracle === "presence_projection") {
      const actual = {};
      for (const field of vector.fields) {
        actual[field] = Object.hasOwn(value, field) ? "present" : "absent";
      }
      assertEqual(actual, vector.expected_value, `${vector.id} presence projection`);
      continue;
    }
    if (vector.lane === "semantic" && vector.oracle.startsWith("step_topology_")) {
      let semanticError = null;
      try {
        if (vector.oracle === "step_topology_session") {
          const request = decodeStepTopologyResolveHitRequestA0Json(bytes);
          validateStepTopologyResolveHitContext(request, {
            sessionHandle: vector.expected_session_handle,
            generation: vector.expected_generation,
          });
        } else if (vector.oracle === "step_topology_inspection") {
          if (vector.prior_pages) {
            const accumulator = new StepTopologyInspectionAccumulator();
            for (const priorPage of vector.prior_pages) {
              referencedFiles.add(priorPage);
              accumulator.addPage(
                decodeStepTopologyInspectResultA0Json(await readFile(join(vectorRoot, priorPage))),
              );
            }
            accumulator.addPage(decodeStepTopologyInspectResultA0Json(bytes));
          } else {
            validateStepTopologyInspection(decodeStepTopologyInspectResultA0Json(bytes));
          }
        } else if (vector.oracle === "step_topology_inspection_high_fan_in") {
          const accumulator = new StepTopologyInspectionAccumulator();
          for (const page of highFanInInspectionPages(
            decodeStepTopologyInspectResultA0Json(bytes),
            vector.fan_in,
          )) {
            accumulator.addPage(page);
          }
        } else if (vector.oracle === "step_topology_render_attachments") {
          const attachments = [];
          for (const attachment of vector.attachments) {
            referencedFiles.add(attachment.file);
            attachments.push({
              name: attachment.name,
              mediaType: attachment.media_type,
              data: await readFile(join(vectorRoot, attachment.file)),
            });
          }
          await validateStepTopologyRenderAttachments(
            decodeStepTopologyRenderResultA0Json(bytes),
            attachments,
          );
        } else if (vector.oracle === "step_topology_logical_group_commands") {
          validateStepTopologyLogicalGroupCommands(
            decodeStepTopologyApplyLogicalGroupsRequestA0Json(bytes),
          );
        } else if (vector.oracle === "step_topology_logical_group_commands_high_fan_in") {
          const request = decodeStepTopologyApplyLogicalGroupsRequestA0Json(bytes);
          const split = Math.floor(vector.fan_in / 2);
          validateStepTopologyLogicalGroupCommands({
            ...request,
            commands: [
              {
                kind: "create",
                authored_id: "wn.geometer.research.group.aggregate-a",
                name: "Aggregate A",
                member_handles: generatedTargetHandles(split),
              },
              {
                kind: "create",
                authored_id: "wn.geometer.research.group.aggregate-b",
                name: "Aggregate B",
                member_handles: generatedTargetHandles(vector.fan_in - split, split),
              },
            ],
          });
        } else if (vector.oracle === "step_topology_logical_group_result") {
          validateStepTopologyLogicalGroupResult(
            decodeStepTopologyApplyLogicalGroupsResultA0Json(bytes),
          );
        } else if (vector.oracle === "step_topology_logical_group_result_high_fan_in") {
          const result = decodeStepTopologyApplyLogicalGroupsResultA0Json(bytes);
          const split = Math.floor(vector.fan_in / 2);
          validateStepTopologyLogicalGroupResult({
            ...result,
            groups: [
              {
                ...result.groups[0],
                authored_id: "wn.geometer.research.group.aggregate-a",
                name: "Aggregate A",
                members: generatedTargetHandles(split).map((target_handle) => ({
                  kind: "face",
                  target_handle,
                })),
              },
              {
                ...result.groups[0],
                authored_id: "wn.geometer.research.group.aggregate-b",
                name: "Aggregate B",
                members: generatedTargetHandles(vector.fan_in - split, split).map(
                  (target_handle) => ({
                    kind: "face",
                    target_handle,
                  }),
                ),
              },
            ],
          });
        } else if (vector.oracle === "step_topology_metadata_probe_result") {
          validateStepTopologyLogicalGroupResult(
            decodeStepTopologyApplyMetadataProbesResultA0Json(bytes),
          );
        } else if (vector.oracle === "step_topology_metadata_probe_commands") {
          validateStepTopologyMetadataProbeCommands(
            decodeStepTopologyApplyMetadataProbesRequestA0Json(bytes),
          );
        } else if (vector.oracle === "step_topology_checkpoint_attachment") {
          const attachments = [];
          for (const attachment of vector.attachments) {
            referencedFiles.add(attachment.file);
            attachments.push({
              name: attachment.name,
              mediaType: attachment.media_type,
              data: await readFile(join(vectorRoot, attachment.file)),
            });
          }
          await validateStepTopologyCheckpointAttachment(
            decodeStepTopologyCheckpointEditJournalResultA0Json(bytes),
            attachments,
          );
        } else if (vector.oracle === "step_topology_hierarchy_commands") {
          validateStepTopologyHierarchyCommands(
            decodeStepTopologyApplyHierarchyRequestA0Json(bytes),
          );
        } else if (vector.oracle === "step_topology_hierarchy_result") {
          validateStepTopologyHierarchyResult(decodeStepTopologyApplyHierarchyResultA0Json(bytes));
        } else if (vector.oracle === "step_topology_recovery_request") {
          validateStepTopologyRecoveryRequest(
            decodeStepTopologyAnalyzeRecoveryRequestA0Json(bytes).groups,
          );
        } else if (vector.oracle === "step_topology_recovery_result") {
          validateStepTopologyRecoveryResults(
            decodeStepTopologyAnalyzeRecoveryResultA0Json(bytes).groups,
          );
        } else if (vector.oracle === "step_topology_save_attachments") {
          referencedFiles.add(vector.context_file);
          await validateStepTopologySaveAttachments(
            decodeStepTopologySaveRequestA0Json(
              await readFile(join(vectorRoot, vector.context_file)),
            ),
            decodeStepTopologySaveResultA0Json(bytes),
            await loadSemanticAttachments(vector, vectorRoot, referencedFiles),
          );
        } else if (vector.oracle === "step_topology_restore_attachments") {
          await validateStepTopologyRestoreAttachments(
            decodeStepTopologyRestoreRequestA0Json(bytes),
            await loadSemanticAttachments(vector, vectorRoot, referencedFiles),
          );
        } else if (vector.oracle === "step_topology_restore_result") {
          referencedFiles.add(vector.context_file);
          validateStepTopologyRestoreResult(
            decodeStepTopologyRestoreRequestA0Json(
              await readFile(join(vectorRoot, vector.context_file)),
            ),
            decodeStepTopologyRestoreResultA0Json(bytes),
          );
        } else if (vector.oracle === "step_topology_ipc_request_pair") {
          validateIpcRequestOperationPair(decodeIpcRequestA0Json(bytes));
        } else if (vector.oracle === "step_topology_ipc_result_pair") {
          validateIpcOutcomeOperationPair(decodeOperationOutcomeA0Json(bytes));
        } else if (vector.oracle === "step_topology_ipc_pair_matrix") {
          validateIpcRequestOperationPair(decodeIpcRequestA0Json(bytes));
          for (const [operation, contract] of vector.request_pairs) {
            if (operationCatalog[operation]?.requestContract !== contract) {
              throw new Error(`Request pair mismatch for ${operation}.`);
            }
          }
          for (const [operation, contract] of vector.result_pairs) {
            if (operationCatalog[operation]?.resultContract !== contract) {
              throw new Error(`Result pair mismatch for ${operation}.`);
            }
          }
        } else {
          throw new Error(`${vector.id}: unknown STEP topology semantic oracle.`);
        }
      } catch (error) {
        semanticError = error;
      }
      assertOutcome(vector, semanticError === null, semanticError);
      continue;
    }
    throw new Error(`${vector.id}: unsupported lane/oracle ${vector.lane}/${vector.oracle}`);
  }

  for (const vector of manifest.operation_vectors) {
    const operation = catalog.operations.find(
      (candidate) => candidate.identity === vector.operation,
    );
    if (!operation) throw new Error(`${vector.id}: unknown operation ${vector.operation}.`);
    const requestPath = join(vectorRoot, vector.request_file);
    if (!existsSync(requestPath)) throw new Error(`${vector.id}: missing ${vector.request_file}`);
    referencedFiles.add(vector.request_file);
    const request = parseStrictJson(await readFile(requestPath));
    const validateRequest = schemas.get(operation.request_contract);
    if (!validateRequest?.(request)) {
      throw new Error(
        `${vector.id}: request does not match ${operation.request_contract}: ${ajv.errorsText(validateRequest?.errors)}`,
      );
    }
    for (const attachment of vector.attachments) {
      const attachmentPath = join(repositoryRoot, attachment.repository_file);
      if (!existsSync(attachmentPath)) {
        throw new Error(`${vector.id}: missing attachment ${attachment.repository_file}`);
      }
      const declaration = operation.input_attachments.find(
        (candidate) => candidate.name === attachment.name,
      );
      if (!declaration?.media_types.includes(attachment.media_type)) {
        throw new Error(`${vector.id}: attachment does not match the operation catalog.`);
      }
    }
    if (vector.expected === "success") {
      const expectedPath = join(vectorRoot, vector.expected_result_file);
      if (!existsSync(expectedPath)) {
        throw new Error(`${vector.id}: missing ${vector.expected_result_file}`);
      }
      referencedFiles.add(vector.expected_result_file);
      const expected = parseStrictJson(await readFile(expectedPath));
      const validateResult = schemas.get(operation.result_contract);
      if (!validateResult?.(expected)) {
        throw new Error(
          `${vector.id}: expected result does not match ${operation.result_contract}: ${ajv.errorsText(validateResult?.errors)}`,
        );
      }
    }
  }

  const caseFiles = (await listFiles(join(vectorRoot, "cases"))).map((path) => `cases/${path}`);
  assertEqual([...referencedFiles].sort(), caseFiles.sort(), "vector case inventory");
  const total = manifest.vectors.length + manifest.operation_vectors.length;
  process.stdout.write(`Validated ${total} governed contract vectors.\n`);
}

function decodeHexVector(storedBytes, id) {
  const text = storedBytes.toString("ascii").trim();
  if (!/^(?:[0-9a-f]{2})+$/u.test(text)) {
    throw new Error(`${id}: invalid lowercase even-length hex fixture`);
  }
  return Buffer.from(text, "hex");
}

function validateManifest(value) {
  if (value.manifest_identity !== "wn.geometer.contract_vectors" || value.generation !== "a0") {
    throw new Error("Unexpected contract-vector manifest identity or generation.");
  }
  if (!Array.isArray(value.vectors) || value.vectors.length === 0) {
    throw new Error("Contract-vector manifest must contain vectors.");
  }
  if (!Array.isArray(value.operation_vectors) || value.operation_vectors.length === 0) {
    throw new Error("Contract-vector manifest must contain operation vectors.");
  }
  const ids = new Set();
  for (const vector of value.vectors) {
    if (!vector.id || ids.has(vector.id)) {
      throw new Error(`Missing or duplicate vector id ${String(vector.id)}.`);
    }
    ids.add(vector.id);
    if (!["strict_json", "schema", "semantic"].includes(vector.lane)) {
      throw new Error(`${vector.id}: unsupported initial assertion lane ${vector.lane}.`);
    }
    if (!["accept", "reject"].includes(vector.expected)) {
      throw new Error(`${vector.id}: expected must be accept or reject.`);
    }
    if (vector.comparison !== "exact" || !Array.isArray(vector.excluded_fields)) {
      throw new Error(`${vector.id}: comparison and excluded_fields must be explicit.`);
    }
    if (vector.tolerance !== null) {
      throw new Error(`${vector.id}: initial exact vectors must declare null tolerance.`);
    }
    if (!vector.file.startsWith("cases/") || vector.file.includes("..")) {
      throw new Error(`${vector.id}: invalid repository-relative case path.`);
    }
    if (vector.oracle.startsWith("step_topology_")) {
      if (vector.lane !== "semantic") {
        throw new Error(`${vector.id}: STEP topology oracle must use the semantic lane.`);
      }
      if (vector.oracle === "step_topology_session") {
        if (
          typeof vector.expected_session_handle !== "string" ||
          !Number.isSafeInteger(vector.expected_generation)
        ) {
          throw new Error(`${vector.id}: session oracle context is incomplete.`);
        }
      } else if (
        vector.oracle === "step_topology_render_attachments" ||
        vector.oracle === "step_topology_checkpoint_attachment" ||
        vector.oracle === "step_topology_save_attachments" ||
        vector.oracle === "step_topology_restore_attachments"
      ) {
        if (!Array.isArray(vector.attachments) || vector.attachments.length === 0) {
          throw new Error(`${vector.id}: attachment oracle inputs are missing.`);
        }
        for (const attachment of vector.attachments) {
          if (
            typeof attachment.name !== "string" ||
            typeof attachment.media_type !== "string" ||
            !attachment.file?.startsWith("cases/") ||
            attachment.file.includes("..")
          ) {
            throw new Error(`${vector.id}: attachment oracle input is invalid.`);
          }
        }
        if (
          vector.oracle === "step_topology_save_attachments" &&
          (!vector.context_file?.startsWith("cases/") || vector.context_file.includes(".."))
        ) {
          throw new Error(`${vector.id}: save oracle context is invalid.`);
        }
      } else if (vector.oracle === "step_topology_restore_result") {
        if (!vector.context_file?.startsWith("cases/") || vector.context_file.includes("..")) {
          throw new Error(`${vector.id}: restore-result oracle context is invalid.`);
        }
      } else if (vector.oracle === "step_topology_inspection_high_fan_in") {
        if (
          !Number.isSafeInteger(vector.fan_in) ||
          vector.fan_in < 1024 ||
          vector.fan_in > 100000
        ) {
          throw new Error(`${vector.id}: high-fan-in size is invalid.`);
        }
      } else if (vector.oracle === "step_topology_inspection") {
        if (vector.prior_pages !== undefined) {
          if (
            !Array.isArray(vector.prior_pages) ||
            vector.prior_pages.length === 0 ||
            vector.prior_pages.some(
              (path) =>
                typeof path !== "string" || !path.startsWith("cases/") || path.includes(".."),
            )
          ) {
            throw new Error(`${vector.id}: inspection prior_pages are invalid.`);
          }
        }
      } else if (
        vector.oracle === "step_topology_logical_group_commands_high_fan_in" ||
        vector.oracle === "step_topology_logical_group_result_high_fan_in"
      ) {
        if (
          !Number.isSafeInteger(vector.fan_in) ||
          (vector.fan_in !== 100000 && vector.fan_in !== 100001)
        ) {
          throw new Error(`${vector.id}: logical-group high-fan-in size is invalid.`);
        }
      } else if (vector.oracle === "step_topology_ipc_pair_matrix") {
        if (
          !Array.isArray(vector.request_pairs) ||
          vector.request_pairs.length !== 12 ||
          !Array.isArray(vector.result_pairs) ||
          vector.result_pairs.length !== 12
        ) {
          throw new Error(`${vector.id}: IPC pair matrix must cover all topology operations.`);
        }
      } else if (
        vector.oracle !== "step_topology_logical_group_commands" &&
        vector.oracle !== "step_topology_metadata_probe_commands" &&
        vector.oracle !== "step_topology_metadata_probe_result" &&
        vector.oracle !== "step_topology_logical_group_result" &&
        vector.oracle !== "step_topology_hierarchy_commands" &&
        vector.oracle !== "step_topology_hierarchy_result" &&
        vector.oracle !== "step_topology_recovery_request" &&
        vector.oracle !== "step_topology_recovery_result" &&
        vector.oracle !== "step_topology_ipc_request_pair" &&
        vector.oracle !== "step_topology_ipc_result_pair"
      ) {
        throw new Error(`${vector.id}: unsupported STEP topology semantic oracle.`);
      }
    }
  }
  for (const vector of value.operation_vectors) {
    if (!vector.id || ids.has(vector.id)) {
      throw new Error(`Missing or duplicate vector id ${String(vector.id)}.`);
    }
    ids.add(vector.id);
    if (!["operation_semantic", "diagnostic"].includes(vector.lane)) {
      throw new Error(`${vector.id}: unsupported operation assertion lane ${vector.lane}.`);
    }
    if (!["success", "failure"].includes(vector.expected)) {
      throw new Error(`${vector.id}: operation expectation must be success or failure.`);
    }
    if (
      !Array.isArray(vector.excluded_fields) ||
      !Array.isArray(vector.computed_fields) ||
      !Array.isArray(vector.runtimes)
    ) {
      throw new Error(`${vector.id}: projections and runtimes must be explicit.`);
    }
    if (!vector.request_file.startsWith("cases/") || vector.request_file.includes("..")) {
      throw new Error(`${vector.id}: invalid request case path.`);
    }
    if (vector.expected === "success") {
      const computedHash = vector.computed_fields.find(
        (field) => field.path === "/result/source/hash",
      );
      if (
        vector.comparison !== "structural_numeric_tolerance" ||
        !(vector.tolerance?.absolute > 0) ||
        !(vector.tolerance?.relative > 0) ||
        !vector.expected_result_file?.startsWith("cases/") ||
        vector.expected_result_file.includes("..") ||
        computedHash?.oracle !== "fnv1a64_attachment" ||
        computedHash.attachment !== "model" ||
        computedHash.comparison !== "exact"
      ) {
        throw new Error(`${vector.id}: success vector lacks governed tolerance/result metadata.`);
      }
    } else if (
      vector.comparison !== "exact" ||
      vector.tolerance !== null ||
      typeof vector.expected_diagnostic?.code !== "string"
    ) {
      throw new Error(`${vector.id}: failure vector lacks exact diagnostic metadata.`);
    }
  }
}

function highFanInInspectionPages(seed, fanIn) {
  const definitionHandle = `gtt_${"a".repeat(64)}`;
  const shellHandle = `gtt_${"e".repeat(64)}`;
  const bodyHandles = Array.from(
    { length: fanIn },
    (_, index) => `gtt_${(index + 1).toString(16).padStart(64, "0")}`,
  );
  const pages = [];
  for (let offset = 0; offset < fanIn; offset += 1024) {
    const finalBodyPage = offset + 1024 >= fanIn;
    const handles = bodyHandles.slice(offset, offset + 1024);
    pages.push({
      schema: "geometry.step_topology.inspect.result.a0",
      session: seed.session,
      counts: {
        definitions: 1,
        root_occurrences: 0,
        component_occurrences: 0,
        bodies: fanIn,
        shells: 1,
        faces: 0,
        memberships: fanIn,
      },
      page: {
        definitions:
          offset === 0
            ? [
                {
                  handle: definitionHandle,
                  name: "high fan-in definition",
                  assembly: false,
                  body_count: fanIn,
                  face_count: 0,
                },
              ]
            : [],
        occurrences: [],
        bodies: handles.map((handle) => ({
          handle,
          definition_handle: definitionHandle,
          topology_kind: "solid",
          shell_count: 1,
          face_count: 0,
          bounds_mm: [0, 0, 0, 1, 1, 1],
          volume_mm3: 1,
        })),
        shells: finalBodyPage
          ? [
              {
                handle: shellHandle,
                definition_handle: definitionHandle,
                body_count: fanIn,
                face_count: 0,
              },
            ]
          : [],
        faces: [],
        memberships: [],
        next_cursor: `body-${offset + handles.length}`,
      },
      diagnostics: [],
    });
  }
  for (let offset = 0; offset < fanIn; offset += 1024) {
    const handles = bodyHandles.slice(offset, offset + 1024);
    const terminal = offset + handles.length >= fanIn;
    pages.push({
      schema: "geometry.step_topology.inspect.result.a0",
      session: seed.session,
      counts: pages[0].counts,
      page: {
        definitions: [],
        occurrences: [],
        bodies: [],
        shells: [],
        faces: [],
        memberships: handles.map((handle) => ({
          kind: "body_shell",
          owner_handle: handle,
          member_handle: shellHandle,
        })),
        ...(terminal ? {} : { next_cursor: `membership-${offset + handles.length}` }),
      },
      diagnostics: [],
    });
  }
  return pages;
}

function generatedTargetHandles(count, offset = 0) {
  return Array.from(
    { length: count },
    (_, index) => `gtt_${(offset + index + 1).toString(16).padStart(64, "0")}`,
  );
}

function assertOutcome(vector, valid, detail) {
  const expectedValid = vector.expected === "accept";
  if (valid !== expectedValid) {
    throw new Error(
      `${vector.id}: expected ${vector.expected}, got ${valid ? "accept" : "reject"}${detail ? `: ${formatError(detail)}` : ""}`,
    );
  }
}

function parseStrictJson(bytes) {
  const text = new TextDecoder("utf-8", { fatal: true }).decode(bytes);
  return new StrictJsonParser(text).parse();
}

class StrictJsonParser {
  constructor(text) {
    this.text = text;
    this.offset = 0;
  }

  parse() {
    this.skipWhitespace();
    const value = this.parseValue();
    this.skipWhitespace();
    if (this.offset !== this.text.length) this.fail("trailing JSON data");
    return value;
  }

  parseValue() {
    const character = this.text[this.offset];
    if (character === "{") return this.parseObject();
    if (character === "[") return this.parseArray();
    if (character === '"') return this.parseString();
    if (character === "t") return this.parseLiteral("true", true);
    if (character === "f") return this.parseLiteral("false", false);
    if (character === "n") return this.parseLiteral("null", null);
    if (character === "-" || (character >= "0" && character <= "9")) return this.parseNumber();
    this.fail("expected a JSON value");
  }

  parseObject() {
    this.offset += 1;
    this.skipWhitespace();
    const value = {};
    const keys = new Set();
    if (this.consume("}")) return value;
    while (true) {
      if (this.text[this.offset] !== '"') this.fail("expected an object key");
      const key = this.parseString();
      if (keys.has(key)) this.fail(`duplicate object key ${JSON.stringify(key)}`);
      keys.add(key);
      this.skipWhitespace();
      if (!this.consume(":")) this.fail("expected ':' after object key");
      this.skipWhitespace();
      value[key] = this.parseValue();
      this.skipWhitespace();
      if (this.consume("}")) return value;
      if (!this.consume(",")) this.fail("expected ',' or '}' in object");
      this.skipWhitespace();
    }
  }

  parseArray() {
    this.offset += 1;
    this.skipWhitespace();
    const value = [];
    if (this.consume("]")) return value;
    while (true) {
      value.push(this.parseValue());
      this.skipWhitespace();
      if (this.consume("]")) return value;
      if (!this.consume(",")) this.fail("expected ',' or ']' in array");
      this.skipWhitespace();
    }
  }

  parseString() {
    const start = this.offset;
    this.offset += 1;
    let escaped = false;
    while (this.offset < this.text.length) {
      const code = this.text.charCodeAt(this.offset);
      const character = this.text[this.offset];
      if (!escaped && character === '"') {
        this.offset += 1;
        return JSON.parse(this.text.slice(start, this.offset));
      }
      if (!escaped && code < 0x20) this.fail("unescaped control character in string");
      if (!escaped && character === "\\") {
        escaped = true;
      } else {
        escaped = false;
      }
      this.offset += 1;
    }
    this.fail("unterminated string");
  }

  parseNumber() {
    const match = /^-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?/u.exec(
      this.text.slice(this.offset),
    );
    if (!match) this.fail("invalid JSON number");
    this.offset += match[0].length;
    const value = Number(match[0]);
    if (!Number.isFinite(value)) this.fail("non-finite JSON number");
    return value;
  }

  parseLiteral(source, value) {
    if (!this.text.startsWith(source, this.offset)) this.fail(`invalid literal ${source}`);
    this.offset += source.length;
    return value;
  }

  skipWhitespace() {
    while ([" ", "\t", "\r", "\n"].includes(this.text[this.offset])) this.offset += 1;
  }

  consume(character) {
    if (this.text[this.offset] !== character) return false;
    this.offset += 1;
    return true;
  }

  fail(message) {
    throw new Error(`${message} at byte-compatible character offset ${this.offset}`);
  }
}

async function loadSemanticAttachments(vector, vectorRoot, referencedFiles) {
  const attachments = [];
  for (const attachment of vector.attachments) {
    referencedFiles.add(attachment.file);
    attachments.push({
      name: attachment.name,
      mediaType: attachment.media_type,
      data: await readFile(join(vectorRoot, attachment.file)),
    });
  }
  return attachments;
}

async function listFiles(root) {
  const output = [];
  async function visit(directory) {
    const entries = await readdir(directory, { withFileTypes: true });
    entries.sort((left, right) => left.name.localeCompare(right.name));
    for (const entry of entries) {
      const absolute = join(directory, entry.name);
      if (entry.isDirectory()) await visit(absolute);
      else if (entry.isFile()) output.push(relative(root, absolute).split(sep).join("/"));
      else throw new Error(`Unsupported vector entry ${absolute}.`);
    }
  }
  await visit(root);
  return output;
}

function assertEqual(actual, expected, label) {
  if (JSON.stringify(actual) !== JSON.stringify(expected)) {
    throw new Error(
      `${label}: expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`,
    );
  }
}

function formatError(value) {
  return value instanceof Error ? value.message : String(value);
}

await main();
