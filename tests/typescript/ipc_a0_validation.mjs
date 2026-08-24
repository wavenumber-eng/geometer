import assert from "node:assert/strict";

import {
  encodeGeometerIpcFrame,
  GeometerIpcFrameDecoder,
  GeometerIpcProtocolError,
  validateIpcOutcomeOperationPair,
  validateIpcRequestOperationPair,
} from "../../dist/wasm/npm/geometer/ipc-a0.js";

const shutdown = encodeGeometerIpcFrame({ attachments: [], json: "{}", kind: 8, requestId: 0n });
assert.equal(shutdown.byteLength, 50);
assert.equal(
  Buffer.from(shutdown).toString("hex"),
  "474d495043413031300000000800000000000000000000000200000000000000000000000000000000000000000000007b7d",
);

const request = encodeGeometerIpcFrame({
  attachments: [{ data: new Uint8Array([1, 2, 3]), mediaType: "application/step", name: "step" }],
  json: '{"operation":"geometry.step_topology.open.a0","request":{"schema":"geometry.step_topology.open.request.a0"}}',
  kind: 3,
  requestId: 42n,
});
const decoder = new GeometerIpcFrameDecoder();
const decoded = [];
for (let offset = 0; offset < request.byteLength; offset += 7) {
  decoded.push(...decoder.push(request.subarray(offset, Math.min(offset + 7, request.byteLength))));
}
decoder.finish();
assert.equal(decoded.length, 1);
assert.equal(decoded[0].requestId, 42n);
assert.equal(decoded[0].attachments[0].name, "step");
assert.deepEqual([...decoded[0].attachments[0].data], [1, 2, 3]);

const bomNameFrame = encodeGeometerIpcFrame({
  attachments: [{ data: new Uint8Array(), mediaType: "application/step", name: "\uFEFFstep" }],
  json: "{}",
  kind: 3,
  requestId: 44n,
});
assert.equal(new GeometerIpcFrameDecoder().push(bomNameFrame)[0].attachments[0].name, "\uFEFFstep");

assert.throws(
  () => encodeGeometerIpcFrame({ attachments: [], json: "{}", kind: 3, requestId: 0n }),
  GeometerIpcProtocolError,
);
const protocolError = encodeGeometerIpcFrame({
  attachments: [],
  json: '{"code":"geometer.transport.protocol_error","message":"bad frame"}',
  kind: 10,
  requestId: 0n,
});
assert.equal(new GeometerIpcFrameDecoder().push(protocolError)[0].requestId, 0n);
assert.throws(
  () =>
    encodeGeometerIpcFrame({
      attachments: [
        { data: new Uint8Array(), mediaType: "application/step", name: "step" },
        { data: new Uint8Array(), mediaType: "application/step", name: "step" },
      ],
      json: "{}",
      kind: 3,
      requestId: 1n,
    }),
  /Duplicate IPC attachment/,
);
const truncated = new GeometerIpcFrameDecoder();
truncated.push(request.subarray(0, request.byteLength - 1));
assert.throws(() => truncated.finish(), /ended within a frame/);
const concatenated = new GeometerIpcFrameDecoder();
const pair = new Uint8Array(shutdown.byteLength * 2);
pair.set(shutdown);
pair.set(shutdown, shutdown.byteLength);
assert.equal(concatenated.push(pair).length, 2);
concatenated.finish();

const invalidMagic = shutdown.slice();
invalidMagic[0] = 0;
assert.throws(() => new GeometerIpcFrameDecoder().push(invalidMagic), /Invalid IPC magic/);

const invalidKindHeader = request.slice(0, 48);
new DataView(invalidKindHeader.buffer).setUint16(12, 0xffff, true);
new DataView(invalidKindHeader.buffer).setUint32(24, 8 * 1024 * 1024, true);
assert.throws(
  () => new GeometerIpcFrameDecoder().push(invalidKindHeader),
  /Unknown IPC frame kind/,
);
const impossibleShutdownHeader = shutdown.slice(0, 48);
const impossibleShutdownView = new DataView(impossibleShutdownHeader.buffer);
impossibleShutdownView.setUint32(28, 1, true);
impossibleShutdownView.setBigUint64(32, 500_000_000n, true);
assert.throws(
  () => new GeometerIpcFrameDecoder().push(impossibleShutdownHeader),
  /control frames cannot carry attachments/,
);
const impossibleAttachmentSection = request.slice(0, 48);
const impossibleAttachmentView = new DataView(impossibleAttachmentSection.buffer);
impossibleAttachmentView.setUint32(28, 0, true);
impossibleAttachmentView.setBigUint64(32, 500_000_000n, true);
assert.throws(
  () => new GeometerIpcFrameDecoder().push(impossibleAttachmentSection),
  /count and section size are inconsistent/,
);

const largeFrame = encodeGeometerIpcFrame({
  attachments: [
    {
      data: new Uint8Array(1024 * 1024),
      mediaType: "application/octet-stream",
      name: "payload",
    },
  ],
  json: "{}",
  kind: 3,
  requestId: 43n,
});
const largeDecoder = new GeometerIpcFrameDecoder();
let largeDecoded = [];
for (let offset = 0; offset < largeFrame.byteLength; offset += 13) {
  largeDecoded = largeDecoder.push(largeFrame.subarray(offset, offset + 13));
}
largeDecoder.finish();
assert.equal(largeDecoded.length, 1);
assert.equal(largeDecoded[0].attachments[0].data.byteLength, 1024 * 1024);

const forgedPackedTopologyRequest = {
  operation: "geometry.step_topology.open.a0",
  request: {
    packet: {
      attachment: "analytic_planar_boolean_request",
      format: "geometry.analytic_planar_boolean.packet.a0",
    },
    schema: "geometry.step_topology.open.request.a0",
  },
};
assert.throws(
  () => validateIpcRequestOperationPair(forgedPackedTopologyRequest),
  /logical DTO operation cannot carry a packed attachment projection/,
);
assert.throws(
  () =>
    validateIpcOutcomeOperationPair({
      ok: true,
      operation: "geometry.step_topology.open.a0",
      result: forgedPackedTopologyRequest.request,
    }),
  /logical DTO operation cannot carry a packed attachment projection/,
);
validateIpcRequestOperationPair({
  operation: "geometry.analytic_planar_boolean_batch.a0",
  request: {
    packet: {
      attachment: "analytic_planar_boolean_request",
      format: "geometry.analytic_planar_boolean.packet.a0",
    },
    schema: "geometry.analytic_planar_boolean_batch.request.a0",
  },
});

process.stdout.write(JSON.stringify({ frameBytes: request.byteLength, fragmented: true }));
