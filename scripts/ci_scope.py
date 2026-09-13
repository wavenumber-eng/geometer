"""Classify a GitHub change so CI runs only affected Geometer subsystems."""

from __future__ import annotations

import json
import os
from pathlib import Path, PurePosixPath
import subprocess
from typing import Iterable


ROOT_DOCUMENTS = {"AGENTS.md", "CHANGELOG.md", "CONTRIBUTING.md", "README.md"}
GENERATED_DOCUMENTATION_METADATA = {"docs/generated/contracts/site-manifest.a0.json"}
PRESENTATION_SUFFIXES = {
    ".css",
    ".gif",
    ".html",
    ".ico",
    ".jpeg",
    ".jpg",
    ".md",
    ".pdf",
    ".png",
    ".rst",
    ".svg",
    ".txt",
    ".webp",
}


def is_presentation_only(path: str) -> bool:
    """Return whether a path cannot affect a shipped executable or contract."""
    file = PurePosixPath(path)
    if path in ROOT_DOCUMENTS or path in GENERATED_DOCUMENTATION_METADATA or file.suffix.casefold() == ".md":
        return True
    if path.startswith("docs/"):
        return file.suffix.casefold() in PRESENTATION_SUFFIXES
    if file.suffix.casefold() in {".html", ".css"} and (
        path.startswith("examples/") or path.startswith("dist/wasm/demos/")
    ):
        return True
    return False


def _matches(path: str, prefixes: Iterable[str], names: Iterable[str] = ()) -> bool:
    return path in names or any(path.startswith(prefix) for prefix in prefixes)


def classify(paths: list[str], *, force_full: bool = False) -> dict[str, bool]:
    """Map changed paths to independently runnable validation lanes."""
    docs_only = bool(paths) and all(is_presentation_only(path) for path in paths)
    if force_full or not paths:
        return {
            "docs_only": False,
            "material": True,
            "native": True,
            "python": True,
            "rust": True,
            "typescript": True,
            "wasm": True,
        }
    if docs_only:
        return {
            "docs_only": True,
            "material": False,
            "native": False,
            "python": False,
            "rust": False,
            "typescript": False,
            "wasm": False,
        }

    workflow_paths = {path for path in paths if path.startswith(".github/workflows/")}
    full_workflow_change = bool(
        workflow_paths
        & {
            ".github/workflows/ci.yml",
            ".github/workflows/release.yml",
        }
    )
    dependency_code = any(
        _matches(
            path,
            ("third_party/",),
            {
                "scripts/build_occt.py",
                "scripts/dependency_versions.py",
                "scripts/occt_binary_cache.py",
            },
        )
        for path in paths
    )
    contract_code = any(_matches(path, ("src/tsp/", "tests/contracts/", "contracts/")) for path in paths)
    native = (
        full_workflow_change
        or dependency_code
        or contract_code
        or any(
            _matches(
                path,
                (
                    "src/cpp/",
                    "tests/cpp/",
                    "tests/fixtures/step/",
                    "examples/cpp/",
                    "dist/native/",
                ),
                {"CMakeLists.txt", "CMakePresets.json", "scripts/validate_native.py"},
            )
            for path in paths
        )
    )
    python = full_workflow_change or any(
        _matches(
            path,
            ("python/geometer/", "tests/python/", "examples/python/"),
            {
                "pyproject.toml",
                "uv.lock",
                "setup.py",
                "scripts/ci_scope.py",
                "scripts/validate_python_package.py",
            },
        )
        for path in paths
    )
    rust = full_workflow_change or any(_matches(path, ("src/rust/", "tests/rust/")) for path in paths)
    typescript = (
        full_workflow_change
        or contract_code
        or any(
            _matches(
                path,
                ("src/ts/", "tests/typescript/", "examples/typescript/"),
                {"package.json", "package-lock.json", "tsconfig.json"},
            )
            or (path.startswith("scripts/") and path.endswith(".mjs"))
            for path in paths
        )
    )
    wasm = (
        full_workflow_change
        or dependency_code
        or contract_code
        or any(
            _matches(
                path,
                (
                    "src/cpp/",
                    "tests/wasm/",
                    "examples/wasm/",
                    "dist/wasm/browser/",
                    "dist/wasm/node-test/",
                    "dist/wasm/planar-browser/",
                ),
                {
                    ".github/workflows/wasm.yml",
                    ".github/workflows/occt-deps.yml",
                    "CMakeLists.txt",
                    "CMakePresets.json",
                    "scripts/build_wasm.py",
                },
            )
            for path in paths
        )
    )
    if ".github/workflows/macos-wheel.yml" in workflow_paths:
        native = True
    if ".github/workflows/occt-deps.yml" in workflow_paths:
        native = True
        wasm = True
    return {
        "docs_only": False,
        "material": True,
        "native": native,
        "python": python and not native,
        "rust": rust and not native,
        "typescript": typescript and not native and not wasm,
        "wasm": wasm,
    }


def git_output(*args: str) -> bytes:
    return subprocess.check_output(["git", *args])


def changed_paths(event_name: str, event: dict[str, object]) -> list[str]:
    """Read the complete PR diff; unknown events intentionally request full CI."""
    if event_name != "pull_request":
        return []
    pull_request = event["pull_request"]
    if not isinstance(pull_request, dict):
        raise TypeError("pull_request must be an object")
    head_value = pull_request["head"]
    base_value = pull_request["base"]
    if not isinstance(head_value, dict) or not isinstance(base_value, dict):
        raise TypeError("pull_request refs must be objects")
    head = str(head_value["sha"])
    base = str(base_value["sha"])
    output = git_output("diff", "--name-only", "--no-renames", "-z", base, head, "--")
    return [path.decode("utf-8") for path in output.split(b"\0") if path]


def main() -> None:
    try:
        event_name = os.environ["GITHUB_EVENT_NAME"]
        event = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text(encoding="utf-8"))
        paths = changed_paths(event_name, event)
        scope = classify(paths, force_full=event_name == "workflow_dispatch")
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        print(f"Cannot establish the change scope; using full CI: {error}")
        scope = classify([], force_full=True)
        paths = []

    enabled = ", ".join(key for key, value in scope.items() if value)
    print(f"Changed paths: {len(paths)}")
    print(f"CI scope: {enabled}")
    with Path(os.environ["GITHUB_OUTPUT"]).open("a", encoding="utf-8") as output:
        for key, value in scope.items():
            output.write(f"{key}={str(value).lower()}\n")


if __name__ == "__main__":
    main()
