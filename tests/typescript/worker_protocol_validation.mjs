import { encodeOperationOutcomeA0Json } from "../../dist/wasm/npm/geometer/generated/index.js";
import {
  createGeometerWorkerClient,
  GEOMETER_WASM_WORKER_PROTOCOL,
  GeometerWorkerError,
} from "../../dist/wasm/npm/geometer/worker.js";

const capabilities = Object.freeze({
  cAbiGeneration: 20260903,
  genericAbi: "a0",
  operations: Object.freeze([
    "geometry.analytic_planar_boolean_batch.a0",
    "geometry.model_bounds.a0",
  ]),
  releaseVersion: "2026.9.3",
});
const outcomeJson = encodeOperationOutcomeA0Json({
  diagnostics: [
    {
      category: "contract",
      code: "geometer.contract.synthetic_failure",
      message: "Synthetic protocol-test outcome.",
      retryable: false,
    },
  ],
  ok: false,
  operation: "geometry.model_bounds.a0",
});
const analyticOutcome = (schema = "geometry.analytic_planar_boolean_batch.result.a0") =>
  encodeOperationOutcomeA0Json({
    ok: true,
    operation: "geometry.analytic_planar_boolean_batch.a0",
    result: {
      schema,
      packet: {
        attachment: "analytic_planar_boolean_result",
        format: "geometry.analytic_planar_boolean.packet.a0",
      },
    },
  });

class ManualWorker {
  constructor() {
    this.listeners = new Map();
    this.messages = [];
    this.terminated = false;
  }

  addEventListener(type, listener) {
    const listeners = this.listeners.get(type) ?? new Set();
    listeners.add(listener);
    this.listeners.set(type, listeners);
  }

  removeEventListener(type, listener) {
    this.listeners.get(type)?.delete(listener);
  }

  postMessage(message) {
    this.messages.push(message);
  }

  terminate() {
    this.terminated = true;
  }

  respond(message) {
    for (const listener of this.listeners.get("message") ?? []) listener({ data: message });
  }
}

async function createManualClient() {
  const worker = new ManualWorker();
  const clientPromise = createGeometerWorkerClient(worker, { wasmBinary: new ArrayBuffer(8) });
  await waitFor(() => worker.messages.length === 1, "initialization request");
  const request = worker.messages[0];
  worker.respond({
    capabilities,
    kind: "ready",
    protocol: GEOMETER_WASM_WORKER_PROTOCOL,
    requestId: request.requestId,
  });
  return { client: await clientPromise, worker };
}

async function expectPromptWorkerRejection(promise, label) {
  let timeout;
  try {
    await Promise.race([
      promise,
      new Promise((_, reject) => {
        timeout = setTimeout(() => reject(new Error(`${label} remained pending.`)), 500);
      }),
    ]);
  } catch (error) {
    if (!(error instanceof GeometerWorkerError)) throw error;
    return error;
  } finally {
    clearTimeout(timeout);
  }
  throw new Error(`${label} unexpectedly resolved.`);
}

async function waitFor(predicate, label) {
  const deadline = Date.now() + 500;
  while (Date.now() < deadline) {
    if (predicate()) return;
    await new Promise((resolve) => setTimeout(resolve, 0));
  }
  throw new Error(`Timed out waiting for ${label}.`);
}

{
  const { client, worker } = await createManualClient();
  const pending = client.execute("geometry.model_bounds.a0", "{}", []);
  await waitFor(() => worker.messages.length === 2, "unknown-ID request");
  worker.respond({
    attachments: [],
    kind: "operation_result",
    outcomeJson,
    protocol: GEOMETER_WASM_WORKER_PROTOCOL,
    requestId: "geometer-worker-unknown",
  });
  const error = await expectPromptWorkerRejection(pending, "unknown-ID request");
  if (!error.message.includes("unknown or completed request ID") || !worker.terminated) {
    throw new Error("Unknown response ID did not terminate the Worker connection.");
  }
}

