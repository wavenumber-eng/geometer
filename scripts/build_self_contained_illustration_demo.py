#!/usr/bin/env python3
"""Build the single-file Geometer STEP Illustration Lab."""

from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path

from standalone_html import assert_self_contained, b64, bundle_es_module, data_uri, replace_once


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "examples" / "wasm" / "illustration_demo.html"
APP_SOURCE = ROOT / "examples" / "wasm" / "illustration_demo.ts"
WORKER_SOURCE = ROOT / "examples" / "wasm" / "illustration_step_worker.js"
OPERATION_WORKER_SOURCE = ROOT / "examples" / "wasm" / "geometer_operation_worker.js"
SHARED_STYLES = ROOT / "examples" / "wasm" / "geometer_demo.css"
SOURCE_MANIFEST = ROOT / "tests" / "fixtures" / "embedded_models_manifest.json"
GEOMETER_BROWSER = ROOT / "dist" / "wasm" / "browser"
GEOMETER_JS = GEOMETER_BROWSER / "geometer.js"
GEOMETER_WASM = GEOMETER_BROWSER / "geometer.wasm"
FONT_REGULAR = ROOT / "docs" / "design" / "assets" / "fonts" / "JetBrainsMono" / "JetBrainsMono-Regular.woff2"
FONT_BOLD = ROOT / "docs" / "design" / "assets" / "fonts" / "JetBrainsMono" / "JetBrainsMono-Bold.woff2"
LOGO_SVG = ROOT / "docs" / "design" / "assets" / "wn_logo_w_text__for_light.svg"
THREE_LICENSE = ROOT / "node_modules" / "three" / "LICENSE"
STAGING = ROOT / ".deps" / "js" / "illustration-demo"
APP_BUNDLE = STAGING / "illustration_demo_bundle.js"
OUT = ROOT / "dist" / "wasm" / "demos" / "illustration_demo.html"

DEMO_MODEL_NAMES = [
    "SOT-23.STEP",
    "ABM8-272-T3.STEP",
    "SOIC-8-W.step",
    "sot223.stp",
    "Cap_SMT_Aluminum_F.STEP",
    "BGA90-8X13mm.step",
]


def embedded_models() -> list[dict[str, object]]:
    manifest = json.loads(SOURCE_MANIFEST.read_text(encoding="utf-8-sig"))
    by_name = {str(entry.get("name")): entry for entry in manifest}
    selected = [by_name[name] for name in DEMO_MODEL_NAMES if name in by_name]
    missing = sorted(set(DEMO_MODEL_NAMES) - set(by_name))
    if missing:
        raise SystemExit(f"Illustration demo manifest missing: {', '.join(missing)}")
    return [
        {
            "name": entry["name"],
            "step": data_uri(ROOT / entry["step"], "application/step"),
            "glb": data_uri(ROOT / entry["glb"], "model/gltf-binary"),
            "stepBytes": (ROOT / entry["step"]).stat().st_size,
            "glbBytes": (ROOT / entry["glb"]).stat().st_size,
        }
        for entry in selected
    ]


def self_contained_worker() -> str:
    source = WORKER_SOURCE.read_text(encoding="utf-8")
    operation_worker = OPERATION_WORKER_SOURCE.read_text(encoding="utf-8")
    tail_start = source.index("function stepToGlb")
    return "\n".join(
        [
            f'const GEOMETER_JS_B64 = "{b64(GEOMETER_JS.read_bytes())}";',
            f'const GEOMETER_WASM_B64 = "{b64(GEOMETER_WASM.read_bytes())}";',
            "let modulePromise = null;",
            "function decodeBase64(value) {",
            "  const binary = atob(value);",
            "  const bytes = new Uint8Array(binary.length);",
            "  for (let index = 0; index < binary.length; index += 1) bytes[index] = binary.charCodeAt(index);",
            "  return bytes;",
            "}",
            "function geometerModule() {",
            "  if (!modulePromise) {",
            "    const script = new TextDecoder().decode(decodeBase64(GEOMETER_JS_B64));",
            '    const url = URL.createObjectURL(new Blob([script], { type: "text/javascript" }));',
            "    importScripts(url);",
            "    URL.revokeObjectURL(url);",
            "    modulePromise = self.createGeometerModule({ wasmBinary: decodeBase64(GEOMETER_WASM_B64) });",
            "  }",
            "  return modulePromise;",
            "}",
            operation_worker,
            source[tail_start:],
        ]
    )


