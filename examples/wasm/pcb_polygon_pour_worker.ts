declare function importScripts(...urls: string[]): void;

const pcbWorkerScope =
  globalThis as unknown as import("@wavenumber/geometer/worker-host").GeometerWorkerScope;

importScripts("/dist/wasm/browser/geometer.js");

const pcbModuleFactory = (
  globalThis as unknown as {
    createGeometerModule: import("@wavenumber/geometer").EmscriptenGeometerFactory;
  }
).createGeometerModule;

void import("/dist/wasm/npm/geometer/worker-host.js")
  .then(({ startGeometerWorkerHost }) => {
    startGeometerWorkerHost(pcbModuleFactory, pcbWorkerScope);
    globalThis.postMessage({ kind: "ready", protocol: "wn.geometer.worker_bootstrap.a0" });
  })
  .catch((error: unknown) => {
    globalThis.postMessage({
      kind: "error",
      message: error instanceof Error ? error.message : String(error),
      protocol: "wn.geometer.worker_bootstrap.a0",
    });
  });
