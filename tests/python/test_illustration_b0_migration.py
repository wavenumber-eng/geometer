from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
A0_OPERATION = re.compile(
    rb"geometry\.(?:model_illustration|model_illustration_geometry|mesh_illustration|"
    rb"mesh_illustration_geometry|mesh_hlr_projection)\.a0"
)
A0_DIRECT_INPUT = re.compile(rb"geometry\.mesh_illustration(?:_geometry)?\.input\.a0")
CONSUMER_ROOTS = (ROOT / "examples", ROOT / "scripts")
COMPATIBILITY_ALLOWLIST = {
    ROOT / "examples" / "rust" / "native_viewer" / "src" / "settings_tests.rs",
    ROOT / "scripts" / "contract_promotion_topology_inventory.py",
}
SOURCE_SUFFIXES = {".cpp", ".h", ".js", ".md", ".mjs", ".py", ".rs", ".ts"}
DOCS = (
    ROOT / "docs" / "contracts" / "public-entrypoints.md",
    ROOT / "docs" / "design" / "illustration-clipping-b0.md",
    ROOT / "docs" / "design" / "mesh-illustration-native.md",
    ROOT / "docs" / "design" / "mesh-illustration-geometry.md",
    ROOT / "docs" / "design" / "model-illustration-a0.md",
    ROOT / "docs" / "design" / "rust-client.md",
)


def test_maintained_consumers_do_not_select_a0_illustration_operations() -> None:
    violations: list[str] = []
    for root in CONSUMER_ROOTS:
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix not in SOURCE_SUFFIXES or path in COMPATIBILITY_ALLOWLIST:
                continue
            try:
                contents = path.read_bytes()
            except OSError:
                continue
            if A0_OPERATION.search(contents) or A0_DIRECT_INPUT.search(contents):
                violations.append(path.relative_to(ROOT).as_posix())
    assert violations == [], "live consumers still select A0 illustration operations: " + ", ".join(violations)


def test_durable_docs_name_b0_as_the_canonical_illustration_generation() -> None:
    combined = "\n".join(path.read_text(encoding="utf-8") for path in DOCS)
    for operation in (
        "geometry.model_illustration.b0",
        "geometry.model_illustration_geometry.b0",
        "geometry.mesh_illustration.b0",
        "geometry.mesh_illustration_geometry.b0",
        "geometry.mesh_hlr_projection.b0",
    ):
        assert operation in combined
    assert "A0 compatibility" in combined or "A0 operations remain" in combined


def test_typescript_unqualified_api_is_transport_backed_b0() -> None:
    renderer = (ROOT / "src" / "ts" / "geometer" / "mesh-illustration.ts").read_text(encoding="utf-8")
    for name in ("createIllustrator", "illustrateMesh", "illustrateMeshGeometry"):
        assert f"export function {name}(" not in renderer
        assert f"export function {name}A0(" in renderer

    wasm = (ROOT / "src" / "ts" / "geometer" / "wasm.ts").read_text(encoding="utf-8")
    assert "async meshIllustration(" in wasm
    assert "async meshIllustrationGeometry(" in wasm
    assert "async meshHlrProjection(request: MeshHlrProjectionRequest): Promise<HlrProjectionResultB0>" in wasm
    assert "async meshHlrProjectionA0(" in wasm
    assert '"geometry.mesh_illustration.b0"' in wasm
    assert '"geometry.mesh_illustration_geometry.b0"' in wasm
    assert '"geometry.mesh_hlr_projection.b0"' in wasm

    for relative in ("worker.ts", "ipc-client-a0.ts"):
        client = (ROOT / "src" / "ts" / "geometer" / relative).read_text(encoding="utf-8")
        assert "async meshHlrProjection(" in client
        assert "async meshHlrProjectionA0(" in client
        assert '"geometry.mesh_hlr_projection.b0"' in client

    compatibility = (ROOT / "src" / "ts" / "geometer" / "illustrated-hlr.ts").read_text(encoding="utf-8")
    assert "export async function createFastHlrIllustrator(" not in compatibility
    assert "export async function illustrateMeshWithFastHlr(" not in compatibility
    assert "export async function createFastHlrIllustratorA0(" in compatibility
    assert "export async function illustrateMeshWithFastHlrA0(" in compatibility


def test_python_and_rust_unqualified_mesh_hlr_api_is_b0() -> None:
    python = (ROOT / "python" / "geometer" / "_ipc_client.py").read_text(encoding="utf-8")
    canonical_python = python.index("    def mesh_hlr_projection(\n")
    explicit_a0_python = python.index("    def mesh_hlr_projection_a0(\n")
    assert '"geometry.mesh_hlr_projection.b0"' in python[canonical_python:explicit_a0_python]

    rust = (ROOT / "src" / "rust" / "geometer-client" / "src" / "hlr.rs").read_text(encoding="utf-8")
    canonical_rust = rust.index("            pub async fn mesh_hlr_projection(\n")
    explicit_b0_rust = rust.index("            pub async fn mesh_hlr_projection_b0(\n")
    assert '"geometry.mesh_hlr_projection.b0"' in rust[canonical_rust:explicit_b0_rust]
    assert "pub async fn mesh_hlr_projection_a0(" in rust
    assert "pub struct MeshHlrProjectionRequestA0" in rust


def test_release_qualification_executes_b0_on_every_native_platform() -> None:
    native_validation = (ROOT / "scripts" / "validate_native.py").read_text(encoding="utf-8")
    assert '"geometer_illustration_clipping_test"' in native_validation
    assert '"geometer_illustration_b0_operation_test"' in native_validation
    assert '"geometer_native_operation_execution_gate_test"' in native_validation

    package_validation = (ROOT / "scripts" / "validate_python_package.py").read_text(encoding="utf-8")
    assert 'client_name="python-wheel-b0-validation"' in package_validation
    assert 'schema="geometry.mesh_illustration.input.b0"' in package_validation
    assert "partial_illustration.fragment.clipping is None" in package_validation
    assert "not empty_illustration.empty" in package_validation

    release = (ROOT / ".github" / "workflows" / "release-candidate.yml").read_text(encoding="utf-8")
    candidate = (ROOT / "scripts" / "build_release_candidate.py").read_text(encoding="utf-8")
    assert "scripts/build_release_candidate.py" in release
    assert '"validate_native.py"' in candidate
    assert '"validate_python_package.py"' in candidate
    assert '"validate_static_sdk.py"' in candidate
