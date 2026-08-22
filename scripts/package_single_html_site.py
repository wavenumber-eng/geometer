#!/usr/bin/env python3
"""Package a self-contained HTML demo as a deploy-unchanged static site."""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
import re
import shutil
import tempfile
from pathlib import Path
from typing import TypedDict

from standalone_html import assert_self_contained


SCRIPT_PATTERN = re.compile(r"<script(?P<attrs>[^>]*)>(?P<body>.*?)</script>", re.DOTALL | re.IGNORECASE)
NON_EXECUTABLE_TYPES = {"application/octet-stream", "application/json", "text/plain"}


class SiteFile(TypedDict):
    path: str
    bytes: int
    sha256: str


class SingleHtmlSiteManifest(TypedDict):
    schema: str
    entrypoint: str
    runtime_files: list[str]
    sha256: str
    files: list[SiteFile]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def csp_hash(value: str) -> str:
    digest = hashlib.sha256(value.encode("utf-8")).digest()
    return f"'sha256-{base64.b64encode(digest).decode('ascii')}'"


def executable_inline_scripts(html: str) -> list[str]:
    scripts: list[str] = []
    for match in SCRIPT_PATTERN.finditer(html):
        attrs = match.group("attrs")
        type_match = re.search(r'\btype=["\']([^"\']+)["\']', attrs, re.IGNORECASE)
        script_type = type_match.group(1).lower() if type_match else "text/javascript"
        if script_type not in NON_EXECUTABLE_TYPES:
            scripts.append(match.group("body"))
    return scripts


def content_security_policies(html: str) -> tuple[str, str]:
    script_hashes = " ".join(csp_hash(script) for script in executable_inline_scripts(html))
    directives = [
        "default-src 'none'",
        f"script-src 'wasm-unsafe-eval' blob: {script_hashes}".rstrip(),
        "worker-src blob:",
        # Demo controls update CSS custom properties through the CSSOM at runtime.
        "style-src 'unsafe-inline'",
        "connect-src data: blob:",
        "img-src data: blob:",
        "font-src data:",
        "object-src 'none'",
        "base-uri 'none'",
    ]
    meta_policy = "; ".join(directives)
    return meta_policy, f"{meta_policy}; frame-ancestors 'none'"


def inject_csp_meta(html: str, policy: str) -> str:
    if 'http-equiv="Content-Security-Policy"' in html:
        raise RuntimeError("Self-contained HTML already declares a CSP meta tag.")
    head = re.search(r"<head>", html, re.IGNORECASE)
    if head is None:
        raise RuntimeError("Self-contained HTML has no <head> element.")
    insertion = f'\n  <meta http-equiv="Content-Security-Policy" content="{policy}">'
    return html[: head.end()] + insertion + html[head.end() :]


def package_single_html_site(source_html: Path, output: Path) -> SingleHtmlSiteManifest:
    html = source_html.read_text(encoding="utf-8")
    assert_self_contained(html)
    meta_policy, header_policy = content_security_policies(html)
    html = inject_csp_meta(html, meta_policy)
    assert_self_contained(html)

    output.parent.mkdir(parents=True, exist_ok=True)
    staging = Path(tempfile.mkdtemp(prefix="single-html-site-stage-", dir=output.parent))
    try:
        (staging / "index.html").write_text(html, encoding="utf-8", newline="\n")
        (staging / "_headers").write_text(
            f"""/*
  X-Content-Type-Options: nosniff
  Referrer-Policy: no-referrer
  Permissions-Policy: camera=(), geolocation=(), microphone=()
  Cache-Control: no-cache
  Content-Security-Policy: {header_policy}

/index.html
  Cache-Control: no-cache
  Content-Type: text/html; charset=utf-8
""",
            encoding="utf-8",
            newline="\n",
        )
        files = sorted(path for path in staging.iterdir() if path.is_file())
        entries: list[SiteFile] = [
            {
                "path": path.name,
                "bytes": path.stat().st_size,
                "sha256": sha256(path),
            }
            for path in files
        ]
        closure = hashlib.sha256(
            "".join(f"{item['path']}\0{item['sha256']}\n" for item in entries).encode("utf-8")
        ).hexdigest()
        manifest: SingleHtmlSiteManifest = {
            "schema": "wn.geometer.single_html_site.a0",
            "entrypoint": "index.html",
            "runtime_files": ["index.html"],
            "sha256": closure,
            "files": entries,
        }
        (staging / "asset-manifest.json").write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
            newline="\n",
        )
        if output.exists():
            shutil.rmtree(output)
        os.replace(staging, output)
        return manifest
    finally:
        if staging.exists():
            shutil.rmtree(staging)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_html", type=Path, help="Self-contained source HTML file")
    parser.add_argument("output", type=Path, help="Static-site output directory")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    manifest = package_single_html_site(args.source_html.resolve(), args.output.resolve())
    print(f"Packaged {args.output} (one runtime file, {manifest['sha256']})")


if __name__ == "__main__":
    main()
