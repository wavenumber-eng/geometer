"""Validate a packaged Geometer static SDK through an external consumer build."""

from __future__ import annotations

import argparse
import glob
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
import xml.etree.ElementTree as ET
from pathlib import Path, PurePosixPath
from typing import Any


REQUIRED_SDK_FILES = {
    "include/geometer/c_api.h",
    "lib/cmake/Geometer/GeometerConfig.cmake",
    "lib/cmake/Geometer/GeometerConfigVersion.cmake",
    "lib/cmake/Geometer/GeometerTargets.cmake",
    "rust/geometer-sys/Cargo.toml",
    "rust/geometer-sys/build.rs",
    "rust/geometer-sys/src/lib.rs",
    "share/geometer/geometer-sdk-attestation.json",
    "share/geometer/geometer-sdk-payload.json",
    "share/geometer/geometer-sdk-payload.schema.json",
    "share/geometer/geometer-sdk.json",
    "share/geometer/geometer-sdk.schema.json",
}
FORBIDDEN_PRIVATE_IMPORTS = re.compile(
    r"^(?:geometer|libgeometer|TK[A-Za-z0-9_]+|libTK[A-Za-z0-9_]+)"
    r"(?:\.dll|\.dylib|\.so(?:\..*)?)$",
    re.IGNORECASE,
)
ROOT = Path(__file__).resolve().parents[1]


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def load_json(archive: zipfile.ZipFile, name: str) -> dict[str, Any]:
    value = json.loads(archive.read(name))
    if not isinstance(value, dict):
        raise ValueError(f"{name} must contain a JSON object")
    return value


def validate_archive_paths(names: list[str]) -> None:
    if names != sorted(names) or len(names) != len(set(names)):
        raise ValueError("static SDK entries must be unique and lexically ordered")
    for name in names:
        path = PurePosixPath(name)
        if path.is_absolute() or ".." in path.parts or "\\" in name:
            raise ValueError(f"unsafe static SDK archive path: {name}")
    missing = REQUIRED_SDK_FILES - set(names)
    if missing:
        raise ValueError("static SDK is missing required files: " + ", ".join(sorted(missing)))


def validate_payload_inventory(archive: zipfile.ZipFile, names: list[str], payload: dict[str, Any]) -> None:
    inventory = payload.get("entries")
    if not isinstance(inventory, list):
        raise ValueError("static SDK payload entries must be an array")
    inventory_by_path: dict[str, dict[str, Any]] = {}
    for entry in inventory:
        if not isinstance(entry, dict) or not isinstance(entry.get("path"), str):
            raise ValueError("static SDK payload entries must have string paths")
        inventory_by_path[entry["path"]] = entry
    expected_paths = set(names) - {"share/geometer/geometer-sdk-payload.json"}
    if set(inventory_by_path) != expected_paths:
        raise ValueError("static SDK payload inventory does not exactly cover the archive")
    for name, entry in inventory_by_path.items():
        value = archive.read(name)
        if entry.get("size") != len(value) or entry.get("sha256") != sha256_bytes(value):
            raise ValueError(f"static SDK payload entry does not match: {name}")


