import { readFile } from "node:fs/promises";
import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import {
  decodeDiagnosticA0Json,
  decodeModelBoundsOptionsA0Json,
  decodeModelBoundsResultA0Json,
  decodeOperationOutcomeA0Json,
  encodeModelBoundsOptionsA0Json,
  encodeOperationOutcomeA0Json,
} from "../../dist/wasm/npm/geometer/generated/index.js";

const root = resolve(fileURLToPath(new URL("../..", import.meta.url)));
const vectorRoot = join(root, "tests", "contracts", "vectors");
const manifest = JSON.parse(await readFile(join(vectorRoot, "manifest.json"), "utf8"));
const decoders = {
  "geometry.common.diagnostic.a0": decodeDiagnosticA0Json,
  "geometry.model_bounds.a0": decodeModelBoundsResultA0Json,
  "geometry.model_bounds.options.a0": decodeModelBoundsOptionsA0Json,
  "geometer.operation.outcome.a0": decodeOperationOutcomeA0Json,
};

for (const vector of manifest.vectors) {
  const decoder = decoders[vector.contract_identity];
  if (!decoder) throw new Error(`${vector.id}: no generated TypeScript decoder.`);
  const stored = await readFile(join(vectorRoot, vector.file));
  const data =
    vector.oracle === "strict_parser_hex"
      ? Buffer.from(stored.toString("ascii").trim(), "hex")
      : stored;
  let decoded;
  let error = null;
  try {
    decoded = decoder(data);
  } catch (caught) {
    error = caught;
  }
  const accepted = error === null;
  if (accepted !== (vector.expected === "accept")) {
    throw new Error(
      `${vector.id}: expected ${vector.expected}, got ${accepted ? "accept" : String(error)}.`,
    );
  }
  if (accepted && vector.oracle === "presence_projection") {
    const projection = Object.fromEntries(
      vector.fields.map((field) => [field, Object.hasOwn(decoded, field) ? "present" : "absent"]),
    );
    if (JSON.stringify(projection) !== JSON.stringify(vector.expected_value)) {
      throw new Error(`${vector.id}: presence projection mismatch.`);
    }
  }
}

if (encodeModelBoundsOptionsA0Json({}) !== "{}") {
  throw new Error("Empty option patches must not materialize defaults.");
}
if (encodeModelBoundsOptionsA0Json({ format: "step" }) !== '{"format":"step"}') {
  throw new Error("Explicit default presence must be preserved by the encoder.");
}
let invalidUnicodeAccepted = false;
try {
  encodeModelBoundsOptionsA0Json({ format: `step\ud800` });
  invalidUnicodeAccepted = true;
} catch {
  // Expected.
}
if (invalidUnicodeAccepted) throw new Error("Encoder accepted an unpaired surrogate.");

for (const [label, encode] of [
  ["fixed tuple", () => encodeModelBoundsOptionsA0Json({ model_transform: new Array(16) })],
  [
    "variable array",
    () =>
      encodeOperationOutcomeA0Json({
        operation: "geometry.model_bounds.a0",
        ok: false,
        diagnostics: new Array(1),
      }),
  ],
]) {
  let sparseArrayAccepted = false;
  try {
    encode();
    sparseArrayAccepted = true;
  } catch {
    // Expected: every array index, including a hole, is validated.
  }
  if (sparseArrayAccepted) throw new Error(`Encoder accepted a sparse ${label}.`);
}

console.log(JSON.stringify({ vectors: manifest.vectors.length, generatedCodecs: 4 }));
