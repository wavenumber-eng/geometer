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
DEMO = ROOT / "dist" / "wasm" / "demos" / "analytic_polygon_pour_demo.html"

CDP_SCRIPT = r"""
const http = require("http");
const port = Number(process.env.CDP_PORT);
const targetUrl = String(process.env.TEST_URL);

function getJson(path) {
  return new Promise((resolve, reject) => {
    http.get({ host: "127.0.0.1", port, path }, (response) => {
      let data = "";
      response.on("data", (chunk) => { data += chunk; });
      response.on("end", () => {
        try { resolve(JSON.parse(data)); } catch (error) { reject(error); }
      });
    }).on("error", reject);
  });
}

async function waitForPage() {
  const deadline = Date.now() + 30000;
  while (Date.now() < deadline) {
    try {
      const pages = await getJson("/json/list");
      const page = pages.find((entry) => entry.type === "page" && entry.url === targetUrl);
      if (page) return page;
    } catch (_) {
      // Chrome may still be starting.
    }
    await new Promise((resolve) => setTimeout(resolve, 100));
  }
  throw new Error("Timed out waiting for standalone demo page target.");
}

async function main() {
  const page = await waitForPage();
  const socket = new WebSocket(page.webSocketDebuggerUrl);
  let nextId = 0;
  const pending = new Map();
  socket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    const callback = pending.get(message.id);
    if (!callback) return;
    pending.delete(message.id);
    if (message.error) callback.reject(new Error(JSON.stringify(message.error)));
    else callback.resolve(message.result);
  };
  await new Promise((resolve, reject) => {
    socket.onopen = resolve;
    socket.onerror = reject;
  });
  const send = (method, params = {}) => new Promise((resolve, reject) => {
    const id = ++nextId;
    pending.set(id, { resolve, reject });
    socket.send(JSON.stringify({ id, method, params }));
  });
  await send("Runtime.enable");
  await new Promise((resolve) => setTimeout(resolve, 500));
  const expression = `(async () => {
    const waitReady = async (minimumCompleted) => {
      const deadline = Date.now() + 120000;
      while (Date.now() < deadline) {
        const shell = document.querySelector("#pour-shell");
        const state = shell?.dataset.state || "missing";
        const completed = Number(document.querySelector("#completed")?.textContent || "0");
        if (state === "error") {
          throw new Error(document.querySelector("#status")?.textContent || "Demo entered error state.");
        }
        if (state === "ready" && completed >= minimumCompleted) return completed;
        await new Promise((resolve) => setTimeout(resolve, 50));
      }
      throw new Error("Timed out waiting for the standalone analytic demo.");
    };
    const firstCompleted = await waitReady(1);
    const firstDigest = document.querySelector("#digest")?.textContent || "";
    const slider = document.querySelector("#slot-position");
    slider.value = "11.25";
    slider.dispatchEvent(new Event("input", { bubbles: true }));
    const secondCompleted = await waitReady(firstCompleted + 1);
    const rootStyle = getComputedStyle(document.documentElement);
    const buttonStyle = getComputedStyle(document.querySelector("#reset-demo"));
    const watermarkStyle = getComputedStyle(document.body, "::after");
    return {
      completed: secondCompleted,
      digest: document.querySelector("#digest")?.textContent || "",
      firstDigest,
      slotValue: document.querySelector("#slot-value")?.textContent || "",
      fontFamily: rootStyle.fontFamily,
      colorScheme: rootStyle.colorScheme,
      buttonRadius: buttonStyle.borderRadius,
      watermarkOpacity: watermarkStyle.opacity,
      watermarkImage: watermarkStyle.backgroundImage.slice(0, 40),
      runtime: document.querySelector("#runtime-label")?.textContent || "",
      state: document.querySelector("#pour-shell")?.dataset.state || "",
      status: document.querySelector("#status")?.textContent || "",
    };
  })()`;
  let result;
  for (let attempt = 0; attempt < 5; attempt += 1) {
    try {
      result = await send("Runtime.evaluate", {
        expression,
        awaitPromise: true,
        returnByValue: true,
        timeout: 130000,
      });
      break;
    } catch (error) {
      if (!String(error).includes("Execution context was destroyed") || attempt === 4) throw error;
      await new Promise((resolve) => setTimeout(resolve, 250));
    }
  }
  if (!result) throw new Error("Standalone demo evaluation did not produce a result.");
  if (result.exceptionDetails) throw new Error(result.exceptionDetails.exception?.description || "Evaluation failed.");
  process.stdout.write(JSON.stringify(result.result.value));
  socket.close();
}

main().catch((error) => {
  console.error(error && error.stack ? error.stack : error);
  process.exit(1);
});
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


def test_analytic_polygon_pour_standalone_file_url() -> None:
    chrome = _find_chrome()
    if chrome is None:
        if os.environ.get("CI"):
            pytest.fail("Chrome/Chromium is required in CI for the standalone demo gate.")
        pytest.skip("Chrome/Chromium is unavailable.")
    node = shutil.which("node")
    if node is None:
        if os.environ.get("CI"):
            pytest.fail("Node.js is required in CI for the standalone demo gate.")
        pytest.skip("Node.js is unavailable.")
    assert DEMO.is_file()

    with tempfile.TemporaryDirectory(prefix="geometer-analytic-demo-chrome-") as profile:
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
                timeout=150,
                check=False,
            )
        finally:
            browser.terminate()
            try:
                browser.wait(timeout=10)
            except subprocess.TimeoutExpired:
                browser.kill()
        assert completed.returncode == 0, completed.stdout + completed.stderr
        result = json.loads(completed.stdout)

    assert result["state"] == "ready"
    assert result["completed"] >= 2
    assert result["firstDigest"].startswith("89190b")
    assert result["digest"]
    assert result["digest"] != result["firstDigest"]
    assert result["slotValue"] == "11.25 mm"
    assert result["runtime"].startswith("Worker ready")
    assert "JetBrains Mono" in result["fontFamily"]
    assert result["colorScheme"] == "light"
    assert result["buttonRadius"] == "0px"
    assert result["watermarkOpacity"] == "0.16"
    assert result["watermarkImage"].startswith('url("data:image/svg+xml;base64,')
