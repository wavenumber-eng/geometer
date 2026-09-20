"""Create a deterministic, relocatable Geometer static C ABI SDK archive."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import tempfile
import tomllib
import zipfile
from pathlib import Path
from typing import Any

import dependency_versions
from release_licenses import release_license_sources


ROOT = Path(__file__).resolve().parents[1]
FIXED_ZIP_TIME = (1980, 1, 1, 0, 0, 0)
SUPPORTED_PLATFORMS = {
    "windows-x64": ("x86_64-pc-windows-msvc", "x86_64", "msvc", "msvc-static", None),
    "linux-x64": ("x86_64-unknown-linux-gnu", "x86_64", "gcc", "libstdc++", "glibc-2.35"),
    "linux-arm64": ("aarch64-unknown-linux-gnu", "aarch64", "gcc", "libstdc++", "glibc-2.35"),
    "macos-arm64": ("aarch64-apple-darwin", "arm64", "apple-clang", "libc++", "macos-11.0"),
}
ARCHIVE_PATTERN = re.compile(r'(?:(?:"([^"\r\n]+\.(?:lib|a))")|([^\s"\r\n]+\.(?:lib|a)))', re.IGNORECASE)
RESPONSE_PATTERN = re.compile(r'@(?:"([^"\r\n]+)"|([^\s"\r\n]+))')
CATALOG_PATTERN = re.compile(r'return "([0-9a-f]{64})";')


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def canonical_json(value: Any) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True, ensure_ascii=True) + "\n").encode()


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(canonical_json(value))


def release_metadata() -> tuple[str, int, str]:
    project = tomllib.loads((ROOT / "pyproject.toml").read_text(encoding="utf-8"))["project"]
    version = str(project["version"])
    parts = [int(part) for part in version.split(".")]
    if len(parts) not in {3, 4}:
        raise RuntimeError(f"Unsupported release version: {version}")
    year, month, day = parts[:3]
    release_tag = f"v{year:04d}-{month:02d}-{day:02d}"
    if len(parts) == 4:
        release_tag = f"{release_tag}-{parts[3]}"
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    match = re.search(r'set\(GEOMETER_ABI_VERSION "([0-9]{8})"', cmake)
    if match is None:
        raise RuntimeError("Could not read GEOMETER_ABI_VERSION")
    return version, int(match.group(1)), release_tag


def catalog_sha256() -> str:
    source = (ROOT / "src/cpp/lib/geometer/generated/contracts/operation_catalog.cpp").read_text(encoding="utf-8")
    matches = CATALOG_PATTERN.findall(source)
    if len(matches) != 1:
        raise RuntimeError("Could not identify the normalized operation catalog digest")
    return matches[0]


def git_revision(allow_dirty: bool) -> str:
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    if not re.fullmatch(r"[0-9a-f]{40}", revision):
        raise RuntimeError("Git did not return a full source revision")
    dirty = subprocess.check_output(["git", "status", "--porcelain"], cwd=ROOT, text=True)
    if dirty and not allow_dirty:
        raise RuntimeError("Static SDK candidates require a clean source tree; use --allow-dirty only locally")
    return revision


def cmake_value(build_dir: Path, variable: str) -> str:
    cache = (build_dir / "CMakeCache.txt").read_text(encoding="utf-8", errors="replace")
    match = re.search(rf"^{re.escape(variable)}(?::[^=]+)?=(.*)$", cache, re.MULTILINE)
    if match is None:
        raise RuntimeError(f"CMake cache does not define {variable}")
    return match.group(1).strip()


def compiler_identity(build_dir: Path) -> tuple[str, str]:
    candidates = sorted((build_dir / "CMakeFiles").glob("*/CMakeCXXCompiler.cmake"))
    if not candidates:
        raise RuntimeError("CMake compiler identity was not found")
    text = candidates[-1].read_text(encoding="utf-8", errors="replace")
    identity = re.search(r'set\(CMAKE_CXX_COMPILER_ID "([^"]+)"\)', text)
    version = re.search(r'set\(CMAKE_CXX_COMPILER_VERSION "([^"]+)"\)', text)
    if identity is None or version is None:
        raise RuntimeError("CMake compiler identity is incomplete")
    return identity.group(1), version.group(1)


def canonical_compiler_family(identity: str) -> str:
    normalized = identity.lower()
    return {"gnu": "gcc", "appleclang": "apple-clang"}.get(normalized, normalized)


def geometer_archive(build_dir: Path, platform: str) -> Path:
    name = "geometer.lib" if platform.startswith("windows-") else "libgeometer.a"
    path = build_dir / "src" / "cpp" / "lib" / name
    if not path.is_file():
        raise FileNotFoundError(f"Missing built Geometer static archive: {path}")
    return path.resolve()


def link_command(build_dir: Path) -> str:
    ninja = shutil.which("ninja")
    if ninja is None:
        raise RuntimeError("The supported SDK producer requires the repository Ninja preset")
    output = subprocess.check_output(
        [ninja, "-C", str(build_dir), "-t", "commands", "geometer_sdk_link_probe"], text=True
    )
    lines = [line for line in output.splitlines() if "geometer_sdk_link_probe" in line]
    if not lines:
        raise RuntimeError("CMake did not expose the static SDK link probe command")
    return expand_response_files(lines[-1], build_dir)


def expand_response_files(command: str, build_dir: Path) -> str:
    def replace(match: re.Match[str]) -> str:
        value = match.group(1) or match.group(2)
        path = Path(value)
        if not path.is_absolute():
            path = build_dir / path
        if not path.is_file():
            raise RuntimeError(f"Static SDK link response file is unavailable: {path}")
        return path.read_text(encoding="utf-8", errors="replace")

    return RESPONSE_PATTERN.sub(replace, command)


def ordered_unique(values: list[str]) -> list[str]:
    return list(dict.fromkeys(values))


def resolve_link_archives(command: str, build_dir: Path, geometer: Path) -> tuple[list[Path], list[str]]:
    private: list[Path] = []
    system: list[str] = []
    for quoted, bare in ARCHIVE_PATTERN.findall(command):
        token = (quoted or bare).strip('"')
        if token.lower().startswith(("/implib:", "/out:", "/pdb:")):
            continue
        candidate = Path(token)
        if not candidate.is_absolute():
            candidate = build_dir / candidate
        if candidate.is_file():
            resolved = candidate.resolve()
            if resolved == geometer:
                continue
            private.append(resolved)
        else:
            name = Path(token).name
            if name.lower().endswith(".lib"):
                system.append(name[:-4])
    system.extend(re.findall(r"(?:^|\s)-l([^\s]+)", command))
    if "-pthread" in command:
        system.append("pthread")
    return private, ordered_unique(system)


def frameworks(command: str) -> list[str]:
    return ordered_unique(re.findall(r"(?:^|\s)-framework\s+([^\s]+)", command))


def ensure_runtime_libraries(platform: str, system: list[str]) -> list[str]:
    if platform.startswith("linux-"):
        system.extend(["stdc++", "pthread", "dl"])
    elif platform == "macos-arm64":
        system.append("c++")
    elif platform == "windows-x64":
        system.extend(["ws2_32", "bcrypt"])
    return ordered_unique(system)


def copy_file(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, destination)


def recipe_digest() -> str:
    paths = [
        ROOT / "CMakeLists.txt",
        ROOT / "CMakePresets.json",
        ROOT / "src/cpp/lib/CMakeLists.txt",
        ROOT / "scripts/build_occt.py",
        ROOT / "src/rust/geometer-sys/Cargo.toml",
        ROOT / "src/rust/geometer-sys/build.rs",
        ROOT / "src/rust/geometer-sys/src/lib.rs",
        Path(__file__).resolve(),
    ]
    digest = hashlib.sha256()
    for path in sorted(paths):
        digest.update(path.relative_to(ROOT).as_posix().encode())
        digest.update(b"\0")
        digest.update(path.read_bytes())
        digest.update(b"\0")
    return digest.hexdigest()


def cmake_targets(manifest: dict[str, Any]) -> str:
    archives = [f"${{_GEOMETER_PREFIX}}/{path}" for path in manifest["archives"]["private"]]
    if manifest["link"]["rescan_private_archives"] and archives:
        private_links = [f"$<LINK_GROUP:RESCAN,{','.join(archives)}>"]
    else:
        private_links = archives
    links = [*private_links, *manifest["link"]["system_libraries"]]
    links.extend(f"$<LINK_LIBRARY:FRAMEWORK,{name}>" for name in manifest["link"]["apple_frameworks"])
    encoded_links = ";".join(links)
    return f'''# Generated by scripts/package_static_sdk.py. Do not edit.
if(TARGET Geometer::c_api_static)
    return()
endif()
get_filename_component(_GEOMETER_PREFIX "${{CMAKE_CURRENT_LIST_DIR}}/../../.." ABSOLUTE)
add_library(Geometer::c_api_static STATIC IMPORTED)
set_target_properties(Geometer::c_api_static PROPERTIES
    IMPORTED_LOCATION "${{_GEOMETER_PREFIX}}/{manifest["archives"]["geometer"]}"
    INTERFACE_INCLUDE_DIRECTORIES "${{_GEOMETER_PREFIX}}/include"
    INTERFACE_LINK_LIBRARIES "{encoded_links}"
)
unset(_GEOMETER_PREFIX)
'''


def cmake_config() -> str:
    return """# Geometer static C ABI SDK package.
