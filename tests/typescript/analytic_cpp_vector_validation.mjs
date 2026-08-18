import { spawnSync } from "node:child_process";
import { createHash } from "node:crypto";
import { existsSync, readFileSync } from "node:fs";
import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import { decodeAnalyticPlanarBooleanBatchResultA0Packet } from "../../dist/wasm/npm/geometer/analytic-packet-a0.js";

const root = resolve(fileURLToPath(new URL("../..", import.meta.url)));
const vectorRoot = join(root, "tests", "contracts", "vectors", "analytic");
const manifest = JSON.parse(readFileSync(join(vectorRoot, "manifest.json"), "utf8"));
if (
  manifest.manifest_identity !== "wn.geometer.analytic_packet_vectors" ||
  manifest.generation !== "a0"
) {
  throw new Error("The governed analytic packet vector manifest is incompatible.");
}

function committedVector(id) {
  const declaration = manifest.vectors.find((candidate) => candidate.id === id);
  if (declaration === undefined) throw new Error(`Committed analytic vector ${id} is absent.`);
  const hex = readFileSync(join(vectorRoot, declaration.file), "ascii").trim();
  const bytes = Uint8Array.from(Buffer.from(hex, "hex"));
  const digest = createHash("sha256").update(bytes).digest("hex");
  if (bytes.byteLength !== declaration.bytes || digest !== declaration.sha256) {
    throw new Error(`Committed analytic vector ${id} does not match its manifest.`);
  }
  return { bytes, hex };
}
const executable = resolve(
  root,
  "build",
  "tests",
  "cpp",
  `geometer_analytic_result_packet_records_test${process.platform === "win32" ? ".exe" : ""}`,
);
let vectors;
if (existsSync(executable)) {
  const completed = spawnSync(executable, [], { encoding: "utf8", maxBuffer: 16 * 1024 * 1024 });
  if (completed.status !== 0) throw new Error(completed.stdout + completed.stderr);
  vectors = new Map(
    completed.stdout
      .split(/\r?\n/u)
      .filter((line) => line.includes("="))
      .map((line) => {
        const separator = line.indexOf("=");
        return [line.slice(0, separator), line.slice(separator + 1)];
      }),
  );
} else {
  vectors = new Map();
  console.log("Native analytic vector producer is not built; validating the governed corpus only.");
}
const canonicalVector = committedVector("result.canonical-mixed");
const canonicalHex = vectors.get("ANALYTIC_RESULT_PACKET_CANONICAL_VECTOR");
if (canonicalHex !== undefined && canonicalHex !== canonicalVector.hex)
  throw new Error("Native canonical result bytes differ from the committed corpus.");
const canonical = canonicalVector.bytes;
if (canonical.byteLength !== 1220)
  throw new Error(`Canonical result vector is ${canonical.byteLength}, expected 1220.`);
const decoded = await decodeAnalyticPlanarBooleanBatchResultA0Packet(canonical);
const success = decoded.job_results.find((job) => job.status === "success");
const failure = decoded.job_results.find((job) => job.status === "failure");
const standaloneHex = vectors.get("ANALYTIC_RESULT_PACKET_STANDALONE_VECTOR");
const standaloneVector = committedVector("result.success-standalone");
if (standaloneHex !== undefined && standaloneHex !== standaloneVector.hex)
  throw new Error("Native standalone result bytes differ from the committed corpus.");
const standalone = await decodeAnalyticPlanarBooleanBatchResultA0Packet(standaloneVector.bytes);
const [standaloneSuccess] = standalone.job_results;
const mixedStandaloneHex = vectors.get("ANALYTIC_RESULT_PACKET_MIXED_SUCCESS_STANDALONE_VECTOR");
const mixedStandaloneVector = committedVector("result.mixed-success-standalone");
if (mixedStandaloneHex !== undefined && mixedStandaloneHex !== mixedStandaloneVector.hex)
  throw new Error("Native mixed-success result bytes differ from the committed corpus.");
