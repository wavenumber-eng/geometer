"use strict";
const analyticWorkerScope = globalThis;
importScripts("/dist/wasm/browser/geometer.js");
const analyticModuleFactory = globalThis.createGeometerModule;
void import("/dist/wasm/npm/geometer/worker-host.js")
    .then(({ startGeometerWorkerHost }) => {
    startGeometerWorkerHost(analyticModuleFactory, analyticWorkerScope);
    globalThis.postMessage({ kind: "ready", protocol: "wn.geometer.worker_bootstrap.a0" });
})
    .catch((error) => {
    globalThis.postMessage({
        kind: "error",
        message: error instanceof Error ? error.message : String(error),
        protocol: "wn.geometer.worker_bootstrap.a0",
    });
});
