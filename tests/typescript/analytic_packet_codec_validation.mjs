import { readFileSync } from "node:fs";
import {
  AnalyticPacketError,
  decodeAnalyticPlanarBooleanBatchResultA0Packet,
  encodeAnalyticPlanarBooleanBatchRequestA0Packet,
} from "../../dist/wasm/npm/geometer/analytic-packet-a0.js";

function requireValue(condition, message) {
  if (!condition) throw new Error(message);
}

const empty = encodeAnalyticPlanarBooleanBatchRequestA0Packet({
  jobs: [],
  relationship_queries: [],
});
const emptyView = new DataView(empty.buffer, empty.byteOffset, empty.byteLength);
requireValue(empty.byteLength === 480, "The canonical empty request must be exactly 480 bytes.");
requireValue(
  new TextDecoder().decode(empty.subarray(0, 8)) === "GMABRQ01",
  "Request magic drifted.",
);
requireValue(emptyView.getBigUint64(16, true) === 480n, "Request size accounting drifted.");
requireValue(emptyView.getUint32(32, true) === 13, "Request directory count drifted.");

const maximumId = (1n << 64n) - 1n;
const encoded = encodeAnalyticPlanarBooleanBatchRequestA0Packet({
  jobs: [
    {
      job_id: maximumId,
      stages: [
        {
          stage_id: maximumId,
          operation: "union",
          operands: [
            {
              operand_id: maximumId,
              kind: "disk",
              feature_id: maximumId,
              center: { x: -9_007_199_254_740_993n, y: 9_007_199_254_740_993n },
              radius_nm: 1_000_000n,
            },
          ],
        },
      ],
    },
  ],
  relationship_queries: [],
});
const view = new DataView(encoded.buffer, encoded.byteOffset, encoded.byteLength);
const jobOffset = Number(view.getBigUint64(72, true));
const diskOffset = Number(view.getBigUint64(64 + 8 * 32 + 8, true));
requireValue(view.getBigUint64(jobOffset, true) === maximumId, "uint64 IDs lost precision.");
requireValue(
  view.getBigInt64(diskOffset + 8, true) === -9_007_199_254_740_993n,
  "int64 coordinates lost precision.",
);

let numberRejected = false;
try {
  encodeAnalyticPlanarBooleanBatchRequestA0Packet({
    jobs: [{ job_id: 1, stages: [] }],
    relationship_queries: [],
  });
} catch (error) {
  numberRejected = error instanceof AnalyticPacketError;
}
requireValue(numberRejected, "A JavaScript number was accepted for a uint64 identity.");

for (const [label, invalid] of [
  [
    "stage operation",
    {
      jobs: [{ job_id: 1n, stages: [{ stage_id: 1n, operation: "bogus", operands: [] }] }],
      relationship_queries: [],
    },
  ],
  [
    "query job reference",
    {
      jobs: [{ job_id: 1n, stages: [] }],
      relationship_queries: [{ query_id: 1n, left_job_id: 1n, right_job_id: 2n }],
    },
  ],
  [
    "length limit",
    {
      jobs: [
        {
          job_id: 1n,
          stages: [
            {
              stage_id: 1n,
              operation: "union",
              operands: [
                {
                  operand_id: 1n,
                  kind: "disk",
                  feature_id: 1n,
                  center: { x: 0n, y: 0n },
                  radius_nm: 1_000_000_000_001n,
                },
              ],
            },
          ],
        },
      ],
      relationship_queries: [],
    },
  ],
]) {
  let rejected = false;
  try {
    encodeAnalyticPlanarBooleanBatchRequestA0Packet(invalid);
  } catch (error) {
    rejected = error instanceof AnalyticPacketError;
  }
  requireValue(rejected, `Invalid ${label} was accepted by the request encoder.`);
}

