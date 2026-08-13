from __future__ import annotations

import json
import subprocess
import sys
import tarfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "src" / "rust" / "geometer-client" / "Cargo.toml"


def run(*args: str, cwd: Path = ROOT) -> None:
    subprocess.run(args, cwd=cwd, check=True)


def test_rust_format_lint_and_live_conformance() -> None:
    run("cargo", "fmt", "--manifest-path", str(MANIFEST), "--all", "--", "--check")
    run(
        "cargo",
        "clippy",
        "--manifest-path",
        str(MANIFEST),
        "--all-targets",
        "--locked",
        "--",
        "-D",
        "warnings",
    )
    run("cargo", "test", "--manifest-path", str(MANIFEST), "--locked")
    run("wn-dev-std", "audit", str(MANIFEST.parent), "--scope", "language")


def test_clean_external_consumer(tmp_path: Path) -> None:
    crate = MANIFEST.parent
    package_dir = tmp_path / "package"
    package_dir.mkdir()
    run("cargo", "package", "--manifest-path", str(MANIFEST), "--allow-dirty")
    archives = sorted((crate / "target" / "package").glob("geometer-client-*.crate"))
    assert archives
    with tarfile.open(archives[-1], "r:gz") as archive:
        if sys.version_info >= (3, 12):
            archive.extractall(package_dir, filter="data")
        else:
            archive.extractall(package_dir)
    unpacked = next(package_dir.iterdir())

    consumer = tmp_path / "consumer"
    (consumer / "src").mkdir(parents=True)
    cargo_toml = {
        "package": {"name": "geometer-client-consumer", "version": "0.0.0", "edition": "2024"},
        "dependencies": {"geometer-client": {"path": str(unpacked)}},
    }
    (consumer / "Cargo.toml").write_text(_toml(cargo_toml), encoding="utf-8")
    (consumer / "src" / "main.rs").write_text(
        "use geometer_client::contracts::ModelBoundsOptionsA0;\n"
        "fn main() { let value = ModelBoundsOptionsA0 { format: None, model_transform: None }; "
        "let _ = geometer_client::contracts::encode_model_bounds_options_a0_json(&value).unwrap(); }\n",
        encoding="utf-8",
    )
    run("cargo", "generate-lockfile", cwd=consumer)
    run("cargo", "check", "--locked", cwd=consumer)


def _toml(document: dict[str, object]) -> str:
    package = document["package"]
    dependency = document["dependencies"]
    assert isinstance(package, dict) and isinstance(dependency, dict)
    lines = ["[package]"]
    lines.extend(f"{key} = {json.dumps(value)}" for key, value in package.items())
    lines.extend(["", "[dependencies]"])
    for name, value in dependency.items():
        assert isinstance(value, dict)
        fields = ", ".join(f"{key} = {json.dumps(item)}" for key, item in value.items())
        lines.append(f"{name} = {{ {fields} }}")
    return "\n".join(lines) + "\n"
