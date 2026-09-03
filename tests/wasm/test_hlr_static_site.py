from __future__ import annotations

import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import urllib.request
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]
SITE = ROOT / "dist" / "wasm" / "demos" / "hlr"

CDP_SCRIPT = r"""
const http = require("http");
const fs = require("fs");
const path = require("path");
const port = Number(process.env.CDP_PORT);
const targetUrl = String(process.env.TEST_URL);
const downloadPath = String(process.env.DOWNLOAD_PATH);
const screenshotPath = String(process.env.HLR_SCREENSHOT_PATH || "");
const bothScreenshotPath = String(process.env.HLR_BOTH_SCREENSHOT_PATH || "");
const axisScreenshotPath = String(process.env.HLR_AXIS_SCREENSHOT_PATH || "");
const edgeScreenshotPath = String(process.env.HLR_EDGE_SCREENSHOT_PATH || "");
const threeScreenshotPath = String(process.env.HLR_THREE_SCREENSHOT_PATH || "");
const uploadBase64 = fs.readFileSync(String(process.env.UPLOAD_FIXTURE)).toString("base64");

function getJson(path) { return new Promise((resolve, reject) => {
  http.get({ host: "127.0.0.1", port, path }, (response) => {
    let data = "";
    response.on("data", (chunk) => { data += chunk; });
    response.on("end", () => { try { resolve(JSON.parse(data)); } catch (error) { reject(error); } });
  }).on("error", reject);
}); }

async function main() {
  const deadline = Date.now() + 30000;
  let page;
  while (Date.now() < deadline && !page) {
    try {
      const pages = await getJson("/json/list");
      page = pages.find((entry) => entry.type === "page" && entry.url === targetUrl);
    } catch (_) {}
    if (!page) await new Promise((resolve) => setTimeout(resolve, 100));
  }
  if (!page) throw new Error("Timed out waiting for the HLR page target.");

  const socket = new WebSocket(page.webSocketDebuggerUrl);
  let nextId = 0;
  const pending = new Map();
  const requests = [];
  const exceptions = [];
  let download;
  let downloadCompleted = false;
  socket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    if (message.method === "Network.requestWillBeSent") requests.push(message.params.request.url);
    if (message.method === "Runtime.exceptionThrown") exceptions.push(message.params.exceptionDetails.text);
    if (message.method === "Page.downloadWillBegin") download = message.params;
    if (message.method === "Page.downloadProgress" && message.params.state === "completed") downloadCompleted = true;
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
    for (let attempt = 0; attempt < 5; attempt += 1) {
      try {
        const result = await send("Runtime.evaluate", {
          expression, awaitPromise, returnByValue: true, timeout: 130000,
        });
        if (result.exceptionDetails) throw new Error(result.exceptionDetails.exception?.description || "Evaluation failed.");
        return result.result.value;
      } catch (error) {
        if (!String(error).includes("Execution context was destroyed") || attempt === 4) throw error;
        await new Promise((resolve) => setTimeout(resolve, 250));
      }
    }
  };

  await send("Runtime.enable");
  await send("Network.enable");
  await send("Page.enable");
  await send("Page.setDownloadBehavior", { behavior: "allow", downloadPath });
  const initial = await evaluate(`(async () => {
    const deadline = Date.now() + 120000;
    while (Date.now() < deadline) {
      const button = document.querySelector("#exportSvgButton");
      if (!document.body.classList.contains("busy") && button && !button.disabled) {
        return {
          model: document.querySelector("#modelSelect")?.value,
          projection: document.querySelector("#projectionMetric")?.textContent,
          paths: document.querySelector("#projectionSvg")?.childElementCount,
          cameraActive: document.querySelector('button[data-view="camera"]')?.classList.contains("active"),
          lens: document.querySelector("#cameraLensSelect")?.value,
          canvasLens: document.querySelector("#modelCanvas")?.dataset.cameraLens,
          axes: {
            top: document.querySelector("#topAxisSelect")?.value,
            front: document.querySelector("#frontAxisSelect")?.value,
          },
          three: {
            material: document.querySelector("#modelCanvas")?.dataset.materialMode,
            shading: document.querySelector("#modelCanvas")?.dataset.shadingMode,
            toneMapping: document.querySelector("#modelCanvas")?.dataset.toneMapping,
            ambient: document.querySelector("#modelCanvas")?.dataset.ambientLight,
            key: document.querySelector("#modelCanvas")?.dataset.keyLight,
            camera: document.querySelector("#modelCanvas")?.dataset.cameraLight,
          },
          emptyRailsHidden: {
            left: document.querySelector(".gdm-panel-rail--left")?.hidden,
            bottom: document.querySelector(".gdm-panel-rail--bottom")?.hidden,
          },
          settingsPanel: {
            active: document.querySelector('.gdm-panel-tab[data-panel-id="settings"]')?.classList.contains("active"),
            open: !document.querySelector('.gdm-panel[data-panel-id="settings"]')?.classList.contains("collapsed"),
            contentMounted: Boolean(document.querySelector('.gdm-panel-body #settingsPanelContent')),
            rightInset: getComputedStyle(document.querySelector("#workspace")).getPropertyValue("--gdm-content-right").trim(),
          },
        };
      }
      if (document.title === "FAIL") throw new Error(document.querySelector("#validationResult")?.textContent);
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("Timed out waiting for the initial HLR projection.");
  })()`, true);

  const panelSystem = await evaluate(`(() => {
    const panel = document.querySelector('.gdm-panel[data-panel-id="settings"]');
    const buttons = panel.querySelectorAll(".gdm-panel-button");
    const collapse = buttons[0];
    const hide = buttons[1];
    const tab = document.querySelector('.gdm-panel-tab[data-panel-id="settings"]');
    const dock = document.querySelector(".gdm-panel-dock--right");
    const handle = dock.querySelector(".gdm-panel-resize");
    const beforeWidth = dock.getBoundingClientRect().width;
    const startX = handle.getBoundingClientRect().x;
    handle.dispatchEvent(new PointerEvent("pointerdown", { bubbles: true, clientX: startX, clientY: 20 }));
    document.dispatchEvent(new PointerEvent("pointermove", { bubbles: true, clientX: startX - 35, clientY: 20 }));
    document.dispatchEvent(new PointerEvent("pointerup", { bubbles: true, clientX: startX - 35, clientY: 20 }));
    const afterWidth = dock.getBoundingClientRect().width;
    collapse.click();
    const collapsed = panel.classList.contains("collapsed");
    collapse.click();
    const reopened = !panel.classList.contains("collapsed");
    hide.click();
    const hidden = dock.hidden && !tab.classList.contains("active");
    tab.click();
    return {
      resized: afterWidth > beforeWidth + 30,
      collapsed,
      reopened,
      hidden,
      restored: !dock.hidden && tab.classList.contains("active"),
    };
  })()`);

  await evaluate(`document.querySelector('.gdm-panel-tab[data-panel-id="three"]').click()`);
  if (threeScreenshotPath) {
    const screenshot = await send("Page.captureScreenshot", { format: "png" });
    fs.writeFileSync(threeScreenshotPath, Buffer.from(screenshot.data, "base64"));
  }
  const threeSettings = await evaluate(`(() => {
    const panel = document.querySelector('.gdm-panel[data-panel-id="three"]');
    const set = (selector, value, eventName = "change") => {
      const control = panel.querySelector(selector);
      if (control.type === "checkbox") control.checked = Boolean(value);
      else control.value = value;
      control.dispatchEvent(new Event(eventName, { bubbles: true }));
    };
    set("#materialModeSelect", "source");
    const sourceObserved = document.querySelector("#modelCanvas").dataset.materialMode;
    set("#materialModeSelect", "basic");
    set("#shadingModeSelect", "flat");
    set("#sidednessSelect", "double");
    set("#wireframeInput", true);
    set("#ambientLightInput", "1.1", "input");
    set("#keyLightInput", "1.2", "input");
    set("#cameraLightInput", "1.3", "input");
    set("#backgroundColorInput", "#ddeeff");
    set("#toneMappingSelect", "aces");
    set("#exposureInput", "1.4", "input");
    const changed = { ...document.querySelector("#modelCanvas").dataset };
    panel.querySelector("#resetThreeButton").click();
    const reset = { ...document.querySelector("#modelCanvas").dataset };
    const buttons = panel.querySelectorAll(".gdm-panel-button");
    buttons[1].click();
    return {
      sourceObserved,
      changed,
      reset,
      hidden: document.querySelector(".gdm-panel-dock--right") && !document.querySelector('.gdm-panel[data-panel-id="three"]'),
    };
  })()`);
  if (screenshotPath) {
    const screenshot = await send("Page.captureScreenshot", { format: "png" });
    fs.writeFileSync(screenshotPath, Buffer.from(screenshot.data, "base64"));
  }
  if (edgeScreenshotPath) {
    await evaluate(`(() => {
      const dock = document.querySelector(".gdm-panel-dock--right");
      dock.scrollTop = dock.scrollHeight;
    })()`);
    await new Promise((resolve) => setTimeout(resolve, 100));
    const screenshot = await send("Page.captureScreenshot", { format: "png" });
    fs.writeFileSync(edgeScreenshotPath, Buffer.from(screenshot.data, "base64"));
    await evaluate(`document.querySelector(".gdm-panel-dock--right").scrollTop = 0`);
  }

  const lensSwitch = await evaluate(`(async () => {
    const select = document.querySelector("#cameraLensSelect");
    select.value = "perspective";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    await new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    const perspective = document.querySelector("#modelCanvas")?.dataset.cameraLens;
    select.value = "orthographic";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    await new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    return {
      perspective,
      restored: document.querySelector("#modelCanvas")?.dataset.cameraLens,
      selected: select.value,
    };
  })()`, true);

  const geometryControls = await evaluate(`(async () => {
    const select = document.querySelector("#meshDeflectionModeSelect");
    const waitForProjection = async () => {
      const deadline = Date.now() + 120000;
      while (Date.now() < deadline) {
        if (!document.body.classList.contains("busy") && !document.querySelector("#exportSvgButton").disabled) return;
        await new Promise((resolve) => setTimeout(resolve, 50));
      }
      throw new Error("Timed out waiting for a tessellation-mode projection.");
    };
    select.value = "absolute";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    await waitForProjection();
    const absolute = {
      linearDisabled: document.querySelector("#linDeflInput").disabled,
      coefficientDisabled: document.querySelector("#deflCoeffInput").disabled,
    };
    select.value = "bbox-relative";
    select.dispatchEvent(new Event("change", { bubbles: true }));
    await waitForProjection();
    return {
      absolute,
      relative: {
        linearDisabled: document.querySelector("#linDeflInput").disabled,
        coefficientDisabled: document.querySelector("#deflCoeffInput").disabled,
      },
    };
  })()`, true);

  const numericAutoProjection = await evaluate(`(async () => {
    let sawBusy = false;
    const observer = new MutationObserver(() => {
      if (document.body.classList.contains("busy")) sawBusy = true;
    });
    observer.observe(document.body, { attributes: true, attributeFilter: ["class"] });
    const input = document.querySelector("#deflCoeffInput");
    input.value = "0.005";
    input.dispatchEvent(new Event("input", { bubbles: true }));
    const deadline = Date.now() + 120000;
    while (Date.now() < deadline) {
      if (sawBusy && !document.body.classList.contains("busy") && !document.querySelector("#exportSvgButton").disabled) break;
      await new Promise((resolve) => setTimeout(resolve, 25));
    }
    observer.disconnect();
    return { value: input.value, sawBusy, settled: !document.body.classList.contains("busy") };
  })()`, true);

  const axisPresets = await evaluate(`(async () => {
    const waitForProjection = async (prefix) => {
      const deadline = Date.now() + 120000;
      while (Date.now() < deadline) {
        const metric = document.querySelector("#projectionMetric")?.textContent || "";
        if (!document.body.classList.contains("busy") && metric.startsWith(prefix)) return metric;
        await new Promise((resolve) => setTimeout(resolve, 50));
      }
      throw new Error("Timed out waiting for " + prefix + " axis-preset projection.");
    };
    const top = document.querySelector("#topAxisSelect");
    const front = document.querySelector("#frontAxisSelect");
    top.value = "+z";
    top.dispatchEvent(new Event("change", { bubbles: true }));
    document.querySelector('button[data-view="top"]').click();
    const metric = await waitForProjection("top ");
    const selected = {
      top: top.value,
      front: front.value,
      topData: document.querySelector("#viewButtons")?.dataset.topAxis,
      frontData: document.querySelector("#viewButtons")?.dataset.frontAxis,
      title: document.querySelector('button[data-view="top"]')?.title,
      metric,
    };
    if (${JSON.stringify(axisScreenshotPath)}) {
      // The screenshot itself is captured by CDP after this evaluation returns.
      document.documentElement.dataset.captureAxisPreset = "ready";
    }
    return { selected, restored: null };
  })()`, true);
  if (axisScreenshotPath) {
    const screenshot = await send("Page.captureScreenshot", { format: "png" });
    fs.writeFileSync(axisScreenshotPath, Buffer.from(screenshot.data, "base64"));
  }
  const axisRestored = await evaluate(`(async () => {
    const waitForProjection = async (prefix) => {
      const deadline = Date.now() + 120000;
      while (Date.now() < deadline) {
        const metric = document.querySelector("#projectionMetric")?.textContent || "";
        if (!document.body.classList.contains("busy") && metric.startsWith(prefix)) return metric;
        await new Promise((resolve) => setTimeout(resolve, 50));
      }
      throw new Error("Timed out waiting for " + prefix + " restored axis projection.");
    };
    const top = document.querySelector("#topAxisSelect");
    const front = document.querySelector("#frontAxisSelect");
    top.value = "+y";
    top.dispatchEvent(new Event("change", { bubbles: true }));
    front.value = "+z";
    front.dispatchEvent(new Event("change", { bubbles: true }));
    await waitForProjection("top ");
    return { top: top.value, front: front.value };
  })()`, true);
  axisPresets.restored = axisRestored;

  const modelSwitch = await evaluate(`(async () => {
    const select = document.querySelector("#modelSelect");
    const previous = select.value;
    select.selectedIndex = select.selectedIndex === 0 ? 1 : 0;
    const selected = select.value;
    select.dispatchEvent(new Event("change", { bubbles: true }));
    const deadline = Date.now() + 120000;
    while (Date.now() < deadline) {
      const metric = document.querySelector("#projectionMetric")?.textContent || "";
      const topActive = document.querySelector('button[data-view="top"]')?.classList.contains("active");
      const cameraView = document.querySelector("#modelCanvas")?.dataset.cameraView;
      if (!document.body.classList.contains("busy") && metric.startsWith("top ") && topActive && cameraView === "top") {
        return { previous, selected, metric, topActive, cameraView };
      }
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("Model switch did not preserve the active Top view.");
  })()`, true);

  const fastSelection = await evaluate(`(async () => {
    const set = (selector, value) => {
      const control = document.querySelector(selector);
      control.value = value;
      control.dispatchEvent(new Event("change", { bubbles: true }));
    };
    const waitForReady = async (view, algorithm = null) => {
      const deadline = Date.now() + 120000;
      while (Date.now() < deadline) {
        const metric = document.querySelector("#projectionMetric")?.textContent || "";
        const svg = document.querySelector("#projectionSvg");
        if (!document.body.classList.contains("busy") && metric.startsWith(view + " ") &&
            (!algorithm || svg?.dataset.projectionAlgorithm === algorithm)) return;
        await new Promise((resolve) => setTimeout(resolve, 50));
      }
      throw new Error("Timed out waiting for " + view + " " + (algorithm || "") + " projection.");
    };
    set("#modelSelect", "sot223.stp");
    await waitForReady("top");
    document.querySelector('button[data-view="front"]').click();
    await waitForReady("front");
    const fastToggle = document.querySelector("#fastBackendInput");
    fastToggle.checked = true;
    fastToggle.dispatchEvent(new Event("change", { bubbles: true }));
    set("#fastCreaseAngleInput", "30");
    set("#outlineAlgoSelect", "fast-mesh-shadow");
    await waitForReady("front", "fast");
    const detailAt30Degrees = document.querySelectorAll("#projectionSvg .detail").length;
    set("#fastCreaseAngleInput", "10");
    const changedDeadline = Date.now() + 120000;
    while (Date.now() < changedDeadline) {
      if (!document.body.classList.contains("busy") &&
          document.querySelectorAll("#projectionSvg .detail").length > detailAt30Degrees) break;
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    const svg = document.querySelector("#projectionSvg");
    if (document.body.classList.contains("busy") || svg?.dataset.projectionAlgorithm !== "fast" ||
        svg?.dataset.outlineAlgorithm !== "fast-mesh-shadow" ||
        document.querySelectorAll("#projectionSvg .detail").length <= detailAt30Degrees) {
      throw new Error("Fast crease angle did not change the SOT-223 projection.");
    }
    const layerCounts = (mode) => {
      document.querySelector('button[data-mode="' + mode + '"]').click();
      return {
        detail: document.querySelectorAll("#projectionSvg .detail").length,
        outline: document.querySelectorAll("#projectionSvg .outline").length,
      };
    };
    const result = {
      algorithm: svg.dataset.projectionAlgorithm,
      fastBackend: fastToggle.checked,
      creaseAngleDegrees: document.querySelector("#fastCreaseAngleInput").value,
      detailAt30Degrees,
      detailAt10Degrees: document.querySelectorAll("#projectionSvg .detail").length,
      model: document.querySelector("#modelSelect").value,
      view: document.querySelector('button[data-view="front"]').classList.contains("active") ? "front" : "other",
      outlineAlgorithm: document.querySelector("#outlineAlgoSelect").value,
      metric: document.querySelector("#projectionMetric").textContent,
      occtSettingsHidden: document.querySelector("#occtSettings").hidden,
      fastSettingsHidden: document.querySelector("#fastSettings").hidden,
      detailOnly: layerCounts("detail"),
      outlineOnly: layerCounts("outline"),
      both: layerCounts("both"),
    };
    document.querySelector('button[data-mode="detail"]').click();
    return result;
  })()`, true);

  const geometryReset = await evaluate(`(async () => {
    const set = (selector, value) => {
      const control = document.querySelector(selector);
      control.value = value;
      control.dispatchEvent(new Event("change", { bubbles: true }));
    };
    set("#algoSelect", "exact");
    const fastToggle = document.querySelector("#fastBackendInput");
    fastToggle.checked = true;
    fastToggle.dispatchEvent(new Event("change", { bubbles: true }));
    set("#outlineAlgoSelect", "hlr-close");
    set("#meshDeflectionModeSelect", "absolute");
    set("#linDeflInput", "0.25");
    set("#angDeflInput", "0.2");
    set("#deflCoeffInput", "0.02");
    set("#hlrTolInput", "0.1");
    document.querySelector('input[data-edge="edge_h_sharp"]').checked = true;
    document.querySelector("#resetGeometryButton").click();
    const deadline = Date.now() + 120000;
    while (Date.now() < deadline) {
      if (!document.body.classList.contains("busy") && !document.querySelector("#exportSvgButton").disabled) break;
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    return {
      fastBackend: fastToggle.checked,
      fastCreaseAngleDegrees: document.querySelector("#fastCreaseAngleInput").value,
      algorithm: document.querySelector("#algoSelect").value,
      outlineAlgorithm: document.querySelector("#outlineAlgoSelect").value,
      meshMode: document.querySelector("#meshDeflectionModeSelect").value,
      coefficient: document.querySelector("#deflCoeffInput").value,
      linear: document.querySelector("#linDeflInput").value,
      angular: document.querySelector("#angDeflInput").value,
      angle: document.querySelector("#hlrTolInput").value,
      edgePreset: document.querySelector("#edgePresetSelect").value,
      hiddenSharp: document.querySelector('input[data-edge="edge_h_sharp"]').checked,
      linearDisabled: document.querySelector("#linDeflInput").disabled,
      coefficientDisabled: document.querySelector("#deflCoeffInput").disabled,
    };
  })()`, true);

  const canvas = await evaluate(`(() => {
    const rect = document.querySelector("#modelCanvas").getBoundingClientRect();
    return { x: rect.x, y: rect.y, width: rect.width, height: rect.height };
  })()`);
  const dragStart = { x: canvas.x + canvas.width * 0.48, y: canvas.y + canvas.height * 0.48 };
  await send("Input.dispatchMouseEvent", { type: "mouseMoved", ...dragStart });
  await send("Input.dispatchMouseEvent", {
    type: "mousePressed", button: "left", buttons: 1, clickCount: 1, ...dragStart,
  });
  for (let step = 1; step <= 8; step += 1) {
    await send("Input.dispatchMouseEvent", {
      type: "mouseMoved",
      buttons: 1,
      x: dragStart.x + step * 8,
      y: dragStart.y + step * 3,
    });
  }
  await send("Input.dispatchMouseEvent", {
    type: "mouseReleased",
    button: "left",
    buttons: 0,
    clickCount: 1,
    x: dragStart.x + 64,
    y: dragStart.y + 24,
  });
  const trackball = await evaluate(`(async () => {
    const deadline = Date.now() + 30000;
    while (Date.now() < deadline) {
      const metric = document.querySelector("#projectionMetric")?.textContent || "";
      const cameraActive = document.querySelector('button[data-view="camera"]')?.classList.contains("active");
      if (!document.body.classList.contains("busy") && cameraActive && metric.startsWith("camera ")) {
        return { metric, cameraActive };
      }
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("Trackball drag did not trigger a camera HLR projection.");
  })()`, true);

  const appearance = await evaluate(`(() => {
    const setValue = (selector, value) => {
      const input = document.querySelector(selector);
      input.value = value;
      input.dispatchEvent(new Event("input", { bubbles: true }));
      input.dispatchEvent(new Event("change", { bubbles: true }));
    };
    setValue("#detailColorInput", "#0f766e");
    setValue("#detailWidthInput", "9");
    setValue("#detailStyleSelect", "dashed");
    setValue("#outlineColorInput", "#dc2626");
    setValue("#outlineWidthInput", "5");
    setValue("#outlineStyleSelect", "dotted");
    setValue("#bboxColorInput", "#7c3aed");
    setValue("#bboxWidthInput", "2");
    setValue("#bboxStyleSelect", "solid");
    const bbox = document.querySelector("#bboxToggleInput");
    bbox.checked = true;
    bbox.dispatchEvent(new Event("change", { bubbles: true }));
    document.querySelector('button[data-mode="both"]').click();
    return {
      mode: document.querySelector('button[data-mode="both"]')?.classList.contains("active"),
      style: document.querySelector("#projectionSvg style")?.textContent,
      detailWidth: document.querySelector("#detailWidthInput")?.value,
      outlineWidth: document.querySelector("#outlineWidthInput")?.value,
      detail: document.querySelectorAll("#projectionSvg .detail").length,
      outline: document.querySelectorAll("#projectionSvg .outline").length,
      bbox: document.querySelectorAll("#projectionSvg .bbox").length,
    };
  })()`);
  if (bothScreenshotPath) {
    const screenshot = await send("Page.captureScreenshot", { format: "png" });
    fs.writeFileSync(bothScreenshotPath, Buffer.from(screenshot.data, "base64"));
  }
  await evaluate(`document.querySelector('button[data-mode="detail"]').click()`);

  await evaluate(`(async () => {
    const binary = atob(${JSON.stringify(uploadBase64)});
    const bytes = new Uint8Array(binary.length);
    for (let index = 0; index < binary.length; index += 1) bytes[index] = binary.charCodeAt(index);
    const transfer = new DataTransfer();
    transfer.items.add(new File([bytes], "upload-test.step", { type: "application/step" }));
    const input = document.querySelector("#stepFileInput");
    input.files = transfer.files;
    input.dispatchEvent(new Event("change", { bubbles: true }));
  })()`, true);

  const uploaded = await evaluate(`(async () => {
    const deadline = Date.now() + 120000;
    while (Date.now() < deadline) {
      const selected = document.querySelector("#modelSelect")?.selectedOptions[0];
      const button = document.querySelector("#exportSvgButton");
      if (!document.body.classList.contains("busy") && selected?.textContent.includes("upload-test.step") && !button.disabled) {
        document.querySelector('button[data-view="front"]').click();
        break;
      }
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    while (Date.now() < deadline) {
      const metric = document.querySelector("#projectionMetric")?.textContent || "";
      const button = document.querySelector("#exportSvgButton");
      if (!document.body.classList.contains("busy") && metric.startsWith("front ") && !button.disabled) {
        return {
          selected: document.querySelector("#modelSelect")?.selectedOptions[0]?.textContent,
          metric,
          options: document.querySelector("#modelSelect")?.options.length,
          paths: document.querySelector("#projectionSvg")?.childElementCount,
        };
      }
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("Timed out waiting for uploaded STEP projection.");
  })()`, true);

  await evaluate(`document.querySelector("#exportSvgButton").click()`);
  const downloadDeadline = Date.now() + 10000;
  while (!download && Date.now() < downloadDeadline) await new Promise((resolve) => setTimeout(resolve, 50));
  if (!download) throw new Error("SVG export did not start a browser download.");
  const exportedPath = path.join(downloadPath, download.suggestedFilename);
  while ((!downloadCompleted || !fs.existsSync(exportedPath)) && Date.now() < downloadDeadline) {
    await new Promise((resolve) => setTimeout(resolve, 50));
  }
  if (!downloadCompleted || !fs.existsSync(exportedPath)) throw new Error("SVG export did not finish writing.");
  process.stdout.write(JSON.stringify({
    initial,
    panelSystem,
    threeSettings,
    lensSwitch,
    geometryControls,
    numericAutoProjection,
    fastSelection,
    geometryReset,
    axisPresets,
    modelSwitch,
    trackball,
    appearance,
    uploaded,
    filename: download.suggestedFilename,
    exceptions,
    externalRequests: requests.filter((url) => /^https?:/u.test(url) && !url.startsWith(targetUrl)),
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
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


def _wait_for_server(url: str) -> None:
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        try:
            with urllib.request.urlopen(url, timeout=1) as response:
                if response.status == 200:
                    return
        except OSError:
            time.sleep(0.05)
    raise RuntimeError(f"Timed out waiting for {url}")


def test_hlr_static_site_upload_projection_and_export() -> None:
    chrome = _find_chrome()
    node = shutil.which("node")
    if chrome is None or node is None:
        if os.environ.get("CI"):
            pytest.fail("Chrome and Node.js are required in CI for the HLR static-site gate.")
        pytest.skip("Chrome or Node.js is unavailable.")
    assert SITE.is_dir()

    with tempfile.TemporaryDirectory(prefix="geometer-hlr-site-", ignore_cleanup_errors=True) as temporary:
        temporary_path = Path(temporary)
        profile = temporary_path / "profile"
        downloads = temporary_path / "downloads"
        downloads.mkdir()
        http_port = _free_port()
        cdp_port = _free_port()
        url = f"http://127.0.0.1:{http_port}/"
        server = subprocess.Popen(
            [sys.executable, "-m", "http.server", str(http_port), "--bind", "127.0.0.1", "--directory", str(SITE)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        _wait_for_server(url)
        browser = subprocess.Popen(
            [
                chrome,
                "--headless=new",
                "--disable-gpu",
                "--no-first-run",
                "--no-default-browser-check",
                f"--user-data-dir={profile}",
                f"--remote-debugging-port={cdp_port}",
                url,
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        try:
            completed = subprocess.run(
                [node, "-e", CDP_SCRIPT],
                cwd=ROOT,
                env={
                    **os.environ,
                    "CDP_PORT": str(cdp_port),
                    "TEST_URL": url,
                    "DOWNLOAD_PATH": str(downloads),
                    "UPLOAD_FIXTURE": str(ROOT / "tests" / "fixtures" / "step" / "embedded_models" / "SOT-23.STEP"),
                },
                capture_output=True,
                text=True,
                timeout=180,
                check=False,
            )
        finally:
            for process in (browser, server):
                if process.poll() is None:
                    process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=10)
        assert completed.returncode == 0, completed.stdout + completed.stderr
        result = json.loads(completed.stdout)
        exported = downloads / result["filename"]
        assert exported.is_file()
        exported_svg = exported.read_text(encoding="utf-8")

    assert result["initial"]["paths"] > 1
    assert result["initial"]["cameraActive"] is True
    assert result["initial"]["projection"].startswith("camera ")
    assert result["initial"]["lens"] == "orthographic"
    assert result["initial"]["canvasLens"] == "orthographic"
    assert result["initial"]["axes"] == {"top": "+y", "front": "+z"}
    assert result["initial"]["three"] == {
        "material": "lambert",
        "shading": "smooth",
        "toneMapping": "none",
        "ambient": "0.20",
        "key": "0.50",
        "camera": "0.75",
    }
    assert result["initial"]["emptyRailsHidden"] == {"left": True, "bottom": True}
    assert result["initial"]["settingsPanel"] == {
        "active": True,
        "open": True,
        "contentMounted": True,
        "rightInset": "368px",
    }
    assert result["panelSystem"] == {
        "resized": True,
        "collapsed": True,
        "reopened": True,
        "hidden": True,
        "restored": True,
    }
    assert result["threeSettings"]["sourceObserved"] == "source"
    assert result["threeSettings"]["changed"]["materialMode"] == "basic"
    assert result["threeSettings"]["changed"]["shadingMode"] == "flat"
    assert result["threeSettings"]["changed"]["sidedness"] == "double"
    assert result["threeSettings"]["changed"]["wireframe"] == "true"
    assert result["threeSettings"]["changed"]["toneMapping"] == "aces"
    assert result["threeSettings"]["changed"]["ambientLight"] == "1.10"
    assert result["threeSettings"]["changed"]["keyLight"] == "1.20"
    assert result["threeSettings"]["changed"]["cameraLight"] == "1.30"
    assert result["threeSettings"]["changed"]["exposure"] == "1.40"
    assert result["threeSettings"]["changed"]["background"] == "#ddeeff"
    assert result["threeSettings"]["reset"]["materialMode"] == "lambert"
    assert result["threeSettings"]["reset"]["shadingMode"] == "smooth"
    assert result["threeSettings"]["reset"]["sidedness"] == "source"
    assert result["threeSettings"]["reset"]["wireframe"] == "false"
    assert result["threeSettings"]["reset"]["toneMapping"] == "none"
    assert result["threeSettings"]["reset"]["ambientLight"] == "0.20"
    assert result["threeSettings"]["reset"]["keyLight"] == "0.50"
    assert result["threeSettings"]["reset"]["cameraLight"] == "0.75"
    assert result["threeSettings"]["reset"]["exposure"] == "1.00"
    assert result["threeSettings"]["reset"]["background"] == "#ffffff"
    assert result["threeSettings"]["hidden"] is True
    assert result["lensSwitch"] == {
        "perspective": "perspective",
        "restored": "orthographic",
        "selected": "orthographic",
    }
    assert result["geometryControls"] == {
        "absolute": {"linearDisabled": False, "coefficientDisabled": True},
        "relative": {"linearDisabled": True, "coefficientDisabled": False},
    }
    assert result["numericAutoProjection"] == {
        "value": "0.005",
        "sawBusy": True,
        "settled": True,
    }
    assert result["fastSelection"]["algorithm"] == "fast"
    assert result["fastSelection"]["fastBackend"] is True
    assert result["fastSelection"]["creaseAngleDegrees"] == "10"
    assert result["fastSelection"]["detailAt10Degrees"] > result["fastSelection"]["detailAt30Degrees"]
    assert result["fastSelection"]["model"] == "sot223.stp"
    assert result["fastSelection"]["view"] == "front"
    assert result["fastSelection"]["outlineAlgorithm"] == "fast-mesh-shadow"
    assert " detail " in result["fastSelection"]["metric"]
    assert " outline " in result["fastSelection"]["metric"]
    assert result["fastSelection"]["occtSettingsHidden"] is True
    assert result["fastSelection"]["fastSettingsHidden"] is False
    assert result["fastSelection"]["detailOnly"]["detail"] > 0
    assert result["fastSelection"]["detailOnly"]["outline"] == 0
    assert result["fastSelection"]["outlineOnly"]["detail"] == 0
    assert result["fastSelection"]["outlineOnly"]["outline"] > 0
    assert result["fastSelection"]["both"]["detail"] > 0
    assert result["fastSelection"]["both"]["outline"] > 0
    assert result["geometryReset"] == {
        "fastBackend": False,
        "fastCreaseAngleDegrees": "30",
        "algorithm": "poly",
        "outlineAlgorithm": "mesh-shadow",
        "meshMode": "bbox-relative",
        "coefficient": "0.004",
        "linear": "0.01",
        "angular": "0.5",
        "angle": "0.0174533",
        "edgePreset": "detail",
        "hiddenSharp": False,
        "linearDisabled": True,
        "coefficientDisabled": False,
    }
    assert result["axisPresets"]["selected"]["top"] == "+z"
    assert result["axisPresets"]["selected"]["front"] == "+x"
    assert result["axisPresets"]["selected"]["topData"] == "+z"
    assert result["axisPresets"]["selected"]["frontData"] == "+x"
    assert result["axisPresets"]["selected"]["title"] == "Top: viewer +Z; page up -X."
    assert result["axisPresets"]["selected"]["metric"].startswith("top ")
    assert result["axisPresets"]["restored"] == {"top": "+y", "front": "+z"}
    assert result["modelSwitch"]["previous"] != result["modelSwitch"]["selected"]
    assert result["modelSwitch"]["metric"].startswith("top ")
    assert result["modelSwitch"]["topActive"] is True
    assert result["modelSwitch"]["cameraView"] == "top"
    assert result["trackball"]["cameraActive"] is True
    assert result["trackball"]["metric"].startswith("camera ")
    assert result["appearance"]["mode"] is True
    assert result["appearance"]["detail"] > 0
    assert result["appearance"]["outline"] > 0
    assert result["appearance"]["bbox"] > 0
    assert result["appearance"]["detailWidth"] == "5"
    assert result["appearance"]["outlineWidth"] == "5"
    assert "stroke: #0f766e; stroke-width: 5px; stroke-dasharray: 20 12.5" in result["appearance"]["style"]
    assert "stroke: #dc2626; stroke-width: 5px; stroke-dasharray: 1 12.5" in result["appearance"]["style"]
    assert "stroke: #7c3aed; stroke-width: 2px; stroke-dasharray: none" in result["appearance"]["style"]
    assert result["uploaded"]["selected"] == "upload-test.step (local)"
    assert result["uploaded"]["metric"].startswith("front ")
    assert result["uploaded"]["options"] == 6
    assert result["uploaded"]["paths"] > 1
    assert result["filename"] == "upload-test-front-detail.svg"
    assert result["exceptions"] == []
    assert result["externalRequests"] == []
    assert exported_svg.startswith('<?xml version="1.0" encoding="UTF-8"?>')
    assert "upload-test.step - front detail" in exported_svg
    assert "<svg" in exported_svg and "viewBox=" in exported_svg
