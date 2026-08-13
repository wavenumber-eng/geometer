import type { EmscriptenGeometerFactory, ModelBoundsResultA0 } from "@wavenumber/geometer";
import { createGeometerWasmClient } from "@wavenumber/geometer/wasm";

declare const createGeometerModule: EmscriptenGeometerFactory;

const runButton = requireElement<HTMLButtonElement>("run-bounds");
const status = requireElement<HTMLElement>("status");
const results = requireElement<HTMLElement>("bounds-results");
const wasmBinaryPromise = fetch("/dist/wasm/browser/geometer.wasm").then(async (response) => {
  if (!response.ok) throw new Error(`Unable to load geometer.wasm (${response.status}).`);
  return response.arrayBuffer();
});
const modelPromise = fetch("/tests/fixtures/step/embedded_models/SOT-23.STEP").then(
  async (response) => {
    if (!response.ok) throw new Error(`Unable to load SOT-23 fixture (${response.status}).`);
    return new Uint8Array(await response.arrayBuffer());
  },
);

runButton.addEventListener("click", () => void run());
void run();

async function run(): Promise<void> {
  runButton.disabled = true;
  results.dataset.state = "loading";
  status.textContent = "Loading generated client and browser kernel…";
  try {
    const [wasmBinary, model] = await Promise.all([wasmBinaryPromise, modelPromise]);
    const client = await createGeometerWasmClient(createGeometerModule, { wasmBinary });
    status.textContent = `Computing through ${client.capabilities.genericAbi.toUpperCase()} generic ABI…`;
    const bounds = await client.modelBounds({ model });
    renderBounds(bounds);
    status.textContent = `Complete · Geometer ${client.capabilities.releaseVersion} · ${bounds.source.hash.slice(0, 12)}…`;
    results.dataset.state = "complete";
  } catch (error) {
    status.textContent = error instanceof Error ? error.message : String(error);
    results.dataset.state = "error";
  } finally {
    runButton.disabled = false;
  }
}

function renderBounds(result: ModelBoundsResultA0): void {
  const values = {
    min: result.bounds.min,
    max: result.bounds.max,
    size: result.bounds.size,
    center: result.bounds.center,
  } as const;
  for (const [name, vector] of Object.entries(values)) {
    const target = requireElement<HTMLElement>(`value-${name}`);
    target.textContent = vector.map((value) => value.toFixed(3)).join("  ");
  }
  const maximum = Math.max(...result.bounds.size, 0.001);
  const axes = [
    { name: "x", value: result.bounds.size[0] },
    { name: "y", value: result.bounds.size[1] },
    { name: "z", value: result.bounds.size[2] },
  ] as const;
  for (const axis of axes) {
    const bar = requireElement<HTMLElement>(`bar-${axis.name}`);
    bar.style.setProperty("--axis-scale", String(axis.value / maximum));
    requireElement<HTMLElement>(`axis-${axis.name}`).textContent = `${axis.value.toFixed(3)} mm`;
  }
  requireElement<HTMLElement>("timing").textContent =
    `${(result.timings.model_read_ms + result.timings.bounds_ms).toFixed(2)} ms kernel time`;
}

function requireElement<T extends HTMLElement>(id: string): T {
  const element = document.getElementById(id);
  if (!element) throw new Error(`Missing demo element #${id}.`);
  return element as T;
}