function emptyResultPacket() {
  const recordBytes = [48, 56, 32, 48, 32, 4, 24, 8, 8, 32, 48, 32, 32, 4];
  const packet = new Uint8Array(64 + 32 * recordBytes.length);
  const packetView = new DataView(packet.buffer);
  packet.set(new TextEncoder().encode("GMABRS01"));
  packetView.setUint16(8, 1, true);
  packetView.setUint16(10, 64, true);
  packetView.setBigUint64(16, BigInt(packet.byteLength), true);
  packetView.setBigUint64(24, 64n, true);
  packetView.setUint32(32, recordBytes.length, true);
  for (const [index, bytes] of recordBytes.entries()) {
    const entry = 64 + index * 32;
    packetView.setUint16(entry, 101 + index, true);
    packetView.setUint16(entry + 2, 1, true);
    packetView.setUint32(entry + 4, bytes, true);
    packetView.setBigUint64(entry + 8, BigInt(packet.byteLength), true);
  }
  return packet;
}

function compactExpansionPacket(count, uses = [[2, 0, 24, 1]]) {
  const recordBytes = [48, 56, 32, 48, 32, 4, 24, 8, 8, 32, 48, 32, 32, 4];
  const counts = recordBytes.map(() => 0);
  for (const [table, record] of uses) counts[table] = Math.max(counts[table], record + 1);
  counts[8] = 1;
  let cursor = 64 + 32 * recordBytes.length;
  const offsets = [];
  for (const [index, bytes] of recordBytes.entries()) {
    offsets.push(cursor);
    cursor += counts[index] * bytes;
    if (index + 1 !== recordBytes.length) cursor = (cursor + 7) & ~7;
  }
  const packet = new Uint8Array(cursor);
  const view = new DataView(packet.buffer);
  packet.set(new TextEncoder().encode("GMABRS01"));
  view.setUint16(8, 1, true);
  view.setUint16(10, 64, true);
  view.setBigUint64(16, BigInt(packet.byteLength), true);
  view.setBigUint64(24, 64n, true);
  view.setUint32(32, recordBytes.length, true);
  view.setBigUint64(
    48,
    BigInt(recordBytes.reduce((total, bytes, index) => total + counts[index] * bytes, 0)),
    true,
  );
  for (const [index, bytes] of recordBytes.entries()) {
    const entry = 64 + index * 32;
    view.setUint16(entry, 101 + index, true);
    view.setUint16(entry + 2, 1, true);
    view.setUint32(entry + 4, bytes, true);
    view.setBigUint64(entry + 8, BigInt(offsets[index]), true);
    view.setBigUint64(entry + 16, BigInt(counts[index] * bytes), true);
    view.setBigUint64(entry + 24, BigInt(counts[index]), true);
  }
  for (const [table, record, fieldOffset, handle] of uses)
    view.setUint32(offsets[table] + record * recordBytes[table] + fieldOffset, handle, true);
  view.setUint32(offsets[8] + 4, count, true);
  return packet;
}

const resultStorage = new Uint8Array(7 + emptyResultPacket().byteLength);
resultStorage.set(emptyResultPacket(), 7);
const decoded = await decodeAnalyticPlanarBooleanBatchResultA0Packet(resultStorage.subarray(7));
requireValue(decoded.job_results.length === 0, "Empty result decoded unexpected jobs.");
requireValue(
  decoded.relationship_results.length === 0,
  "Empty result decoded unexpected relationships.",
);

const malformed = emptyResultPacket();
malformed[12] = 1;
let malformedRejected = false;
try {
  await decodeAnalyticPlanarBooleanBatchResultA0Packet(malformed);
} catch (error) {
  malformedRejected = error instanceof AnalyticPacketError;
}
requireValue(malformedRejected, "A nonzero reserved packet flag was accepted.");

for (const truncatedLength of [64, 95]) {
  let truncatedRejected = false;
  try {
    await decodeAnalyticPlanarBooleanBatchResultA0Packet(new Uint8Array(truncatedLength));
  } catch (error) {
    truncatedRejected = error instanceof AnalyticPacketError;
  }
  requireValue(truncatedRejected, `A ${truncatedLength}-byte packet escaped typed rejection.`);
}

