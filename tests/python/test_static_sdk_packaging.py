from __future__ import annotations

import hashlib
import json
import zipfile
from pathlib import Path

import package_static_sdk
import validate_static_sdk
from build_static_sdk import validate_windows_static_runtime


def _fake_build(tmp_path: Path) -> tuple[Path, Path, list[Path]]:
    build = tmp_path / "build"
    (build / "CMakeFiles/3.31.0").mkdir(parents=True)
    (build / "CMakeCache.txt").write_text(
        "CMAKE_BUILD_TYPE:STRING=Release\n"
        "GEOMETER_OCCT_LIBRARY_TYPE:STRING=Static\n"
        "GEOMETER_MSVC_RUNTIME:STRING=Static\n",
        encoding="utf-8",
    )
    (build / "CMakeFiles/3.31.0/CMakeCXXCompiler.cmake").write_text(
        'set(CMAKE_CXX_COMPILER_ID "MSVC")\nset(CMAKE_CXX_COMPILER_VERSION "19.44.35219.0")\n',
        encoding="utf-8",
    )
    geometer = build / "src/cpp/lib/geometer.lib"
    geometer.parent.mkdir(parents=True)
    geometer.write_bytes(b"geometer-static-archive")
    private = [tmp_path / "occt/TKernel.lib", tmp_path / "occt/TKMath.lib"]
    for index, archive in enumerate(private):
        archive.parent.mkdir(parents=True, exist_ok=True)
        archive.write_bytes(f"private-{index}".encode())
    return build, geometer, private


def test_static_sdk_is_deterministic_complete_and_relocatable(tmp_path: Path, monkeypatch) -> None:
    build, geometer, private = _fake_build(tmp_path)
    license_file = tmp_path / "LICENSE.txt"
    license_file.write_text("test license\n", encoding="utf-8")
    command = (
        f"link.exe {geometer} {private[0]} {private[1]} "
        "windowscodecs.lib ws2_32.lib bcrypt.lib /out:geometer_sdk_link_probe.exe"
    )
    monkeypatch.setattr(package_static_sdk, "link_command", lambda _build: command)
    monkeypatch.setattr(package_static_sdk, "git_revision", lambda _allow_dirty: "a" * 40)
    monkeypatch.setattr(
        package_static_sdk,
        "release_license_sources",
        lambda _root, _platform: {"TEST_LICENSE.txt": license_file},
    )

    first = tmp_path / "one/geometer-sdk.zip"
    second = tmp_path / "two/geometer-sdk.zip"
    package_static_sdk.package(build, "windows-x64", first, allow_dirty=True)
    package_static_sdk.package(build, "windows-x64", second, allow_dirty=True)
    assert first.read_bytes() == second.read_bytes()

    with zipfile.ZipFile(first) as archive:
        names = archive.namelist()
        assert names == sorted(names)
        assert "include/geometer/c_api.h" in names
        assert "lib/geometer.lib" in names
        assert {"lib/occt/TKernel.lib", "lib/occt/TKMath.lib"}.issubset(names)
        assert "lib/cmake/Geometer/GeometerConfig.cmake" in names
        assert "lib/cmake/Geometer/GeometerTargets.cmake" in names
        assert "rust/geometer-sys/Cargo.toml" in names
        assert "rust/geometer-sys/build.rs" in names
        assert "rust/geometer-sys/src/lib.rs" in names
        assert "share/geometer/geometer-sdk.json" in names
        assert "share/geometer/geometer-sdk-payload.json" in names
        manifest = json.loads(archive.read("share/geometer/geometer-sdk.json"))
        payload = json.loads(archive.read("share/geometer/geometer-sdk-payload.json"))
        targets = archive.read("lib/cmake/Geometer/GeometerTargets.cmake").decode()

        assert manifest["target_triple"] == "x86_64-pc-windows-msvc"
        assert manifest["profile"]["msvc_runtime"] == "static"
        assert manifest["archives"]["private"] == ["lib/occt/TKernel.lib", "lib/occt/TKMath.lib"]
        assert manifest["link"]["system_libraries"] == ["windowscodecs", "ws2_32", "bcrypt"]
        assert "Geometer::c_api_static" in targets
        assert "${_GEOMETER_PREFIX}/lib/occt/TKernel.lib" in targets
        assert str(tmp_path) not in targets

        inventory = {entry["path"]: entry for entry in payload["entries"]}
        assert "share/geometer/geometer-sdk-payload.json" not in inventory
        assert set(inventory) == set(names) - {"share/geometer/geometer-sdk-payload.json"}
        for name, entry in inventory.items():
            value = archive.read(name)
            assert entry["size"] == len(value)
            assert entry["sha256"] == hashlib.sha256(value).hexdigest()

    assert first.with_suffix(".zip.sha256").is_file()
    provenance = json.loads(first.with_suffix(".zip.provenance.json").read_text(encoding="utf-8"))
    assert provenance["archive"]["sha256"] == hashlib.sha256(first.read_bytes()).hexdigest()
    assert provenance["source_revision"] == "a" * 40
    assert validate_static_sdk.validate_archive(first)["platform"] == "windows-x64"


