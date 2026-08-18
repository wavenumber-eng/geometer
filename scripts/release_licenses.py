"""Locate the governed license inputs included in release artifacts."""

from __future__ import annotations

from pathlib import Path


def _occt_license_source(root: Path, name: str, platform: str | None) -> Path:
    candidates = [root / ".deps" / "occt-src" / name]
    if platform:
        candidates.append(root / ".deps" / "native" / platform / "occt-install" / name)
    candidates.append(root / ".deps" / "occt-wasm-install" / "share" / "doc" / "opencascade" / name)
    return next((path for path in candidates if path.is_file()), candidates[0])


def release_license_sources(root: Path, platform: str | None) -> dict[str, Path]:
    return {
        "WN_GEOMETER_LICENSE.txt": root / "LICENSE",
        "THIRD_PARTY_NOTICES.md": root / "THIRD_PARTY_NOTICES.md",
        "CLIPPER2_LICENSE.txt": root / "third_party" / "clipper2" / "LICENSE",
        "RAPIDJSON_LICENSE.txt": root / "third_party" / "rapidjson" / "license.txt",
        "OCCT_LICENSE_LGPL_21.txt": _occt_license_source(root, "LICENSE_LGPL_21.txt", platform),
        "OCCT_LGPL_EXCEPTION.txt": _occt_license_source(root, "OCCT_LGPL_EXCEPTION.txt", platform),
    }