{
  const { client, worker } = await createManualClient();
  const pending = client.execute("geometry.analytic_planar_boolean_batch.a0", "{}", []);
  await waitFor(() => worker.messages.length === 2, "duplicate-attachment request");
  const request = worker.messages[1];
  const attachment = {
    data: new ArrayBuffer(8),
    mediaType: "application/vnd.wavenumber.geometer.analytic-planar-boolean-result",
    name: "analytic_planar_boolean_result",
  };
  worker.respond({
    attachments: [attachment, attachment],
    kind: "operation_result",
    outcomeJson: analyticOutcome(),
    protocol: GEOMETER_WASM_WORKER_PROTOCOL,
    requestId: request.requestId,
  });
  const error = await expectPromptWorkerRejection(pending, "duplicate-attachment request");
  if (!error.message.includes("duplicate attachment"))
    throw new Error("Worker duplicate output attachment was not rejected.");
}

{
  const { client, worker } = await createManualClient();
  const pending = client.execute("geometry.analytic_planar_boolean_batch.a0", "{}", []);
  await waitFor(() => worker.messages.length === 2, "wrong-projection request");
  const request = worker.messages[1];
  worker.respond({
    attachments: [
      {
        data: new ArrayBuffer(8),
        mediaType: "application/vnd.wavenumber.geometer.analytic-planar-boolean-result",
        name: "analytic_planar_boolean_result",
      },
    ],
    kind: "operation_result",
    outcomeJson: analyticOutcome("geometry.wrong.result.a0"),
    protocol: GEOMETER_WASM_WORKER_PROTOCOL,
    requestId: request.requestId,
  });
  const error = await expectPromptWorkerRejection(pending, "wrong-projection request");
  if (!error.message.includes("incompatible packed result projection"))
    throw new Error("Worker packed result projection mismatch was not rejected.");
}

{
  const { client, worker } = await createManualClient();
  const completed = client.execute("geometry.model_bounds.a0", "{}", []);
  const pending = client.execute("geometry.model_bounds.a0", "{}", []);
  await waitFor(() => worker.messages.length === 3, "duplicate-ID requests");
  const completedRequest = worker.messages[1];
  const response = {
    attachments: [],
    kind: "operation_result",
    outcomeJson,
    protocol: GEOMETER_WASM_WORKER_PROTOCOL,
    requestId: completedRequest.requestId,
  };
  worker.respond(response);
  await completed;
  worker.respond(response);
  const error = await expectPromptWorkerRejection(pending, "duplicate-ID sibling request");
  if (!error.message.includes("unknown or completed request ID") || !worker.terminated) {
    throw new Error("Duplicate response ID did not terminate the Worker connection.");
  }
}

{
  const { client, worker } = await createManualClient();
  const first = client.execute("geometry.model_bounds.a0", "{}", []);
  const second = client.execute("geometry.model_bounds.a0", "{}", []);
  await waitFor(() => worker.messages.length === 3, "wrong-kind requests");
  worker.respond({
    capabilities,
    kind: "ready",
    protocol: GEOMETER_WASM_WORKER_PROTOCOL,
    requestId: worker.messages[1].requestId,
  });
  const errors = await Promise.all([
    expectPromptWorkerRejection(first, "wrong-kind request"),
    expectPromptWorkerRejection(second, "wrong-kind sibling request"),
  ]);
  if (!errors[0].message.includes("expected operation_result") || !worker.terminated) {
    throw new Error("Wrong response kind did not terminate the Worker connection.");
  }
}

{
  const { client, worker } = await createManualClient();
  const first = client.execute("geometry.model_bounds.a0", "{}", []);
  const second = client.execute("geometry.model_bounds.a0", "{}", []);
  await waitFor(() => worker.messages.length === 3, "termination requests");
  client.terminate();
  await Promise.all([
    expectPromptWorkerRejection(first, "first terminated request"),
    expectPromptWorkerRejection(second, "second terminated request"),
  ]);
  if (!worker.terminated) throw new Error("Immediate termination did not terminate the Worker.");
}

console.log(
  JSON.stringify({
    duplicateId: "rejected",
    immediateTermination: 2,
    unknownId: "rejected",
    wrongKind: "rejected",
  }),
);
