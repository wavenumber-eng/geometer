const STARTUP_TIMEOUT_MS = 12_000;
var GEOMETER_STANDALONE_BUILD = false;

window.addEventListener("DOMContentLoaded", () => {
  const shell = document.querySelector<HTMLElement>("#pour-shell");
  const runtime = document.querySelector<HTMLElement>("#runtime-label");
  const status = document.querySelector<HTMLElement>("#status");
  if (shell === null || runtime === null || status === null) return;

  if (window.location.protocol === "file:") {
    shell.dataset.state = "error";
    runtime.textContent = "HTTP server required";
    status.textContent =
      "This Worker/WASM demo cannot run from file://. Serve this directory over HTTP.";
    return;
  }

  window.setTimeout(() => {
    if (shell.dataset.state === "ready" || shell.dataset.state === "error") return;
    shell.dataset.state = "error";
    runtime.textContent = "Kernel startup stalled";
    status.textContent =
      "Verify that this page is served from the analytic-polygon-pour static directory and reload.";
  }, STARTUP_TIMEOUT_MS);
});