def embedded_styles() -> str:
    styles = SHARED_STYLES.read_text(encoding="utf-8")
    replacements = {
        "/docs/design/assets/fonts/JetBrainsMono/JetBrainsMono-Regular.woff2": data_uri(FONT_REGULAR, "font/woff2"),
        "/docs/design/assets/fonts/JetBrainsMono/JetBrainsMono-Bold.woff2": data_uri(FONT_BOLD, "font/woff2"),
        "/docs/design/assets/wn_logo_w_text__for_light.svg": data_uri(LOGO_SVG, "image/svg+xml"),
    }
    for source, target in replacements.items():
        if source not in styles:
            raise SystemExit(f"Shared demo style URL not found: {source}")
        styles = styles.replace(source, target)
    return styles


def main() -> None:
    required = (
        SOURCE,
        APP_SOURCE,
        WORKER_SOURCE,
        OPERATION_WORKER_SOURCE,
        SHARED_STYLES,
        SOURCE_MANIFEST,
        GEOMETER_JS,
        GEOMETER_WASM,
        FONT_REGULAR,
        FONT_BOLD,
        LOGO_SVG,
        THREE_LICENSE,
    )
    missing = [path for path in required if not path.exists()]
    if missing:
        raise SystemExit("Missing illustration demo input(s): " + ", ".join(map(str, missing)))

    STAGING.mkdir(parents=True, exist_ok=True)
    bundle_es_module(
        APP_SOURCE,
        APP_BUNDLE,
        target="es2022",
        aliases={
            "@wavenumber/geometer/mesh-illustration": ROOT / "src" / "ts" / "geometer" / "mesh-illustration.ts",
        },
    )
    bundle = "\n".join(line.rstrip() for line in APP_BUNDLE.read_text(encoding="utf-8").splitlines()) + "\n"
    embedded = {
        "models": embedded_models(),
        "workerSource": self_contained_worker(),
    }

    html = SOURCE.read_text(encoding="utf-8")
    html = replace_once(
        html,
        '  <link rel="stylesheet" href="/examples/wasm/geometer_demo.css">',
        f"  <style>\n{embedded_styles()}\n  </style>",
        label="shared illustration stylesheet",
    )
    html, import_count = re.subn(r'\n\s*<script type="importmap">.*?</script>\n', "\n", html, flags=re.DOTALL)
    if import_count != 1:
        raise SystemExit("Expected one illustration demo import map.")
    html = replace_once(
        html,
        '  <script type="module" src="/dist/wasm/demos/illustration_demo.js"></script>',
        "  <script>\n"
        f"window.GeometerIllustrationEmbedded = {json.dumps(embedded, separators=(',', ':'))};\n"
        f"{bundle}\n"
        "  </script>",
        label="illustration application module",
    )
    license_text = THREE_LICENSE.read_text(encoding="utf-8")
    license_digest = hashlib.sha256(license_text.encode("utf-8")).hexdigest()
    html = replace_once(
        html,
        "\n</body>",
        f'\n  <script id="three-license" type="text/plain" data-sha256="{license_digest}">'
        f"{license_text}</script>\n</body>",
        label="body close",
    )
    assert_self_contained(html)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(html, encoding="utf-8", newline="\n")
    print(f"Wrote {OUT} ({OUT.stat().st_size:,} bytes)")


if __name__ == "__main__":
    main()
