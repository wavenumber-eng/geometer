from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _unique(values: list[Any], label: str) -> None:
    assert len(values) == len(set(values)), f"duplicate {label}"


def _assert_candidate_contracts(contracts: list[dict[str, Any]], operations: list[dict[str, Any]]) -> None:
    experimental_contracts = {item["id"] for item in contracts if item["status"] == "experimental_candidate"}
    assert experimental_contracts == {
        "geometry.step_topology.open.request.a0",
        "geometry.step_topology.open.result.a0",
        "geometry.step_topology.close.request.a0",
        "geometry.step_topology.close.result.a0",
        "geometry.step_topology.inspect.request.a0",
        "geometry.step_topology.inspect.result.a0",
        "geometry.step_topology.render.request.a0",
        "geometry.step_topology.render.result.a0",
        "geometry.step_topology.resolve_hit.request.a0",
        "geometry.step_topology.resolve_hit.result.a0",
        "geometry.step_topology.apply_logical_groups.request.a0",
        "geometry.step_topology.apply_logical_groups.result.a0",
        "geometry.step_topology.apply_metadata_probes.request.a0",
        "geometry.step_topology.apply_metadata_probes.result.a0",
        "geometry.step_topology.checkpoint_edit_journal.request.a0",
        "geometry.step_topology.checkpoint_edit_journal.result.a0",
        "geometry.step_topology.apply_hierarchy.request.a0",
        "geometry.step_topology.apply_hierarchy.result.a0",
        "geometry.step_topology.save.request.a0",
        "geometry.step_topology.save.result.a0",
        "geometry.step_topology.restore.request.a0",
        "geometry.step_topology.restore.result.a0",
        "geometry.step_topology.analyze_recovery.request.a0",
        "geometry.step_topology.analyze_recovery.result.a0",
    }
    assert all(
        item["current_authority"] == "typespec_candidate"
        for item in contracts
        if item["status"] == "experimental_candidate"
    )
    assert {item["id"] for item in operations if item["status"] == "experimental_candidate"} == {
        "geometry.step_topology.open.a0",
        "geometry.step_topology.close.a0",
        "geometry.step_topology.inspect.a0",
        "geometry.step_topology.render.a0",
        "geometry.step_topology.resolve_hit.a0",
        "geometry.step_topology.apply_logical_groups.a0",
        "geometry.step_topology.apply_metadata_probes.a0",
        "geometry.step_topology.checkpoint_edit_journal.a0",
        "geometry.step_topology.apply_hierarchy.a0",
        "geometry.step_topology.save.a0",
        "geometry.step_topology.restore.a0",
        "geometry.step_topology.analyze_recovery.a0",
    }


def _assert_slice_a(manifest: dict[str, Any]) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    slice_a = manifest["experimental_evidence"]["step_topology_slice_a"]
    assert slice_a == {
        "status": "unpromoted_structural_candidate",
        "catalog_sha256": "568219edea253812467edf179faa2f2fc35dc2e29855524ac6918b284aa6574c",
        "vector_manifest_sha256": ("d43333ddc5a216fc02eeb909561ee436d211060d308be29c8e1a1de4ac531496"),
        "governed_vector_ids": [
            "strict.ipc-step-topology-open-envelope.accept",
            "strict.operation-step-topology-open-outcome.accept",
            "semantic.step-topology-ipc-envelope-pair-matrix.accept",
            "semantic.step-topology-ipc-request-operation-mismatch.reject",
            "semantic.step-topology-operation-result-operation-mismatch.reject",
            "strict.step-topology-resolve-hit.accept",
            "schema.step-topology-resolve-hit-unknown-field.reject",
            "schema.step-topology-resolve-hit-zero-generation.reject",
            "schema.step-topology-resolve-hit-range.reject",
            "schema.step-topology-render-attachment-relationship.reject",
            "schema.step-topology-resolve-hit-result-max.accept",
            "schema.step-topology-resolve-hit-result-instance-over-max.reject",
            "schema.step-topology-resolve-hit-result-primitive-over-max.reject",
            "schema.step-topology-resolve-hit-result-triangle-over-max.reject",
            "schema.step-topology-inspect-depth-max.accept",
            "schema.step-topology-inspect-depth-over-max.reject",
            "semantic.step-topology-stale-generation.reject",
            "semantic.step-topology-duplicate-target.reject",
            "semantic.step-topology-inspection-membership.accept",
            "semantic.step-topology-cross-page-duplicate.reject",
            "semantic.step-topology-cross-page-edge.reject",
            "semantic.step-topology-over-count-before-terminal.reject",
            "semantic.step-topology-empty-continuation.reject",
            "semantic.step-topology-high-fan-in.accept",
            "semantic.step-topology-render-attachments.accept",
            "semantic.step-topology-render-digest-relationship.reject",
        ],
        "governed_vector_count": 26,
        "structural_vector_count": 13,
        "semantic_vector_count": 13,
        "operation_vector_count": 0,
        "runtime_advertised": False,
        "native_runtime_operations": [
            "geometry.step_topology.close.a0",
            "geometry.step_topology.inspect.a0",
            "geometry.step_topology.open.a0",
            "geometry.step_topology.render.a0",
            "geometry.step_topology.resolve_hit.a0",
        ],
        "projections": ["json_schema", "cpp", "typescript", "rust", "python", "html"],
        "behavioral_authority": "focused_cpp_step_topology_session",
        "legacy_gltf_enrichment_reused": False,
    }
    assert _sha256(ROOT / manifest["toolchain"]["catalog"]) == slice_a["catalog_sha256"]
    assert _sha256(ROOT / "tests/contracts/vectors/manifest.json") == slice_a["vector_manifest_sha256"]
    vector_manifest = json.loads((ROOT / "tests/contracts/vectors/manifest.json").read_text())
    topology_vectors = [item for item in vector_manifest["vectors"] if "step-topology" in item["id"]]
    slice_a_vectors = [item for item in topology_vectors if item["id"] in set(slice_a["governed_vector_ids"])]
    assert [item["id"] for item in slice_a_vectors] == slice_a["governed_vector_ids"]
    assert sum(item["lane"] == "semantic" for item in slice_a_vectors) == slice_a["semantic_vector_count"]

    return topology_vectors, slice_a