def validate_archive(archive_path: Path) -> dict[str, Any]:
    checksum_path = archive_path.with_suffix(archive_path.suffix + ".sha256")
    provenance_path = archive_path.with_suffix(archive_path.suffix + ".provenance.json")
    expected_checksum = checksum_path.read_text(encoding="utf-8").strip()
    actual_digest = sha256_bytes(archive_path.read_bytes())
    if expected_checksum != f"{actual_digest}  {archive_path.name}":
        raise ValueError("static SDK checksum sidecar does not match the archive")

    provenance = json.loads(provenance_path.read_text(encoding="utf-8"))
    if provenance.get("schema") != "wn.geometer.static_sdk_provenance.a0":
        raise ValueError("static SDK provenance has an unsupported schema")
    if provenance.get("archive") != {
        "name": archive_path.name,
        "sha256": actual_digest,
        "size": archive_path.stat().st_size,
    }:
        raise ValueError("static SDK provenance is not bound to the archive")

    with zipfile.ZipFile(archive_path) as archive:
        names = archive.namelist()
        validate_archive_paths(names)

        manifest = load_json(archive, "share/geometer/geometer-sdk.json")
        payload = load_json(archive, "share/geometer/geometer-sdk-payload.json")
        attestation = load_json(archive, "share/geometer/geometer-sdk-attestation.json")
        if manifest.get("schema") != "wn.geometer.static_sdk.a0":
            raise ValueError("static SDK manifest has an unsupported schema")
        if payload.get("schema") != "wn.geometer.static_sdk_payload.a0":
            raise ValueError("static SDK payload inventory has an unsupported schema")
        if attestation.get("schema") != "wn.geometer.static_sdk_attestation.a0":
            raise ValueError("static SDK attestation has an unsupported schema")

        manifest_digest = sha256_bytes(archive.read("share/geometer/geometer-sdk.json"))
        if payload.get("manifest_sha256") != manifest_digest:
            raise ValueError("static SDK payload inventory is not bound to its manifest")
        if attestation.get("manifest_sha256") != manifest_digest:
            raise ValueError("static SDK attestation is not bound to its manifest")
        payload_digest = sha256_bytes(archive.read("share/geometer/geometer-sdk-payload.json"))
        if provenance.get("payload_inventory_sha256") != payload_digest:
            raise ValueError("static SDK provenance is not bound to its payload inventory")

        validate_payload_inventory(archive, names, payload)

        geometer_archive = manifest.get("archives", {}).get("geometer")
        private_archives = manifest.get("archives", {}).get("private")
        if geometer_archive not in names or not isinstance(private_archives, list) or not private_archives:
            raise ValueError("static SDK manifest has an incomplete archive closure")
        if any(name not in names for name in private_archives):
            raise ValueError("static SDK manifest references a missing private archive")
    return manifest


def consumer_cmake() -> str:
    return """cmake_minimum_required(VERSION 3.25)
project(geometer_sdk_consumer LANGUAGES C CXX)
find_package(Geometer CONFIG REQUIRED)
add_executable(geometer_sdk_consumer main.c)
set_property(TARGET geometer_sdk_consumer PROPERTY LINKER_LANGUAGE CXX)
if(MSVC)
  set_property(TARGET geometer_sdk_consumer PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
endif()
target_link_libraries(geometer_sdk_consumer PRIVATE Geometer::c_api_static)
"""


def consumer_source() -> str:
    return r"""#include <geometer/c_api.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int execute_model_bounds(const char* path) {
    FILE* stream = fopen(path, "rb");
    if (stream == NULL || fseek(stream, 0, SEEK_END) != 0) {
        fputs("could not open model fixture\n", stderr);
        return 20;
    }
    const long length = ftell(stream);
    if (length <= 0 || fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        fputs("could not size model fixture\n", stderr);
        return 21;
    }
    unsigned char* model = (unsigned char*)malloc((size_t)length);
    if (model == NULL || fread(model, 1, (size_t)length, stream) != (size_t)length) {
        free(model);
        fclose(stream);
        fputs("could not read model fixture\n", stderr);
        return 22;
    }
    fclose(stream);
    GeometerAttachmentView attachment = {0};
    attachment.struct_size = sizeof(attachment);
    attachment.name = "model";
    attachment.name_size = 5;
    attachment.media_type = "application/step";
    attachment.media_type_size = 16;
    attachment.data = model;
    attachment.data_size = (uint32_t)length;
    GeometerOperationResult* result = NULL;
    char* error = NULL;
    const char* operation = "geometry.model_bounds.a0";
    const unsigned char request[] = "{}";
    const int code = geometer_operation_execute(
        operation, (uint32_t)strlen(operation), request, 2, &attachment, 1, &result, &error);
    free(model);
    if (code != GEOMETER_OPERATION_ABI_OK || result == NULL ||
        geometer_operation_result_json_size(result) == 0 ||
        geometer_operation_result_attachment_count(result) != 0) {
        fprintf(stderr, "model bounds failed: code=%d error=%s\n", code, error == NULL ? "" : error);
        geometer_operation_result_free(result);
        geometer_free_string(error);
        return 23;
    }
    geometer_operation_result_free(result);
    geometer_free_string(error);
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 3 && strcmp(argv[1], "serve") == 0 && strcmp(argv[2], "--stdio") == 0) {
        return geometer_serve_stdio();
    }
    if (argc == 2) {
        return execute_model_bounds(argv[1]);
    }
    char* catalog = NULL;
    char* error = NULL;
    const char* version = geometer_version_string();
    if (version == NULL || version[0] == '\0') {
        fputs("empty Geometer version\n", stderr);
        return 10;
    }
    if (geometer_operation_catalog_json(&catalog, &error) != 0) {
        fprintf(stderr, "catalog failed: %s\n", error == NULL ? "unknown" : error);
        geometer_free_string(error);
        return 11;
    }
    if (catalog == NULL || strstr(catalog, "geometry.model_bounds.a0") == NULL) {
        fputs("catalog is missing the governed operation\n", stderr);
        geometer_free_string(catalog);
        return 12;
    }
    printf("geometer=%s abi=%d catalog_bytes=%zu\n", version, geometer_abi_version(), strlen(catalog));
    geometer_free_string(catalog);
    return 0;
}
"""


