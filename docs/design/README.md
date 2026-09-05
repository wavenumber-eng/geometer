# Geometer Interfaces

Start with the [executable IPC consumer guide](executable-ipc.md) for persistent
process integration. These documents specify callable behavior and wire/value
contracts. Experimental specifications are retained without implying production
readiness. Build and demo procedures live in [developer documentation](../developer/README.md);
measurements and abandoned approaches live in [research](../research/README.md).

## Supported Interface Boundaries

- [Interface policy and versioning](interface-policy.md)
- [STEP geometry](step-geometry.md) and [planar geometry](planar-geometry.md)
- [Colored model tessellation A0 (development candidate)](model-tessellation-a0.md)
- [Python package](python-package.md)
- [CLI commands](cli.md)
- [Executable IPC consumer guide](executable-ipc.md) and [A0 wire protocol](executable-ipc-a0.md)
- [C ABI](c-abi.md) and [generic operation C ABI](generic-operation-c-abi.md)
- [WASM](wasm.md), [TypeScript clients](typescript-client.md), and [Rust client](rust-client.md)
- [HLR projection and browser illustration](hlr-projection-a0.md)
- [Contract semantics](contract-semantics.md)
- [JSON formats](json-formats.md), [binary packets](binary-formats.md), and [distribution](distribution.md)

## Experimental Interfaces

The analytic solver is not production-ready. Topology is an unpromoted native
research API; some declarations are structural only. Consult the actual runtime
catalog rather than inferring availability from a specification.

- [Analytic logical contract and limitations](analytic-planar-boolean-a0.md)
- [Analytic packed A0 contract](analytic-planar-boolean-packet-a0.md)
- [Topology lifecycle/inspection/render Slice A](step-topology-contract-a0.md)
- [Topology mutation/checkpoint Slice B](step-topology-contract-slice-b.md)
- [Topology restore/persistence Slice C](step-topology-contract-slice-c.md)
- [Native inspection value API](step-topology-native-inspection.md)
- [Render bindings](step-topology-render-binding.md) and [GLB work packets](step-topology-glb-binding.md)
- [Logical groups](step-topology-logical-groups.md), [metadata probes](step-topology-metadata-probes.md), and [edit journal](step-topology-edit-journal.md)
- [Hierarchy value model](step-topology-hierarchy.md) and [recovery value model](step-topology-recovery.md)

## Contract Ownership And Background

- [TypeSpec and contract authority](../contracts/README.md)
- [Generated contract and operation reference](../generated/contracts/index.html)
- [Migration roadmap](../contracts/typespec-coverage-assessment.md)
- [Research and historical evidence](../research/README.md)
- [Documentation maintenance and disposition map](../developer/documentation.md)
