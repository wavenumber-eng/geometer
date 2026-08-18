"""Run the full-browser HLR smoke against an explicit WASM artifact pair."""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlencode


ROOT = Path(__file__).resolve().parent.parent


class QuietHandler(SimpleHTTPRequestHandler):
    def log_message(self, format: str, *args: object) -> None:
        del format, args


def find_browser() -> Path:
    names = (
        "chrome",
        "google-chrome",
        "google-chrome-stable",
        "chromium",
        "chromium-browser",
    )
    for name in names:
        candidate = shutil.which(name)
        if candidate:
            return Path(candidate).resolve()
    candidates: list[Path] = []
    if sys.platform == "win32":
        candidates.extend(
            Path(os.environ.get(variable, "")) / "Google/Chrome/Application/chrome.exe"
            for variable in ("ProgramFiles", "ProgramFiles(x86)", "LOCALAPPDATA")
        )
    elif sys.platform == "darwin":
        candidates.extend(
            (
                Path("/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"),
                Path("/Applications/Chromium.app/Contents/MacOS/Chromium"),
            )
        )
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    raise FileNotFoundError("Chrome or Chromium is required for full-browser WASM validation")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--browser-dist", type=Path, required=True)
    parser.add_argument(
        "--step",
        type=Path,
        default=ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP",
    )
    parser.add_argument("--timeout-seconds", type=int, default=60)
    args = parser.parse_args()

    browser_dist = args.browser_dist.resolve()
    step = args.step.resolve()
    for required in (browser_dist / "geometer.js", browser_dist / "geometer.wasm", step):
        if not required.is_file():
            raise FileNotFoundError(required)
    try:
        browser_relative = browser_dist.relative_to(ROOT).as_posix()
        step_relative = step.relative_to(ROOT).as_posix()
    except ValueError as exc:
        raise ValueError("browser artifacts and STEP fixture must remain below the repository root") from exc

    browser = find_browser()
    handler = partial(QuietHandler, directory=str(ROOT))
    server = ThreadingHTTPServer(("127.0.0.1", 0), handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    query = urlencode(
        {
            "browserDist": f"/{browser_relative}",
            "step": f"/{step_relative}",
        }
    )
    url = (
        f"http://127.0.0.1:{server.server_port}/tests/wasm/browser_hlr_validation.html?{query}"
    )
    try:
        with tempfile.TemporaryDirectory(prefix="geometer-browser-validation-") as profile:
            completed = subprocess.run(
                [
                    str(browser),
                    "--headless=new",
                    "--disable-gpu",
                    "--disable-background-networking",
                    "--no-first-run",
                    f"--user-data-dir={profile}",
                    "--virtual-time-budget=15000",
                    "--dump-dom",
                    url,
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
                check=False,
                timeout=args.timeout_seconds,
            )
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)

    if completed.returncode != 0:
        raise RuntimeError(f"headless browser failed with exit code {completed.returncode}\n{completed.stderr}")
    match = re.search(r'<pre id="result">(PASS [^<]+)</pre>', completed.stdout)
    if '<title>PASS</title>' not in completed.stdout or match is None:
        raise RuntimeError(f"full-browser HLR validation did not pass\n{completed.stdout[-2000:]}")
    evidence = {
        "status": "passed",
        "browser_executable": browser.name,
        "result": match.group(1),
    }
    print(json.dumps(evidence, sort_keys=True, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
