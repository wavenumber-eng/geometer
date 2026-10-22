from __future__ import annotations

import ssl
from pathlib import Path

import certifi
import pytest

import build_wasm
import check_tls_trust
import validate_python_package as package_validation


def test_installed_requests_bundle_excludes_affected_roots() -> None:
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    context.load_verify_locations(certifi.where())

    assert tuple(map(int, certifi.__version__.split("."))) >= (2026, 10, 19)
    assert check_tls_trust.affected_roots(context) == []


def test_old_uv_is_rejected_before_package_build_downloads(monkeypatch, tmp_path: Path) -> None:
    calls = []
    monkeypatch.setattr(check_tls_trust.shutil, "which", lambda name: "uv")
    monkeypatch.setattr(check_tls_trust.subprocess, "check_output", lambda *args, **kwargs: "uv 0.11.27")
    monkeypatch.setattr(package_validation, "run", lambda *args, **kwargs: calls.append(args))

    with pytest.raises(RuntimeError, match=r"uv >=0.13.4 is required"):
        package_validation.build_wheel(tmp_path)

    assert calls == []


@pytest.mark.parametrize("operation", ["build", "install"])
def test_packaging_bootstraps_pip_through_fixed_uv(monkeypatch, tmp_path: Path, operation: str) -> None:
    class BootstrapReached(Exception):
        pass

    calls = []
    monkeypatch.setattr(package_validation, "require_uv", lambda: "patched-uv")

    def run(command, **kwargs):
        calls.append(command)
        if command[0] == "patched-uv":
            assert command[1:4] == ["pip", "install", "--python"]
            assert "pip>=26.2.2" in command
            raise BootstrapReached

    monkeypatch.setattr(package_validation, "run", run)
    with pytest.raises(BootstrapReached):
        if operation == "build":
            package_validation.build_wheel(tmp_path)
        else:
            package_validation.validate_wheel_install(tmp_path / "test.whl", tmp_path / "test.step", keep_temp=False)

    assert len(calls) == 2
    assert calls[0][1:3] == ["-m", "venv"]


def test_nested_wheel_build_uses_uv_installer(monkeypatch, tmp_path: Path) -> None:
    calls = []
    monkeypatch.setattr(package_validation, "require_uv", lambda: "patched-uv")

    def run(command, **kwargs):
        calls.append(command)
        if command[1:3] == ["-m", "build"]:
            (tmp_path / "wn_geometer-test.whl").write_bytes(b"wheel")

    monkeypatch.setattr(package_validation, "run", run)
    package_validation.build_wheel(tmp_path)

    assert calls[-1][3:5] == ["--installer", "uv"]


def test_wasm_does_not_select_cached_sdk_node(monkeypatch, tmp_path: Path) -> None:
    sdk = tmp_path / "emsdk"
    cached_node = sdk / "node" / "old" / "bin"
    cached_node.mkdir(parents=True)
    fixed_node = str(tmp_path / "patched-node" / "node")
    monkeypatch.setattr(build_wasm, "EMSDK_DIR", sdk)
    monkeypatch.setattr(build_wasm, "require_node", lambda: fixed_node)
    monkeypatch.setenv("PATH", "existing-path")

    environment = build_wasm._emscripten_env()

    assert str(cached_node) not in environment["PATH"]
    assert environment["EM_NODE_JS"] == fixed_node
    assert environment["EMSDK_NODE"] == fixed_node
