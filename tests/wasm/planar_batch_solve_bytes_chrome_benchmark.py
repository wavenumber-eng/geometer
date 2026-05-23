"""Run a minimal Chrome benchmark for Geometer browser planar batch solve bytes."""

from __future__ import annotations

import argparse
import base64
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DIST = ROOT / "dist"


CDP_SCRIPT = r"""
const http = require("http");
const port = Number(process.env.CDP_PORT);
const targetUrl = String(process.env.TEST_URL || "");

function getJson(path) {
  return new Promise((resolve, reject) => {
    http.get({ host: "127.0.0.1", port, path }, (res) => {
      let data = "";
      res.on("data", (chunk) => { data += chunk; });
      res.on("end", () => {
        try { resolve(JSON.parse(data)); } catch (error) { reject(error); }
      });
    }).on("error", reject);
  });
}

async function waitForPage() {
  const deadline = Date.now() + 30000;
  const baseUrl = targetUrl.split("?")[0];
  while (Date.now() < deadline) {
    try {
      const pages = await getJson("/json/list");
      const page = pages.find((entry) => (
        entry.type === "page"
        && String(entry.url || "").startsWith(baseUrl)
      ));
      if (page) {
        return page;
      }
    } catch (error) {
      // Chrome may still be starting.
    }
    await new Promise((resolve) => setTimeout(resolve, 100));
  }
  throw new Error("Timed out waiting for Chrome page target");
}

async function main() {
  const page = await waitForPage();
  const ws = new WebSocket(page.webSocketDebuggerUrl);
  let id = 0;
  const pending = new Map();
  ws.onmessage = (event) => {
    const message = JSON.parse(event.data);
    if (message.id && pending.has(message.id)) {
      const callbacks = pending.get(message.id);
      pending.delete(message.id);
      if (message.error) {
        callbacks.reject(new Error(JSON.stringify(message.error)));
      } else {
        callbacks.resolve(message.result);
      }
    }
  };
  await new Promise((resolve, reject) => {
    ws.onopen = resolve;
    ws.onerror = reject;
  });
  const send = (method, params = {}) => new Promise((resolve, reject) => {
    const messageId = ++id;
    pending.set(messageId, { resolve, reject });
    ws.send(JSON.stringify({ id: messageId, method, params }));
  });
  await send("Runtime.enable");
  const expression = `(async () => {
    const deadline = Date.now() + 120000;
    while (Date.now() < deadline) {
      if (window.__GEOMETER_PLANAR_BENCHMARK_RESULT__) {
        return window.__GEOMETER_PLANAR_BENCHMARK_RESULT__;
      }
      if (window.__GEOMETER_PLANAR_BENCHMARK_ERROR__) {
        throw new Error(window.__GEOMETER_PLANAR_BENCHMARK_ERROR__);
      }
      await new Promise((resolve) => setTimeout(resolve, 50));
    }
    throw new Error("Timed out waiting for benchmark result");
  })()`;
  const result = await send("Runtime.evaluate", {
    expression,
    awaitPromise: true,
    returnByValue: true,
    timeout: 130000,
  });
  process.stdout.write(JSON.stringify(result.result.value));
  ws.close();
}

main().catch((error) => {
  console.error(error && error.stack ? error.stack : error);
  process.exit(1);
});
"""


