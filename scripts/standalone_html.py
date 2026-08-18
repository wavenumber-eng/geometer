"""Shared primitives for deterministic, single-file browser demos."""

from __future__ import annotations

import base64
import hashlib
import json
import os
import shutil
import subprocess
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urlsplit


ROOT = Path(__file__).resolve().parents[1]
ESBUILD_VERSION = "0.27.2"


def _esbuild_command(
    node: str, executable: Path, *, platform_name: str = os.name
) -> tuple[str, ...]:
    if platform_name == "nt":
        return (node, str(executable))
    return (str(executable),)


def b64(data: bytes) -> str:
    return base64.b64encode(data).decode("ascii")


def data_uri(path: Path, mime: str) -> str:
    return f"data:{mime};base64,{b64(path.read_bytes())}"


def replace_once(text: str, old: str, new: str, *, label: str = "template") -> str:
    if old not in text:
        raise RuntimeError(f"Expected {label} anchor not found: {old[:90]!r}")
    return text.replace(old, new, 1)


def inline_script(text: str) -> str:
    """Prevent an inline classic script from terminating its own HTML element."""

    return text.replace("</script", "<\\/script").replace("</SCRIPT", "<\\/SCRIPT")


def script_carrier(carrier_id: str, data: bytes) -> str:
    return (
        f'<script id="{carrier_id}" type="application/octet-stream" '
        f'data-encoding="base64" data-bytes="{len(data)}" '
        f'data-sha256="{hashlib.sha256(data).hexdigest()}">{b64(data)}</script>'
    )


def ensure_esbuild() -> tuple[str, ...]:
    node = shutil.which("node")
    if node is None:
        raise RuntimeError("Node.js is required to bundle standalone browser demos.")
    executable = ROOT / "node_modules" / "esbuild" / "bin" / "esbuild"
    if not executable.is_file():
        raise RuntimeError(
            f"esbuild {ESBUILD_VERSION} is unavailable; run npm install from the repository root."
        )
    command = _esbuild_command(node, executable)
    version = subprocess.run(
        [*command, "--version"],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    if version != ESBUILD_VERSION:
        raise RuntimeError(f"Expected esbuild {ESBUILD_VERSION}, found {version}.")
    return command


def bundle_es_module(
    entry: Path,
    output: Path,
    *,
    aliases: dict[str, Path] | None = None,
    defines: dict[str, str] | None = None,
    target: str = "es2022",
) -> None:
    esbuild = ensure_esbuild()
    output.parent.mkdir(parents=True, exist_ok=True)
    metafile = output.with_suffix(output.suffix + ".meta.json")
    command = [
        *esbuild,
        str(entry),
        "--bundle",
        "--format=iife",
        "--platform=browser",
        f"--target={target}",
        "--minify",
        "--log-level=warning",
        "--legal-comments=none",
        f"--metafile={metafile}",
        f"--outfile={output}",
    ]
    for name, path in sorted((aliases or {}).items()):
        command.append(f"--alias:{name}={path.resolve()}")
    for name, value in sorted((defines or {}).items()):
        command.append(f"--define:{name}={value}")
    subprocess.run(command, cwd=ROOT, check=True)
    metadata = json.loads(metafile.read_text(encoding="utf-8"))
    external = [
        item["path"]
        for details in metadata["outputs"].values()
        for item in details.get("imports", [])
        if item.get("external")
    ]
    if external:
        raise RuntimeError(f"Standalone bundle retained external imports: {external}")
    metafile.unlink()


class _ExternalReferenceParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=False)
        self.references: list[tuple[str, str, str]] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        for name, value in attrs:
            if value is None:
                continue
            if name in {"src", "href", "poster", "data"}:
                self.references.append((tag, name, value))


def assert_self_contained(html: str) -> None:
    """Reject network/file dependencies in a generated single-HTML artifact."""

    parser = _ExternalReferenceParser()
    parser.feed(html)
    external = []
    for tag, name, value in parser.references:
        scheme = urlsplit(value).scheme.lower()
        if value.startswith("#") or scheme == "data":
            continue
        external.append(f"<{tag} {name}={value!r}>")
    if external:
        raise RuntimeError("Standalone HTML contains external references: " + ", ".join(external))
    if '<script type="module"' in html or '<script type="importmap"' in html:
        raise RuntimeError("Standalone HTML still contains an ESM/import-map dependency.")
