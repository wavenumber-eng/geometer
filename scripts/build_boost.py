"""Restore the pinned header-only Boost source tree under ignored .deps state."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import tarfile
import tempfile
import urllib.request
from pathlib import Path, PurePosixPath

import dependency_versions

ROOT = Path(__file__).resolve().parent.parent
DEPS_DIR = ROOT / ".deps"
BOOST_DIR = DEPS_DIR / "boost_1_92_0"
DOWNLOAD_DIR = DEPS_DIR / "downloads"
ARCHIVE_PATH = DOWNLOAD_DIR / "boost_1_92_0.tar.gz"
SENTINEL_PATH = BOOST_DIR / ".geometer-source.json"


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _expected_sentinel() -> dict[str, str]:
    return {
        "version": dependency_versions.BOOST_VERSION,
        "archive_url": dependency_versions.BOOST_ARCHIVE_URL,
        "archive_sha256": dependency_versions.BOOST_ARCHIVE_SHA256,
        "upstream_commit": dependency_versions.BOOST_UPSTREAM_COMMIT,
    }


def _is_ready() -> bool:
    version_header = BOOST_DIR / "boost" / "version.hpp"
    if not version_header.is_file() or not SENTINEL_PATH.is_file():
        return False
    try:
        version_text = version_header.read_text(encoding="utf-8")
        return (
            json.loads(SENTINEL_PATH.read_text(encoding="utf-8")) == _expected_sentinel()
            and "#define BOOST_VERSION 109200" in version_text
            and '#define BOOST_LIB_VERSION "1_92"' in version_text
        )
    except (OSError, json.JSONDecodeError):
        return False


def _download() -> None:
    DOWNLOAD_DIR.mkdir(parents=True, exist_ok=True)
    if ARCHIVE_PATH.is_file() and _sha256(ARCHIVE_PATH) == dependency_versions.BOOST_ARCHIVE_SHA256:
        return
    partial = ARCHIVE_PATH.with_suffix(".partial")
    with urllib.request.urlopen(dependency_versions.BOOST_ARCHIVE_URL) as response, partial.open("wb") as output:
        shutil.copyfileobj(response, output, length=1024 * 1024)
    actual = _sha256(partial)
    if actual != dependency_versions.BOOST_ARCHIVE_SHA256:
        partial.unlink(missing_ok=True)
        raise RuntimeError(
            f"Boost archive SHA-256 mismatch: expected {dependency_versions.BOOST_ARCHIVE_SHA256}, got {actual}"
        )
    partial.replace(ARCHIVE_PATH)


def _validate_member(member: tarfile.TarInfo) -> None:
    path = PurePosixPath(member.name)
    if path.is_absolute() or ".." in path.parts:
        raise RuntimeError(f"Unsafe Boost archive member: {member.name}")
    if member.issym() or member.islnk() or member.isdev():
        raise RuntimeError(f"Unsupported Boost archive member type: {member.name}")


def restore() -> None:
    if _is_ready():
        print(f"Boost {dependency_versions.BOOST_VERSION} already ready at {BOOST_DIR}")
        return
    if BOOST_DIR.exists():
        raise RuntimeError(
            f"Boost destination exists without the expected provenance sentinel: {BOOST_DIR}. "
            "Move it aside and rerun the restore."
        )
    _download()
    DEPS_DIR.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="boost-extract-", dir=DEPS_DIR) as temporary:
        temporary_root = Path(temporary)
        with tarfile.open(ARCHIVE_PATH, "r:gz") as archive:
            members = archive.getmembers()
            for member in members:
                _validate_member(member)
            archive.extractall(temporary_root, members=members)
        extracted = temporary_root / "boost_1_92_0"
        if not (extracted / "boost" / "multiprecision" / "cpp_int.hpp").is_file():
            raise RuntimeError("Boost archive is missing boost/multiprecision/cpp_int.hpp")
        shutil.move(str(extracted), str(BOOST_DIR))
    SENTINEL_PATH.write_text(json.dumps(_expected_sentinel(), indent=2) + "\n", encoding="utf-8")
    print(f"Restored Boost {dependency_versions.BOOST_VERSION} at {BOOST_DIR}")


def verify() -> None:
    if not _is_ready():
        raise RuntimeError(
            f"Boost {dependency_versions.BOOST_VERSION} is not ready at {BOOST_DIR}; "
            "run scripts/build_boost.py to restore the verified source tree."
        )
    if (
        not ARCHIVE_PATH.is_file()
        or _sha256(ARCHIVE_PATH) != dependency_versions.BOOST_ARCHIVE_SHA256
    ):
        raise RuntimeError(f"Boost archive cache is missing or invalid: {ARCHIVE_PATH}")
    print(f"Verified Boost {dependency_versions.BOOST_VERSION} at {BOOST_DIR}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--print-source-dir", action="store_true")
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    if args.print_source_dir:
        print(BOOST_DIR)
        return
    if args.verify:
        verify()
        return
    restore()


if __name__ == "__main__":
    main()