def find_chrome(explicit: str) -> str:
    candidates = [
        explicit,
        os.environ.get("CHROME_PATH", ""),
        shutil.which("chrome") or "",
        shutil.which("chrome.exe") or "",
        shutil.which("google-chrome") or "",
        shutil.which("chromium") or "",
        r"C:\Program Files\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return str(candidate)
    raise SystemExit("Chrome executable not found. Pass --chrome or set CHROME_PATH.")


def find_free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def b64(path: Path) -> str:
    return base64.b64encode(path.read_bytes()).decode("ascii")


def benchmark_script(
    request_path: Path,
    repeat: int,
    warmup: int,
    success: str,
    failure: str,
) -> str:
    geometer_js = (DIST / "geometer.js").read_text(encoding="utf-8")
    wasm_base64 = b64(DIST / "geometer.wasm")
    request_base64 = b64(request_path)
    return f"""{geometer_js}
(async () => {{
  try {{
    const wasmBase64 = "{wasm_base64}";
    const requestBase64 = "{request_base64}";
    const repeat = {max(1, repeat)};
    const warmup = {max(0, warmup)};
    const decode = (text) => {{
      const binary = atob(text);
      const bytes = new Uint8Array(binary.length);
      for (let index = 0; index < binary.length; index += 1) {{
        bytes[index] = binary.charCodeAt(index);
      }}
      return bytes;
    }};
    const requestBytes = decode(requestBase64);
    const moduleStart = performance.now();
    const module = await createGeometerModule({{
      wasmBinary: decode(wasmBase64),
    }});
    const moduleMs = performance.now() - moduleStart;
    const callSolve = () => {{
      const requestPtr = module._malloc(requestBytes.byteLength);
      const valueOut = module._malloc(4);
      const valueSizeOut = module._malloc(4);
      const errorOut = module._malloc(4);
      try {{
        module.HEAPU8.set(requestBytes, requestPtr);
        module.HEAPU32[valueOut >> 2] = 0;
        module.HEAPU32[valueSizeOut >> 2] = 0;
        module.HEAPU32[errorOut >> 2] = 0;
        const code = module.ccall(
          "geometer_planar_batch_solve_bytes",
          "number",
          ["number", "number", "number", "number", "number"],
          [requestPtr, requestBytes.byteLength, valueOut, valueSizeOut, errorOut],
        );
        const valuePtr = module.getValue(valueOut, "*");
        const valueSize = module.getValue(valueSizeOut, "i32");
        const errorPtr = module.getValue(errorOut, "*");
        const error = errorPtr ? module.UTF8ToString(errorPtr) : "";
        const value = valuePtr
          ? new Uint8Array(module.HEAPU8.subarray(valuePtr, valuePtr + valueSize))
          : new Uint8Array(0);
        if (valuePtr) {{
          module._geometer_free_bytes(valuePtr);
        }}
        if (errorPtr) {{
          module._geometer_free_string(errorPtr);
        }}
        if (code !== 0) {{
          throw new Error(error || `Geometer planar solve failed with code ${{code}}`);
        }}
        return value.byteLength;
      }} finally {{
        module._free(requestPtr);
        module._free(valueOut);
        module._free(valueSizeOut);
        module._free(errorOut);
      }}
    }};
    for (let index = 0; index < warmup; index += 1) {{
      callSolve();
    }}
    let responseBytes = 0;
    const timings = [];
    for (let index = 0; index < repeat; index += 1) {{
      const started = performance.now();
      responseBytes = callSolve();
      timings.push(performance.now() - started);
    }}
    const sorted = timings.slice().sort((a, b) => a - b);
    const meanMs = timings.reduce((total, value) => total + value, 0) / timings.length;
    const metrics = {{
      version: module.ccall("geometer_version_string", "string", [], []),
      abi: module.ccall("geometer_abi_version", "number", [], []),
      requestBytes: requestBytes.byteLength,
      responseBytes,
      warmup,
      repeat,
      moduleMs,
      minMs: sorted[0],
      meanMs,
      maxMs: sorted[sorted.length - 1],
      lastMs: timings[timings.length - 1],
      userAgent: typeof navigator !== "undefined" ? navigator.userAgent : "",
    }};
    {success}
  }} catch (error) {{
    {failure}
  }}
}})();
"""


def make_html(request_path: Path, repeat: int, warmup: int, worker: bool) -> str:
    if worker:
        worker_source = benchmark_script(
            request_path,
            repeat,
            warmup,
            success='self.postMessage({ ok: true, metrics });',
            failure='self.postMessage({ ok: false, error: String(error && error.stack ? error.stack : error) });',
        )
        return f"""<!doctype html>
<meta charset="utf-8">
<title>Geometer Planar Batch Worker Benchmark</title>
<script>
(() => {{
  const workerSource = {json.dumps(worker_source)};
  const worker = new Worker(URL.createObjectURL(new Blob([workerSource], {{ type: "application/javascript" }})));
  worker.onmessage = (event) => {{
    const data = event.data || {{}};
    if (data.ok) {{
      window.__GEOMETER_PLANAR_BENCHMARK_RESULT__ = data.metrics || {{}};
    }} else {{
      window.__GEOMETER_PLANAR_BENCHMARK_ERROR__ = String(data.error || "Worker benchmark failed");
    }}
    worker.terminate();
  }};
  worker.onerror = (event) => {{
    window.__GEOMETER_PLANAR_BENCHMARK_ERROR__ = String((event && event.message) || "Worker benchmark failed");
    worker.terminate();
  }};
}})();
</script>
"""
    script = benchmark_script(
        request_path,
        repeat,
        warmup,
        success="window.__GEOMETER_PLANAR_BENCHMARK_RESULT__ = metrics;",
        failure='window.__GEOMETER_PLANAR_BENCHMARK_ERROR__ = String(error && error.stack ? error.stack : error);',
    )
    return f"""<!doctype html>
<meta charset="utf-8">
<title>Geometer Planar Batch Benchmark</title>
<script>
{script}
</script>
"""


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("request", type=Path)
    parser.add_argument("--repeat", type=int, default=5)
    parser.add_argument("--warmup", type=int, default=1)
    parser.add_argument("--chrome", default="")
    parser.add_argument("--output-html", type=Path, default=None)
    parser.add_argument("--metrics", type=Path, default=None)
    parser.add_argument("--keep-profile", action="store_true")
    parser.add_argument("--worker", action="store_true", help="Run the solve inside a Blob Web Worker.")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    chrome = find_chrome(args.chrome)
    request_path = args.request.resolve()
    html = make_html(request_path, args.repeat, args.warmup, args.worker)
    output_html = args.output_html or (ROOT / "output" / "wasm-bench" / f"{request_path.stem}.chrome.html")
    output_html.parent.mkdir(parents=True, exist_ok=True)
    output_html.write_text(html, encoding="utf-8")
    profile_obj = None if args.keep_profile else tempfile.TemporaryDirectory(prefix="geometer-chrome-bench-")
    profile_dir = Path(profile_obj.name) if profile_obj else output_html.parent / "chrome-profile"
    profile_dir.mkdir(parents=True, exist_ok=True)
    port = find_free_port()
    url = output_html.resolve().as_uri()
    chrome_proc = subprocess.Popen(
        [
            chrome,
            "--headless=new",
            "--disable-gpu",
            "--no-first-run",
            "--no-default-browser-check",
            f"--user-data-dir={profile_dir}",
            f"--remote-debugging-port={port}",
            url,
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    try:
        completed = subprocess.run(
            [shutil.which("node") or "node", "-e", CDP_SCRIPT],
            cwd=ROOT,
            text=True,
            encoding="utf-8",
            errors="replace",
            capture_output=True,
            timeout=150,
            check=False,
            env={**os.environ, "CDP_PORT": str(port), "TEST_URL": url},
        )
        if completed.returncode != 0:
            raise RuntimeError(completed.stderr + completed.stdout[-4000:])
        metrics = json.loads(completed.stdout)
        if args.metrics:
            args.metrics.parent.mkdir(parents=True, exist_ok=True)
            args.metrics.write_text(json.dumps(metrics, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(metrics, indent=2))
    finally:
        chrome_proc.terminate()
        try:
            chrome_proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            chrome_proc.kill()
        if profile_obj is not None:
            profile_obj.cleanup()
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
