#!/usr/bin/env python3
"""Build a one-file browser HLR demo.

The development viewer in `examples/wasm/embedded_model_viewer.html` and its
application module load fixtures, Three.js, a Worker script, and Geometer WASM
from repo-relative paths. This script turns that viewer into a literal
standalone HTML file under `dist/wasm/demos/hlr_demo.html`.

Run after `python scripts/build_wasm.py`:

    python scripts/build_self_contained_hlr_demo.py
"""

from __future__ import annotations

import hashlib
import json
import re
import textwrap
from pathlib import Path

from standalone_html import b64, bundle_es_module, data_uri


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "examples" / "wasm" / "embedded_model_viewer.html"
APP_SOURCE = ROOT / "examples" / "wasm" / "embedded_model_viewer.js"
VIEW_SOURCE = ROOT / "examples" / "wasm" / "hlr_projection_views.js"
WORKER_SOURCE = ROOT / "examples" / "wasm" / "hlr_projection_worker.js"
OPERATION_WORKER_SOURCE = ROOT / "examples" / "wasm" / "geometer_operation_worker.js"
PANEL_SOURCE = ROOT / "examples" / "wasm" / "demo-tooling" / "panels.ts"
PANEL_STYLES = ROOT / "examples" / "wasm" / "demo-tooling" / "panels.css"
SOURCE_MANIFEST = ROOT / "tests" / "fixtures" / "embedded_models_manifest.json"
LOGO_SVG = ROOT / "tests" / "wasm" / "vendor" / "wn" / "logo.svg"
GEOMETER_BROWSER = ROOT / "dist" / "wasm" / "browser"
GEOMETER_JS = GEOMETER_BROWSER / "geometer.js"
GEOMETER_WASM = GEOMETER_BROWSER / "geometer.wasm"
OUT = ROOT / "dist" / "wasm" / "demos" / "hlr_demo.html"
JS_DEPS_DIR = ROOT / ".deps" / "js" / "hlr-demo"
THREE_BUNDLE = JS_DEPS_DIR / "three_hlr_bundle.js"
PANEL_BUNDLE = JS_DEPS_DIR / "demo_panels_bundle.js"
THREE_LICENSE = ROOT / "node_modules" / "three" / "LICENSE"

LOGO_ATTR = 'src="/tests/wasm/vendor/wn/logo.svg"'
PANEL_STYLESHEET = '<link rel="stylesheet" href="/examples/wasm/demo-tooling/panels.css">'
DEMO_MODEL_NAMES = {
    "ABM8-272-T3.STEP",
    "BGA90-8X13mm.step",
    "SOT-23.STEP",
    "sot223.stp",
    "SOIC-8-W.step",
    "TSOT-23-5.STEP",
}


def ensure_three_bundle() -> str:
    JS_DEPS_DIR.mkdir(parents=True, exist_ok=True)
    entry_js = JS_DEPS_DIR / "entry.js"
    entry_js.write_text(
        "\n".join(
            [
                'import * as THREE from "three";',
                'import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader.js";',
                'import { TrackballControls } from "three/examples/jsm/controls/TrackballControls.js";',
                "globalThis.GeometerHlrDemoDeps = { THREE, GLTFLoader, TrackballControls };",
                "",
            ]
        ),
        encoding="utf-8",
        newline="\n",
    )
    bundle_es_module(entry_js, THREE_BUNDLE, target="es2020")
    return "\n".join(line.rstrip() for line in THREE_BUNDLE.read_text(encoding="utf-8").splitlines()) + "\n"


def ensure_panel_bundle() -> str:
    JS_DEPS_DIR.mkdir(parents=True, exist_ok=True)
    entry_ts = JS_DEPS_DIR / "demo_panels_entry.ts"
    entry_ts.write_text(
        "\n".join(
            [
                f'import {{ PanelManager }} from "{PANEL_SOURCE.as_posix()}";',
                "globalThis.GeometerDemoPanels = { PanelManager };",
                "",
            ]
        ),
        encoding="utf-8",
        newline="\n",
    )
    bundle_es_module(entry_ts, PANEL_BUNDLE, target="es2020")
    return "\n".join(line.rstrip() for line in PANEL_BUNDLE.read_text(encoding="utf-8").splitlines()) + "\n"