include(\"${CMAKE_CURRENT_LIST_DIR}/GeometerTargets.cmake\")
set(Geometer_FOUND TRUE)
"""


def cmake_version_config(version: str) -> str:
    return f'''set(PACKAGE_VERSION "{version}")
if(PACKAGE_FIND_VERSION STREQUAL PACKAGE_VERSION)
    set(PACKAGE_VERSION_EXACT TRUE)
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
else()
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
endif()
'''


def internal_attestation(
    *, platform: str, source_revision: str, compiler_id: str, compiler_version: str, manifest_sha256: str
) -> dict[str, Any]:
    return {
        "schema": "wn.geometer.static_sdk_attestation.a0",
        "source_revision": source_revision,
        "manifest_sha256": manifest_sha256,
        "tool": {
            "identity": "scripts/package_static_sdk.py",
            "sha256": sha256_file(Path(__file__).resolve()),
        },
        "build": {
            "platform": platform,
            "compiler_id": compiler_id,
            "compiler_version": compiler_version,
            "occt_repository": dependency_versions.OCCT_REPO,
            "occt_tag": dependency_versions.OCCT_TAG,
        },
    }


def payload_inventory(stage: Path, catalog_digest: str, manifest_digest: str) -> dict[str, Any]:
    own_path = "share/geometer/geometer-sdk-payload.json"
    entries = []
    files = [item for item in stage.rglob("*") if item.is_file()]
    for path in sorted(files, key=lambda item: item.relative_to(stage).as_posix()):
        relative = path.relative_to(stage).as_posix()
        if relative == own_path:
            continue
        entries.append({"path": relative, "sha256": sha256_file(path), "size": path.stat().st_size})
    return {
        "schema": "wn.geometer.static_sdk_payload.a0",
        "catalog_sha256": catalog_digest,
        "manifest_sha256": manifest_digest,
        "entries": entries,
    }


def write_archive(stage: Path, output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        files = [item for item in stage.rglob("*") if item.is_file()]
        for path in sorted(files, key=lambda item: item.relative_to(stage).as_posix()):
            name = path.relative_to(stage).as_posix()
            info = zipfile.ZipInfo(name, FIXED_ZIP_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            archive.writestr(info, path.read_bytes())


def package(build_dir: Path, platform: str, output: Path, allow_dirty: bool) -> list[Path]:
    if platform not in SUPPORTED_PLATFORMS:
        raise ValueError(f"Unsupported SDK platform: {platform}")
    build_dir = build_dir.resolve()
    if cmake_value(build_dir, "CMAKE_BUILD_TYPE") != "Release":
        raise RuntimeError("Static SDK packaging requires a Release CMake build")
    if cmake_value(build_dir, "GEOMETER_OCCT_LIBRARY_TYPE") != "Static":
        raise RuntimeError("Static SDK packaging requires static OCCT")
    if platform == "windows-x64" and cmake_value(build_dir, "GEOMETER_MSVC_RUNTIME") != "Static":
        raise RuntimeError("The Windows static SDK requires the isolated /MT profile")

    version, abi_generation, release_tag = release_metadata()
    source_revision = git_revision(allow_dirty)
    target, architecture, expected_compiler, cxx_runtime, minimum_os = SUPPORTED_PLATFORMS[platform]
    compiler_id, compiler_version = compiler_identity(build_dir)
    compiler_family = canonical_compiler_family(compiler_id)
    if compiler_family != expected_compiler:
        raise RuntimeError(f"SDK profile expected {expected_compiler}, got {compiler_id}")

    geometer = geometer_archive(build_dir, platform)
    command = link_command(build_dir)
    private_archives, system_libraries = resolve_link_archives(command, build_dir, geometer)
    if not private_archives:
        raise RuntimeError("The CMake link probe did not expose the private static archive closure")
    system_libraries = ensure_runtime_libraries(platform, system_libraries)
    apple_frameworks = frameworks(command)
    catalog_digest = catalog_sha256()

    with tempfile.TemporaryDirectory(prefix="geometer-static-sdk-") as temp_text:
        stage = Path(temp_text) / "sdk"
        geometer_relative = f"lib/{geometer.name}"
        copy_file(geometer, stage / geometer_relative)
        private_relative: list[str] = []
        seen_names: dict[str, Path] = {}
        for archive in private_archives:
            previous = seen_names.get(archive.name)
            if previous is not None and previous != archive:
                raise RuntimeError(f"Private archive basename collision: {previous} and {archive}")
            seen_names[archive.name] = archive
            relative = f"lib/occt/{archive.name}"
            if relative not in private_relative:
                copy_file(archive, stage / relative)
                private_relative.append(relative)

        copy_file(ROOT / "src/cpp/lib/geometer/c_api.h", stage / "include/geometer/c_api.h")
        for relative in ("Cargo.toml", "build.rs", "src/lib.rs"):
            copy_file(
                ROOT / "src/rust/geometer-sys" / relative,
                stage / "rust/geometer-sys" / relative,
            )
        copy_file(ROOT / "schemas/geometer-sdk.schema.json", stage / "share/geometer/geometer-sdk.schema.json")
        copy_file(
            ROOT / "schemas/geometer-sdk-payload.schema.json",
            stage / "share/geometer/geometer-sdk-payload.schema.json",
        )
        for name, source in release_license_sources(ROOT, platform).items():
            if not source.is_file():
                raise FileNotFoundError(f"Missing SDK license input: {source}")
            copy_file(source, stage / "licenses" / name)

        entries = [{"kind": "archive", "value": path} for path in private_relative]
        entries.extend({"kind": "system_library", "value": name} for name in system_libraries)
        entries.extend({"kind": "apple_framework", "value": name} for name in apple_frameworks)
        manifest = {
            "schema": "wn.geometer.static_sdk.a0",
            "release_version": version,
            "c_abi_generation": abi_generation,
            "platform": platform,
            "target_triple": target,
            "catalog": {"projection": "native", "sha256": catalog_digest},
            "profile": {
                "architecture": architecture,
                "build_type": "Release",
                "compiler_family": compiler_family,
                "compiler_version": compiler_version,
                "cxx_runtime": cxx_runtime,
                "minimum_os": minimum_os,
                "msvc_runtime": "static" if platform == "windows-x64" else "not-applicable",
            },
            "archives": {"geometer": geometer_relative, "private": private_relative},
            "link": {
                "entries": entries,
                "rescan_private_archives": platform.startswith("linux-"),
                "system_libraries": system_libraries,
                "apple_frameworks": apple_frameworks,
            },
            "build_recipe": {
                "identity": "wn.geometer.static_sdk_recipe.a0",
                "sha256": recipe_digest(),
            },
        }
        manifest_path = stage / "share/geometer/geometer-sdk.json"
        write_json(manifest_path, manifest)
        manifest_digest = sha256_file(manifest_path)
        write_json(
            stage / "share/geometer/geometer-sdk-attestation.json",
            internal_attestation(
                platform=platform,
                source_revision=source_revision,
                compiler_id=compiler_id,
                compiler_version=compiler_version,
                manifest_sha256=manifest_digest,
            ),
        )
        cmake_dir = stage / "lib/cmake/Geometer"
        cmake_dir.mkdir(parents=True, exist_ok=True)
        (cmake_dir / "GeometerTargets.cmake").write_text(cmake_targets(manifest), encoding="utf-8", newline="\n")
        (cmake_dir / "GeometerConfig.cmake").write_text(cmake_config(), encoding="utf-8", newline="\n")
        (cmake_dir / "GeometerConfigVersion.cmake").write_text(
            cmake_version_config(version), encoding="utf-8", newline="\n"
        )
        payload_path = stage / "share/geometer/geometer-sdk-payload.json"
        write_json(payload_path, payload_inventory(stage, catalog_digest, manifest_digest))
        write_archive(stage, output)

        archive_digest = sha256_file(output)
        checksum = output.with_suffix(output.suffix + ".sha256")
        checksum.write_text(f"{archive_digest}  {output.name}\n", encoding="utf-8", newline="\n")
        provenance = output.with_suffix(output.suffix + ".provenance.json")
        write_json(
            provenance,
            {
                "schema": "wn.geometer.static_sdk_provenance.a0",
                "archive": {"name": output.name, "sha256": archive_digest, "size": output.stat().st_size},
                "payload_inventory_sha256": sha256_file(payload_path),
                "release_tag": release_tag,
                "source_revision": source_revision,
                "workflow": {
                    "repository": os.environ.get("GITHUB_REPOSITORY"),
                    "run_id": os.environ.get("GITHUB_RUN_ID"),
                    "run_attempt": os.environ.get("GITHUB_RUN_ATTEMPT"),
                },
            },
        )
    return [output, checksum, provenance]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--platform", choices=sorted(SUPPORTED_PLATFORMS), required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--allow-dirty", action="store_true", help="Local development only")
    args = parser.parse_args()
    outputs = package(args.build_dir, args.platform, args.output.resolve(), args.allow_dirty)
    for output in outputs:
        print(f"{output}: sha256={sha256_file(output)}")


if __name__ == "__main__":
    main()