def _assert_slice_b(manifest: dict[str, Any], topology_vectors: list[dict[str, Any]]) -> dict[str, Any]:
    slice_b = manifest["experimental_evidence"]["step_topology_slice_b"]
    assert slice_b == {
        "status": "unpromoted_structural_candidate",
        "catalog_sha256": "568219edea253812467edf179faa2f2fc35dc2e29855524ac6918b284aa6574c",
        "vector_manifest_sha256": ("d43333ddc5a216fc02eeb909561ee436d211060d308be29c8e1a1de4ac531496"),
        "governed_vector_ids": [
            "strict.step-topology-apply-groups.accept",
            "semantic.step-topology-apply-groups-uppercase.accept",
            "schema.step-topology-apply-groups-unknown-field.reject",
            "semantic.step-topology-apply-groups-short-handle.reject",
            "schema.step-topology-apply-groups-create-revision.reject",
            "semantic.step-topology-apply-groups-duplicate-member.reject",
            "semantic.step-topology-apply-groups-invalid-namespace.reject",
            "semantic.step-topology-apply-groups-duplicate-create.reject",
            "strict.step-topology-probe-duplicate-discriminator.reject",
            "strict.step-topology-probe-duplicate-payload-field.reject",
            "schema.step-topology-probe-erase-payload.reject",
            "schema.step-topology-probe-invalid-target-shape.reject",
            "semantic.step-topology-probe-invalid-key-namespace.reject",
            "semantic.step-topology-checkpoint-attachment.accept",
            "semantic.step-topology-checkpoint-digest.reject",
            "semantic.step-topology-checkpoint-revision.reject",
            "schema.step-topology-checkpoint-bytes-over-max.reject",
            "semantic.step-topology-apply-groups-lifecycle.accept",
            "semantic.step-topology-apply-probes-targets-lifecycle.accept",
            "semantic.step-topology-apply-groups-result-max-revision.accept",
            "schema.step-topology-apply-probes-result.accept",
            "semantic.step-topology-apply-probes-result-groups.accept",
            "strict.step-topology-checkpoint-request.accept",
            "semantic.step-topology-probe-invalid-group-namespace.reject",
            "semantic.step-topology-probe-duplicate-attach.reject",
            "semantic.step-topology-group-request-aggregate-members-exact.accept",
            "semantic.step-topology-group-request-aggregate-members.reject",
            "semantic.step-topology-group-result-aggregate-members-exact.accept",
            "semantic.step-topology-group-result-aggregate-members.reject",
            "semantic.step-topology-checkpoint-name.reject",
            "semantic.step-topology-checkpoint-media.reject",
            "semantic.step-topology-checkpoint-bytes.reject",
            "semantic.step-topology-checkpoint-count.reject",
            "semantic.step-topology-apply-hierarchy.accept",
            "semantic.step-topology-apply-hierarchy-lifecycle.accept",
            "semantic.step-topology-apply-hierarchy-uppercase.reject",
            "semantic.step-topology-apply-hierarchy-revision-max.accept",
            "schema.step-topology-apply-hierarchy-revision-over-max.reject",
            "semantic.step-topology-hierarchy-result.accept",
            "semantic.step-topology-hierarchy-cycle.reject",
        ],
        "governed_vector_count": 40,
        "structural_vector_count": 11,
        "semantic_vector_count": 29,
        "operation_vector_count": 0,
        "runtime_advertised": False,
        "native_runtime_operations": [
            "geometry.step_topology.apply_logical_groups.a0",
            "geometry.step_topology.apply_metadata_probes.a0",
            "geometry.step_topology.checkpoint_edit_journal.a0",
        ],
        "projections": ["json_schema", "cpp", "typescript", "rust", "python", "html"],
        "behavioral_authority": "focused_cpp_step_topology_logical_groups_hierarchy_and_edit_journal",
        "legacy_gltf_enrichment_reused": False,
    }
    slice_b_vectors = [item for item in topology_vectors if item["id"] in set(slice_b["governed_vector_ids"])]
    assert [item["id"] for item in slice_b_vectors] == slice_b["governed_vector_ids"]
    assert sum(item["lane"] == "semantic" for item in slice_b_vectors) == slice_b["semantic_vector_count"]
    return slice_b