const canonicalHex = readFileSync(
  new URL("../contracts/vectors/analytic/result-canonical-mixed.hex", import.meta.url),
  "ascii",
).trim();
const duplicateFragmentReference = Uint8Array.from(Buffer.from(canonicalHex, "hex"));
const duplicateView = new DataView(
  duplicateFragmentReference.buffer,
  duplicateFragmentReference.byteOffset,
  duplicateFragmentReference.byteLength,
);
const fragmentReferenceDirectory = 64 + 5 * 32;
const fragmentReferenceOffset = Number(
  duplicateView.getBigUint64(fragmentReferenceDirectory + 8, true),
);
duplicateView.setUint32(
  fragmentReferenceOffset + 4,
  duplicateView.getUint32(fragmentReferenceOffset, true),
  true,
);
let duplicateFragmentRejected = false;
try {
  await decodeAnalyticPlanarBooleanBatchResultA0Packet(duplicateFragmentReference);
} catch (error) {
  duplicateFragmentRejected = error instanceof AnalyticPacketError;
}
requireValue(
  duplicateFragmentRejected,
  "A directed fragment referenced more than once was accepted.",
);

const authoritativeLogicalExpansionLimit = 1_048_576;
const logicalSourceUseSlots = [
  [2, 0, 24, 1],
  [3, 0, 32, 1],
  [3, 0, 36, 1],
  [6, 0, 12, 1],
  [10, 0, 20, 1],
];
for (const useSlot of logicalSourceUseSlots) {
  let exactExpansionMessage = "";
  try {
    await decodeAnalyticPlanarBooleanBatchResultA0Packet(
      compactExpansionPacket(authoritativeLogicalExpansionLimit, [useSlot]),
    );
  } catch (error) {
    if (error instanceof AnalyticPacketError) exactExpansionMessage = error.message;
  }
  requireValue(
    exactExpansionMessage !== "" && !exactExpansionMessage.includes("expansion limit"),
    "A logical source-set use failed the exact independent expansion limit.",
  );
  let excessiveExpansionMessage = "";
  try {
    await decodeAnalyticPlanarBooleanBatchResultA0Packet(
      compactExpansionPacket(authoritativeLogicalExpansionLimit + 1, [useSlot]),
    );
  } catch (error) {
    if (error instanceof AnalyticPacketError) excessiveExpansionMessage = error.message;
  }
  requireValue(
    excessiveExpansionMessage.includes("expansion limit exceeded"),
    `Logical source-set use ${useSlot.join(":")} accepted the independent expansion limit plus one (${excessiveExpansionMessage}).`,
  );
}

const repeatedSourceSetUses = [logicalSourceUseSlots[0], logicalSourceUseSlots[4]];
let repeatedExactMessage = "";
try {
  await decodeAnalyticPlanarBooleanBatchResultA0Packet(
    compactExpansionPacket(authoritativeLogicalExpansionLimit / 2, repeatedSourceSetUses),
  );
} catch (error) {
  if (error instanceof AnalyticPacketError) repeatedExactMessage = error.message;
}
requireValue(
  repeatedExactMessage !== "" && !repeatedExactMessage.includes("expansion limit"),
  "Repeated source-set handles were not charged to the exact aggregate limit.",
);
let repeatedExcessiveMessage = "";
try {
  await decodeAnalyticPlanarBooleanBatchResultA0Packet(
    compactExpansionPacket(authoritativeLogicalExpansionLimit / 2 + 1, repeatedSourceSetUses),
  );
} catch (error) {
  if (error instanceof AnalyticPacketError) repeatedExcessiveMessage = error.message;
}
requireValue(
  repeatedExcessiveMessage.includes("expansion limit exceeded"),
  "Repeated source-set handles were deduplicated during expansion accounting.",
);

let invalidExpansionHandleMessage = "";
try {
  await decodeAnalyticPlanarBooleanBatchResultA0Packet(compactExpansionPacket(1, [[2, 0, 24, 2]]));
} catch (error) {
  if (error instanceof AnalyticPacketError) invalidExpansionHandleMessage = error.message;
}
requireValue(
  invalidExpansionHandleMessage.includes("handle is out of range"),
  "An invalid nonzero logical source-set handle escaped typed rejection.",
);

console.log(JSON.stringify({ emptyBytes: empty.byteLength, nonemptyBytes: encoded.byteLength }));
