"use strict";
const analyticWorkerScope = globalThis;
importScripts("./geometer.js");
const analyticModuleFactory = globalThis.createGeometerModule;
void import("./package/worker-host.js")
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