def _assert_slice_c(
    manifest: dict[str, Any],
    topology_vectors: list[dict[str, Any]],
    slice_a: dict[str, Any],
    slice_b: dict[str, Any],
) -> dict[str, Any]:
    slice_c = manifest["experimental_evidence"]["step_topology_slice_c"]
    assert slice_c == {
        "status": "unpromoted_structural_candidate",
        "catalog_sha256": "568219edea253812467edf179faa2f2fc35dc2e29855524ac6918b284aa6574c",
        "vector_manifest_sha256": ("d43333ddc5a216fc02eeb909561ee436d211060d308be29c8e1a1de4ac531496"),
        "governed_vector_ids": [
            "schema.step-topology-save-result.accept",
            "semantic.step-topology-save-attachments.accept",
            "semantic.step-topology-save-media.reject",
            "semantic.step-topology-save-capabilities.reject",
            "semantic.step-topology-save-request-carrier.reject",
            "strict.step-topology-restore-request.accept",
            "semantic.step-topology-restore-attachments.accept",
            "semantic.step-topology-restore-replay-source.reject",
            "semantic.step-topology-restore-model-step.accept",
            "semantic.step-topology-restore-source-media.reject",
            "semantic.step-topology-restore-result.accept",
            "semantic.step-topology-restore-result-source.reject",
            "semantic.step-topology-restore-result-count.reject",
            "semantic.step-topology-restore-result-recovery.reject",
            "semantic.step-topology-recovery-request.accept",
            "semantic.step-topology-recovery-candidates-max.accept",
            "schema.step-topology-recovery-candidates-over-max.reject",
            "semantic.step-topology-recovery-missing-authored-id.reject",
            "semantic.step-topology-recovery-empty-locator.reject",
            "semantic.step-topology-recovery-uppercase-digest.reject",
            "semantic.step-topology-recovery-invalid-bounds.reject",
            "semantic.step-topology-recovery-partial.accept",
            "semantic.step-topology-recovery-count-mismatch.reject",
            "semantic.step-topology-recovery-evidence-count.reject",
            "semantic.step-topology-recovery-resolved-rejected-overlap.reject",
            "semantic.step-topology-recovery-ambiguous.accept",
            "semantic.step-topology-recovery-changed.accept",
            "semantic.step-topology-recovery-unavailable.accept",
            "schema.step-topology-recovery-empty-group.reject",
        ],
        "governed_vector_count": 29,
        "structural_vector_count": 4,
        "semantic_vector_count": 25,
        "operation_vector_count": 0,
        "runtime_advertised": False,
        "native_runtime_operations": ["geometry.step_topology.restore.a0"],
        "projections": ["json_schema", "cpp", "typescript", "rust", "python", "html"],
        "behavioral_authority": "focused_cpp_step_topology_recovery_and_persistence_evidence",
        "legacy_gltf_enrichment_reused": False,
    }
    slice_c_vectors = [item for item in topology_vectors if item["id"] in set(slice_c["governed_vector_ids"])]
    assert [item["id"] for item in slice_c_vectors] == slice_c["governed_vector_ids"]
    assert sum(item["lane"] == "semantic" for item in slice_c_vectors) == slice_c["semantic_vector_count"]
    assert len(topology_vectors) == sum(item["governed_vector_count"] for item in (slice_a, slice_b, slice_c))
    return slice_c