def run(command: list[str], *, cwd: Path) -> str:
    print("  >", " ".join(command), flush=True)
    result = subprocess.run(command, cwd=cwd, text=True, capture_output=True, check=False)
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)
    if result.returncode != 0:
        raise subprocess.CalledProcessError(result.returncode, command)
    return result.stdout


def imported_libraries(executable: Path) -> list[str]:
    if sys.platform == "win32":
        tool = shutil.which("dumpbin")
        if tool is None:
            raise RuntimeError("dumpbin is required to qualify a Windows static SDK")
        output = run([tool, "/nologo", "/dependents", str(executable)], cwd=executable.parent)
        return re.findall(r"^\s+([^\s]+\.dll)\s*$", output, re.MULTILINE | re.IGNORECASE)
    if sys.platform == "darwin":
        output = run(["otool", "-L", str(executable)], cwd=executable.parent)
        return [Path(line.strip().split(" ", 1)[0]).name for line in output.splitlines()[1:] if line.strip()]
    output = run(["ldd", str(executable)], cwd=executable.parent)
    return [Path(match).name for match in re.findall(r"^\s*([^\s]+)\s+=>", output, re.MULTILINE)]


def validate_cargo_consumer(sdk: Path, root: Path) -> None:
    cargo = shutil.which("cargo")
    if cargo is None:
        raise RuntimeError("cargo is required to qualify the static SDK Rust boundary")
    source = root / "rust consumer"
    source.mkdir()
    crate_path = (sdk / "rust/geometer-sys").as_posix().replace("'", "\\'")
    (source / "Cargo.toml").write_text(
        f"""[package]\nname = "geometer-sdk-consumer"\nversion = "0.0.0"\nedition = "2024"\n\n"""
        f"""[dependencies]\ngeometer-sys = {{ path = '{crate_path}' }}\n""",
        encoding="utf-8",
        newline="\n",
    )
    rust_source = source / "src"
    rust_source.mkdir()
    (rust_source / "main.rs").write_text(
        """fn main() {\n    let catalog = geometer_sys::operation_catalog_json().expect("operation catalog");\n    assert!(String::from_utf8(catalog).expect("catalog UTF-8").contains("geometry.model_bounds.a0"));\n    let model = std::fs::read(std::env::args_os().nth(1).expect("STEP path")).expect("STEP bytes");\n    let output = geometer_sys::operation_execute(\n        "geometry.model_bounds.a0",\n        b"{}",\n        &[geometer_sys::Attachment {\n            name: "model",\n            media_type: "application/step",\n            data: &model,\n        }],\n    ).expect("direct model bounds");\n    assert!(!output.json.is_empty());\n    assert!(output.attachments.is_empty());\n}\n""",
        encoding="utf-8",
        newline="\n",
    )
    environment = os.environ.copy()
    environment["GEOMETER_SDK_DIR"] = str(sdk)
    if sys.platform == "win32":
        flags = environment.get("RUSTFLAGS", "")
        environment["RUSTFLAGS"] = f"{flags} -C target-feature=+crt-static".strip()
    print("  > cargo generate-lockfile", flush=True)
    subprocess.check_call([cargo, "generate-lockfile"], cwd=source, env=environment)
    fixture = ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP"
    print("  > cargo run --release --locked -- <STEP fixture>", flush=True)
    result = subprocess.run(
        [cargo, "run", "--release", "--locked", "--", str(fixture)],
        cwd=source,
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)
    if result.returncode != 0:
        raise subprocess.CalledProcessError(
            result.returncode, [cargo, "run", "--release", "--locked", "--", str(fixture)]
        )
    executable = (
        source
        / "target/release"
        / ("geometer-sdk-consumer.exe" if sys.platform == "win32" else "geometer-sdk-consumer")
    )
    forbidden = sorted(name for name in imported_libraries(executable) if FORBIDDEN_PRIVATE_IMPORTS.match(name))
    if forbidden:
        raise ValueError("Cargo consumer imports private Geometer DLLs: " + ", ".join(forbidden))