def embedded_manifest() -> list[dict[str, object]]:
    manifest = json.loads(SOURCE_MANIFEST.read_text(encoding="utf-8-sig"))
    selected = [entry for entry in manifest if entry.get("name") in DEMO_MODEL_NAMES]
    missing = sorted(DEMO_MODEL_NAMES - {entry.get("name") for entry in selected})
    if missing:
        raise SystemExit(f"Demo manifest missing model(s): {', '.join(missing)}")

    result = []
    for entry in selected:
        step_path = ROOT / entry["step"]
        glb_path = ROOT / entry["glb"]
        result.append(
            {
                "name": entry["name"],
                "step": data_uri(step_path, "application/step"),
                "glb": data_uri(glb_path, "model/gltf-binary"),
                "stepBytes": step_path.stat().st_size,
                "glbBytes": glb_path.stat().st_size,
            }
        )
    return result


def self_contained_worker_source() -> str:
    worker = WORKER_SOURCE.read_text(encoding="utf-8")
    operation_worker = OPERATION_WORKER_SOURCE.read_text(encoding="utf-8")
    edge_index = worker.index("const EDGE_FLAGS")
    worker_tail = worker[edge_index:]
    return "\n".join(
        [
            f'const GEOMETER_JS_B64 = "{b64(GEOMETER_JS.read_bytes())}";',
            f'const GEOMETER_WASM_B64 = "{b64(GEOMETER_WASM.read_bytes())}";',
            "",
            'let activeBackend = "embedded";',
            "let modulePromise = null;",
            "",
            "function decodeBase64Bytes(value) {",
            "  const binary = atob(value);",
            "  const bytes = new Uint8Array(binary.length);",
            "  for (let index = 0; index < binary.length; index += 1) bytes[index] = binary.charCodeAt(index);",
            "  return bytes;",
            "}",
            "",
            "function geometerModule() {",
            "  if (!modulePromise) {",
            "    const scriptBytes = decodeBase64Bytes(GEOMETER_JS_B64);",
            "    const scriptText = new TextDecoder().decode(scriptBytes);",
            '    const scriptUrl = URL.createObjectURL(new Blob([scriptText], { type: "text/javascript" }));',
            "    importScripts(scriptUrl);",
            "    URL.revokeObjectURL(scriptUrl);",
            "    modulePromise = self.createGeometerModule({",
            "      wasmBinary: decodeBase64Bytes(GEOMETER_WASM_B64),",
            "    });",
            "  }",
            "  return modulePromise;",
            "}",
            "",
            operation_worker,
            "",
            worker_tail,
        ]
    )


def replace_once(text: str, old: str, new: str) -> str:
    if old not in text:
        raise SystemExit(f"Expected HTML anchor not found: {old[:90]!r}")
    return text.replace(old, new, 1)


def strip_importmap(html: str) -> str:
    updated, count = re.subn(r"\n\s*<script type=\"importmap\">.*?</script>\n", "\n", html, flags=re.DOTALL)
    if count != 1:
        raise SystemExit("Expected exactly one importmap block in HLR demo source.")
    return updated


def assert_self_contained(html: str) -> None:
    forbidden = [
        'src="/',
        'href="/',
        "https://cdn.jsdelivr.net",
        "/examples/wasm/",
        "/tests/fixtures/",
        "/dist/wasm/",
        "embedded_models_manifest.json",
        "hlr_projection_worker.js",
    ]
    hits = [needle for needle in forbidden if needle in html]
    if hits:
        raise SystemExit(f"Generated HLR demo is not self-contained; found: {', '.join(hits)}")


