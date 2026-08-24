# Geometer Design Docs

These documents are the maintained interface and format record for Geometer.
Implementation plans are temporary working notes; once work is complete, the
durable record belongs here, in ADRs, and in requirements.

## Interface Docs

- [Interface policy and versioning](interface-policy.md)
- [STEP geometry interfaces](step-geometry.md)
- [Planar geometry interfaces](planar-geometry.md)
- [C ABI](c-abi.md)
- [Contract semantics](contract-semantics.md)
- [TypeSpec toolchain and normalized catalog](typespec-toolchain.md)
- [Model-bounds contract compatibility](model-bounds-contract-compatibility.md)
- [Generic operation C ABI](generic-operation-c-abi.md)
- [Executable IPC A0](executable-ipc-a0.md)
- [Transport design review packet](transport-design-review.md)
- [Generated contract reference](generated-contract-reference.md)
- [TypeScript contracts and browser WASM client](typescript-client.md)
- [Rust contracts and executable IPC client](rust-client.md)
- [Browse generated contracts](../generated/contracts/index.html)
- [Geom A0 contract alignment](geom-a0-contract-alignment.md)
- [Analytic planar Boolean A0](analytic-planar-boolean-a0.md)
- [Analytic planar Boolean packet A0](analytic-planar-boolean-packet-a0.md)
- [Exact real-algebraic backend A0](exact-real-algebraic-a0.md) (non-primary
  reference oracle and bounded fallback)
- [Python package interface](python-package.md)
- [WASM interfaces](wasm.md)
- [Browser demo packaging and UI](browser-demos.md)
- [CLI interfaces](cli.md)
- [Dependency cache](dependency-cache.md)
- [STEP topology annotation research](step-topology-annotation-research.md)
- [STEP topology fixture baseline](step-topology-fixture-baseline.md)
- [Native STEP topology inspection research](step-topology-native-inspection.md)
- [Direct STEP topology render-binding research](step-topology-render-binding.md)
- [STEP topology GLB work-packet research](step-topology-glb-binding.md)
- [STEP topology contract Slice A](step-topology-contract-a0.md)
- [STEP topology contract Slice B](step-topology-contract-slice-b.md)
- [STEP topology contract Slice C](step-topology-contract-slice-c.md)
- [Native STEP topology logical-group transactions](step-topology-logical-groups.md)
- [STEP topology edit-journal checkpoint](step-topology-edit-journal.md)
- [STEP topology metadata probes](step-topology-metadata-probes.md)
- [Standard XCAF binary/XML persistence baseline](step-topology-xcaf-persistence.md)
- [AP242 product/body/face persistence baseline](step-topology-ap242-persistence.md)
- [STEP topology multidimensional recovery](step-topology-recovery.md)
- [STEP topology synthetic product hierarchy](step-topology-hierarchy.md)
- [STEP topology Appz Annotation Lab handoff](step-topology-appz-annotation-lab-handoff.md)

## Format Docs

- [JSON formats](json-formats.md)
- [Binary formats](binary-formats.md)
- [Distribution artifacts](distribution.md)
