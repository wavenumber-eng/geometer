from __future__ import annotations

import json
import os
import shutil
import socket
import subprocess
import tempfile
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]
DEMO = ROOT / "dist" / "wasm" / "demos" / "pcb_polygon_pour_demo.html"

CDP_SCRIPT = r"""
const http = require("http");
const port = Number(process.env.CDP_PORT);
const targetUrl = String(process.env.TEST_URL);
function getJson(path) { return new Promise((resolve, reject) => {
  http.get({ host: "127.0.0.1", port, path }, (response) => {
    let data = "";
    response.on("data", (chunk) => { data += chunk; });
    response.on("end", () => { try { resolve(JSON.parse(data)); } catch (error) { reject(error); } });
  }).on("error", reject);
}); }
async function waitForPage() {
  const deadline = Date.now() + 30000;
  while (Date.now() < deadline) {
    try {
      const pages = await getJson("/json/list");
      const page = pages.find((entry) => entry.type === "page" && entry.url === targetUrl);
      if (page) return page;
    } catch (_) {}
    await new Promise((resolve) => setTimeout(resolve, 100));
  }
  throw new Error("Timed out waiting for PCB demo page target.");
}
async function main() {
  const page = await waitForPage();
  const socket = new WebSocket(page.webSocketDebuggerUrl);
  let nextId = 0;
  const pending = new Map();
  const requests = [];
  socket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    if (message.method === "Network.requestWillBeSent") requests.push(message.params.request.url);
    const callback = pending.get(message.id);
    if (!callback) return;
    pending.delete(message.id);
    if (message.error) callback.reject(new Error(JSON.stringify(message.error)));
    else callback.resolve(message.result);
  };
  await new Promise((resolve, reject) => { socket.onopen = resolve; socket.onerror = reject; });
  const send = (method, params = {}) => new Promise((resolve, reject) => {
    const id = ++nextId;
    pending.set(id, { resolve, reject });
    socket.send(JSON.stringify({ id, method, params }));
  });
  const evaluate = async (expression, awaitPromise = false) => {
    const result = await send("Runtime.evaluate", { expression, awaitPromise, returnByValue: true, timeout: 130000 });
    if (result.exceptionDetails) throw new Error(result.exceptionDetails.exception?.description || "Evaluation failed.");
    return result.result.value;
  };
  await send("Runtime.enable");
  await send("Network.enable");
  await evaluate(`(() => { window.addEventListener("error", (event) => { document.body.dataset.lastError = event.error?.stack || event.message; }); })()`);
  const waitReady = async (minimum) => evaluate(`(async () => {
    const deadline = Date.now() + 30000;
    while (Date.now() < deadline) {
      const shell = document.querySelector("#pcb-shell");
      const debug = window.__GEOMETER_PCB_DEMO__?.snapshot();
      if (shell?.dataset.state === "error") throw new Error(document.querySelector("#status")?.textContent || "PCB demo error");
      if (shell?.dataset.state === "ready" && debug && debug.completed >= ${minimum}) return debug;
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("Timed out waiting for PCB demo solve: " + JSON.stringify({
      state: document.querySelector("#pcb-shell")?.dataset.state,
      runtime: document.querySelector("#runtime-label")?.textContent,
      status: document.querySelector("#status")?.textContent,
      debug: window.__GEOMETER_PCB_DEMO__?.snapshot(),
      lastError: document.body.dataset.lastError,
    }));
  })()`, true);
  const screen = (x, y) => evaluate(`(() => {
    const point = window.__GEOMETER_PCB_DEMO__.screenPoint({x:${x}, y:${y}});
    const rect = document.querySelector("#pcb-canvas").getBoundingClientRect();
    return { x: rect.left + point.x, y: rect.top + point.y };
  })()`);
  const sampleWorld = (x, y) => evaluate(`(() => {
    const canvas = document.querySelector("#pcb-canvas");
    const rect = canvas.getBoundingClientRect();
    const point = window.__GEOMETER_PCB_DEMO__.screenPoint({x:${x}, y:${y}});
    const data = canvas.getContext("2d").getImageData(
      Math.round(point.x * canvas.width / rect.width) - 1,
      Math.round(point.y * canvas.height / rect.height) - 1,
      3,
      3,
    ).data;
    return Array.from({length: 9}, (_, index) => [...data.slice(index * 4, index * 4 + 4)]);
  })()`);
  const mouse = (type, point, buttons, button = "left", modifiers = 0) => send("Input.dispatchMouseEvent", {
    type, x: point.x, y: point.y, button, buttons, modifiers, clickCount: type === "mousePressed" ? 1 : 0,
  });
  const click = async (point) => {
    await mouse("mouseMoved", point, 0, "none");
    await mouse("mousePressed", point, 1);
    await mouse("mouseReleased", point, 0);
  };
  const key = async (value, code, modifiers = 0) => {
    await send("Input.dispatchKeyEvent", { type: "keyDown", key: value, code, modifiers });
    await send("Input.dispatchKeyEvent", { type: "keyUp", key: value, code, modifiers });
  };

  const initial = await waitReady(1);
  const viaStart = await screen(7, 4);
  const viaEnd = await screen(8.25, 5.25);
  await mouse("mouseMoved", viaStart, 0, "none");
  await mouse("mousePressed", viaStart, 1);
  await evaluate(`(() => {
    const canvas = document.querySelector("#pcb-canvas");
    for (let step = 1; step <= 8; step += 1) {
      canvas.dispatchEvent(new PointerEvent("pointermove", {
        bubbles: true,
        pointerId: 1,
        pointerType: "mouse",
        clientX: ${viaStart.x} + (${viaEnd.x} - ${viaStart.x}) * step / 8,
        clientY: ${viaStart.y} + (${viaEnd.y} - ${viaStart.y}) * step / 8,
        button: -1,
        buttons: 1,
        pressure: 0.5,
      }));
    }
  })()`);
  await mouse("mouseReleased", viaEnd, 0);
  const moved = await waitReady(initial.completed + 1);
  const thermalPixel = await sampleWorld(8.70, 5.25);

  const viaLocation = await screen(13, 9);
  await mouse("mouseMoved", viaLocation, 0, "none");
  await key("v", "KeyV");
  const added = await waitReady(moved.completed + 1);

  await key("r", "KeyR");
  if (await evaluate(`document.querySelector("#pcb-shell")?.dataset.tool`) !== "route")
    throw new Error("R did not activate the route tool.");
  const routeStart = await screen(9, 3);
  await click(routeStart);
  if ((await evaluate(`window.__GEOMETER_PCB_DEMO__.snapshot().routePointCount`)) !== 1) {
    const routeDiagnostic = await evaluate(`(() => {
      const point = ${JSON.stringify(routeStart)};
      const target = document.elementFromPoint(point.x, point.y);
      const rect = document.querySelector("#pcb-canvas").getBoundingClientRect();
      return { point, target: target?.id || target?.tagName, rect: {left:rect.left, top:rect.top, right:rect.right, bottom:rect.bottom}, snapshot: window.__GEOMETER_PCB_DEMO__.snapshot() };
    })()`);
    throw new Error("The first route click was not accepted: " + JSON.stringify(routeDiagnostic));
  }
  await mouse("mouseMoved", await screen(11, 5), 0, "none");
  await evaluate(`new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)))`, true);
  const previewPixels = {
    trace: await sampleWorld(10, 4),
    clearance: await sampleWorld(9.88, 4.12),
  };
  await click(await screen(11, 5));
  if ((await evaluate(`window.__GEOMETER_PCB_DEMO__.snapshot().routePointCount`)) !== 2)
    throw new Error("The second route click was not accepted.");
  await key("Enter", "Enter");
  const routed = await waitReady(added.completed + 1);

  await key("Escape", "Escape");
  const vertexStart = await screen(0, 0);
  const vertexEnd = await screen(0.75, 0.5);
  await mouse("mouseMoved", vertexStart, 0, "none");
  await mouse("mousePressed", vertexStart, 1);
  await mouse("mouseMoved", vertexEnd, 1);
  await mouse("mouseReleased", vertexEnd, 0);
  const edited = await waitReady(routed.completed + 1);
  await key("z", "KeyZ", 2);
  const undone = await waitReady(edited.completed + 1);
  await key("y", "KeyY", 2);
  const redone = await waitReady(undone.completed + 1);
  const zoomAnchor = await screen(13, 7);
  await send("Input.dispatchMouseEvent", {
    type: "mouseWheel", x: zoomAnchor.x, y: zoomAnchor.y, button: "none", buttons: 0,
    deltaX: 0, deltaY: -180,
  });
  const zoomed = await evaluate(`window.__GEOMETER_PCB_DEMO__.snapshot()`);
  const panStart = await screen(13, 7);
  const panEnd = { x: panStart.x + 48, y: panStart.y + 32 };
  await mouse("mouseMoved", panStart, 0, "none", 8);
  await mouse("mousePressed", panStart, 1, "left", 8);
  await mouse("mouseMoved", panEnd, 1, "left", 8);
  await mouse("mouseReleased", panEnd, 0, "left", 8);
  await evaluate(`new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)))`, true);
  const panned = await evaluate(`window.__GEOMETER_PCB_DEMO__.snapshot()`);
  const closureDigest = await evaluate(`(async () => {
    const snapshot = window.__GEOMETER_PCB_DEMO__.snapshot();
    const closure = snapshot.layerRecords.map((record) => record.jobId + "\\0" + record.digest + "\\n").join("");
    const digest = await crypto.subtle.digest("SHA-256", new TextEncoder().encode(closure));
    return [...new Uint8Array(digest)].map((value) => value.toString(16).padStart(2, "0")).join("");
  })()`, true);
  const visual = await evaluate(`(() => ({
    shellState: document.querySelector("#pcb-shell")?.dataset.state,
    tool: document.querySelector("#pcb-shell")?.dataset.tool,
    font: getComputedStyle(document.documentElement).fontFamily,
    radius: getComputedStyle(document.querySelector("#route-tool")).borderRadius,
    watermark: getComputedStyle(document.body, "::after").backgroundImage.slice(0, 40),
    runtime: document.querySelector("#runtime-label")?.textContent,
    status: document.querySelector("#status")?.textContent,
    clearance: getComputedStyle(document.querySelector("#pcb-shell")).getPropertyValue("--pcb-clearance").trim(),
    paper: getComputedStyle(document.querySelector("#pcb-shell")).getPropertyValue("--wn-paper").trim(),
    thermal: getComputedStyle(document.querySelector("#pcb-shell")).getPropertyValue("--pcb-thermal-fill").trim(),
    copper: getComputedStyle(document.querySelector("#pcb-shell")).getPropertyValue("--pcb-copper-fill").trim(),
    trace: getComputedStyle(document.querySelector("#pcb-shell")).getPropertyValue("--pcb-trace").trim(),
    via: getComputedStyle(document.querySelector("#pcb-shell")).getPropertyValue("--pcb-via-fill").trim(),
    pad: getComputedStyle(document.querySelector("#pcb-shell")).getPropertyValue("--pcb-pad-fill").trim(),
    routePreview: getComputedStyle(document.querySelector("#pcb-shell")).getPropertyValue("--pcb-route-preview").trim(),
    accent: getComputedStyle(document.querySelector("#pcb-shell")).getPropertyValue("--wn-accent").trim(),
  }))()`);
  process.stdout.write(JSON.stringify({
    initial, zoomed, panned, moved, added, routed, edited, undone, redone, closureDigest, visual,
    thermalPixel, previewPixels,
    network: requests.filter((url) => /^https?:/u.test(url)),
  }));
  await send("Browser.close");
}
main().catch((error) => { console.error(error?.stack || error); process.exit(1); });
"""


