"""Build the deploy-unchanged analytic polygon-pour static site."""

from __future__ import annotations

import hashlib
import base64
import json
import os
import re
import shutil
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "dist" / "wasm" / "demos" / "analytic-polygon-pour"


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
    required = [
        browser / "geometer.js",
        browser / "geometer.wasm",
        demos / "analytic_canvas_arc.js",
        demos / "analytic_polygon_pour_bootstrap_guard.js",
        demos / "analytic_polygon_pour_demo.js",
        demos / "analytic_polygon_pour_fixture.js",
        demos / "analytic_polygon_pour_worker.js",
        package / "worker.js",
        ROOT / "docs" / "design" / "assets" / "fonts" / "JetBrainsMono" / "OFL.txt",
    ]
    missing = [path for path in required if not path.is_file()]
    if missing:
        raise FileNotFoundError(f"Build WASM and TypeScript examples first: {missing}")

    demos_resolved = demos.resolve()
    if OUTPUT.resolve().parent != demos_resolved:
        raise RuntimeError("Analytic demo output escaped dist/wasm/demos")
    staging = Path(tempfile.mkdtemp(prefix="analytic-pour-stage-", dir=demos))
    try:
        rewrite(
            source / "analytic_polygon_pour_demo.html",
            staging / "index.html",
            {
                "/examples/wasm/geometer_demo.css": "./geometer_demo.css",
                "/dist/wasm/npm/geometer/worker.js": "./package/worker.js",
                "/dist/wasm/demos/analytic_polygon_pour_bootstrap_guard.js": "./analytic_polygon_pour_bootstrap_guard.js",
                "/dist/wasm/demos/analytic_polygon_pour_demo.js": "./analytic_polygon_pour_demo.js",
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
            demos / "analytic_polygon_pour_demo.js",
            staging / "analytic_polygon_pour_demo.js",
            {
                "/dist/wasm/browser/geometer.wasm": "./geometer.wasm",
                "/dist/wasm/demos/analytic_polygon_pour_worker.js": "./analytic_polygon_pour_worker.js",
            },
        )
        rewrite(
            demos / "analytic_polygon_pour_worker.js",
            staging / "analytic_polygon_pour_worker.js",
            {
                "/dist/wasm/browser/geometer.js": "./geometer.js",
                "/dist/wasm/npm/geometer/worker-host.js": "./package/worker-host.js",
            },
        )
        shutil.copy2(demos / "analytic_canvas_arc.js", staging / "analytic_canvas_arc.js")
        shutil.copy2(
            demos / "analytic_polygon_pour_bootstrap_guard.js",
            staging / "analytic_polygon_pour_bootstrap_guard.js",
        )
        shutil.copy2(
            demos / "analytic_polygon_pour_fixture.js",
            staging / "analytic_polygon_pour_fixture.js",
        )
        shutil.copy2(browser / "geometer.js", staging / "geometer.js")
        shutil.copy2(browser / "geometer.wasm", staging / "geometer.wasm")
        shutil.copy2(
            ROOT / "docs" / "design" / "assets" / "fonts" / "JetBrainsMono" / "JetBrainsMono-Regular.woff2",
            staging / "JetBrainsMono-Regular.woff2",
        )
        shutil.copy2(
            ROOT / "docs" / "design" / "assets" / "fonts" / "JetBrainsMono" / "JetBrainsMono-Bold.woff2",
            staging / "JetBrainsMono-Bold.woff2",
        )
        shutil.copy2(
            ROOT / "docs" / "design" / "assets" / "fonts" / "JetBrainsMono" / "OFL.txt",
            staging / "JetBrainsMono-OFL.txt",
        )
        shutil.copy2(
            ROOT / "docs" / "design" / "assets" / "wn_logo_w_text__for_light.svg",
            staging / "wn-logo.svg",
        )
        shutil.copytree(package, staging / "package")
        index_text = (staging / "index.html").read_text(encoding="utf-8")
        import_map = re.search(r'<script type="importmap">(.*?)</script>', index_text, re.DOTALL)
        if import_map is None:
            raise RuntimeError("Analytic demo import map is missing")
        import_map_hash = base64.b64encode(hashlib.sha256(import_map.group(1).encode("utf-8")).digest()).decode("ascii")
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

/index.html
  Cache-Control: no-cache
""",
            encoding="utf-8",
            newline="\n",
        )
        files = sorted(path for path in staging.rglob("*") if path.is_file())
        entries = [
            {
                "path": path.relative_to(staging).as_posix(),
                "bytes": path.stat().st_size,
                "sha256": digest(path),
            }
            for path in files
        ]
        closure = hashlib.sha256(
            "".join(f"{item['path']}\0{item['sha256']}\n" for item in entries).encode("utf-8")
        ).hexdigest()
        (staging / "asset-manifest.json").write_text(
            json.dumps(
                {"schema": "wn.geometer.static_site.a0", "sha256": closure, "files": entries},
                indent=2,
                sort_keys=True,
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
