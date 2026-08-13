declare function importScripts(...urls: string[]): void;

declare const createGeometerModule: import("@wavenumber/geometer").EmscriptenGeometerFactory;

const workerScope =
  globalThis as unknown as import("@wavenumber/geometer/worker-host").GeometerWorkerScope;

importScripts("/dist/wasm/browser/geometer.js");

void import("/dist/npm/geometer/worker-host.js")
  .then(({ startGeometerWorkerHost }) => {
    startGeometerWorkerHost(createGeometerModule, workerScope);
    globalThis.postMessage({
      kind: "ready",
      protocol: "wn.geometer.worker_bootstrap.a0",
    });
  })
  .catch((error: unknown) => {
    globalThis.postMessage({
      kind: "error",
      message: error instanceof Error ? error.message : String(error),
      protocol: "wn.geometer.worker_bootstrap.a0",
    });
  });