def _find_chrome() -> str | None:
    candidates = [
        os.environ.get("CHROME_PATH", ""),
        shutil.which("chrome") or "",
        shutil.which("chrome.exe") or "",
        shutil.which("google-chrome") or "",
        shutil.which("chromium") or "",
        r"C:\Program Files\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
    ]
    return next((item for item in candidates if item and Path(item).is_file()), None)


def _free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


def test_pcb_polygon_pour_standalone_interactions() -> None:
    chrome = _find_chrome()
    node = shutil.which("node")
    if chrome is None or node is None:
        if os.environ.get("CI"):
            pytest.fail("Chrome and Node.js are required in CI for the PCB demo gate.")
        pytest.skip("Chrome or Node.js is unavailable.")
    assert DEMO.is_file()
    with tempfile.TemporaryDirectory(prefix="geometer-pcb-demo-chrome-") as profile:
        port = _free_port()
        url = DEMO.resolve().as_uri()
        browser = subprocess.Popen(
            [
                chrome,
                "--headless=new",
                "--disable-gpu",
                "--no-first-run",
                "--no-default-browser-check",
                f"--user-data-dir={profile}",
                f"--remote-debugging-port={port}",
                url,
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        try:
            completed = subprocess.run(
                [node, "-e", CDP_SCRIPT],
                cwd=ROOT,
                env={**os.environ, "CDP_PORT": str(port), "TEST_URL": url},
                capture_output=True,
                text=True,
                timeout=180,
                check=False,
            )
        finally:
            if browser.poll() is None:
                browser.terminate()
            try:
                browser.wait(timeout=10)
            except subprocess.TimeoutExpired:
                browser.kill()
                browser.wait(timeout=10)
        output = completed.stdout + completed.stderr
        assert completed.returncode == 0, output
        result = json.loads(completed.stdout)

    snapshots = [result[name] for name in ["initial", "moved", "added", "routed", "edited"]]
    assert all(snapshot["digest"] for snapshot in snapshots)
    assert len({snapshot["digest"] for snapshot in snapshots}) == len(snapshots)
    assert snapshots[1]["state"]["vias"][0]["x"] == 8.25
    assert len(snapshots[2]["state"]["vias"]) == len(snapshots[1]["state"]["vias"]) + 1
    assert len(snapshots[3]["state"]["traces"]) == len(snapshots[2]["state"]["traces"]) + 1
    route = snapshots[3]["state"]["traces"][-1]["points"]
    assert route == [{"x": 9, "y": 3}, {"x": 11, "y": 5}]
    assert snapshots[4]["state"]["board"][0] == {"x": 0.75, "y": 0.5}
    assert result["zoomed"]["camera"]["pixelsPerWorldUnit"] > result["redone"]["camera"]["pixelsPerWorldUnit"]
    assert result["zoomed"]["digest"] == result["redone"]["digest"]
    assert result["panned"]["camera"]["center"] != result["zoomed"]["camera"]["center"]
    assert result["panned"]["digest"] == result["redone"]["digest"]
    assert result["undone"]["state"] == result["routed"]["state"]
    assert result["undone"]["digest"] == result["routed"]["digest"]
    assert result["redone"]["state"] == result["edited"]["state"]
    assert result["redone"]["digest"] == result["edited"]["digest"]
    assert result["closureDigest"] == result["redone"]["digest"]
    layer_ids = [int(record["jobId"]) for record in result["redone"]["layerRecords"]]
    assert layer_ids == sorted(layer_ids)
    assert layer_ids[0] == 31
    assert any(40_000 <= job_id < 50_000 for job_id in layer_ids)
    assert any(job_id >= 50_000 for job_id in layer_ids)
    assert snapshots[-1]["replaced"] > 0
    assert result["network"] == []
    assert result["visual"]["shellState"] == "ready"
    assert result["visual"]["tool"] == "select"
    assert "JetBrains Mono" in result["visual"]["font"]
    assert result["visual"]["radius"] == "0px"
    assert result["visual"]["watermark"].startswith('url("data:image/svg+xml;base64,')
    assert result["visual"]["runtime"].startswith("Worker ready")
    assert result["visual"]["clearance"] == result["visual"]["paper"]
    assert result["visual"]["copper"] == result["visual"]["accent"]
    assert result["visual"]["thermal"] == result["visual"]["copper"]
    assert result["visual"]["trace"] == result["visual"]["copper"]
    assert result["visual"]["via"] == result["visual"]["copper"]
    assert result["visual"]["pad"] == result["visual"]["copper"]
    assert result["visual"]["routePreview"] == result["visual"]["copper"]
    copper = [0, 108, 103, 255]
    white = [255, 255, 255, 255]
    assert sum(pixel == copper for pixel in result["thermalPixel"]) >= 3
    assert sum(pixel == copper for pixel in result["previewPixels"]["trace"]) >= 3
    assert sum(pixel == white for pixel in result["previewPixels"]["clearance"]) >= 3
