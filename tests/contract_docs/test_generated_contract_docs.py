from __future__ import annotations

import json
import os
import re
import subprocess
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urlsplit


ROOT = Path(__file__).resolve().parents[2]
SITE = ROOT / "docs" / "generated" / "contracts"


class _References(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.references: list[tuple[str, str]] = []

    def handle_starttag(
        self,
        tag: str,
        attrs: list[tuple[str, str | None]],
    ) -> None:
        del tag
        for name, value in attrs:
            if name in {"href", "src"} and value:
                self.references.append((name, value))


def _html_references(path: Path) -> list[tuple[str, str]]:
    parser = _References()
    parser.feed(path.read_text(encoding="utf-8"))
    return parser.references


def test_generated_contract_docs_are_current() -> None:
    npm = "npm.cmd" if os.name == "nt" else "npm"
    subprocess.run([npm, "run", "check:docs"], cwd=ROOT, check=True)


def test_generated_contract_docs_are_complete_and_offline() -> None:
    site_manifest = json.loads((SITE / "site-manifest.a0.json").read_text(encoding="utf-8"))
    catalog = json.loads(
        (ROOT / "contracts" / "geometer" / "generated" / "wn_geometer_contract_catalog.a0.json").read_text(
            encoding="utf-8"
        )
    )
    assert site_manifest["generator_identity"] == "wn.geometer.contract_docs"
    assert site_manifest["generation"] == "a0"
    assert set(site_manifest["contracts"]) == {root["contract_identity"] for root in catalog["roots"]}
    assert set(site_manifest["operations"]) == {operation["identity"] for operation in catalog["operations"]}

    html_paths = sorted(SITE.rglob("*.html"))
    expected_pages = {
        SITE / "index.html",
        SITE / site_manifest["coverage"],
        *(SITE / path for path in site_manifest["contracts"].values()),
        *(SITE / path for path in site_manifest["operations"].values()),
        *(SITE / path for path in site_manifest["guides"]),
    }
    assert set(html_paths) == expected_pages

    linked_pages: set[Path] = set()
    for path in html_paths:
        text = path.read_text(encoding="utf-8")
        assert '<meta name="viewport"' in text
        assert 'data-doc-status="generated"' in text
        assert 'data-generated="true"' in text
        assert 'data-wn-watermark="true"' in text
        assert site_manifest["catalog_sha256"] in text
        assert "file:" not in text
        for attribute, reference in _html_references(path):
            parsed = urlsplit(reference)
            if parsed.scheme or parsed.netloc:
                # Reference guides may cite external sources, but the generated
                # site must never load a remote image, script, or stylesheet.
                assert attribute == "href", (path, reference)
                assert parsed.scheme in {"http", "https"} and parsed.netloc, (path, reference)
                continue
            if not parsed.path:
                continue
            target = (path.parent / parsed.path).resolve()
            assert target.is_file(), (path, reference)
            if SITE in target.parents and target.suffix == ".html":
                linked_pages.add(target)

    assert expected_pages - {SITE / "index.html"} <= linked_pages
    assert linked_pages <= expected_pages


def test_vendored_visual_system_is_local_and_responsive() -> None:
    stylesheet_path = ROOT / "docs" / "design" / "styles.css"
    stylesheet = stylesheet_path.read_text(encoding="utf-8")
    assert 'font-family: "Cousine"' in stylesheet
    assert "Berkeley Mono" not in stylesheet
    assert "@media (max-width: 700px)" in stylesheet
    assert 'body[data-wn-watermark="true"]::after' in stylesheet
    assert "http://" not in stylesheet and "https://" not in stylesheet

    urls = re.findall(r'url\(["\']?([^"\')]+)', stylesheet)
    assert urls == [
        "assets/fonts/Cousine/Cousine-Regular.ttf",
        "assets/fonts/Cousine/Cousine-Bold.ttf",
        "assets/wn_logo_w_text__for_light.svg",
    ]
    assert all((stylesheet_path.parent / url).is_file() for url in urls)
    assert (ROOT / "docs" / "design" / "assets" / "fonts" / "Cousine" / "OFL.txt").is_file()
