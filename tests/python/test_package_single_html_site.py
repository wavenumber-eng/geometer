from __future__ import annotations

import json
import sys
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from package_single_html_site import csp_hash, package_single_html_site  # noqa: E402


HTML = """<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><style>body { color: black; }</style></head>
<body><img alt="embedded" src="data:image/svg+xml;base64,PHN2Zy8+">
<script>document.body.dataset.ready = "true";</script></body>
</html>
"""


def test_packages_one_runtime_file_deterministically(tmp_path: Path) -> None:
    source = tmp_path / "demo.html"
    output = tmp_path / "site"
    source.write_text(HTML, encoding="utf-8", newline="\n")

    first = package_single_html_site(source, output)
    first_index = (output / "index.html").read_bytes()
    second = package_single_html_site(source, output)

    assert first == second
    assert first["schema"] == "wn.geometer.single_html_site.a0"
    assert first["runtime_files"] == ["index.html"]
    assert [item["path"] for item in first["files"]] == ["_headers", "index.html"]
    assert (output / "index.html").read_bytes() == first_index
    assert {path.name for path in output.iterdir()} == {
        "_headers",
        "asset-manifest.json",
        "index.html",
    }

    index = first_index.decode("utf-8")
    headers = (output / "_headers").read_text(encoding="utf-8")
    assert 'http-equiv="Content-Security-Policy"' in index
    assert csp_hash('document.body.dataset.ready = "true";') in index
    assert csp_hash('document.body.dataset.ready = "true";') in headers
    assert "worker-src blob:" in headers
    assert "frame-ancestors 'none'" in headers

    persisted = json.loads((output / "asset-manifest.json").read_text(encoding="utf-8"))
    assert persisted == first


def test_rejects_external_runtime_dependency(tmp_path: Path) -> None:
    source = tmp_path / "external.html"
    source.write_text(
        '<!doctype html><html><head></head><body><script src="https://example.com/demo.js"></script></body></html>',
        encoding="utf-8",
    )

    with pytest.raises(RuntimeError, match="external references"):
        package_single_html_site(source, tmp_path / "site")
