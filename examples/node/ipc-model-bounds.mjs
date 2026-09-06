import { readFile } from "node:fs/promises";
import { resolve } from "node:path";
import { GeometerNodeProcessA0 } from "../../dist/wasm/npm/geometer/node-process-a0.js";

const [executable, modelPath] = process.argv.slice(2);
if (!executable || !modelPath) {
  throw new Error("Usage: node examples/node/ipc-model-bounds.mjs <geometer executable> <STEP file>");
}
const model = new Uint8Array(await readFile(modelPath));
const processClient = await GeometerNodeProcessA0.spawn(resolve(executable), {
  clientName: "geometer-ipc-quick-start",
  clientVersion: "a0",
});
try {
  const operation = "geometry.model_bounds.a0";
  if (!processClient.client.welcome.operation_catalog.operations.some((item) => item.identity === operation)) {
    throw new Error(`Executable does not advertise ${operation}`);
  }
  const response = await processClient.client.execute(operation, {}, [
    { data: model, mediaType: "application/step", name: "model" },
  ]);
  if (!response.outcome.ok) {
    throw new Error(JSON.stringify(response.outcome.diagnostics));
  }
  process.stdout.write(`${JSON.stringify(response.outcome.result, null, 2)}\n`);
} finally {
  await processClient.close("quick start complete");
}
