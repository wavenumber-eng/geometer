import assert from "node:assert/strict";

import {
  encodeIpcCancelledA0Json,
  encodeIpcCancelRejectedA0Json,
  encodeIpcReasonA0Json,
  encodeIpcRequestA0Json,
  encodeIpcShutdownAckA0Json,
  encodeIpcWelcomeA0Json,
  encodeOperationOutcomeA0Json,
  NORMALIZED_CONTRACT_CATALOG_SHA256,
  operationCatalog,
} from "../../dist/wasm/npm/geometer/generated/index.js";
import {
  encodeGeometerIpcFrame,
  GeometerIpcFrameDecoder,
} from "../../dist/wasm/npm/geometer/ipc-a0.js";
import {
  GeometerIpcClientA0,
  GeometerIpcClientError,
} from "../../dist/wasm/npm/geometer/ipc-client-a0.js";

const runtimeOperations = Object.values(operationCatalog)
  .filter((item) => item.runtimeAvailable)
  .map((item) => ({
    identity: item.identity,
    input_attachments: item.inputAttachments,
    output_attachments: item.outputAttachments,
    request_contract: item.requestContract,
    ...("requestProjection" in item ? { request_projection: item.requestProjection } : {}),
    result_contract: item.resultContract,
    ...("resultProjection" in item ? { result_projection: item.resultProjection } : {}),
    runtime_dispatch: item.runtimeDispatch,
  }));

function welcome(catalogSha256 = NORMALIZED_CONTRACT_CATALOG_SHA256, limitOverrides = {}) {
  return {
    c_abi_generation: 20260821,
    capabilities: ["serialized_execution", "queue_only_cancellation", "raw_attachments"],
    catalog_sha256: catalogSha256,
    ipc: "a0",
    limits: {
      attachment_bytes: 256 * 1024 * 1024,
      attachment_count: 16,
      attachment_media_type_bytes: 128,
      attachment_name_bytes: 128,
      frame_bytes: 512 * 1024 * 1024,
      json_bytes: 8 * 1024 * 1024,
      pending_writer_bytes: 512 * 1024 * 1024,
      queued_bytes: 512 * 1024 * 1024,
      queued_requests: 8,
      resident_request_bytes: 512 * 1024 * 1024,
      ...limitOverrides,
    },
    operation_catalog: {
      attachment_descriptor: {
        pointer64: {
          offsets: {
            data: 40,
            data_size: 48,
            flags: 4,
            media_type: 24,
            media_type_size: 32,
            name: 8,
            name_size: 16,
            reserved0: 52,
            struct_size: 0,
          },
          size: 56,
        },
        wasm32: {
          offsets: {
            data: 24,
            data_size: 28,
            flags: 4,
            media_type: 16,
            media_type_size: 20,
            name: 8,
            name_size: 12,
            reserved0: 32,
            struct_size: 0,
          },
          size: 36,
        },
      },
      c_abi_generation: 20260821,
      catalog: "wn.geometer.operation_catalog.a0",
      generic_abi: "a0",
      limits: {
        aggregate_attachment_bytes_native: 512 * 1024 * 1024,
        aggregate_attachment_bytes_wasm: 256 * 1024 * 1024,
        attachment_bytes: 256 * 1024 * 1024,
        attachment_count: 16,
        attachment_media_type_bytes: 128,
        attachment_name_bytes: 128,
        operation_id_bytes: 128,
        request_json_bytes: 8 * 1024 * 1024,
        response_json_bytes: 8 * 1024 * 1024,
      },
      operations: runtimeOperations,
      release_version: "2026.8.21",
    },
    release_version: "2026.8.21",
  };
}

function fakeDuplex(catalogSha256, responseRequestIdOffset = 0n, limitOverrides = {}) {
  let controller;
  let terminated = false;
  const decoder = new GeometerIpcFrameDecoder();
  const readable = new ReadableStream({
    start(value) {
      controller = value;
    },
  });
  const send = (kind, requestId, json) => {
    controller.enqueue(encodeGeometerIpcFrame({ attachments: [], json, kind, requestId }));
  };
  const writable = new WritableStream({
    write(chunk) {
      for (const frame of decoder.push(chunk)) {
        if (frame.kind === 1) {
          send(2, 0n, encodeIpcWelcomeA0Json(welcome(catalogSha256, limitOverrides)));
        } else if (frame.kind === 3) {
          const envelope = JSON.parse(frame.json);
          send(
            4,
            frame.requestId + responseRequestIdOffset,
            encodeOperationOutcomeA0Json({
              diagnostics: [
                {
                  category: "operation",
                  code: "geometer.operation.test_failure",
                  message: "expected fake failure",
                  operation: envelope.operation,
                  retryable: false,
                },
              ],
              ok: false,
              operation: envelope.operation,
            }),
          );
        } else if (frame.kind === 8) {
          send(
            9,
            0n,
            encodeIpcShutdownAckA0Json({
              activeRequestCompleted: true,
              rejectedQueuedRequestCount: 0,
              status: "complete",
            }),
          );
          controller.close();
        }
      }
    },
  });
  return {
    duplex: {
      readable,
      terminate(reason) {
        terminated = true;
        try {
          controller.error(reason);
        } catch {}
      },
      writable,
    },
    wasTerminated: () => terminated,
  };
}