def validate_direct_rust_client(sdk: Path) -> None:
    cargo = shutil.which("cargo")
    if cargo is None:
        raise RuntimeError("cargo is required to qualify the Geometer direct Rust client")
    environment = os.environ.copy()
    environment["GEOMETER_SDK_DIR"] = str(sdk)
    if sys.platform == "win32":
        flags = environment.get("RUSTFLAGS", "")
        environment["RUSTFLAGS"] = f"{flags} -C target-feature=+crt-static".strip()
    cargo_manifest = ROOT / "src/rust/geometer-client/Cargo.toml"
    command = [
        cargo,
        "test",
        "--release",
        "--locked",
        "--manifest-path",
        str(cargo_manifest),
        "--features",
        "direct-static",
        "--test",
        "direct_static",
    ]
    print("  > cargo test --release --locked --features direct-static --test direct_static", flush=True)
    subprocess.check_call(command, cwd=ROOT, env=environment)


def validate_direct_static_illustration(sdk: Path, root: Path) -> None:
    cargo = shutil.which("cargo")
    if cargo is None:
        raise RuntimeError("cargo is required to package the direct static illustration example")
    target = root / "direct illustration target"
    environment = os.environ.copy()
    environment["CARGO_TARGET_DIR"] = str(target)
    environment["GEOMETER_SDK_DIR"] = str(sdk)
    if sys.platform == "win32":
        flags = environment.get("RUSTFLAGS", "")
        environment["RUSTFLAGS"] = f"{flags} -C target-feature=+crt-static".strip()
    cargo_manifest = ROOT / "src/rust/geometer-client/Cargo.toml"
    command = [
        cargo,
        "build",
        "--release",
        "--locked",
        "--manifest-path",
        str(cargo_manifest),
        "--features",
        "direct-static",
        "--example",
        "direct_static_illustration",
    ]
    print(
        "  > cargo build --release --locked --features direct-static --example direct_static_illustration",
        flush=True,
    )
    subprocess.check_call(command, cwd=ROOT, env=environment)

    executable_name = "direct_static_illustration.exe" if sys.platform == "win32" else "direct_static_illustration"
    built = target / "release/examples" / executable_name
    package = root / "packaged direct illustration"
    package.mkdir()
    packaged = package / (
        "geometer-static-illustration.exe" if sys.platform == "win32" else "geometer-static-illustration"
    )
    shutil.copy2(built, packaged)
    if sorted(path.name for path in package.iterdir()) != [packaged.name]:
        raise ValueError("direct illustration package must initially contain only its app binary")

    isolated_path = root / "empty executable search path"
    isolated_path.mkdir()
    run_environment = environment.copy()
    run_environment["PATH"] = str(isolated_path)
    run_environment.pop("GEOMETER_EXE", None)
    run_environment.pop("GEOMETER_EXECUTABLE", None)
    observed = []
    for name, distance in (("unclipped", "-100"), ("partial", "0"), ("empty", "100")):
        output = root / f"clip {name}"
        print(f"  > packaged direct illustration --clip --distance {distance}", flush=True)
        result = subprocess.run(
            [
                str(packaged),
                "--clip",
                "--normal",
                "0,0,1",
                "--distance",
                distance,
                "--cap-policy",
                "none",
                "--output-dir",
                str(output),
            ],
            cwd=package,
            env=run_environment,
            text=True,
            capture_output=True,
            check=False,
        )
        if result.stdout:
            print(result.stdout, end="")
        if result.stderr:
            print(result.stderr, end="", file=sys.stderr)
        if result.returncode != 0:
            raise subprocess.CalledProcessError(result.returncode, [str(packaged)])
        svg = output / "clipped.svg"
        html = (output / "comparison.html").read_text(encoding="utf-8")
        minimum_svg_bytes = 100 if name == "empty" else 500
        if ET.parse(svg).getroot().tag != "{http://www.w3.org/2000/svg}svg" or svg.stat().st_size < minimum_svg_bytes:
            raise ValueError("packaged direct illustration did not write a valid clipped SVG")
        if "cap_policy: none" not in html or "fragment_sha256" not in html:
            raise ValueError("packaged direct illustration comparison omitted governed clipping metadata")
        observed.append(result.stdout)
    if "empty=true" not in observed[2] or "empty=false" not in observed[0] + observed[1]:
        raise ValueError("packaged direct illustration did not exercise partial and empty clip states")
    forbidden = sorted(name for name in imported_libraries(packaged) if FORBIDDEN_PRIVATE_IMPORTS.match(name))
    if forbidden:
        raise ValueError("packaged direct illustration imports private Geometer DLLs: " + ", ".join(forbidden))
    if any(path.name.casefold() in {"geometer", "geometer.exe"} for path in package.rglob("*")):
        raise ValueError("packaged direct illustration unexpectedly contains a Geometer executable")
