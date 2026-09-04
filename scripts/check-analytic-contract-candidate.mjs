// @ts-check

import { spawnSync } from "node:child_process";
import { mkdtemp, readFile, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import Ajv from "ajv";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const output = await mkdtemp(join(tmpdir(), "geometer-analytic-contract-"));

try {
  const compiler = join(root, "node_modules", "@typespec", "compiler", "cmd", "tsp.js");
  const result = spawnSync(
    process.execPath,
    [
      compiler,
      "compile",
      "src/tsp/geometer/analytic-candidate.tsp",
      "--config",
      "tspconfig.yaml",
      "--output-dir",
      output,
      "--warn-as-error",
    ],
    { cwd: root, encoding: "utf8", stdio: "pipe" },
  );
  if (result.error) throw result.error;
  if (result.status !== 0) {
    throw new Error(
      `Analytic candidate TypeSpec compilation failed.\n${result.stdout ?? ""}${result.stderr ?? ""}`.trimEnd(),
    );
  }

  const catalog = JSON.parse(
    await readFile(join(output, "wn_geometer_contract_catalog.a0.json"), "utf8"),
  );
  const numericCatalog = parseTomlScalarTables(
    await readFile(
      join(root, "docs", "contracts", "analytic-planar-boolean-a0-catalog.toml"),
      "utf8",
    ),
  );
  const schema = JSON.parse(
    await readFile(join(root, "contracts", "geometer", "catalog-schema.a0.json"), "utf8"),
  );
  const validate = new Ajv({ allErrors: true, strict: true }).compile(schema);
  if (!validate(catalog)) {
    throw new Error(`Analytic candidate catalog is invalid: ${JSON.stringify(validate.errors)}`);
  }

  const expectedRoots = [
    "geometry.analytic_planar_boolean_batch.request.a0",
    "geometry.analytic_planar_boolean_batch.result.a0",
  ];
  const analyticRoots = catalog.roots
    .map((item) => item.contract_identity)
    .filter((identity) => identity.startsWith("geometry.analytic_planar_boolean_batch."))
    .sort();
  assertEqual(analyticRoots, expectedRoots, "analytic root identities");

  const operation = catalog.operations.find(
    (item) => item.identity === "geometry.analytic_planar_boolean_batch.a0",
  );
  if (!operation) throw new Error("Analytic candidate operation is missing.");
  assertEqual(operation.runtime_dispatch, "packed_attachment", "runtime dispatch lane");
  assertEqual(operation.request_contract, expectedRoots[0], "request contract");
  assertEqual(operation.result_contract, expectedRoots[1], "result contract");
  assertEqual(
    operation.request_projection,
    {
      kind: "packed_attachment",
      attachment_name: "analytic_planar_boolean_request",
      format: "geometry.analytic_planar_boolean.packet.a0",
    },
    "request packed projection",
  );
  assertEqual(
    operation.result_projection,
    {
      kind: "packed_attachment",
      attachment_name: "analytic_planar_boolean_result",
      format: "geometry.analytic_planar_boolean.packet.a0",
    },
    "result packed projection",
  );
  assertEqual(
    operation.input_attachments,
    [
      {
        name: "analytic_planar_boolean_request",
        required: true,
        media_types: ["application/vnd.wavenumber.geometer.analytic-planar-boolean-request"],
        max_bytes: 268435456,
      },
    ],
    "request attachment",
  );
  assertEqual(
    operation.output_attachments,
    [
      {
        name: "analytic_planar_boolean_result",
        required: true,
        media_types: ["application/vnd.wavenumber.geometer.analytic-planar-boolean-result"],
        max_bytes: 268435456,
      },
    ],
    "result attachment",
  );

  const declarations = catalog.declarations.filter((item) =>
    item.name.includes(".AnalyticPlanarBooleanA0."),
  );
  const declarationByName = new Map(declarations.map((item) => [shortName(item.name), item]));
  const declaredNames = new Set(declarations.map((item) => shortName(item.name)));
  for (const required of [
    "AnalyticPlanarBooleanBatchRequestA0",
    "AnalyticPlanarBooleanBatchResultA0",
    "AnalyticPlanarOperand",
    "PlanarRing",
    "PlanarPath",
    "DirectedFragment",
    "SourceReference",
    "OperandOutcomeEvent",
    "PlanarRelationshipResult",
  ]) {
    if (!declaredNames.has(required)) throw new Error(`Missing analytic declaration ${required}.`);
  }

  const idScalars = declarations.filter(
    (item) => item.kind === "scalar" && shortName(item.name).endsWith("Id"),
  );
  assertEqual(idScalars.length, 15, "declared identity-space count");
  for (const scalar of idScalars) {
    assertEqual(scalar.base, { kind: "primitive", name: "uint64" }, `${scalar.name} base`);
    assertEqual(scalar.constraints.min_value, 1, `${scalar.name} nonzero constraint`);
  }

  assertModelFields(declarationByName, {
    AnalyticPlanarBooleanBatchRequestA0: ["jobs", "relationship_queries"],
    AnalyticPlanarBooleanBatchResultA0: ["job_results", "relationship_results"],
    AnalyticPlanarBooleanJob: ["job_id", "stages"],
    AnalyticPlanarBooleanStage: ["stage_id", "operation", "operands"],
    AnnulusOperand: [
      "operand_id",
      "kind",
      "feature_id",
      "center",
      "inner_radius_nm",
      "outer_radius_nm",
    ],
    AuthoredCircularArcSegment: [
      "segment_id",
      "curve_id",
      "kind",
      "center",
      "direction",
      "major_arc",
    ],
    AuthoredCircularArcByRadiusSegment: [
      "segment_id",
      "curve_id",
      "kind",
      "radius_nm",
      "direction",
      "major_arc",
    ],
    AuthoredLineSegment: ["segment_id", "curve_id", "kind"],
    AuthoredVertex: ["vertex_id", "point"],
    CapsuleOperand: ["operand_id", "kind", "feature_id", "start", "end", "width_nm"],
    DiskOperand: ["operand_id", "kind", "feature_id", "center", "radius_nm"],
    FailedJobResult: ["job_id", "status", "diagnostics", "digest_sha256"],
    JobDiagnostic: [
      "code",
      "severity",
      "job_id",
      "stage_id?",
      "operand_id?",
      "geometry_id?",
      "path_identity?",
    ],
    OperandOutcomeEvent: ["operand_id", "kind", "result_ring_ids", "result_region_ids", "sources"],
    PlanarPath: ["path_id", "vertices", "segments"],
    PlanarRegionOperand: ["operand_id", "kind", "region_id", "outer", "holes"],
    PlanarRelationshipQuery: ["query_id", "left_job_id", "right_job_id"],
    PlanarRelationshipResult: ["query_id", "status", "aggregate_dimension", "pairs"],
    PlanarRing: ["ring_id", "vertices", "segments"],
    PointNm: ["x", "y"],
    RelationshipRegionPair: [
      "left_result_region_id",
      "right_result_region_id",
      "dimension",
      "equality",
      "left_contains_right",
      "right_contains_left",
    ],
    ResultCircularArcFragment: [
      "fragment_id",
      "kind",
      "start_vertex_id",
      "end_vertex_id",
      "radius_nm",
      "direction",
      "major_arc",
      "coincident_positive_sources",
      "surviving_subtraction_sources",
    ],
    ResultLineFragment: [
      "fragment_id",
      "kind",
      "start_vertex_id",
      "end_vertex_id",
      "coincident_positive_sources",
      "surviving_subtraction_sources",
    ],
    ResultRegion: ["result_region_id", "outer_ring_id", "positive_contributors"],
    ResultRing: ["ring_id", "fragment_ids", "parent_ring_id?", "depth", "hole"],
    ResultVertex: ["vertex_id", "point", "intersection_sources"],
    SourceReference: ["kind", "role", "operand_id", "primary_id", "secondary_id"],
    SourceSet: ["sources"],
    SuccessfulJobResult: [
      "job_id",
      "status",
      "diagnostics",
      "vertices",
      "directed_fragments",
      "rings",
      "result_regions",
      "operand_outcomes",
      "digest_sha256",
    ],
    SweptPathOperand: ["operand_id", "kind", "feature_id", "centerline", "width_nm", "cap", "join"],
  });
  assertEqual(
    declarationByName.get("AnalyticPlanarBooleanBatchResultA0").annotations[
      "x-wn-max-logical-source-reference-expansions"
    ],
    1048576,
    "logical source-reference expansion budget",
  );
  assertEqual(
    modelProperty(declarationByName, "SourceSet", "sources").constraints.max_items,
    1048576,
    "logical source-set member maximum",
  );
  assertEqual(
    numericCatalog.limit.logical_source_reference_expansions_per_batch,
    1048576,
    "numeric logical source-reference expansion budget",
  );

  assertUnions(declarationByName, {
    AnalyticPlanarBooleanJobResult: {
      success: "SuccessfulJobResult",
      failure: "FailedJobResult",
    },
    AnalyticPlanarOperand: {
      planar_region: "PlanarRegionOperand",
      disk: "DiskOperand",
      annulus: "AnnulusOperand",
      capsule: "CapsuleOperand",
      swept_path: "SweptPathOperand",
    },
    AuthoredSegment: {
      line: "AuthoredLineSegment",
      circular_arc: "AuthoredCircularArcSegment",
      circular_arc_by_radius: "AuthoredCircularArcByRadiusSegment",
    },
    AuthoredPathSegment: {
      line: "AuthoredLineSegment",
      circular_arc: "AuthoredCircularArcSegment",
    },
    DirectedFragment: {
      line: "ResultLineFragment",
      circular_arc: "ResultCircularArcFragment",
    },
  });

  const expectedWireEnums = {
    "enum.stage_operation": { underlying: "u8", union: 1, difference: 2 },
    "enum.operand_geometry_kind": {
      underlying: "u16",
      planar_region: 1,
      disk: 2,
      annulus: 3,
      capsule: 4,
      swept_path: 5,
    },
    "enum.authored_segment_kind": {
      underlying: "u8",
      line: 1,
      circular_arc: 2,
      circular_arc_by_radius: 3,
    },
    "enum.fragment_kind": { underlying: "u8", line: 1, circular_arc: 2 },
    "enum.arc_direction": { underlying: "u8", not_applicable: 0, ccw: 1, cw: 2 },
    "enum.job_status": { underlying: "u8", success: 0, failure: 1 },
    "enum.diagnostic_severity": { underlying: "u8", error: 1, warning: 2 },
    "enum.diagnostic_scope": { underlying: "u8", job: 1 },
    "enum.reference_kind": { underlying: "u32_high_half", ring: 1, result_region: 2 },
    "enum.source_kind": {
      underlying: "u16",
      authored_segment_curve: 1,
      compact_feature_role: 2,
      subtractive_operand_effect: 3,
    },
    "enum.source_role": {
      underlying: "u16",
      none: 0,
      authored_line: 1,
      authored_circular_arc: 2,
      primitive_outer_circle: 16,
      primitive_inner_circle: 17,
      capsule_left_line: 32,
      capsule_end_cap: 33,
      capsule_right_line: 34,
      capsule_start_cap: 35,
      swept_left_offset_line: 48,
      swept_left_offset_arc: 49,
      swept_right_offset_line: 50,
      swept_right_offset_arc: 51,
      swept_round_join: 52,
      swept_start_cap: 53,
      swept_end_cap: 54,
    },
    "enum.operand_event": {
      underlying: "u16",
      contributes_final_material: 1,
      redundant_or_absorbed_coverage: 2,
      partially_removed_later: 3,
      completely_removed_later: 4,
      subtraction_effect_survives: 5,
      subtraction_effect_overwritten_later: 6,
      no_effect: 7,
    },
    "enum.relationship_status": {
      underlying: "u8",
      success: 0,
      skipped_dependency_failed: 1,
    },
    "enum.intersection_dimension": {
      underlying: "u8",
      disjoint: 0,
      point: 1,
      curve: 2,
      area: 3,
    },
  };
  for (const [table, expected] of Object.entries(expectedWireEnums)) {
    assertEqual(numericCatalog[table], expected, `${table} numeric mapping`);
  }

  assertEnumValues(declarationByName, "StageOperation", ["union", "difference"]);
  assertEnumValues(declarationByName, "ArcDirection", ["ccw", "cw"]);
  assertEnumValues(declarationByName, "DiagnosticSeverity", ["error", "warning"]);
  assertEnumValues(declarationByName, "SourceKind", [
    "authored_segment_curve",
    "compact_feature_role",
    "subtractive_operand_effect",
  ]);
  assertEnumValues(
    declarationByName,
    "SourceRole",
    Object.keys(expectedWireEnums["enum.source_role"]).filter((item) => item !== "underlying"),
  );
  assertEnumValues(
    declarationByName,
    "OperandOutcomeKind",
    Object.keys(expectedWireEnums["enum.operand_event"]).filter((item) => item !== "underlying"),
  );
  assertEnumValues(declarationByName, "RelationshipStatus", [
    "success",
    "skipped_dependency_failed",
  ]);
  assertEnumValues(declarationByName, "IntersectionDimension", [
    "disjoint",
    "point",
    "curve",
    "area",
  ]);
  assertEqual(
    [...declarationByName.values()]
      .filter((item) => item.kind === "enum")
      .map((item) => shortName(item.name))
      .sort(),
    [
      "ArcDirection",
      "DiagnosticSeverity",
      "IntersectionDimension",
      "JobDiagnosticCode",
      "JobDiagnosticPath",
      "OperandOutcomeKind",
      "RelationshipStatus",
      "SourceKind",
      "SourceRole",
      "StageOperation",
    ].sort(),
    "analytic enum inventory",
  );

  const expectedDiagnosticCodes = {
    invalid_topology: 65539,
    invalid_arc: 65540,
    unsupported_geometry: 65541,
    normalization_error_exceeded: 65543,
    normalization_topology_collapse: 65544,
    nonanalytic_result: 65545,
    solver_failed: 65546,
    resource_limit_exceeded: 65547,
    resolution_coalesced: 65548,
  };
  const expectedDiagnosticIdentities = Object.fromEntries(
    Object.keys(expectedDiagnosticCodes).map((name) => [
      name,
      `geometer.operation.analytic_planar_boolean.${name}`,
    ]),
  );
  assertEqual(
    numericCatalog.operation_diagnostic,
    expectedDiagnosticCodes,
    "operation diagnostic numeric mapping",
  );
  assertEqual(
    numericCatalog.operation_diagnostic_identity,
    expectedDiagnosticIdentities,
    "operation diagnostic identity mapping",
  );
  assertEnumValues(
    declarationByName,
    "JobDiagnosticCode",
    Object.values(expectedDiagnosticIdentities),
  );
  assertEqual(
    numericCatalog["reserved.operation_diagnostic"],
    {
      normalization_ambiguous_tie: 65542,
    },
    "reserved operation diagnostic mapping",
  );
  assertEqual(
    numericCatalog.contract_diagnostic,
    {
      invalid_packet: "geometer.contract.analytic_planar_boolean_packet.invalid_packet",
      invalid_id: "geometer.contract.analytic_planar_boolean_packet.invalid_id",
      invalid_reference: "geometer.contract.analytic_planar_boolean_packet.invalid_reference",
      limit_exceeded: "geometer.contract.analytic_planar_boolean_packet.limit_exceeded",
    },
    "contract diagnostic mapping",
  );
  assertEqual(
    numericCatalog["source_reference_mapping.authored_segment_curve"],
    {
      primary_id: "authored_segment_id",
      secondary_id: "authored_curve_id",
      allowed_roles: [1, 2],
    },
    "authored source-reference projection",
  );
  assertEqual(
    numericCatalog["source_reference_mapping.compact_feature_role"],
    {
      primary_id: "compact_feature_id",
      secondary_id: "boundary_occurrence_key",
      allowed_roles: [16, 17, 32, 33, 34, 35, 48, 49, 50, 51, 52, 53, 54],
    },
    "compact source-reference projection",
  );
  assertEqual(
    numericCatalog["source_reference_mapping.subtractive_operand_effect"],
    { primary_id: "stage_id", secondary_id: "zero", allowed_roles: [0] },
    "subtractive source-reference projection",
  );

  const expectedPathTokens = {
    none: 0,
    request_jobs: 1,
    job_id: 2,
    job_stages: 3,
    stage_id: 4,
    stage_operation: 5,
    stage_operands: 6,
    operand_id: 7,
    operand_geometry: 8,
    region_outer: 9,
    region_holes: 10,
    ring_vertices: 11,
    ring_segments: 12,
    path_vertices: 13,
    path_segments: 14,
    segment_curve: 15,
    disk_radius: 16,
    annulus_inner_radius: 17,
    annulus_outer_radius: 18,
    capsule_start: 19,
    capsule_end: 20,
    capsule_width: 21,
    swept_path_centerline: 22,
    swept_path_width: 23,
    relationship_queries: 24,
    relationship_left_job_id: 25,
    relationship_right_job_id: 26,
  };
  assertEqual(numericCatalog.path_token, expectedPathTokens, "diagnostic path-token mapping");
  const expectedPathIdentities = Object.keys(expectedPathTokens).filter((name) => name !== "none");
  assertEnumValues(declarationByName, "JobDiagnosticPath", expectedPathIdentities);
  const pathIdentityDeclaration = declarationByName.get("JobDiagnosticPath");
  assertEqual(
    Object.fromEntries(
      pathIdentityDeclaration.members.map((member) => [member.name, member.value]),
    ),
    Object.fromEntries(expectedPathIdentities.map((name) => [name, name])),
    "diagnostic path-token identity projection",
  );

  const diagnosticPath = modelProperty(declarationByName, "JobDiagnostic", "path_identity");
  assertEqual(diagnosticPath.optional, true, "logical diagnostic path-identity optionality");
  assertEqual(
    shortName(diagnosticPath.type.target),
    "JobDiagnosticPath",
    "logical diagnostic path-identity type",
  );
  assertEqual(
    diagnosticPath.annotations["x-wn-packed-field"],
    "path_token",
    "logical diagnostic path-identity packed projection",
  );
  for (const resultName of ["SuccessfulJobResult", "FailedJobResult"]) {
    const digest = modelProperty(declarationByName, resultName, "digest_sha256");
    assertEqual(
      digest.annotations["x-wn-derived-field"],
      "standalone_job_result_sha256",
      `${resultName} digest projection`,
    );
    assertEqual(digest.constraints.pattern, "^[0-9a-f]{64}$", `${resultName} digest shape`);
  }

  const requestSchema = JSON.parse(
    await readFile(join(output, "schema", "AnalyticPlanarBooleanBatchRequestA0.json"), "utf8"),
  );
  const resultSchema = JSON.parse(
    await readFile(join(output, "schema", "AnalyticPlanarBooleanBatchResultA0.json"), "utf8"),
  );
  assertEqual(
    requestSchema.$id,
    "urn:wavenumber:schema:geometer:geometry.analytic_planar_boolean_batch.request:a0",
    "request schema id",
  );
  assertEqual(
    resultSchema.$id,
    "urn:wavenumber:schema:geometer:geometry.analytic_planar_boolean_batch.result:a0",
    "result schema id",
  );
  assertEqual(
    resultSchema["x-wn-max-logical-source-reference-expansions"],
    1048576,
    "result schema logical source-reference expansion budget",
  );
  assertEqual(
    resultSchema.$defs.SourceSet.properties.sources.maxItems,
    1048576,
    "result schema source-set member maximum",
  );

  process.stdout.write("Analytic planar Boolean TypeSpec candidate is valid and isolated.\n");
} finally {
  await rm(output, { recursive: true, force: true });
}

function assertEqual(actual, expected, label) {
  if (JSON.stringify(actual) !== JSON.stringify(expected)) {
    throw new Error(
      `${label} mismatch:\n  expected ${JSON.stringify(expected)}\n  actual   ${JSON.stringify(actual)}`,
    );
  }
}

function shortName(name) {
  return name.slice(name.lastIndexOf(".") + 1);
}

function assertModelFields(declarations, expectedModels) {
  for (const [name, expectedFields] of Object.entries(expectedModels)) {
    const declaration = declarations.get(name);
    if (declaration?.kind !== "model") {
      throw new Error(`Missing analytic model ${name}.`);
    }
    const actualFields = declaration.properties.map(
      (property) => `${property.name}${property.optional ? "?" : ""}`,
    );
    assertEqual(actualFields, expectedFields, `${name} fields`);
  }
  const actualModels = [...declarations.values()]
    .filter((item) => item.kind === "model")
    .map((item) => shortName(item.name))
    .sort();
  assertEqual(actualModels, Object.keys(expectedModels).sort(), "analytic model inventory");
}

function assertUnions(declarations, expectedUnions) {
  for (const [name, expectedVariants] of Object.entries(expectedUnions)) {
    const declaration = declarations.get(name);
    if (declaration?.kind !== "union" || declaration.composition !== "one_of") {
      throw new Error(`Missing analytic one-of union ${name}.`);
    }
    const actualVariants = Object.fromEntries(
      declaration.variants.map((variant) => [variant.name, shortName(variant.type.target)]),
    );
    assertEqual(actualVariants, expectedVariants, `${name} variants`);
  }
  const actualUnions = [...declarations.values()]
    .filter((item) => item.kind === "union")
    .map((item) => shortName(item.name))
    .sort();
  assertEqual(actualUnions, Object.keys(expectedUnions).sort(), "analytic union inventory");
}

function assertEnumValues(declarations, name, expectedValues) {
  const declaration = declarations.get(name);
  if (declaration?.kind !== "enum") {
    throw new Error(`Missing analytic enum ${name}.`);
  }
  assertEqual(
    declaration.members.map((member) => member.value),
    expectedValues,
    `${name} logical values`,
  );
}

function modelProperty(declarations, modelName, propertyName) {
  const declaration = declarations.get(modelName);
  if (declaration?.kind !== "model") {
    throw new Error(`Missing analytic model ${modelName}.`);
  }
  const property = declaration.properties.find((item) => item.name === propertyName);
  if (!property) throw new Error(`Missing ${modelName}.${propertyName}.`);
  return property;
}

function parseTomlScalarTables(text) {
  const tables = { root: {} };
  let current = tables.root;
  for (const rawLine of text.split(/\r?\n/u)) {
    const line = rawLine.trim();
    if (line === "" || line.startsWith("#")) continue;
    const section = /^\[([^\]]+)\]$/u.exec(line);
    if (section) {
      current = {};
      tables[section[1]] = current;
      continue;
    }
    const assignment = /^([A-Za-z0-9_]+)\s*=\s*(.+)$/u.exec(line);
    if (!assignment) throw new Error(`Unsupported analytic numeric-catalog line: ${rawLine}`);
    current[assignment[1]] = parseTomlScalar(assignment[2]);
  }
  return tables;
}

function parseTomlScalar(value) {
  if (value.startsWith('"') || value.startsWith("[")) return JSON.parse(value);
  if (value === "true") return true;
  if (value === "false") return false;
  if (/^-?[0-9]+$/u.test(value)) return Number(value);
  throw new Error(`Unsupported analytic numeric-catalog value: ${value}`);
}
