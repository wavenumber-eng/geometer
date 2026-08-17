import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { resolve } from "node:path";
import { fileURLToPath } from "node:url";

import { decodeAnalyticPlanarBooleanBatchResultA0Packet } from "../../dist/wasm/npm/geometer/analytic-packet-a0.js";

const root = resolve(fileURLToPath(new URL("../..", import.meta.url)));
const executable = resolve(
  root,
  "build",
  "tests",
  "cpp",
  `geometer_analytic_result_packet_records_test${process.platform === "win32" ? ".exe" : ""}`,
);
if (!existsSync(executable))
  throw new Error(`Native analytic packet vector producer is missing: ${executable}`);
const completed = spawnSync(executable, [], { encoding: "utf8", maxBuffer: 16 * 1024 * 1024 });
if (completed.status !== 0) throw new Error(completed.stdout + completed.stderr);
const vectors = new Map(
  completed.stdout
    .split(/\r?\n/u)
    .filter((line) => line.includes("="))
    .map((line) => {
      const separator = line.indexOf("=");
      return [line.slice(0, separator), line.slice(separator + 1)];
    }),
);
const canonicalHex = vectors.get("ANALYTIC_RESULT_PACKET_CANONICAL_VECTOR");
if (canonicalHex === undefined) throw new Error("Native canonical result vector is absent.");
const canonical = Uint8Array.from(Buffer.from(canonicalHex, "hex"));
if (canonical.byteLength !== 1220)
  throw new Error(`Canonical result vector is ${canonical.byteLength}, expected 1220.`);
const decoded = await decodeAnalyticPlanarBooleanBatchResultA0Packet(canonical);
const success = decoded.job_results.find((job) => job.status === "success");
const failure = decoded.job_results.find((job) => job.status === "failure");
const standaloneHex = vectors.get("ANALYTIC_RESULT_PACKET_STANDALONE_VECTOR");
if (standaloneHex === undefined) throw new Error("Native standalone result vector is absent.");
const standalone = await decodeAnalyticPlanarBooleanBatchResultA0Packet(
  Uint8Array.from(Buffer.from(standaloneHex, "hex")),
);
const [standaloneSuccess] = standalone.job_results;
const mixedStandaloneHex = vectors.get("ANALYTIC_RESULT_PACKET_MIXED_SUCCESS_STANDALONE_VECTOR");
if (mixedStandaloneHex === undefined) throw new Error("Native mixed success closure is absent.");
const mixedStandalone = await decodeAnalyticPlanarBooleanBatchResultA0Packet(
  Uint8Array.from(Buffer.from(mixedStandaloneHex, "hex")),
);
const [mixedStandaloneSuccess] = mixedStandalone.job_results;
if (
  standaloneSuccess?.status !== "success" ||
  standaloneSuccess.digest_sha256 !== vectors.get("ANALYTIC_RESULT_PACKET_STANDALONE_DIGEST") ||
  failure?.digest_sha256 !== vectors.get("ANALYTIC_RESULT_PACKET_FAILED_STANDALONE_DIGEST") ||
  mixedStandaloneSuccess?.status !== "success" ||
  mixedStandaloneSuccess.digest_sha256 !== success?.digest_sha256 ||
  success.digest_sha256 !== vectors.get("ANALYTIC_RESULT_PACKET_MIXED_SUCCESS_STANDALONE_DIGEST")
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