def validate_external_consumer(
    archive_path: Path,
    manifest: dict[str, Any],
    keep_work: bool = False,
) -> None:
    root = Path(tempfile.mkdtemp(prefix="geometer sdk qualification "))
    try:
        sdk = root / "relocated sdk"
        source = root / "external consumer"
        build = root / "consumer build"
        sdk.mkdir()
        source.mkdir()
        with zipfile.ZipFile(archive_path) as archive:
            archive.extractall(sdk)
        (source / "CMakeLists.txt").write_text(consumer_cmake(), encoding="utf-8", newline="\n")
        (source / "main.c").write_text(consumer_source(), encoding="utf-8", newline="\n")

        configure = ["cmake", "-S", str(source), "-B", str(build), "-G", "Ninja", f"-DCMAKE_PREFIX_PATH={sdk}"]
        if sys.platform == "win32":
            configure.append("-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded")
        run(configure, cwd=root)
        run(["cmake", "--build", str(build), "--config", "Release"], cwd=root)
        executable = build / ("geometer_sdk_consumer.exe" if sys.platform == "win32" else "geometer_sdk_consumer")
        run([str(executable)], cwd=root)
        run([str(executable), str(ROOT / "tests/fixtures/step/embedded_models/SOT-23.STEP")], cwd=root)
        from geometer._ipc_client import GeometerIpcClient

        with GeometerIpcClient(executable, client_name="static-sdk-qualification") as client:
            if not client.welcome.operation_catalog.operations:
                raise ValueError("embedded stdio server returned an empty operation catalog")
        validate_cargo_consumer(sdk, root)
        validate_direct_rust_client(sdk)
        validate_direct_static_illustration(sdk, root)
        imports = imported_libraries(executable)
        forbidden = sorted(name for name in imports if FORBIDDEN_PRIVATE_IMPORTS.match(name))
        if forbidden:
            raise ValueError("external consumer imports private Geometer DLLs: " + ", ".join(forbidden))
        expected_platform = {
            "win32": "windows-x64",
            "darwin": "macos-arm64",
        }.get(sys.platform)
        if expected_platform is None and sys.platform.startswith("linux"):
            expected_platform = "linux-arm64" if platform.machine().lower() in {"aarch64", "arm64"} else "linux-x64"
        if manifest.get("platform") != expected_platform:
            raise ValueError(f"SDK platform {manifest.get('platform')} does not match host {expected_platform}")
        print(f"qualified external static SDK consumer; imports={','.join(imports)}")
        if keep_work:
            print(f"qualification workspace retained at {root}")
    finally:
        if not keep_work:
            shutil.rmtree(root)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", help="Archive path or a pattern resolving to exactly one archive")
    parser.add_argument("--metadata-only", action="store_true")
    parser.add_argument("--keep-work", action="store_true")
    args = parser.parse_args()
    matches = [Path(path) for path in glob.glob(args.archive)]
    if len(matches) != 1:
        parser.error(f"archive pattern must resolve to exactly one file, found {len(matches)}")
    archive = matches[0].resolve()
    manifest = validate_archive(archive)
    print(f"validated static SDK archive: {archive}")
    if not args.metadata_only:
        validate_external_consumer(archive, manifest, args.keep_work)


if __name__ == "__main__":
    main()