def test_static_sdk_validation_rejects_payload_tampering(tmp_path: Path, monkeypatch) -> None:
    build, geometer, private = _fake_build(tmp_path)
    license_file = tmp_path / "LICENSE.txt"
    license_file.write_text("test license\n", encoding="utf-8")
    monkeypatch.setattr(
        package_static_sdk,
        "link_command",
        lambda _build: f"link.exe {geometer} {private[0]} {private[1]} ws2_32.lib /out:probe.exe",
    )
    monkeypatch.setattr(package_static_sdk, "git_revision", lambda _allow_dirty: "a" * 40)
    monkeypatch.setattr(
        package_static_sdk,
        "release_license_sources",
        lambda _root, _platform: {"TEST_LICENSE.txt": license_file},
    )
    archive_path = tmp_path / "geometer-sdk.zip"
    package_static_sdk.package(build, "windows-x64", archive_path, allow_dirty=True)
    rewritten = tmp_path / "rewritten.zip"
    with zipfile.ZipFile(archive_path) as source, zipfile.ZipFile(rewritten, "w") as destination:
        for info in source.infolist():
            value = b"tampered" if info.filename == "include/geometer/c_api.h" else source.read(info)
            destination.writestr(info, value)
    rewritten.replace(archive_path)
    archive_path.with_suffix(".zip.sha256").write_text(
        f"{hashlib.sha256(archive_path.read_bytes()).hexdigest()}  {archive_path.name}\n", encoding="utf-8"
    )
    provenance_path = archive_path.with_suffix(".zip.provenance.json")
    provenance = json.loads(provenance_path.read_text(encoding="utf-8"))
    provenance["archive"]["sha256"] = hashlib.sha256(archive_path.read_bytes()).hexdigest()
    provenance["archive"]["size"] = archive_path.stat().st_size
    provenance_path.write_text(json.dumps(provenance), encoding="utf-8")
    try:
        validate_static_sdk.validate_archive(archive_path)
    except ValueError as error:
        assert "payload entry" in str(error)
    else:
        raise AssertionError("tampered SDK archive was accepted")


def test_direct_static_illustration_sample_cannot_fall_back_to_the_cli() -> None:
    source = (
        package_static_sdk.ROOT
        / "src/rust/geometer-client/examples/direct_static_illustration.rs"
    ).read_text(encoding="utf-8")
    assert "GeometerDirectClient::new()" in source
    assert "GeometerClient::spawn" not in source
    assert "find_executable" not in source

    for name in (
        "geometer.dll",
        "libgeometer.so",
        "libgeometer.dylib",
        "TKBRep.dll",
        "libTKBRep.so.7.8",
        "libTKBRep.dylib",
    ):
        assert validate_static_sdk.FORBIDDEN_PRIVATE_IMPORTS.fullmatch(name)
    for name in ("KERNEL32.dll", "libc.so.6", "libSystem.B.dylib"):
        assert validate_static_sdk.FORBIDDEN_PRIVATE_IMPORTS.fullmatch(name) is None