const fake = fakeDuplex();
const client = await GeometerIpcClientA0.connect(fake.duplex, {
  clientName: "ipc-client-test",
  clientVersion: "a0",
});
assert.equal(client.welcome.catalog_sha256, NORMALIZED_CONTRACT_CATALOG_SHA256);
assert.throws(
  () =>
    client.start("geometry.step_topology.open.a0", {
      schema: "geometry.step_topology.open.request.a0",
    }),
  /structural-only and absent from the negotiated runtime catalog/,
);
const response = await client.execute("geometry.model_bounds.a0", {}, [
  { data: new Uint8Array([1, 2, 3]), mediaType: "application/step", name: "model" },
]);
assert.equal(response.outcome.ok, false);
assert.equal(response.outcome.operation, "geometry.model_bounds.a0");
assert.equal((await client.close()).status, "complete");
assert.equal(fake.wasTerminated(), false);

const drifted = fakeDuplex("0".repeat(64));
await assert.rejects(
  GeometerIpcClientA0.connect(drifted.duplex, {
    clientName: "ipc-client-drift-test",
    clientVersion: "a0",
  }),
  /unsupported contract catalog/,
);
assert.equal(drifted.wasTerminated(), true);

const corrupt = fakeDuplex(undefined, 1n);
const corruptClient = await GeometerIpcClientA0.connect(corrupt.duplex, {
  clientName: "ipc-client-correlation-test",
  clientVersion: "a0",
});
await assert.rejects(
  corruptClient.execute("geometry.model_bounds.a0", {}, [
    { data: new Uint8Array([1, 2, 3]), mediaType: "application/step", name: "model" },
  ]),
  /unknown request id/,
);
assert.equal(corrupt.wasTerminated(), true);

function controlledDuplex(mode, limitOverrides = {}) {
  let controller;
  let terminated = false;
  const decoder = new GeometerIpcFrameDecoder();
  const readable = new ReadableStream({
    start(value) {
      controller = value;
    },
  });
  const send = (kind, requestId, json) => {
    controller.enqueue(encodeGeometerIpcFrame({ attachments: [], json, kind, requestId }));
  };
  const sendFailure = (frame) => {
    const envelope = JSON.parse(frame.json);
    send(
      4,
      frame.requestId,
      encodeOperationOutcomeA0Json({
        diagnostics: [
          {
            category: "operation",
            code: "geometer.operation.test_failure",
            message: "expected controlled failure",
            operation: envelope.operation,
            retryable: false,
          },
        ],
        ok: false,
        operation: envelope.operation,
      }),
    );
  };
  const requests = new Map();
  const writable = new WritableStream({
    write(chunk) {
      for (const frame of decoder.push(chunk)) {
        if (frame.kind === 1) {
          send(2, 0n, encodeIpcWelcomeA0Json(welcome(undefined, limitOverrides)));
        } else if (frame.kind === 3) {
          requests.set(frame.requestId, frame);
          if (mode === "unsolicited_cancelled") {
            send(6, frame.requestId, encodeIpcCancelledA0Json({ status: "cancelled" }));
          } else if (mode === "oversized_response") {
            const envelope = JSON.parse(frame.json);
            send(
              4,
              frame.requestId,
              encodeOperationOutcomeA0Json({
                diagnostics: [
                  {
                    category: "operation",
                    code: "geometer.operation.test_failure",
                    message: "x".repeat(512),
                    operation: envelope.operation,
                    retryable: false,
                  },
                ],
                ok: false,
                operation: envelope.operation,
              }),
            );
          }
        } else if (frame.kind === 5) {
          const request = requests.get(frame.requestId);
          assert.ok(request);
          if (mode === "cancelled") {
            send(6, frame.requestId, encodeIpcCancelledA0Json({ status: "cancelled" }));
          } else {
            if (mode === "response_cancel_race") sendFailure(request);
            send(
              7,
              frame.requestId,
              encodeIpcCancelRejectedA0Json({
                diagnostic: {
                  category: "transport",
                  code: "geometer.transport.not_cancellable",
                  message: "request is active",
                  operation: "geometry.model_bounds.a0",
                  request_id: String(frame.requestId),
                  retryable: false,
                },
                status: "rejected",
              }),
            );
            if (mode === "rejected") sendFailure(request);
          }
        }
      }
    },
  });
  return {
    duplex: {
      readable,
      terminate(reason) {
        terminated = true;
        try {
          controller.error(reason);
        } catch {}
      },
      writable,
    },
    wasTerminated: () => terminated,
  };
}

