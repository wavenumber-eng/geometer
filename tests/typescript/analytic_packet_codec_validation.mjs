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

console.log(JSON.stringify({ emptyBytes: empty.byteLength, nonemptyBytes: encoded.byteLength }));
