#!/usr/bin/env python3
"""Bake the planar ring solver browser demo into one self-contained HTML file.

The source example in `examples/wasm/planar_ring_solver_demo.html` can run from
a local web server by loading the planar-only WASM bundle from `dist/wasm/`.
This script injects base64 carriers for that JS/WASM pair so
`dist/wasm/demos/planar_ring_solver_demo.html` can be opened directly from disk.

Run:  python scripts/build_self_contained_planar_ring_solver_demo.py
"""

from __future__ import annotations

from pathlib import Path

from standalone_html import b64


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "examples" / "wasm" / "planar_ring_solver_demo.html"
PLANAR_BROWSER = ROOT / "dist" / "wasm" / "planar-browser"
GEOMETER_JS = PLANAR_BROWSER / "geometer-planar-browser.js"
GEOMETER_WASM = PLANAR_BROWSER / "geometer-planar-browser.wasm"
LOGO_SVG = ROOT / "tests" / "wasm" / "vendor" / "wn" / "logo.svg"
OUT = ROOT / "dist" / "wasm" / "demos" / "planar_ring_solver_demo.html"

APP_TAG = '  <script id="planarDemoApp">'
LOGO_ATTR = 'src="/tests/wasm/vendor/wn/logo.svg"'


def script_carrier(carrier_id: str, text: str) -> str:
    return f'  <script type="text/plain" id="{carrier_id}">{text}</script>\n'


def main() -> None:
    for required in (SOURCE, GEOMETER_JS, GEOMETER_WASM, LOGO_SVG):
        if not required.exists():
            raise SystemExit(f"Missing {required}. Build the planar WASM bundle first.")

    html = SOURCE.read_text(encoding="utf-8")
    if APP_TAG not in html:
        raise SystemExit(f"Demo anchor not found: {APP_TAG!r}")
    if LOGO_ATTR not in html:
        raise SystemExit(f"Demo logo anchor not found: {LOGO_ATTR!r}")

    carriers = script_carrier("planarGeometerJsB64", b64(GEOMETER_JS.read_bytes())) + script_carrier(
        "planarWasmB64", b64(GEOMETER_WASM.read_bytes())
    )
    html = html.replace(APP_TAG, carriers + APP_TAG, 1)
    html = html.replace(LOGO_ATTR, f'src="data:image/svg+xml;base64,{b64(LOGO_SVG.read_bytes())}"', 1)
    html = make_embedded_only(html)
    assert_self_contained(html)

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(html, encoding="utf-8", newline="\n")
    print(f"Wrote {OUT} ({OUT.stat().st_size:,} bytes)")


def replace_once(text: str, old: str, new: str) -> str:
    if old not in text:
        raise SystemExit(f"Expected demo anchor not found: {old[:90]!r}")
    return text.replace(old, new, 1)


def make_embedded_only(html: str) -> str:
    html = replace_once(
        html,
        "    function loadScript(src) {\n"
        "      return new Promise((resolve, reject) => {\n"
        '        const script = document.createElement("script");\n'
        "        script.src = src;\n"
        "        script.onload = () => resolve(src);\n"
        "        script.onerror = () => {\n"
        "          script.remove();\n"
        "          reject(new Error(`failed to load ${src}`));\n"
        "        };\n"
        "        document.head.appendChild(script);\n"
        "      });\n"
        "    }\n\n"
        "    async function loadExternalPlanarModuleScript() {\n"
        "      const candidates = [\n"
        '        "wasm/planar-browser/geometer-planar-browser.js",\n'
        '        "../../dist/wasm/planar-browser/geometer-planar-browser.js",\n'
        '        "/dist/wasm/planar-browser/geometer-planar-browser.js",\n'
        "      ];\n"
        "      let lastError = null;\n"
        "      for (const candidate of candidates) {\n"
        "        try {\n"
        "          return await loadScript(candidate);\n"
        "        } catch (error) {\n"
        "          lastError = error;\n"
        "        }\n"
        "      }\n"
        '      throw lastError || new Error("geometer-planar-browser.js was not found");\n'
        "    }\n",
        "    function loadScript(src) {\n"
        "      return new Promise((resolve, reject) => {\n"
        '        const script = document.createElement("script");\n'
        "        script.src = src;\n"
        "        script.onload = () => resolve(src);\n"
        "        script.onerror = () => {\n"
        "          script.remove();\n"
        "          reject(new Error(`failed to load ${src}`));\n"
        "        };\n"
        "        document.head.appendChild(script);\n"
        "      });\n"
        "    }\n",
    )
    html = replace_once(
        html,
        "\n"
        "      const scriptSrc = await loadExternalPlanarModuleScript();\n"
        "      const baseUrl = new URL(scriptSrc, window.location.href);\n"
        "      return createGeometerPlanarModule({\n"
        "        locateFile(path) {\n"
        "          return new URL(path, baseUrl).toString();\n"
        "        },\n"
        "      });",
        '\n      throw new Error("Embedded Geometer planar WASM carrier is missing.");',
    )
    return html


def assert_self_contained(html: str) -> None:
    forbidden = [
        'src="/',
        'href="/',
        "/dist/wasm/",
        "wasm/planar-browser/",
        "../../dist/",
        "geometer-planar-browser.js was not found",
    ]
    hits = [needle for needle in forbidden if needle in html]
    if hits:
        raise SystemExit(f"Generated planar demo is not self-contained; found: {', '.join(hits)}")


if __name__ == "__main__":
    main()
