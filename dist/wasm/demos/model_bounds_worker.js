"use strict";
const workerScope = globalThis;
importScripts("/dist/wasm/browser/geometer.js");
void import("/dist/wasm/npm/geometer/worker-host.js")
    .then(({ startGeometerWorkerHost }) => {
    startGeometerWorkerHost(createGeometerModule, workerScope);
    globalThis.postMessage({
        kind: "ready",
        protocol: "wn.geometer.worker_bootstrap.a0",
    });
})
    .catch((error) => {
    globalThis.postMessage({
        kind: "error",
        message: error instanceof Error ? error.message : String(error),
        protocol: "wn.geometer.worker_bootstrap.a0",
    });
});
