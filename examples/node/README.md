# Node STEP Topology Annotation Reference

This student-facing TypeScript example exercises the bounded native research
surface without exposing OCCT or raw STEP internals. It opens and inspects a
STEP model, validates a sealed GLB work packet before Three.js parses it, uses
`GLTFLoader` and `Raycaster` to select a real triangle, resolves that hit to a
transient face, creates a logical group and metadata probe, checkpoints the edit
journal, replaces the native process, and restores under exact replay
preconditions.

Build the generated contracts and bundled reference application from the
repository root:

```powershell
npm run generate:contracts
```

Run it against a native Geometer executable and a STEP model:

```powershell
node dist/native/examples/step-topology-annotation-reference.mjs `
  dist/native/windows-x64/geometer.exe `
  tests/fixtures/step/embedded_models/SOT-23.STEP
```

The command prints a small JSON evidence report. It deliberately reports
source and artifact digests plus stable authored ids, but does not serialize
session handles, topology handles, Three.js object ids, or triangle indexes as
durable annotation identity.

The example is a Geometer transport/research reference, not an Appz annotation
domain contract. See the
[Annotation Lab handoff](../../docs/design/step-topology-appz-annotation-lab-handoff.md)
before adapting the flow in Appz. General save/export and changed-source
recovery analysis are intentionally absent because those operations are not yet
advertised by the native runtime.