const modelAttachment = {
  data: new Uint8Array([1, 2, 3]),
  mediaType: "application/step",
  name: "model",
};
for (const mode of ["cancelled", "rejected", "response_cancel_race"]) {
  const controlled = controlledDuplex(mode);
  const controlledClient = await GeometerIpcClientA0.connect(controlled.duplex, {
    clientName: `ipc-client-${mode}-test`,
    clientVersion: "a0",
  });
  const call = controlledClient.start("geometry.model_bounds.a0", {}, [modelAttachment]);
  const cancellation = call.cancel("test cancellation");
  if (mode === "cancelled") {
    await assert.rejects(call.response, /was cancelled/);
    assert.equal(await cancellation, "cancelled");
  } else {
    assert.equal(await cancellation, "rejected");
    assert.equal((await call.response).outcome.ok, false);
  }
  controlledClient.terminate();
}

const unsolicited = controlledDuplex("unsolicited_cancelled");
const unsolicitedClient = await GeometerIpcClientA0.connect(unsolicited.duplex, {
  clientName: "ipc-client-unsolicited-cancel-test",
  clientVersion: "a0",
});
await assert.rejects(
  unsolicitedClient.execute("geometry.model_bounds.a0", {}, [modelAttachment]),
  /without a pending cancellation/,
);
assert.equal(unsolicited.wasTerminated(), true);

const oversized = controlledDuplex("oversized_response", { json_bytes: 256 });
const oversizedClient = await GeometerIpcClientA0.connect(oversized.duplex, {
  clientName: "ipc-client-inbound-limit-test",
  clientVersion: "a0",
});
await assert.rejects(
  oversizedClient.execute("geometry.model_bounds.a0", {}, [modelAttachment]),
  /out-of-range payload/,
);
assert.equal(oversized.wasTerminated(), true);

const exactCancelReason = "x".repeat(100);
const exactCancelJsonBytes = new TextEncoder().encode(
  encodeIpcReasonA0Json({ reason: exactCancelReason }),
).byteLength;
const exactCancel = controlledDuplex("cancelled", { json_bytes: exactCancelJsonBytes });
const exactCancelClient = await GeometerIpcClientA0.connect(exactCancel.duplex, {
  clientName: "ipc-client-exact-cancel-limit-test",
  clientVersion: "a0",
});
const exactCancelCall = exactCancelClient.start("geometry.model_bounds.a0", {}, [modelAttachment]);
const exactCancellation = exactCancelCall.cancel(exactCancelReason);
await assert.rejects(exactCancelCall.response, /was cancelled/);
assert.equal(await exactCancellation, "cancelled");
exactCancelClient.terminate();

const overCancel = controlledDuplex("cancelled", { json_bytes: exactCancelJsonBytes });
const overCancelClient = await GeometerIpcClientA0.connect(overCancel.duplex, {
  clientName: "ipc-client-over-cancel-limit-test",
  clientVersion: "a0",
});
const overCancelCall = overCancelClient.start("geometry.model_bounds.a0", {}, [modelAttachment]);
void overCancelCall.response.catch(() => {});
await assert.rejects(overCancelCall.cancel(`${exactCancelReason}x`), /effective limit/);
assert.equal(overCancel.wasTerminated(), false);
overCancelClient.terminate();

const exactShutdownReason = "s".repeat(256);
const exactShutdownJson = encodeIpcReasonA0Json({ reason: exactShutdownReason });
const exactShutdownBytes = new TextEncoder().encode(exactShutdownJson).byteLength;
const exactShutdown = fakeDuplex(undefined, 0n, {
  frame_bytes: 48 + exactShutdownBytes,
  json_bytes: exactShutdownBytes,
});
const exactShutdownClient = await GeometerIpcClientA0.connect(exactShutdown.duplex, {
  clientName: "ipc-client-exact-shutdown-limit-test",
  clientVersion: "a0",
});
assert.equal((await exactShutdownClient.close(exactShutdownReason)).status, "complete");

const overShutdown = fakeDuplex(undefined, 0n, {
  frame_bytes: 48 + exactShutdownBytes,
  json_bytes: exactShutdownBytes,
});
const overShutdownClient = await GeometerIpcClientA0.connect(overShutdown.duplex, {
  clientName: "ipc-client-over-shutdown-limit-test",
  clientVersion: "a0",
});
await assert.rejects(overShutdownClient.close(`${exactShutdownReason}x`), /effective limit/);
assert.equal(overShutdown.wasTerminated(), false);
assert.equal((await overShutdownClient.close(exactShutdownReason)).status, "complete");

