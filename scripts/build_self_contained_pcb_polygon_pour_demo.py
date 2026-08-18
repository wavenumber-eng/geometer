#!/usr/bin/env python3
"""Build the interactive PCB polygon-pour demo as one directly openable HTML file."""

from __future__ import annotations

import argparse
import hashlib
import tempfile
from pathlib import Path

from standalone_html import (
    ROOT,
    assert_self_contained,
    b64,
    bundle_es_module,
    data_uri,
    inline_script,
    replace_once,
    script_carrier,
)


SOURCE = ROOT / "examples" / "wasm" / "pcb_polygon_pour_demo.html"
STYLE = ROOT / "examples" / "wasm" / "geometer_demo.css"
FONT_REGULAR = ROOT / "docs" / "design" / "assets" / "fonts" / "JetBrainsMono" / "JetBrainsMono-Regular.woff2"
FONT_BOLD = FONT_REGULAR.with_name("JetBrainsMono-Bold.woff2")
FONT_LICENSE = FONT_REGULAR.with_name("OFL.txt")
LOGO = ROOT / "docs" / "design" / "assets" / "wn_logo_w_text__for_light.svg"
DEMOS = ROOT / "dist" / "wasm" / "demos"
BROWSER = ROOT / "dist" / "wasm" / "browser"
PACKAGE = ROOT / "dist" / "wasm" / "npm" / "geometer"
OUTPUT = DEMOS / "pcb_polygon_pour_demo.html"

STYLE_LINK = '    <link rel="stylesheet" href="/examples/wasm/geometer_demo.css" />'
GUARD_SCRIPT = '    <script src="/dist/wasm/demos/analytic_polygon_pour_bootstrap_guard.js"></script>\n'
IMPORT_MAP = (
    '    <script type="importmap">\n'
    '      {"imports":{"@wavenumber/geometer/worker":"/dist/wasm/npm/geometer/worker.js"}}\n'
    "    </script>\n"
)
APP_SCRIPT = '    <script type="module" src="/dist/wasm/demos/pcb_polygon_pour_demo.js"></script>'


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    return parser.parse_args()


def make_worker_source(worker_bundle: str, glue: bytes) -> str:
    return "\n".join(
        [
            '"use strict";',
            f'const GEOMETER_JS_B64 = "{b64(glue)}";',
            "function decodeGeometerBase64(value) {",
            "  const binary = atob(value);",
            "  const bytes = new Uint8Array(binary.length);",
            "  for (let index = 0; index < binary.length; index += 1) bytes[index] = binary.charCodeAt(index);",
            "  return bytes;",
            "}",
            "const geometerScriptText = new TextDecoder().decode(decodeGeometerBase64(GEOMETER_JS_B64));",
            'const geometerScriptUrl = URL.createObjectURL(new Blob([geometerScriptText], { type: "text/javascript" }));',
            "try { importScripts(geometerScriptUrl); } finally { URL.revokeObjectURL(geometerScriptUrl); }",
            worker_bundle,
        ]
    )


def build_html() -> str:
    required = [
        SOURCE,
        STYLE,
        FONT_REGULAR,
        FONT_BOLD,
        FONT_LICENSE,
        LOGO,
        DEMOS / "pcb_polygon_pour_demo.js",
        BROWSER / "geometer.js",
        BROWSER / "geometer.wasm",
        PACKAGE / "worker.js",
        PACKAGE / "worker-host.js",
    ]
    missing = [path for path in required if not path.is_file()]
    if missing:
        raise RuntimeError(f"Build WASM and TypeScript examples first; missing: {missing}")

    with tempfile.TemporaryDirectory(prefix="pcb-pour-standalone-", dir=DEMOS) as raw_stage:
        stage = Path(raw_stage)
        main_bundle = stage / "main.js"
        worker_entry = stage / "worker-entry.js"
        worker_bundle = stage / "worker.js"
        worker_entry.write_text(
            "\n".join(
                [
                    'import { startGeometerWorkerHost } from "@wavenumber/geometer/worker-host";',
                    "const scope = globalThis;",
                    "try {",
                    "  const factory = scope.createGeometerModule;",
                    '  if (typeof factory !== "function") throw new Error("Embedded Geometer module factory is unavailable.");',
                    "  startGeometerWorkerHost(factory, scope);",
                    '  scope.postMessage({ kind: "ready", protocol: "wn.geometer.worker_bootstrap.a0" });',
                    "} catch (error) {",
                    '  scope.postMessage({ kind: "error", message: error instanceof Error ? error.message : String(error), protocol: "wn.geometer.worker_bootstrap.a0" });',
                    "}",
                    "",
                ]
            ),
            encoding="utf-8",
            newline="\n",
        )
        bundle_es_module(
            DEMOS / "pcb_polygon_pour_demo.js",
            main_bundle,
            aliases={"@wavenumber/geometer/worker": PACKAGE / "worker.js"},
            defines={"GEOMETER_STANDALONE_BUILD": "true"},
        )
        bundle_es_module(
            worker_entry,
            worker_bundle,
            aliases={"@wavenumber/geometer/worker-host": PACKAGE / "worker-host.js"},
        )

        css = STYLE.read_text(encoding="utf-8")
        for source, mime in ((FONT_REGULAR, "font/woff2"), (FONT_BOLD, "font/woff2")):
            css = css.replace(
                f'url("/docs/design/assets/fonts/JetBrainsMono/{source.name}")',
                f'url("{data_uri(source, mime)}")',
            )
        css = css.replace(
            'url("/docs/design/assets/wn_logo_w_text__for_light.svg")',
            f'url("{data_uri(LOGO, "image/svg+xml")}")',
        )
        html = SOURCE.read_text(encoding="utf-8")
        html = replace_once(html, STYLE_LINK, f"    <style>\n{css}\n    </style>", label="style")
        html = replace_once(html, GUARD_SCRIPT, "", label="bootstrap guard")
        html = replace_once(html, IMPORT_MAP, "", label="import map")
        worker_source = make_worker_source(
            worker_bundle.read_text(encoding="utf-8"),
            (BROWSER / "geometer.js").read_bytes(),
        ).encode("utf-8")
        application = "\n".join(
            [
                '<script id="jetbrains-mono-license" type="text/plain" '
                f'data-sha256="{hashlib.sha256(FONT_LICENSE.read_bytes()).hexdigest()}">'
                f"{FONT_LICENSE.read_text(encoding='utf-8')}</script>",
                script_carrier("geometer-pcb-wasm", (BROWSER / "geometer.wasm").read_bytes()),
                script_carrier("geometer-pcb-worker", worker_source),
                f"<script>\n{inline_script(main_bundle.read_text(encoding='utf-8'))}\n</script>",
            ]
        )
        html = replace_once(html, APP_SCRIPT, application, label="application script")
        assert_self_contained(html)
        return html.replace("\r\n", "\n")


def main() -> None:
    args = parse_args()
    encoded = build_html().encode("utf-8")
    digest = hashlib.sha256(encoded).hexdigest()
    if args.check:
        if not OUTPUT.is_file() or OUTPUT.read_bytes() != encoded:
            raise SystemExit("Standalone PCB polygon-pour demo is stale; run this builder.")
        print(f"Standalone PCB polygon-pour demo is current ({digest}).")
        return
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    temporary = OUTPUT.with_suffix(".html.tmp")
    temporary.write_bytes(encoded)
    temporary.replace(OUTPUT)
    print(f"Wrote {OUTPUT.relative_to(ROOT)} ({len(encoded):,} bytes, {digest})")


if __name__ == "__main__":
    main()
