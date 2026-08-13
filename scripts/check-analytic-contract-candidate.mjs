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
