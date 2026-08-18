"use strict";
const pcbWorkerScope = globalThis;
importScripts("./geometer.js");
const pcbModuleFactory = globalThis.createGeometerModule;
void import("./package/worker-host.js")
    .then(({ startGeometerWorkerHost }) => {
    startGeometerWorkerHost(pcbModuleFactory, pcbWorkerScope);
    globalThis.postMessage({ kind: "ready", protocol: "wn.geometer.worker_bootstrap.a0" });
})
    .catch((error) => {
    globalThis.postMessage({
        kind: "error",
        message: error instanceof Error ? error.message : String(error),
        protocol: "wn.geometer.worker_bootstrap.a0",
    });
});
