"""Create deterministic, license-complete Geometer release archives."""

from __future__ import annotations

import argparse
import hashlib
import zipfile
from pathlib import Path

from release_licenses import release_license_sources


ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
FIXED_ZIP_TIME = (1980, 1, 1, 0, 0, 0)


def is_native_runtime_file(path: Path) -> bool:
    """Return whether a native dist file belongs in a runtime archive."""

    return path.is_file() and path.suffix.lower() not in {".a", ".lib"}


def require_license_sources(platform: str | None) -> dict[str, Path]:
    sources = release_license_sources(ROOT, platform)
    missing = [str(path) for path in sources.values() if not path.is_file()]
    if missing:
        raise FileNotFoundError("Missing release license sources: " + ", ".join(missing))
    return sources


def runtime_files(kind: str, platform: str | None) -> list[tuple[Path, str]]:
    if kind == "native":
        if not platform:
            raise ValueError("--platform is required for a native archive")
        source = DIST / "native" / platform
        if not source.is_dir():
            raise FileNotFoundError(f"Missing native distribution directory: {source}")
        files = [
            (path, path.relative_to(source).as_posix())
            for path in source.rglob("*")
            if is_native_runtime_file(path)
        ]
    else:
        files = []
        for relative in ("browser", "node-test", "planar-browser", "npm/geometer"):
            source = DIST / "wasm" / relative
            if not source.is_dir():
                raise FileNotFoundError(f"Missing WASM distribution directory: {source}")
            files.extend(
                (path, path.relative_to(DIST / "wasm").as_posix()) for path in source.rglob("*") if path.is_file()
            )
    for name, source in require_license_sources(platform).items():
        files.append((source, f"licenses/{name}"))
    return sorted(files, key=lambda item: item[1])


def write_archive(output: Path, files: list[tuple[Path, str]]) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for source, name in files:
            info = zipfile.ZipInfo(name, FIXED_ZIP_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = (0o755 if name in {"geometer", "geometer.exe"} else 0o644) << 16
            archive.writestr(info, source.read_bytes())
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    checksum = output.with_suffix(output.suffix + ".sha256")
    checksum.write_text(f"{digest}  {output.name}\n", encoding="utf-8", newline="\n")
    print(f"{output}: sha256={digest}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kind", choices=("native", "wasm"))
    parser.add_argument("--platform")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    write_archive(args.output.resolve(), runtime_files(args.kind, args.platform))


if __name__ == "__main__":
    main()
