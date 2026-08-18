"""Build the deploy-unchanged interactive PCB polygon-pour static site."""

from __future__ import annotations

import base64
import hashlib
import json
import os
import re
import shutil
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "dist" / "wasm" / "demos" / "pcb-polygon-pour"


def rewrite(source: Path, destination: Path, replacements: dict[str, str]) -> None:
    text = source.read_text(encoding="utf-8")
    for old, new in replacements.items():
        if old not in text:
            raise RuntimeError(f"Expected {old!r} in {source}")
        text = text.replace(old, new)
    destination.write_text(text, encoding="utf-8", newline="\n")


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    demos = ROOT / "dist" / "wasm" / "demos"
    browser = ROOT / "dist" / "wasm" / "browser"
    package = ROOT / "dist" / "wasm" / "npm" / "geometer"
    source = ROOT / "examples" / "wasm"
    demo_files = [
        "analytic_canvas_arc.js",
        "analytic_polygon_pour_bootstrap_guard.js",
        "pcb_polygon_pour_demo.js",
        "pcb_polygon_pour_model.js",
        "pcb_polygon_pour_worker.js",
    ]
    tooling_files = [
        "animation.js",
        "camera2d.js",
        "commands.js",
        "geometry.js",
        "history.js",
        "index.js",
        "input.js",
        "tool-controller.js",
    ]
    required = [browser / "geometer.js", browser / "geometer.wasm", package / "worker.js"]
    required += [demos / item for item in demo_files]
    required += [demos / "demo-tooling" / item for item in tooling_files]
    missing = [path for path in required if not path.is_file()]
    if missing:
        raise FileNotFoundError(f"Build WASM and TypeScript examples first: {missing}")

    staging = Path(tempfile.mkdtemp(prefix="pcb-pour-stage-", dir=demos))
    try:
        rewrite(
            source / "pcb_polygon_pour_demo.html",
            staging / "index.html",
            {
                "/examples/wasm/geometer_demo.css": "./geometer_demo.css",
                "/dist/wasm/npm/geometer/worker.js": "./package/worker.js",
                "/dist/wasm/demos/analytic_polygon_pour_bootstrap_guard.js": "./analytic_polygon_pour_bootstrap_guard.js",
                "/dist/wasm/demos/pcb_polygon_pour_demo.js": "./pcb_polygon_pour_demo.js",
            },
        )
        rewrite(
            source / "geometer_demo.css",
            staging / "geometer_demo.css",
            {
                "/docs/design/assets/fonts/JetBrainsMono/JetBrainsMono-Regular.woff2": "./JetBrainsMono-Regular.woff2",
                "/docs/design/assets/fonts/JetBrainsMono/JetBrainsMono-Bold.woff2": "./JetBrainsMono-Bold.woff2",
                "/docs/design/assets/wn_logo_w_text__for_light.svg": "./wn-logo.svg",
            },
        )
        rewrite(
            demos / "pcb_polygon_pour_demo.js",
            staging / "pcb_polygon_pour_demo.js",
            {
                "/dist/wasm/browser/geometer.wasm": "./geometer.wasm",
                "/dist/wasm/demos/pcb_polygon_pour_worker.js": "./pcb_polygon_pour_worker.js",
            },
        )
        rewrite(
            demos / "pcb_polygon_pour_worker.js",
            staging / "pcb_polygon_pour_worker.js",
            {
                "/dist/wasm/browser/geometer.js": "./geometer.js",
                "/dist/wasm/npm/geometer/worker-host.js": "./package/worker-host.js",
            },
        )
        for item in ["analytic_canvas_arc.js", "analytic_polygon_pour_bootstrap_guard.js", "pcb_polygon_pour_model.js"]:
            shutil.copy2(demos / item, staging / item)
        shutil.copytree(demos / "demo-tooling", staging / "demo-tooling")
        shutil.copy2(browser / "geometer.js", staging / "geometer.js")
        shutil.copy2(browser / "geometer.wasm", staging / "geometer.wasm")
        for name in ["JetBrainsMono-Regular.woff2", "JetBrainsMono-Bold.woff2"]:
            shutil.copy2(ROOT / "docs" / "design" / "assets" / "fonts" / "JetBrainsMono" / name, staging / name)
        (staging / "JetBrainsMono-OFL.txt").write_text(
            (ROOT / "docs" / "design" / "assets" / "fonts" / "JetBrainsMono" / "OFL.txt").read_text(encoding="utf-8"),
            encoding="utf-8",
            newline="\n",
        )
        shutil.copy2(ROOT / "docs" / "design" / "assets" / "wn_logo_w_text__for_light.svg", staging / "wn-logo.svg")
        shutil.copytree(package, staging / "package")

        index_text = (staging / "index.html").read_text(encoding="utf-8")
        import_map = re.search(r'<script type="importmap">(.*?)</script>', index_text, re.DOTALL)
        if import_map is None:
            raise RuntimeError("PCB demo import map is missing")
        import_map_hash = base64.b64encode(hashlib.sha256(import_map.group(1).encode()).digest()).decode()
        (staging / "_headers").write_text(
            f"""/*
  X-Content-Type-Options: nosniff
  Referrer-Policy: no-referrer
  Permissions-Policy: camera=(), geolocation=(), microphone=()
  Cache-Control: no-cache
  Content-Security-Policy: default-src 'self'; script-src 'self' 'wasm-unsafe-eval' 'sha256-{import_map_hash}'; worker-src 'self'; style-src 'self'; font-src 'self'; connect-src 'self'; img-src 'self' data:; object-src 'none'; base-uri 'none'; frame-ancestors 'none'

/geometer.wasm
  Cache-Control: no-cache
  Content-Type: application/wasm

/*.js
  Cache-Control: no-cache
  Content-Type: text/javascript; charset=utf-8

/package/*.js
  Cache-Control: no-cache
  Content-Type: text/javascript; charset=utf-8
""",
            encoding="utf-8",
            newline="\n",
        )
        files = sorted(path for path in staging.rglob("*") if path.is_file())
        entries = [
            {"path": path.relative_to(staging).as_posix(), "bytes": path.stat().st_size, "sha256": digest(path)}
            for path in files
        ]
        closure = hashlib.sha256(
            "".join(f"{item['path']}\0{item['sha256']}\n" for item in entries).encode()
        ).hexdigest()
        (staging / "asset-manifest.json").write_text(
            json.dumps(
                {"schema": "wn.geometer.static_site.a0", "sha256": closure, "files": entries}, indent=2, sort_keys=True
            )
            + "\n",
            encoding="utf-8",
            newline="\n",
        )
        if OUTPUT.exists():
            shutil.rmtree(OUTPUT)
        os.replace(staging, OUTPUT)
        print(f"Built {OUTPUT.relative_to(ROOT)} ({len(entries) + 1} files, {closure})")
    finally:
        if staging.exists():
            shutil.rmtree(staging)


if __name__ == "__main__":
    main()