function stalledDuplex(limitOverrides, rejectCancel = false) {
  let controller;
  let writes = 0;
  let terminated = false;
  const decoder = new GeometerIpcFrameDecoder();
  const readable = new ReadableStream({
    start(value) {
      controller = value;
    },
  });
  const writable = new WritableStream({
    write(chunk) {
      writes += 1;
      if (rejectCancel && writes === 3) throw new Error("cancel write failed");
      for (const frame of decoder.push(chunk)) {
        if (frame.kind === 1) {
          controller.enqueue(
            encodeGeometerIpcFrame({
              attachments: [],
              json: encodeIpcWelcomeA0Json(welcome(undefined, limitOverrides)),
              kind: 2,
              requestId: 0n,
            }),
          );
        }
      }
      if (writes >= 2 && !rejectCancel) return new Promise(() => {});
    },
  });
  return {
    duplex: {
      readable,
      terminate(reason) {
        terminated = true;
        try {
          controller.error(reason);
        } catch {}
      },
      writable,
    },
    wasTerminated: () => terminated,
  };
}

const countBounded = stalledDuplex({ queued_requests: 2 });
const countClient = await GeometerIpcClientA0.connect(countBounded.duplex, {
  clientName: "ipc-client-count-limit-test",
  clientVersion: "a0",
});
const countCalls = [
  countClient.start("geometry.model_bounds.a0", {}, [modelAttachment]),
  countClient.start("geometry.model_bounds.a0", {}, [modelAttachment]),
];
for (const call of countCalls) void call.response.catch(() => {});
assert.throws(
  () => countClient.start("geometry.model_bounds.a0", {}, [modelAttachment]),
  /request-count limit/,
);
countClient.terminate();

const requestBytes = encodeGeometerIpcFrame({
  attachments: [modelAttachment],
  json: encodeIpcRequestA0Json({ operation: "geometry.model_bounds.a0", request: {} }),
  kind: 3,
  requestId: 1n,
}).byteLength;
const byteBounded = stalledDuplex({
  queued_bytes: requestBytes,
  resident_request_bytes: requestBytes,
});
const byteClient = await GeometerIpcClientA0.connect(byteBounded.duplex, {
  clientName: "ipc-client-byte-limit-test",
  clientVersion: "a0",
});
const byteCall = byteClient.start("geometry.model_bounds.a0", {}, [modelAttachment]);
void byteCall.response.catch(() => {});
assert.throws(
  () => byteClient.start("geometry.model_bounds.a0", {}, [modelAttachment]),
  /request-byte limit/,
);
byteClient.terminate();

const draining = stalledDuplex({});
const drainingClient = await GeometerIpcClientA0.connect(draining.duplex, {
  clientName: "ipc-client-draining-cancel-test",
  clientVersion: "a0",
});
const drainingCall = drainingClient.start("geometry.model_bounds.a0", {}, [modelAttachment]);
void drainingCall.response.catch(() => {});
const drainingClose = drainingClient.close();
void drainingClose.catch(() => {});
await assert.rejects(drainingCall.cancel(), /not accepting cancellation/);
drainingClient.terminate();

const cancelWriteFailure = stalledDuplex({}, true);
const cancelWriteClient = await GeometerIpcClientA0.connect(cancelWriteFailure.duplex, {
  clientName: "ipc-client-cancel-write-test",
  clientVersion: "a0",
});
const failedCancelCall = cancelWriteClient.start("geometry.model_bounds.a0", {}, [modelAttachment]);
const failedCancellation = failedCancelCall.cancel();
await Promise.all([
  assert.rejects(failedCancelCall.response, /cancel write failed/),
  assert.rejects(failedCancellation, /cancel write failed/),
]);
assert.equal(cancelWriteFailure.wasTerminated(), true);

let handshakeTerminated = false;
await assert.rejects(
  GeometerIpcClientA0.connect(
    {
      readable: new ReadableStream(),
      terminate() {
        handshakeTerminated = true;
      },
      writable: new WritableStream({
        write() {
          throw new Error("hello write failed");
        },
      }),
    },
    { clientName: "ipc-client-hello-write-test", clientVersion: "a0" },
  ),
  /hello write failed/,
);
assert.equal(handshakeTerminated, true);

assert.ok(new GeometerIpcClientError("typed") instanceof Error);
process.stdout.write(
  JSON.stringify({ handshake: true, runtimeOperations: runtimeOperations.length }),
);
