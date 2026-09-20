from __future__ import annotations

import argparse
import os
from pathlib import Path

import occt_lock


_OCCT_LOCK = occt_lock.load_lock()
_OCCT_DEPENDENCY = _OCCT_LOCK["dependency"]
_OCCT_WASM_PROFILE = occt_lock.profile_for_selector(
    _OCCT_LOCK,
    kind="wasm",
    platform="wasm-emscripten",
)
OCCT_REPO = _OCCT_DEPENDENCY["source"]["repository"]
OCCT_TAG = _OCCT_DEPENDENCY["source"]["tag"]
OCCT_COMMIT = _OCCT_DEPENDENCY["source"]["commit"]
OCCT_TAG_OBJECT = _OCCT_DEPENDENCY["source"]["tag_object"]
OCCT_VERSION = _OCCT_DEPENDENCY["version"]
EMSDK_REPO = "https://github.com/emscripten-core/emsdk.git"
EMSDK_VERSION = _OCCT_WASM_PROFILE["abi"]["compiler"].removeprefix("emscripten-")
BOOST_VERSION = "1.92.0"
BOOST_ARCHIVE_URL = "https://archives.boost.io/release/1.92.0/source/boost_1_92_0.tar.gz"
BOOST_ARCHIVE_SHA256 = "c4a3b310ddd2472416e091067166b0713be97c63f38c212c484ada022fd296ce"
BOOST_UPSTREAM_COMMIT = "afdfa32505af73e3d208144b3f623f0096cb62b6"


def write_github_output(values: dict[str, str]) -> None:
    output_path = os.environ.get("GITHUB_OUTPUT")
    if not output_path:
        raise RuntimeError("GITHUB_OUTPUT is not set.")
    with Path(output_path).open("a", encoding="utf-8") as handle:
        for key, value in values.items():
            handle.write(f"{key}={value}\n")


def main() -> None:
    parser = argparse.ArgumentParser(description="Print Geometer dependency versions.")
    parser.add_argument(
        "--github-output",
        action="store_true",
        help="Write dependency version values to GitHub Actions GITHUB_OUTPUT.",
    )
    args = parser.parse_args()

    values = {
        "occt_repo": OCCT_REPO,
        "occt_tag": OCCT_TAG,
        "occt_commit": OCCT_COMMIT,
        "occt_tag_object": OCCT_TAG_OBJECT,
        "occt_version": OCCT_VERSION,
        "emsdk_repo": EMSDK_REPO,
        "emsdk_version": EMSDK_VERSION,
        "boost_version": BOOST_VERSION,
        "boost_archive_url": BOOST_ARCHIVE_URL,
        "boost_archive_sha256": BOOST_ARCHIVE_SHA256,
        "boost_upstream_commit": BOOST_UPSTREAM_COMMIT,
    }
    if args.github_output:
        write_github_output(values)
        return
    for key, value in values.items():
        print(f"{key}={value}")


if __name__ == "__main__":
    main()
