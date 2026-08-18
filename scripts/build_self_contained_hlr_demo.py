#!/usr/bin/env python3
"""Build a one-file browser HLR demo.

The development viewer in `examples/wasm/embedded_model_viewer.html` loads
fixtures, Three.js, a Worker script, and Geometer WASM from repo-relative paths.
This script turns that viewer into a literal standalone HTML file under
`dist/wasm/demos/hlr_demo.html`.

Run after `python scripts/build_wasm.py`:

    python scripts/build_self_contained_hlr_demo.py
"""

from __future__ import annotations

import json
import re
from pathlib import Path

from standalone_html import b64, bundle_es_module, data_uri


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "examples" / "wasm" / "embedded_model_viewer.html"
WORKER_SOURCE = ROOT / "examples" / "wasm" / "hlr_projection_worker.js"
SOURCE_MANIFEST = ROOT / "tests" / "fixtures" / "embedded_models_manifest.json"
LOGO_SVG = ROOT / "tests" / "wasm" / "vendor" / "wn" / "logo.svg"
GEOMETER_BROWSER = ROOT / "dist" / "wasm" / "browser"
GEOMETER_JS = GEOMETER_BROWSER / "geometer.js"
GEOMETER_WASM = GEOMETER_BROWSER / "geometer.wasm"
OUT = ROOT / "dist" / "wasm" / "demos" / "hlr_demo.html"
JS_DEPS_DIR = ROOT / ".deps" / "js" / "hlr-demo"
THREE_BUNDLE = JS_DEPS_DIR / "three_hlr_bundle.js"

LOGO_ATTR = 'src="/tests/wasm/vendor/wn/logo.svg"'
DEMO_MODEL_NAMES = {
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
                'import { OrbitControls } from "three/examples/jsm/controls/OrbitControls.js";',
                "globalThis.GeometerHlrDemoDeps = { THREE, GLTFLoader, OrbitControls };",
                "",
            ]
        ),
        encoding="utf-8",
        newline="\n",
    )
    bundle_es_module(entry_js, THREE_BUNDLE, target="es2020")
    return "\n".join(
        line.rstrip() for line in THREE_BUNDLE.read_text(encoding="utf-8").splitlines()
    ) + "\n"


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
    for required in (SOURCE, WORKER_SOURCE, SOURCE_MANIFEST, LOGO_SVG, GEOMETER_JS, GEOMETER_WASM):
        if not required.exists():
            raise SystemExit(f"Missing {required}")

    three_bundle = ensure_three_bundle()
    manifest_json = json.dumps(embedded_manifest(), separators=(",", ":"))
    worker_json = json.dumps(self_contained_worker_source())

    html = SOURCE.read_text(encoding="utf-8")
    html = strip_importmap(html)
    html = replace_once(
        html,
        '  <script type="module">',
        f"  <script>\n{three_bundle}\n  </script>\n  <script>",
    )
    html = replace_once(
        html,
        '    import * as THREE from "three";\n'
        '    import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";\n'
        '    import { OrbitControls } from "three/addons/controls/OrbitControls.js";',
        "    const { THREE, GLTFLoader, OrbitControls } = window.GeometerHlrDemoDeps;",
    )
    html = replace_once(
        html,
        '    const manifestUrl = "/tests/fixtures/embedded_models_manifest.json";',
        f"    const embeddedManifest = {manifest_json};",
    )
    html = replace_once(
        html,
        '    function assetUrl(path) {\n      return `/${path.split("/").map(encodeURIComponent).join("/")}`;\n    }',
        "    function assetUrl(path) {\n"
        '      if (path.startsWith("data:") || path.startsWith("blob:")) return path;\n'
        '      return path.split("/").map(encodeURIComponent).join("/");\n'
        "    }",
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

    assert_self_contained(html)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(html, encoding="utf-8", newline="\n")
    print(f"Wrote {OUT} ({OUT.stat().st_size:,} bytes)")


if __name__ == "__main__":
    main()