def main() -> None:
    for required in (
        SOURCE,
        APP_SOURCE,
        VIEW_SOURCE,
        WORKER_SOURCE,
        OPERATION_WORKER_SOURCE,
        PANEL_SOURCE,
        PANEL_STYLES,
        SOURCE_MANIFEST,
        LOGO_SVG,
        GEOMETER_JS,
        GEOMETER_WASM,
        THREE_LICENSE,
    ):
        if not required.exists():
            raise SystemExit(f"Missing {required}")

    three_bundle = ensure_three_bundle()
    panel_bundle = ensure_panel_bundle()
    manifest_json = json.dumps(embedded_manifest(), separators=(",", ":"))
    worker_json = json.dumps(self_contained_worker_source())

    html = SOURCE.read_text(encoding="utf-8")
    view_source = VIEW_SOURCE.read_text(encoding="utf-8").replace("export ", "")
    app_text = APP_SOURCE.read_text(encoding="utf-8").replace(
        'import { AXIS_VECTORS, buildProjectionViews } from "./hlr_projection_views.js";\n',
        view_source,
    )
    app_source = textwrap.indent(app_text, "    ")
    html = replace_once(
        html,
        '  <script type="module" src="/examples/wasm/embedded_model_viewer.js"></script>',
        f'  <script type="module">\n{app_source}\n  </script>',
    )
    html = strip_importmap(html)
    html = replace_once(
        html,
        f"  {PANEL_STYLESHEET}",
        f"  <style>\n{PANEL_STYLES.read_text(encoding='utf-8')}\n  </style>",
    )
    html = replace_once(
        html,
        '  <script type="module">',
        f"  <script>\n{three_bundle}\n{panel_bundle}\n  </script>\n  <script>",
    )
    html = replace_once(
        html,
        '    import * as THREE from "three";\n'
        '    import { TrackballControls } from "three/addons/controls/TrackballControls.js";\n'
        '    import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";',
        "    const { THREE, GLTFLoader, TrackballControls } = window.GeometerHlrDemoDeps;",
    )
    html = replace_once(
        html,
        '    import { PanelManager } from "/dist/wasm/demos/demo-tooling/panels.js";',
        "    const { PanelManager } = window.GeometerDemoPanels;",
    )
    html = replace_once(
        html,
        '    const manifestUrl = "/tests/fixtures/embedded_models_manifest.json";',
        f"    const embeddedManifest = {manifest_json};",
    )
    html = replace_once(
        html,
        '      state.projectionWorker = new Worker("/examples/wasm/hlr_projection_worker.js");',
        "      state.projectionWorker = new Worker(createProjectionWorkerUrl());",
    )
    html = replace_once(
        html,
        "    function ensureProjectionWorker(backend) {",
        "    const projectionWorkerSource = " + worker_json + ";\n"
        '    let projectionWorkerUrl = "";\n'
        "    function createProjectionWorkerUrl() {\n"
        "      if (!projectionWorkerUrl) {\n"
        '        projectionWorkerUrl = URL.createObjectURL(new Blob([projectionWorkerSource], { type: "text/javascript" }));\n'
        "      }\n"
        "      return projectionWorkerUrl;\n"
        "    }\n\n"
        "    function ensureProjectionWorker(backend) {",
    )
    html = replace_once(
        html,
        '      setStatus("Loading manifest", true, "Reading embedded model fixture list.");\n'
        "      const response = await fetch(manifestUrl);\n"
        "      if (!response.ok) throw new Error(`Manifest fetch failed: ${response.status}`);\n"
        "      state.models = await response.json();",
        '      setStatus("Loading bundled models", true, "Preparing embedded demo model list.");\n'
        "      state.models = embeddedManifest;",
    )
    if LOGO_ATTR in html:
        logo_uri = f"data:image/svg+xml;base64,{b64(LOGO_SVG.read_bytes())}"
        html = html.replace(LOGO_ATTR, f'src="{logo_uri}"', 1)

    three_license = THREE_LICENSE.read_text(encoding="utf-8")
    license_digest = hashlib.sha256(three_license.encode("utf-8")).hexdigest()
    html = replace_once(
        html,
        "\n</body>",
        f'\n  <script id="three-license" type="text/plain" data-sha256="{license_digest}">'
        f"{three_license}</script>\n</body>",
    )

    assert_self_contained(html)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(html, encoding="utf-8", newline="\n")
    print(f"Wrote {OUT} ({OUT.stat().st_size:,} bytes)")


if __name__ == "__main__":
    main()