def test_linux_cmake_projection_preserves_rescan_group() -> None:
    manifest = {
        "archives": {
            "geometer": "lib/libgeometer.a",
            "private": ["lib/occt/libTKernel.a", "lib/occt/libTKMath.a"],
        },
        "link": {
            "rescan_private_archives": True,
            "system_libraries": ["stdc++", "pthread", "dl"],
            "apple_frameworks": [],
        },
    }
    targets = package_static_sdk.cmake_targets(manifest)
    assert "$<LINK_GROUP:RESCAN,${_GEOMETER_PREFIX}/lib/occt/libTKernel.a," in targets
    assert "${_GEOMETER_PREFIX}/lib/occt/libTKMath.a>" in targets
    assert "stdc++;pthread;dl" in targets


def test_macos_cmake_projection_uses_framework_link_feature() -> None:
    manifest = {
        "archives": {"geometer": "lib/libgeometer.a", "private": ["lib/occt/libTKernel.a"]},
        "link": {
            "rescan_private_archives": False,
            "system_libraries": ["c++"],
            "apple_frameworks": ["Foundation", "AppKit"],
        },
    }
    targets = package_static_sdk.cmake_targets(manifest)
    assert "$<LINK_LIBRARY:FRAMEWORK,Foundation>" in targets
    assert "$<LINK_LIBRARY:FRAMEWORK,AppKit>" in targets
    assert "-framework Foundation" not in targets


def test_windows_static_occt_recipe_is_isolated() -> None:
    source = (package_static_sdk.ROOT / "scripts/build_occt.py").read_text(encoding="utf-8")
    cmake = (package_static_sdk.ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    assert "occt-static-crt-build" in source
    assert "occt-static-crt-install" in source
    assert 'definition("CMAKE_MSVC_RUNTIME_LIBRARY", "MultiThreaded")' in source
    assert 'definition("CMAKE_CXX_FLAGS_RELEASE", "/MT /O2 /Ob2 /DNDEBUG")' in source
    assert 'GEOMETER_MSVC_RUNTIME STREQUAL "Static"' in cmake
    assert "occt-static-crt-install" in cmake


def test_windows_sdk_rejects_dynamic_crt_compile_commands(tmp_path: Path) -> None:
    build = tmp_path / "build"
    build.mkdir()
    compile_commands = build / "compile_commands.json"
    compile_commands.write_text(
        json.dumps([{"file": "C:\\work\\src\\cpp\\lib\\c_api.cpp", "command": "cl /MD /c c_api.cpp"}]),
        encoding="utf-8",
    )
    try:
        validate_windows_static_runtime(build)
    except RuntimeError as error:
        assert "/MD" in str(error)
    else:
        raise AssertionError("dynamic CRT compile command was accepted")

    compile_commands.write_text(
        json.dumps([{"file": "C:\\work\\src\\cpp\\lib\\c_api.cpp", "command": "cl -MT /c c_api.cpp"}]),
        encoding="utf-8",
    )
    validate_windows_static_runtime(build)


def test_static_sdk_expands_preserved_ninja_link_response_file(tmp_path: Path) -> None:
    response = tmp_path / "CMakeFiles/geometer_sdk_link_probe.rsp"
    response.parent.mkdir(parents=True)
    response.write_text('lib/geometer.lib "occt/TKernel.lib" ws2_32.lib', encoding="utf-8")
    command = package_static_sdk.expand_response_files(
        'link.exe @"CMakeFiles/geometer_sdk_link_probe.rsp" /out:probe.exe',
        tmp_path,
    )
    assert command == 'link.exe lib/geometer.lib "occt/TKernel.lib" ws2_32.lib /out:probe.exe'


def test_static_sdk_uses_canonical_cmake_compiler_families() -> None:
    assert package_static_sdk.canonical_compiler_family("GNU") == "gcc"
    assert package_static_sdk.canonical_compiler_family("AppleClang") == "apple-clang"
    assert package_static_sdk.canonical_compiler_family("MSVC") == "msvc"