def _assert_runtime_lockstep(
    manifest: dict[str, Any],
    contracts: list[dict[str, Any]],
    slices: tuple[dict[str, Any], dict[str, Any], dict[str, Any]],
) -> None:
    slice_a, slice_b, slice_c = slices
    catalog = json.loads((ROOT / manifest["toolchain"]["catalog"]).read_text(encoding="utf-8"))
    topology_operations = [
        item for item in catalog["operations"] if item["identity"].startswith("geometry.step_topology.")
    ]
    assert len(topology_operations) == 12
    assert all(item["runtime_available"] is False for item in topology_operations)
    catalog_native_operations = {item["identity"] for item in topology_operations if item["native_runtime_available"]}
    manifest_native_operations = {
        operation for evidence in (slice_a, slice_b, slice_c) for operation in evidence["native_runtime_operations"]
    }
    assert manifest_native_operations == catalog_native_operations
    assert sorted(catalog_native_operations) == [
        "geometry.step_topology.apply_logical_groups.a0",
        "geometry.step_topology.apply_metadata_probes.a0",
        "geometry.step_topology.checkpoint_edit_journal.a0",
        "geometry.step_topology.close.a0",
        "geometry.step_topology.inspect.a0",
        "geometry.step_topology.open.a0",
        "geometry.step_topology.render.a0",
        "geometry.step_topology.resolve_hit.a0",
        "geometry.step_topology.restore.a0",
    ]
    topology_contracts = [item for item in contracts if item["id"].startswith("geometry.step_topology.")]
    for contract in topology_contracts:
        operation = re.sub(r"\.(request|result)\.a0$", ".a0", contract["id"])
        expected_native_operation = operation if operation in catalog_native_operations else None
        assert contract.get("native_runtime_operation") == expected_native_operation
    cpp_operation_catalog = (ROOT / "src/cpp/lib/geometer/generated/contracts/operation_catalog.cpp").read_text(
        encoding="utf-8"
    )
    portable_catalog_source = cpp_operation_catalog.split("const char* native_operation_catalog_json", 1)[0]
    native_catalog_source = cpp_operation_catalog.split("const char* native_operation_catalog_json", 1)[1].split(
        "const char* normalized_contract_catalog_sha256", 1
    )[0]
    assert "geometry.step_topology." not in portable_catalog_source
    assert "geometry.step_topology.open.a0" in native_catalog_source
    assert "geometry.step_topology.inspect.a0" in native_catalog_source
    assert "geometry.step_topology.close.a0" in native_catalog_source
    assert "geometry.step_topology.render.a0" in native_catalog_source
    assert "geometry.step_topology.apply_logical_groups.a0" in native_catalog_source
    assert "geometry.step_topology.apply_metadata_probes.a0" in native_catalog_source
    assert "step_topology.checkpoint_edit_journal.a0" in native_catalog_source
    assert "step_topology.restore.a0" in native_catalog_source
    for runtime_catalog in (
        "src/rust/geometer-client/src/generated/operations.rs",
        "python/geometer/_generated/contracts/operations.py",
    ):
        runtime_catalog_text = (ROOT / runtime_catalog).read_text(encoding="utf-8")
        assert "geometry.step_topology.open.a0" in runtime_catalog_text
        assert "geometry.step_topology.inspect.a0" in runtime_catalog_text
        assert "geometry.step_topology.close.a0" in runtime_catalog_text
        assert "geometry.step_topology.render.a0" in runtime_catalog_text
        assert "geometry.step_topology.resolve_hit.a0" in runtime_catalog_text
        assert "geometry.step_topology.apply_logical_groups.a0" in runtime_catalog_text
        assert "geometry.step_topology.apply_metadata_probes.a0" in runtime_catalog_text
        assert "geometry.step_topology.checkpoint_edit_journal.a0" in runtime_catalog_text
        assert "geometry.step_topology.restore.a0" in runtime_catalog_text
    typescript_operations = (ROOT / "src/ts/geometer/generated/operations.ts").read_text(encoding="utf-8")
    assert typescript_operations.count("runtimeAvailable: false") == 12
    assert typescript_operations.count("nativeRuntimeAvailable: true") == 9


def assert_step_topology_inventory(
    manifest: dict[str, Any],
    contracts: list[dict[str, Any]],
    operations: list[dict[str, Any]],
) -> None:
    _assert_candidate_contracts(contracts, operations)
    topology_vectors, slice_a = _assert_slice_a(manifest)
    slice_b = _assert_slice_b(manifest, topology_vectors)
    slice_c = _assert_slice_c(manifest, topology_vectors, slice_a, slice_b)
    _assert_runtime_lockstep(manifest, contracts, (slice_a, slice_b, slice_c))