const mixedStandalone = await decodeAnalyticPlanarBooleanBatchResultA0Packet(
  mixedStandaloneVector.bytes,
);
const [mixedStandaloneSuccess] = mixedStandalone.job_results;
if (
  standaloneSuccess?.status !== "success" ||
  standaloneSuccess.digest_sha256 !== manifest.job_digests.success_standalone ||
  failure?.digest_sha256 !== manifest.job_digests.failure_standalone ||
  mixedStandaloneSuccess?.status !== "success" ||
  mixedStandaloneSuccess.digest_sha256 !== success?.digest_sha256 ||
  success.digest_sha256 !== manifest.job_digests.success_in_mixed_batch
) {
  throw new Error("TypeScript standalone job digests differ from governed native vectors.");
}

const duplicateJob = canonical.slice();
const view = new DataView(duplicateJob.buffer);
const jobTableOffset = Number(view.getBigUint64(64 + 8, true));
view.setBigUint64(jobTableOffset + 48, view.getBigUint64(jobTableOffset, true), true);
let duplicateRejected = false;
try {
  await decodeAnalyticPlanarBooleanBatchResultA0Packet(duplicateJob);
} catch {
  duplicateRejected = true;
}
if (!duplicateRejected) throw new Error("Duplicate result job identities were accepted.");

async function requireRejected(bytes, label) {
  try {
    await decodeAnalyticPlanarBooleanBatchResultA0Packet(bytes);
  } catch {
    return;
  }
  throw new Error(`${label} was accepted.`);
}

const reorderedFragments = canonical.slice();
const reorderedView = new DataView(reorderedFragments.buffer);
const fragmentTableOffset = Number(reorderedView.getBigUint64(64 + 3 * 32 + 8, true));
const fragmentCount = Number(reorderedView.getBigUint64(64 + 3 * 32 + 24, true));
const fragmentReferenceOffset = Number(reorderedView.getBigUint64(64 + 5 * 32 + 8, true));
for (let byte = 0; byte < 48; byte += 1) {
  const temporary = reorderedFragments[fragmentTableOffset + byte];
  reorderedFragments[fragmentTableOffset + byte] =
    reorderedFragments[fragmentTableOffset + 48 + byte];
  reorderedFragments[fragmentTableOffset + 48 + byte] = temporary;
}
reorderedView.setBigUint64(fragmentTableOffset, 1n, true);
reorderedView.setBigUint64(fragmentTableOffset + 48, 2n, true);
const fragmentReferenceCount = Number(reorderedView.getBigUint64(64 + 5 * 32 + 24, true));
for (let index = 0; index < fragmentReferenceCount; index += 1) {
  const at = fragmentReferenceOffset + index * 4;
  const value = reorderedView.getUint32(at, true);
  if (value === 0) reorderedView.setUint32(at, 1, true);
  else if (value === 1) reorderedView.setUint32(at, 0, true);
}
if (fragmentCount < 2) throw new Error("Native vector lacks fragment-order coverage.");
await requireRejected(reorderedFragments, "Semantically reordered fragment table");

const invalidSource = canonical.slice();
const sourceView = new DataView(invalidSource.buffer);
const sourceOffset = Number(sourceView.getBigUint64(64 + 9 * 32 + 8, true));
const sourceCount = Number(sourceView.getBigUint64(64 + 9 * 32 + 24, true));
let authoredSource = -1;
for (let index = 0; index < sourceCount; index += 1) {
  if (sourceView.getUint16(sourceOffset + index * 32, true) === 1) {
    authoredSource = sourceOffset + index * 32;
    break;
  }
}
if (authoredSource < 0) throw new Error("Native vector lacks an authored source reference.");
sourceView.setBigUint64(authoredSource + 24, 0n, true);
await requireRejected(invalidSource, "Authored source with zero occurrence identity");

console.log(
  JSON.stringify({
    bytes: canonical.byteLength,
    jobs: decoded.job_results.length,
    successDigest: success.digest_sha256,
  }),
);
