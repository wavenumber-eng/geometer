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
SITE = ROOT / "dist" / "wasm" / "demos" / "illustration"

CDP_SCRIPT = r"""
const http = require("http");
const fs = require("fs");
const path = require("path");
const port = Number(process.env.CDP_PORT);
const targetUrl = String(process.env.TEST_URL);
const siteRoot = targetUrl.split("?")[0];
const downloadPath = String(process.env.DOWNLOAD_PATH);
const uploadBase64 = fs.readFileSync(String(process.env.UPLOAD_FIXTURE)).toString("base64");

function getJson(requestPath) { return new Promise((resolve, reject) => {
  http.get({ host: "127.0.0.1", port, path: requestPath }, (response) => {
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
  if (!page) throw new Error("Timed out waiting for the Illustration Lab target.");

  const socket = new WebSocket(page.webSocketDebuggerUrl);
  let nextId = 0;
  const pending = new Map();
  const exceptions = [];
  let download;
  let downloadCompleted = false;
  socket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    if (message.method === "Runtime.exceptionThrown")
      exceptions.push(message.params.exceptionDetails.exception?.description || message.params.exceptionDetails.text);
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
        if (result.exceptionDetails)
          throw new Error(result.exceptionDetails.exception?.description || "Evaluation failed.");
        return result.result.value;
      } catch (error) {
        if (!String(error).includes("Execution context was destroyed") || attempt === 4) throw error;
        await new Promise((resolve) => setTimeout(resolve, 250));
      }
    }
  };

  await send("Runtime.enable");
  await send("Page.enable");
  await send("Page.setDownloadBehavior", { behavior: "allow", downloadPath });

  const initial = await evaluate(`(async () => {
    const deadline = Date.now() + 120000;
    while (Date.now() < deadline) {
      const output = document.querySelector("#illustrationOutputPane");
      if (document.title.startsWith("PASS") && !document.querySelector("#illustrationDownloadSvg").disabled) {
        const bounds = (selector) => {
          const boxes = [...document.querySelectorAll(selector)].map((element) => element.getBBox());
          return {
            minX: Math.min(...boxes.map((box) => box.x)),
            minY: Math.min(...boxes.map((box) => box.y)),
            maxX: Math.max(...boxes.map((box) => box.x + box.width)),
            maxY: Math.max(...boxes.map((box) => box.y + box.height)),
          };
        };
        return {
          title: document.title,
          model: document.querySelector("#illustrationModelSelect").value,
          view: output.dataset.view,
          direction: output.dataset.direction,
          triangles: Number(output.dataset.triangles),
          visibleTriangles: Number(output.dataset.visibleTriangles),
          generation: Number(output.dataset.prepareGeneration),
          surfaceDraws: Number(output.dataset.surfaceDraws),
          surfaces: document.querySelectorAll('#illustrationSvgHost polygon[class^="gms"], #illustrationSvgHost path[class^="gms"]').length,
          paths: document.querySelectorAll('#illustrationSvgHost path[class^="gml"]').length,
          details: Number(output.dataset.canvasDetails),
          lineSegments: Number(output.dataset.canvasOutlines),
          background: document.querySelector("#illustrationBackground").value,
          svgBackground: document.querySelector("#illustrationSvgHost svg > rect")?.getAttribute("fill"),
          surfaceBounds: bounds('#illustrationSvgHost polygon[class^="gms"], #illustrationSvgHost path[class^="gms"]'),
          outlineBounds: bounds('#illustrationSvgHost path[class^="gml"]'),
          output: output.dataset.output,
          engine: output.dataset.engine,
          engineLabel: document.querySelector("#illustrationOutputLabel").textContent,
          shading: output.dataset.shading,
          svgBytes: new TextEncoder().encode(document.querySelector("#illustrationSvgHost svg").outerHTML).byteLength,
        };
      }
      if (document.title.startsWith("FAIL")) throw new Error(document.title);
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("Timed out waiting for the initial illustration.");
  })()`, true);

  const gpuHlr = await evaluate(`(async () => {
    const pane = document.querySelector("#illustrationOutputPane");
    const before = Number(pane.dataset.prepareGeneration);
    document.querySelector('button[data-output="gpu"]').click();
    for (let frame = 0; frame < 30; frame += 1)
      await new Promise((resolve) => requestAnimationFrame(resolve));
    const canvas = document.querySelector("#illustrationFastCanvas");
    const gl = canvas.getContext("webgl2");
    const debugRenderer = gl.getExtension("WEBGL_debug_renderer_info");
    const pixels = new Uint8Array(canvas.width * canvas.height * 4);
    gl.readPixels(0, 0, canvas.width, canvas.height, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
    let distinctPixels = false;
    for (let index = 4; index < pixels.length; index += 4) {
      if (
        pixels[index] !== pixels[0] ||
        pixels[index + 1] !== pixels[1] ||
        pixels[index + 2] !== pixels[2] ||
        pixels[index + 3] !== pixels[3]
      ) {
        distinctPixels = true;
        break;
      }
    }
    const result = {
      before,
      after: Number(pane.dataset.prepareGeneration),
      output: pane.dataset.output,
      engine: pane.dataset.engine,
      engineLabel: document.querySelector("#illustrationOutputLabel").textContent,
      visible: !document.querySelector("#illustrationFastCanvas").classList.contains("hidden"),
      buildMs: Number(pane.dataset.fastHlrBuildMs),
      triangles: Number(pane.dataset.fastHlrTriangles),
      edges: Number(pane.dataset.fastHlrEdges),
      cpuMs: Number(pane.dataset.fastHlrCpuMs),
      gpuMs: pane.dataset.fastHlrGpuMs === "unavailable"
        ? null
        : Number(pane.dataset.fastHlrGpuMs),
      frameMs: Number(pane.dataset.fastHlrFrameMs),
      fps: Number(pane.dataset.fastHlrFps),
      drawCalls: Number(pane.dataset.fastHlrDrawCalls),
      distinctPixels,
      renderer: debugRenderer
        ? gl.getParameter(debugRenderer.UNMASKED_RENDERER_WEBGL)
        : gl.getParameter(gl.RENDERER),
    };
    document.querySelector('button[data-output="canvas"]').click();
    return result;
  })()`, true);

  const restyle = await evaluate(`(async () => {
    const pane = document.querySelector("#illustrationOutputPane");
    const before = Number(pane.dataset.prepareGeneration);
    const previousSvg = document.querySelector("#illustrationSvgHost svg").outerHTML;
    const shading = document.querySelector("#illustrationShading");
    shading.value = "banded";
    shading.dispatchEvent(new Event("change", { bubbles: true }));
    const bands = document.querySelector("#illustrationBands");
    bands.value = "32";
    bands.dispatchEvent(new Event("input", { bubbles: true }));
    await new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    document.querySelector('button[data-output="canvas"]').click();
    return {
      before,
      after: Number(pane.dataset.prepareGeneration),
      changed: previousSvg !== document.querySelector("#illustrationSvgHost svg").outerHTML,
      shading: pane.dataset.shading,
      bands: Number(bands.value),
      output: pane.dataset.output,
      engine: pane.dataset.engine,
      engineLabel: document.querySelector("#illustrationOutputLabel").textContent,
      canvasVisible: !document.querySelector("#illustrationCanvas").classList.contains("hidden"),
      canvasOutlines: Number(pane.dataset.canvasOutlines),
      canvasDetails: Number(pane.dataset.canvasDetails),
      paths: document.querySelectorAll('#illustrationSvgHost path[class^="gml"]').length,
    };
  })()`, true);

  const fusion = await evaluate(`(async () => {
    const pane = document.querySelector("#illustrationOutputPane");
    const checkbox = document.querySelector("#illustrationFuseSurfaces");
    const generation = Number(pane.dataset.prepareGeneration);
    const fused = Number(pane.dataset.surfaceDraws);
    checkbox.checked = false;
    checkbox.dispatchEvent(new Event("change", { bubbles: true }));
    await new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    const unfused = Number(pane.dataset.surfaceDraws);
    const triangles = Number(pane.dataset.visibleTriangles);
    checkbox.checked = true;
    checkbox.dispatchEvent(new Event("change", { bubbles: true }));
    await new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    return {
      generation,
      after: Number(pane.dataset.prepareGeneration),
      fused,
      unfused,
      restored: Number(pane.dataset.surfaceDraws),
      triangles,
      checked: checkbox.checked,
    };
  })()`, true);

  const detailToggle = await evaluate(`(async () => {
    const pane = document.querySelector("#illustrationOutputPane");
    const checkbox = document.querySelector("#illustrationHlrDetail");
    const before = Number(pane.dataset.prepareGeneration);
    checkbox.checked = false;
    checkbox.dispatchEvent(new Event("change", { bubbles: true }));
    await new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    if (Number(pane.dataset.canvasDetails) !== 0) throw new Error("HLR detail did not hide.");
    const withoutDetails = document.querySelectorAll('#illustrationSvgHost path[class^="gml"]').length;
    const afterOff = Number(pane.dataset.prepareGeneration);
    checkbox.checked = true;
    checkbox.dispatchEvent(new Event("change", { bubbles: true }));
    await new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    if (Number(pane.dataset.canvasDetails) <= 0) throw new Error("HLR detail did not show.");
    return {
      before,
      afterOff,
      afterOn: Number(pane.dataset.prepareGeneration),
      withoutDetails,
      withDetails: document.querySelectorAll('#illustrationSvgHost path[class^="gml"]').length,
      details: Number(pane.dataset.canvasDetails),
    };
  })()`, true);

  const lazyLinework = await evaluate(`(async () => {
    const pane = document.querySelector("#illustrationOutputPane");
    const busy = document.querySelector("#illustrationBusy");
    const select = document.querySelector("#illustrationModelSelect");
    const outline = document.querySelector("#illustrationHlrOutline");
    const detail = document.querySelector("#illustrationHlrDetail");
    outline.checked = false;
    outline.dispatchEvent(new Event("change", { bubbles: true }));
    detail.checked = false;
    detail.dispatchEvent(new Event("change", { bubbles: true }));

    const option = [...select.options].find(
      (candidate) => candidate.value !== select.value && candidate.textContent.includes("SOIC-8-W"),
    );
    if (!option) throw new Error("SOIC-8-W fixture is unavailable.");
    const beforeModel = Number(pane.dataset.prepareGeneration);
    select.value = option.value;
    select.dispatchEvent(new Event("change", { bubbles: true }));
    const modelDeadline = Date.now() + 120000;
    while (Date.now() < modelDeadline) {
      if (
        select.value === option.value &&
        Number(pane.dataset.prepareGeneration) > beforeModel &&
        !document.querySelector("#illustrationDownloadSvg").disabled
      )
        break;
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    if (Number(pane.dataset.prepareGeneration) <= beforeModel)
      throw new Error("Linework-free model preparation did not complete.");
    const generation = Number(pane.dataset.prepareGeneration);
    if (Number(pane.dataset.canvasDetails) !== 0)
      throw new Error("Disabled HLR detail was rendered on the new model.");

    detail.checked = true;
    detail.dispatchEvent(new Event("change", { bubbles: true }));
    const busyStarted = !busy.hidden;
    const busyPresentation = busy.dataset.presentation;
    detail.checked = false;
    detail.dispatchEvent(new Event("change", { bubbles: true }));
    await new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    const canceled = {
      busyHidden: busy.hidden,
      state: document.querySelector("#illustrationApp").dataset.state,
      details: Number(pane.dataset.canvasDetails),
      paths: document.querySelectorAll('#illustrationSvgHost path[class^="gml"]').length,
    };

    detail.checked = true;
    detail.dispatchEvent(new Event("change", { bubbles: true }));
    const lineworkDeadline = Date.now() + 120000;
    while (Date.now() < lineworkDeadline) {
      if (Number(pane.dataset.canvasDetails) > 0 && busy.hidden) break;
      if (document.title.startsWith("FAIL")) throw new Error(document.title);
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    if (Number(pane.dataset.canvasDetails) <= 0)
      throw new Error("Lazy HLR detail tracing did not complete.");
    return {
      generation,
      finalGeneration: Number(pane.dataset.prepareGeneration),
      busyStarted,
      busyPresentation,
      canceled,
      details: Number(pane.dataset.canvasDetails),
      paths: document.querySelectorAll('#illustrationSvgHost path[class^="gml"]').length,
    };
  })()`, true);

  const meshQuality = await evaluate(`(async () => {
    const pane = document.querySelector("#illustrationOutputPane");
    const select = document.querySelector("#illustrationModelSelect");
    const quality = document.querySelector("#illustrationMeshQuality");
    const tsot = [...select.options].find((candidate) => candidate.textContent.includes("TSOT-23-5"));
    if (!tsot) throw new Error("TSOT-23-5 fixture is unavailable.");
    const beforeModel = Number(pane.dataset.prepareGeneration);
    select.value = tsot.value;
    select.dispatchEvent(new Event("change", { bubbles: true }));
    const balancedDeadline = Date.now() + 120000;
    while (Date.now() < balancedDeadline) {
      if (
        select.value === tsot.value &&
        Number(pane.dataset.prepareGeneration) > beforeModel &&
        !document.querySelector("#illustrationDownloadSvg").disabled
      )
        break;
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    const balanced = {
      generation: Number(pane.dataset.prepareGeneration),
      triangles: Number(pane.dataset.triangles),
      meshKey: pane.dataset.meshKey,
    };

    quality.value = "fine";
    quality.dispatchEvent(new Event("change", { bubbles: true }));
    const fineDeadline = Date.now() + 120000;
    while (Date.now() < fineDeadline) {
      if (
        Number(pane.dataset.prepareGeneration) > balanced.generation &&
        pane.dataset.meshKey !== balanced.meshKey &&
        !document.querySelector("#illustrationDownloadSvg").disabled
      )
        break;
      if (document.title.startsWith("FAIL")) throw new Error(document.title);
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    const fine = {
      generation: Number(pane.dataset.prepareGeneration),
      triangles: Number(pane.dataset.triangles),
      meshKey: pane.dataset.meshKey,
      linear: Number(document.querySelector("#illustrationLinearDeflection").value),
      angular: Number(document.querySelector("#illustrationAngularDeflection").value),
      hlr: Number(document.querySelector("#illustrationHlrDeflection").value),
      hlrAngular: Number(document.querySelector("#illustrationHlrAngularDeflection").value),
    };

    quality.value = "balanced";
    quality.dispatchEvent(new Event("change", { bubbles: true }));
    const resetDeadline = Date.now() + 120000;
    while (Date.now() < resetDeadline) {
      if (
        Number(pane.dataset.prepareGeneration) > fine.generation &&
        pane.dataset.meshKey === balanced.meshKey &&
        !document.querySelector("#illustrationDownloadSvg").disabled
      )
        return { balanced, fine, resetGeneration: Number(pane.dataset.prepareGeneration) };
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("Mesh quality reset did not complete.");
  })()`, true);

  const top = await evaluate(`(async () => {
    const pane = document.querySelector("#illustrationOutputPane");
    const before = Number(pane.dataset.prepareGeneration);
    document.querySelector('button[data-view="top"]').click();
    const deadline = Date.now() + 30000;
    while (Date.now() < deadline) {
      if (pane.dataset.view === "top" && Number(pane.dataset.prepareGeneration) > before)
        return { generation: Number(pane.dataset.prepareGeneration), direction: pane.dataset.direction };
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("Top illustration did not complete.");
  })()`, true);

  const detailBeforeCamera = await evaluate(`(async () => {
    const pane = document.querySelector("#illustrationOutputPane");
    const detail = document.querySelector("#illustrationHlrDetail");
    detail.checked = false;
    detail.dispatchEvent(new Event("change", { bubbles: true }));
    await new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    return { checked: detail.checked, details: Number(pane.dataset.canvasDetails) };
  })()`, true);

  const canvas = await evaluate(`(() => {
    const rect = document.querySelector("#illustrationModelCanvas").getBoundingClientRect();
    return { x: rect.x, y: rect.y, width: rect.width, height: rect.height };
  })()`);
  const start = { x: canvas.x + canvas.width * 0.5, y: canvas.y + canvas.height * 0.5 };
  await send("Input.dispatchMouseEvent", { type: "mouseMoved", ...start });
  await send("Input.dispatchMouseEvent", { type: "mousePressed", button: "left", buttons: 1, clickCount: 1, ...start });
  for (let step = 1; step <= 9; step += 1)
    await send("Input.dispatchMouseEvent", {
      type: "mouseMoved", buttons: 1, x: start.x + step * 7, y: start.y + step * 3,
    });
  await send("Input.dispatchMouseEvent", {
    type: "mouseReleased", button: "left", buttons: 0, clickCount: 1,
    x: start.x + 63, y: start.y + 27,
  });
  const camera = await evaluate(`(async () => {
    const pane = document.querySelector("#illustrationOutputPane");
    const deadline = Date.now() + 30000;
    let settledGeneration = -1;
    let settledSince = Date.now();
    while (Date.now() < deadline) {
      if (pane.dataset.view === "camera" && pane.dataset.direction !== ${JSON.stringify(top.direction)}) {
        const generation = Number(pane.dataset.prepareGeneration);
        if (generation !== settledGeneration) {
          settledGeneration = generation;
          settledSince = Date.now();
        }
        if (document.querySelector("#illustrationBusy").hidden && Date.now() - settledSince >= 500)
          return {
            view: pane.dataset.view,
            direction: pane.dataset.direction,
            generation,
            detailChecked: document.querySelector("#illustrationHlrDetail").checked,
            details: Number(pane.dataset.canvasDetails),
          };
      }
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("Trackball did not drive a current-camera illustration.");
  })()`, true);

  const detailAfterCamera = await evaluate(`(async () => {
    const pane = document.querySelector("#illustrationOutputPane");
    const detail = document.querySelector("#illustrationHlrDetail");
    const generation = Number(pane.dataset.prepareGeneration);
    detail.checked = true;
    detail.dispatchEvent(new Event("change", { bubbles: true }));
    const deadline = Date.now() + 120000;
    while (Date.now() < deadline) {
      if (Number(pane.dataset.canvasDetails) > 0 && document.querySelector("#illustrationBusy").hidden)
        return {
          checked: detail.checked,
          details: Number(pane.dataset.canvasDetails),
          generation: Number(pane.dataset.prepareGeneration),
          previousGeneration: generation,
        };
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("HLR detail did not restore after the camera regression check.");
  })()`, true);

  const cameraRemesh = await evaluate(`(async () => {
    const pane = document.querySelector("#illustrationOutputPane");
    const chord = document.querySelector("#illustrationLinearDeflection");
    const quality = document.querySelector("#illustrationMeshQuality");
    const pose = () => ({
      view: pane.dataset.view,
      direction: pane.dataset.direction,
      position: pane.dataset.cameraPosition,
      target: pane.dataset.cameraTarget,
      zoom: pane.dataset.cameraZoom,
      halfHeight: pane.dataset.cameraHalfHeight,
    });
    const before = { generation: Number(pane.dataset.prepareGeneration), pose: pose() };
    chord.value = "0.08";
    chord.dispatchEvent(new Event("change", { bubbles: true }));
    const customDeadline = Date.now() + 120000;
    while (Date.now() < customDeadline) {
      if (
        Number(pane.dataset.prepareGeneration) > before.generation &&
        !document.querySelector("#illustrationDownloadSvg").disabled &&
        document.querySelector("#illustrationBusy").hidden
      )
        break;
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    const custom = {
      generation: Number(pane.dataset.prepareGeneration),
      pose: pose(),
      quality: quality.value,
    };
    quality.value = "balanced";
    quality.dispatchEvent(new Event("change", { bubbles: true }));
    const resetDeadline = Date.now() + 120000;
    while (Date.now() < resetDeadline) {
      if (
        Number(pane.dataset.prepareGeneration) > custom.generation &&
        !document.querySelector("#illustrationDownloadSvg").disabled &&
        document.querySelector("#illustrationBusy").hidden
      )
        return { before, custom, reset: { generation: Number(pane.dataset.prepareGeneration), pose: pose() } };
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("Camera-preserving mesh reset did not complete.");
  })()`, true);

  const bga = await evaluate(`(async () => {
    const pane = document.querySelector("#illustrationOutputPane");
    const select = document.querySelector("#illustrationModelSelect");
    const option = [...select.options].find((candidate) => candidate.textContent.includes("BGA90"));
    if (!option) throw new Error("BGA90 fixture is unavailable.");
    const before = Number(pane.dataset.prepareGeneration);
    const started = performance.now();
    document.querySelector('button[data-output="svg"]').click();
    select.value = option.value;
    select.dispatchEvent(new Event("change", { bubbles: true }));
    const deadline = Date.now() + 120000;
    while (Date.now() < deadline) {
      if (
        select.selectedOptions[0]?.textContent.includes("BGA90") &&
        Number(pane.dataset.prepareGeneration) > before &&
        !document.querySelector("#illustrationDownloadSvg").disabled
      ) {
        const svg = document.querySelector("#illustrationSvgHost svg");
        const viewBox = svg.viewBox.baseVal;
        const surfaces = [...svg.querySelectorAll('polygon[class^="gms"], path[class^="gms"]')];
        const probes = [];
        for (const y of [0.25, 0.35, 0.45, 0.55, 0.65, 0.75]) {
          for (const x of [0.25, 0.35, 0.45, 0.55, 0.65, 0.75]) {
            const point = new DOMPoint(viewBox.x + viewBox.width * x, viewBox.y + viewBox.height * y);
            const covering = surfaces.filter((surface) => surface.isPointInFill(point));
            if (covering.length > 0)
              probes.push({ x, y, count: covering.length, fill: getComputedStyle(covering.at(-1)).fill });
          }
        }
        return {
          selected: select.selectedOptions[0].textContent,
          triangles: Number(pane.dataset.triangles),
          surfaceDraws: Number(pane.dataset.surfaceDraws),
          layeredSurfaces: Number(pane.dataset.layeredSurfaces),
          layeringChecked: document.querySelector("#illustrationLayerCoplanar").checked,
          surfaces: document.querySelectorAll('#illustrationSvgHost polygon[class^="gms"], #illustrationSvgHost path[class^="gms"]').length,
          paths: document.querySelectorAll('#illustrationSvgHost path[class^="gml"]').length,
          elapsedMs: performance.now() - started,
          probes,
        };
      }
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("BGA90 illustration did not complete.");
  })()`, true);

  const gpuHlrLarge = await evaluate(`(async () => {
    const pane = document.querySelector("#illustrationOutputPane");
    const before = Number(pane.dataset.prepareGeneration);
    document.querySelector('button[data-output="gpu"]').click();
    for (let frame = 0; frame < 60; frame += 1)
      await new Promise((resolve) => requestAnimationFrame(resolve));
    const result = {
      before,
      after: Number(pane.dataset.prepareGeneration),
      buildMs: Number(pane.dataset.fastHlrBuildMs),
      triangles: Number(pane.dataset.fastHlrTriangles),
      edges: Number(pane.dataset.fastHlrEdges),
      cpuMs: Number(pane.dataset.fastHlrCpuMs),
      gpuMs: pane.dataset.fastHlrGpuMs === "unavailable"
        ? null
        : Number(pane.dataset.fastHlrGpuMs),
      frameMs: Number(pane.dataset.fastHlrFrameMs),
      fps: Number(pane.dataset.fastHlrFps),
      drawCalls: Number(pane.dataset.fastHlrDrawCalls),
    };
    document.querySelector('button[data-output="svg"]').click();
    return result;
  })()`, true);

  await evaluate(`(async () => {
    const binary = atob(${JSON.stringify(uploadBase64)});
    const bytes = new Uint8Array(binary.length);
    for (let index = 0; index < binary.length; index += 1) bytes[index] = binary.charCodeAt(index);
    const transfer = new DataTransfer();
    transfer.items.add(new File([bytes], "illustration-upload.step", { type: "application/step" }));
    const input = document.querySelector("#illustrationStepInput");
    input.files = transfer.files;
    input.dispatchEvent(new Event("change", { bubbles: true }));
  })()`, true);
  const uploaded = await evaluate(`(async () => {
    const deadline = Date.now() + 120000;
    while (Date.now() < deadline) {
      const selected = document.querySelector("#illustrationModelSelect").selectedOptions[0]?.textContent || "";
      const pane = document.querySelector("#illustrationOutputPane");
      if (selected.includes("illustration-upload.step") && !document.querySelector("#illustrationDownloadSvg").disabled) {
        const bounds = (selector) => {
          const boxes = [...document.querySelectorAll(selector)].map((element) => element.getBBox());
          return {
            minX: Math.min(...boxes.map((box) => box.x)),
            minY: Math.min(...boxes.map((box) => box.y)),
            maxX: Math.max(...boxes.map((box) => box.x + box.width)),
            maxY: Math.max(...boxes.map((box) => box.y + box.height)),
          };
        };
        return {
          selected,
          triangles: Number(pane.dataset.triangles),
          paths: document.querySelectorAll('#illustrationSvgHost path[class^="gml"]').length,
          details: Number(pane.dataset.canvasDetails),
          surfaceBounds: bounds('#illustrationSvgHost polygon[class^="gms"], #illustrationSvgHost path[class^="gms"]'),
          outlineBounds: bounds('#illustrationSvgHost path[class^="gml"]'),
          view: pane.dataset.view,
        };
      }
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("Uploaded STEP illustration did not complete.");
  })()`, true);

  await evaluate(`document.querySelector("#illustrationDownloadSvg").click()`);
  const downloadDeadline = Date.now() + 10000;
  while (!download && Date.now() < downloadDeadline) await new Promise((resolve) => setTimeout(resolve, 50));
  if (!download) throw new Error("Illustration SVG download did not start.");
  const exportedPath = path.join(downloadPath, download.suggestedFilename);
  while ((!downloadCompleted || !fs.existsSync(exportedPath)) && Date.now() < downloadDeadline)
    await new Promise((resolve) => setTimeout(resolve, 50));
  if (!downloadCompleted || !fs.existsSync(exportedPath)) throw new Error("Illustration SVG download did not finish.");

  const resources = await evaluate(`performance.getEntriesByType("resource").map((entry) => entry.name)`);
  process.stdout.write(JSON.stringify({
    initial, gpuHlr, restyle, fusion, detailToggle, lazyLinework, meshQuality, top, detailBeforeCamera,
    camera, detailAfterCamera, cameraRemesh, bga, gpuHlrLarge, uploaded,
    filename: download.suggestedFilename,
    exceptions,
    externalRequests: resources.filter((url) => /^https?:/u.test(url) && !url.startsWith(siteRoot)),
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


def test_illustration_static_site_mesh_render_upload_and_export() -> None:
    chrome = _find_chrome()
    node = shutil.which("node")
    if chrome is None or node is None:
        if os.environ.get("CI"):
            pytest.fail("Chrome and Node.js are required in CI for the Illustration Lab gate.")
        pytest.skip("Chrome or Node.js is unavailable.")
    assert SITE.is_dir()

    with tempfile.TemporaryDirectory(prefix="geometer-illustration-site-", ignore_cleanup_errors=True) as temporary:
        temporary_path = Path(temporary)
        profile = temporary_path / "profile"
        downloads = temporary_path / "downloads"
        downloads.mkdir()
        http_port = _free_port()
        cdp_port = _free_port()
        url = f"http://127.0.0.1:{http_port}/?validation=1"
        server = subprocess.Popen(
            [sys.executable, "-m", "http.server", str(http_port), "--bind", "127.0.0.1", "--directory", str(SITE)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        _wait_for_server(url)
        chrome_arguments = [chrome, "--headless=new"]
        if not os.environ.get("GEOMETER_GPU_HLR_HARDWARE"):
            chrome_arguments.append("--disable-gpu")
        chrome_arguments.extend(
            [
                "--no-first-run",
                "--no-default-browser-check",
                f"--user-data-dir={profile}",
                f"--remote-debugging-port={cdp_port}",
                url,
            ]
        )
        browser = subprocess.Popen(
            chrome_arguments,
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
                    "UPLOAD_FIXTURE": str(
                        ROOT / "tests" / "fixtures" / "step" / "generated_topology" / "generated_fused_slab.step"
                    ),
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
        if os.environ.get("GEOMETER_GPU_HLR_REPORT"):
            print(json.dumps({"small": result["gpuHlr"], "large": result["gpuHlrLarge"]}, sort_keys=True))
        exported = downloads / result["filename"]
        assert exported.is_file()
        exported_svg = exported.read_text(encoding="utf-8")

    assert result["initial"]["title"].startswith("PASS")
    assert result["initial"]["triangles"] > 0
    assert result["initial"]["surfaces"] > 0
    assert result["initial"]["surfaces"] == result["initial"]["surfaceDraws"]
    assert result["initial"]["surfaceDraws"] < result["initial"]["visibleTriangles"]
    assert result["initial"]["svgBytes"] < 50_000
    assert result["initial"]["paths"] > 0
    assert result["initial"]["details"] > 0
    assert result["initial"]["background"] == "#ffffff"
    assert result["initial"]["svgBackground"] == "#ffffff"
    surface_bounds = result["initial"]["surfaceBounds"]
    outline_bounds = result["initial"]["outlineBounds"]
    surface_width = surface_bounds["maxX"] - surface_bounds["minX"]
    surface_height = surface_bounds["maxY"] - surface_bounds["minY"]
    outline_width = outline_bounds["maxX"] - outline_bounds["minX"]
    outline_height = outline_bounds["maxY"] - outline_bounds["minY"]
    assert outline_width == pytest.approx(surface_width, rel=0.03)
    assert outline_height == pytest.approx(surface_height, rel=0.03)
    assert (outline_bounds["minX"] + outline_bounds["maxX"]) == pytest.approx(
        surface_bounds["minX"] + surface_bounds["maxX"], abs=surface_width * 0.03
    )
    assert (outline_bounds["minY"] + outline_bounds["maxY"]) == pytest.approx(
        surface_bounds["minY"] + surface_bounds["maxY"], abs=surface_height * 0.03
    )
    assert result["initial"]["output"] == "svg"
    assert result["initial"]["engine"] == "fast-vector"
    assert "FAST VECTOR / C++ WASM CPU / SVG" in result["initial"]["engineLabel"]
    assert "FAST MESH-SHADOW + FAST DETAIL" in result["initial"]["engineLabel"]
    assert result["initial"]["shading"] == "toon"
    assert result["gpuHlr"]["before"] == result["gpuHlr"]["after"]
    assert result["gpuHlr"]["output"] == "gpu"
    assert result["gpuHlr"]["engine"] == "gpu-preview"
    assert result["gpuHlr"]["engineLabel"] == "GPU EDGE PREVIEW / SEPARATE FROM FAST VECTOR OUTPUT"
    assert result["gpuHlr"]["visible"] is True
    assert result["gpuHlr"]["buildMs"] >= 0
    assert result["gpuHlr"]["triangles"] == result["initial"]["triangles"]
    assert result["gpuHlr"]["edges"] > 0
    assert result["gpuHlr"]["cpuMs"] >= 0
    assert result["gpuHlr"]["gpuMs"] is None or result["gpuHlr"]["gpuMs"] >= 0
    assert result["gpuHlr"]["frameMs"] > 0
    assert result["gpuHlr"]["fps"] > 0
    assert result["gpuHlr"]["drawCalls"] > 0
    assert result["gpuHlr"]["distinctPixels"] is True
    assert result["gpuHlr"]["renderer"]
    assert result["restyle"] == {
        "before": result["initial"]["generation"],
        "after": result["initial"]["generation"],
        "changed": True,
        "shading": "banded",
        "bands": 32,
        "output": "canvas",
        "engine": "fast-vector",
        "engineLabel": result["restyle"]["engineLabel"],
        "canvasVisible": True,
        "canvasOutlines": result["restyle"]["canvasOutlines"],
        "canvasDetails": result["restyle"]["canvasDetails"],
        "paths": result["restyle"]["paths"],
    }
    assert "FAST VECTOR / C++ WASM CPU / CANVAS" in result["restyle"]["engineLabel"]
    assert "FAST MESH-SHADOW + FAST DETAIL" in result["restyle"]["engineLabel"]
    assert result["restyle"]["paths"] == result["initial"]["paths"]
    assert result["restyle"]["canvasOutlines"] == result["initial"]["lineSegments"]
    assert result["restyle"]["canvasDetails"] == result["initial"]["details"]
    assert result["fusion"]["generation"] == result["fusion"]["after"]
    assert result["fusion"]["unfused"] == result["fusion"]["triangles"]
    assert result["fusion"]["fused"] <= result["fusion"]["unfused"]
    assert result["fusion"]["restored"] == result["fusion"]["fused"]
    assert result["fusion"]["checked"] is True
    assert result["detailToggle"]["details"] == result["initial"]["details"]
    assert result["detailToggle"]["withDetails"] == result["initial"]["paths"]
    assert result["detailToggle"]["withoutDetails"] < result["detailToggle"]["withDetails"]
    assert result["detailToggle"]["afterOff"] == result["detailToggle"]["before"]
    assert result["detailToggle"]["afterOn"] == result["detailToggle"]["before"]
    assert result["lazyLinework"]["busyStarted"] is True
    assert result["lazyLinework"]["busyPresentation"] == "indicator"
    assert result["lazyLinework"]["canceled"] == {
        "busyHidden": True,
        "state": "ready",
        "details": 0,
        "paths": 0,
    }
    assert result["lazyLinework"]["details"] > 0
    assert result["lazyLinework"]["paths"] > 0
    assert result["lazyLinework"]["finalGeneration"] == result["lazyLinework"]["generation"]
    assert result["meshQuality"]["balanced"]["triangles"] > 0
    assert result["meshQuality"]["fine"]["triangles"] > result["meshQuality"]["balanced"]["triangles"]
    assert result["meshQuality"]["fine"]["meshKey"] != result["meshQuality"]["balanced"]["meshKey"]
    assert result["meshQuality"]["fine"]["linear"] == pytest.approx(0.03)
    assert result["meshQuality"]["fine"]["angular"] == pytest.approx(15)
    assert result["meshQuality"]["fine"]["hlr"] == pytest.approx(0.002)
    assert result["meshQuality"]["fine"]["hlrAngular"] == pytest.approx(15)
    assert result["meshQuality"]["resetGeneration"] > result["meshQuality"]["fine"]["generation"]
    assert result["top"]["generation"] > result["initial"]["generation"]
    assert result["top"]["direction"] == "0.000000,1.000000,0.000000"
    assert result["detailBeforeCamera"] == {"checked": False, "details": 0}
    assert result["camera"]["view"] == "camera"
    assert result["camera"]["direction"] != result["top"]["direction"]
    assert result["camera"]["detailChecked"] is False
    assert result["camera"]["details"] == 0
    assert result["detailAfterCamera"]["checked"] is True
    assert result["detailAfterCamera"]["details"] > 0
    assert result["detailAfterCamera"]["previousGeneration"] == result["camera"]["generation"]
    assert result["detailAfterCamera"]["generation"] == result["detailAfterCamera"]["previousGeneration"]
    assert result["cameraRemesh"]["custom"]["quality"] == "custom"
    assert result["cameraRemesh"]["custom"]["generation"] > result["cameraRemesh"]["before"]["generation"]
    assert result["cameraRemesh"]["reset"]["generation"] > result["cameraRemesh"]["custom"]["generation"]
    camera_pose_before = result["cameraRemesh"]["before"]["pose"]
    for camera_pose_after in (
        result["cameraRemesh"]["custom"]["pose"],
        result["cameraRemesh"]["reset"]["pose"],
    ):
        assert camera_pose_after["view"] == camera_pose_before["view"] == "camera"
        assert camera_pose_after["target"] == camera_pose_before["target"]
        assert camera_pose_after["zoom"] == camera_pose_before["zoom"]
        assert camera_pose_after["halfHeight"] == camera_pose_before["halfHeight"]
        for field in ("position", "direction"):
            before_values = tuple(float(value) for value in camera_pose_before[field].split(","))
            after_values = tuple(float(value) for value in camera_pose_after[field].split(","))
            assert after_values == pytest.approx(before_values, abs=0.0001)
    assert "BGA90" in result["bga"]["selected"]
    assert result["bga"]["triangles"] > 20_000
    assert result["bga"]["layeredSurfaces"] > 0
    assert result["bga"]["layeringChecked"] is True
    assert 0 < result["bga"]["surfaceDraws"] <= result["bga"]["triangles"]
    assert result["bga"]["surfaces"] == result["bga"]["surfaceDraws"]
    assert result["bga"]["paths"] > 0
    assert result["bga"]["elapsedMs"] < 30_000
    assert result["gpuHlrLarge"]["before"] == result["gpuHlrLarge"]["after"]
    assert result["gpuHlrLarge"]["triangles"] > 20_000
    assert result["gpuHlrLarge"]["triangles"] == pytest.approx(result["bga"]["triangles"], rel=0.01)
    assert result["gpuHlrLarge"]["edges"] > result["gpuHlr"]["edges"]
    assert result["gpuHlrLarge"]["buildMs"] >= 0
    assert result["gpuHlrLarge"]["cpuMs"] >= 0
    assert result["gpuHlrLarge"]["gpuMs"] is None or result["gpuHlrLarge"]["gpuMs"] >= 0
    assert result["gpuHlrLarge"]["frameMs"] > 0
    assert result["gpuHlrLarge"]["fps"] > 0
    assert result["gpuHlrLarge"]["drawCalls"] > 0
    assert len(result["bga"]["probes"]) >= 20
    assert all(probe["count"] >= 2 for probe in result["bga"]["probes"])
    for fill in {probe["fill"] for probe in result["bga"]["probes"]}:
        channels = tuple(int(channel) for channel in fill.removeprefix("rgb(").removesuffix(")").split(","))
        assert len(channels) == 3
        assert channels[0] == channels[1] == channels[2]
        assert channels[0] <= 20 or channels[0] == 255
    assert result["uploaded"]["selected"] == "illustration-upload.step (local)"
    assert result["uploaded"]["triangles"] > 0
    assert result["uploaded"]["paths"] > 0
    assert result["uploaded"]["details"] > 0
    uploaded_surface = result["uploaded"]["surfaceBounds"]
    uploaded_outline = result["uploaded"]["outlineBounds"]
    uploaded_surface_width = uploaded_surface["maxX"] - uploaded_surface["minX"]
    uploaded_surface_height = uploaded_surface["maxY"] - uploaded_surface["minY"]
    assert (uploaded_outline["maxX"] - uploaded_outline["minX"]) == pytest.approx(uploaded_surface_width, rel=0.03)
    assert (uploaded_outline["maxY"] - uploaded_outline["minY"]) == pytest.approx(uploaded_surface_height, rel=0.03)
    assert (uploaded_outline["minX"] + uploaded_outline["maxX"]) == pytest.approx(
        uploaded_surface["minX"] + uploaded_surface["maxX"], abs=uploaded_surface_width * 0.03
    )
    assert (uploaded_outline["minY"] + uploaded_outline["maxY"]) == pytest.approx(
        uploaded_surface["minY"] + uploaded_surface["maxY"], abs=uploaded_surface_height * 0.03
    )
    assert result["filename"].endswith(".svg")
    assert result["exceptions"] == []
    assert result["externalRequests"] == []
    assert exported_svg.startswith('<?xml version="1.0" encoding="UTF-8"?>')
    assert "geometry.mesh_illustration.prototype.a0" in exported_svg
    assert "<style>" in exported_svg
    assert 'class="gms' in exported_svg
    assert 'class="gml' in exported_svg
