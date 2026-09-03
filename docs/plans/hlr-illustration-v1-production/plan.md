+++
type = "plan"
id = "hlr-illustration-v1-production"
status = "active"
created = "2026-09-03"

[[steps]]
id = "evaluation-handoff"
title = "Freeze the accepted V1 scope and transfer remaining evaluation work into required or explicitly deferred production items"
status = "done"

[[steps]]
id = "compatibility-contract"
title = "Specify canonical additive HLR and illustration contracts plus the old-option compatibility matrix"
status = "done"
depends_on = ["evaluation-handoff"]

[[steps]]
id = "mesh-input-contract"
title = "Specify a bounded indexed-mesh input contract for synthesized geometry without requiring STEP"
status = "done"
depends_on = ["compatibility-contract"]

[[steps]]
id = "fast-runtime-api"
title = "Promote one-shot and reusable prepared-mesh Fast HLR APIs without changing exact or polygonal paths"
status = "active"
depends_on = ["mesh-input-contract"]

[[steps]]
id = "operation-transports"
title = "Register governed HLR operations through the generic C ABI, native IPC, WASM, Python, TypeScript, and Rust clients"
status = "pending"
depends_on = ["fast-runtime-api"]

[[steps]]
id = "illustration-package"
title = "Move illustration preparation and SVG/Canvas rendering from the demo into the production TypeScript package"
status = "pending"
depends_on = ["compatibility-contract"]

[[steps]]
id = "raster-hlr-package"
title = "Publish retained GPU raster HLR as an explicitly browser-only renderer API separate from vector projection"
status = "pending"
depends_on = ["compatibility-contract"]

[[steps]]
id = "convenience-composition"
title = "Add a convenience facade that composes mesh preparation, Fast vector linework, colorization, and rendering"
status = "pending"
depends_on = ["operation-transports", "illustration-package"]

[[steps]]
id = "demo-migration"
title = "Migrate HLR Lab and Illustration Lab to consume only production package APIs and typed clients"
status = "pending"
depends_on = ["convenience-composition", "raster-hlr-package"]

[[steps]]
id = "consumer-documentation"
title = "Publish algorithm discovery, option compatibility, examples, generated contract reference, and migration guidance"
status = "pending"
depends_on = ["operation-transports", "convenience-composition"]

[[steps]]
id = "compatibility-validation"
title = "Prove old defaults and options remain unchanged and validate new native, WASM, package, and downstream paths"
status = "pending"
depends_on = ["demo-migration", "consumer-documentation"]

[[steps]]
id = "design-doc-intent-audit"
title = "Audit design docs, ADRs, requirements, generated references, and compatibility guidance against implementation"
status = "pending"
depends_on = ["compatibility-validation"]

[[steps]]
id = "test-runtime-impact-audit"
title = "Audit correctness coverage, Rack registration, benchmark evidence, and added test runtime"
status = "pending"
depends_on = ["compatibility-validation"]

[[steps]]
id = "external-review"
title = "Obtain independent contract, compatibility, numerical, implementation, and package review"
status = "pending"
depends_on = ["design-doc-intent-audit", "test-runtime-impact-audit"]

[[steps]]
id = "user-demo-signoff"
title = "Present the packaged HLR and Illustration demos for explicit user acceptance"
status = "pending"
depends_on = ["external-review"]

[[steps]]
id = "release-closeout"
title = "Run release gates, update distributions and durable records, and remove completed temporary plans"
status = "pending"
depends_on = ["user-demo-signoff"]

[[exit_criteria]]
id = "additive-compatibility"
title = "Existing exact and polygonal HLR options, aliases, defaults, results, CLI calls, Python calls, and C ABI compatibility entry points retain their supported behavior"
status = "pending"

[[exit_criteria]]
id = "discoverable-fast-mode"
title = "Consumers can discover Fast selection, applicable options, defaults, units, limitations, and examples through generated contracts, package types, CLI help, and maintained documentation"
status = "pending"

[[exit_criteria]]
id = "mesh-source"
title = "Synthesized indexed meshes can use Fast HLR without STEP or application-specific policy"
status = "pending"

[[exit_criteria]]
id = "prepared-runtime"
title = "Direct runtimes can prepare once and project multiple views while one-shot consumers retain a simple operation"
status = "pending"

[[exit_criteria]]
id = "illustration-api"
title = "Consumers call a production TypeScript illustration API and do not reimplement demo preparation, visibility ordering, fusion, colorization, SVG, Canvas, or caching logic"
status = "pending"

[[exit_criteria]]
id = "raster-boundary"
title = "Browser raster HLR is separately named, documented as pixel output, and cannot be confused with vector Fast HLR"
status = "pending"

[[exit_criteria]]
id = "production-demos"
title = "HLR Lab and Illustration Lab use only production APIs for governed geometry and illustration behavior"
status = "pending"

[[exit_criteria]]
id = "cross-transport-hlr"
title = "Governed Fast HLR contracts and equivalent results are available through native, C ABI, executable IPC, WASM, Python, TypeScript, and Rust surfaces in their documented support lanes"
status = "pending"

[[exit_criteria]]
id = "user-demo-signoff"
title = "The user explicitly approves the final packaged HLR and Illustration demos before release closeout"
status = "pending"

[[exit_criteria]]
id = "design-doc-intent-audit"
title = "Design docs, ADRs, requirements, generated references, and compatibility guidance match implementation"
status = "pending"

[[exit_criteria]]
id = "test-runtime-impact-audit"
title = "New tests are registered, runtime impact is reviewed, and benchmark evidence remains reproducible"
status = "pending"

[[exit_criteria]]
id = "external-review"
title = "Independent contract, compatibility, numerical, implementation, and package review findings are resolved"
status = "pending"

[[exit_criteria]]
id = "signoff"
title = "Focused checks, contract/package freshness, native validation, Rack strata, L99, distribution validation, and downstream compatibility snapshots pass"
status = "pending"
+++

# Fast HLR and Illustration V1 Production

## Purpose

Promote the accepted Fast vector HLR, Fast mesh-shadow outline, retained browser
raster HLR, and mesh illustration work into supported additive interfaces. V1
must make the new behavior easy to discover and use without changing the
meaning, defaults, or availability of existing polygonal and exact HLR calls.

This plan supersedes the delivery portion of `fast-hlr-backend`. That plan
remains the evaluation record until its accepted evidence is transferred and
its temporary files can be removed during closeout.

## V1 product boundary

V1 has three deliberately separate implementation products:

1. **Vector HLR geometry** is implemented in generic C++ and returns the
   renderer-neutral projection result. STEP, indexed-mesh, native, WASM,
   executable, Python, TypeScript, and Rust consumers use this product.
2. **Mesh illustration** becomes a supported TypeScript/browser/Node package.
   It owns preparation, visibility ordering, safe fusion and coplanar layering,
   shading, colorization, SVG/Canvas rendering, caching, and disposal. The demo
   is a consumer, not an implementation authority. Native illustration
   execution is deferred until a named native consumer requires a C++ port.
3. **Raster HLR** becomes an explicitly browser-only renderer helper. It owns a
   framebuffer result, not vector geometry, and is named and documented so it
   cannot be mistaken for the deterministic Fast vector projection backend.

The production package exposes both a simple one-shot facade and reusable
prepared objects. Combining HLR and illustration is a convenience composition;
the underlying contracts remain independently usable.

## Minimal public interface

The portable HLR contract introduces two operations sharing one option and
result family:

- `geometry.model_hlr_projection.a0`: model/STEP attachment to projection;
- `geometry.mesh_hlr_projection.a0`: bounded indexed-mesh attachment to the
  same projection result.

The semantic API exposes these concepts:

- indexed mesh or supported source-model bytes;
- one or more orthographic views;
- common HLR options plus an explicitly nested Fast option block;
- independent `detail`, `outline`, and `bbox` result layers;
- a reusable prepared Fast HLR model for multiple views; and
- one-shot wrappers for documentation, CLI, Python, and executable consumers.

The TypeScript package adds explicit `illustration` and `raster-hlr` exports.
Illustration exposes one-shot `illustrateMesh` plus reusable
`createIllustrator`/render/dispose behavior. Its prepared scene remains opaque
so triangle sorting and fusion internals can evolve without breaking consumers.

Production illustration contract identities begin at generation `a0`:

- `geometry.mesh_illustration.input.a0`;
- `geometry.mesh_illustration.style.a0`; and
- `geometry.mesh_illustration.result.a0`.

The experimental `geometry.mesh_illustration.prototype.a0` identity is not a
production predecessor and creates no compatibility obligation. It is replaced
by the reviewed production `a0` family rather than promoted or renamed to a
later generation.

## Compatibility rules

- `projection_algorithm` remains additive: `poly`, `exact`, or `fast`.
- The default remains `poly`; V1 does not silently select Fast.
- `outline_algorithm` retains `hlr-close` and `mesh-shadow` and adds
  `fast-mesh-shadow` without changing either old algorithm.
- All existing common options, ten OCCT edge-category options, legacy aliases,
  CLI flags, Python helpers, and C ABI compatibility functions remain accepted
  in their documented lanes.
- Fast-only controls remain under `fast`. They never redefine OCCT smooth,
  sewn, isoline, exact-curve, or gap-closure behavior.
- Existing result layers remain independently composable. Illustration styling
  never leaks into the HLR geometry result.
- Illustration starts at production generation `a0`. Package functions use
  ordinary unversioned ergonomic names while serialized DTOs carry the explicit
  `a0` identities.
- Canonical strict contracts use one spelling and preserve absent-versus-
  present fields. Existing aliases live in an explicit compatibility adapter.
- Every option is classified in a maintained matrix as common,
  exact/poly-only, Fast-supported, delegated to outline, ignored for a stated
  compatibility reason, or rejected. The matrix records its default, units,
  and effect.

Golden compatibility vectors must cover an omitted options object, every old
algorithm and outline combination, each legacy alias, each existing edge flag,
layer-selection combinations, new Fast defaults, and nested Fast overrides.

## Consumer discovery and documentation

Consumers must not need to inspect demo source to learn that Fast exists. V1
publishes the following documentation surfaces:

- generated TypeSpec/HTML contract references and typed enums/codecs;
- generic operation capability-catalog entries and attachment descriptions;
- C++ API reference and runnable prepared/one-shot examples;
- TypeScript direct-WASM and Worker examples;
- Rust executable-client and Python wrapper examples;
- CLI `--help` algorithm names and a maintained HLR guide;
- an algorithm/option applicability table with defaults, units, output kind,
  performance posture, and known limitations; and
- a migration guide showing an unchanged old request, the smallest Fast STEP
  request, a synthesized-mesh request, and composed illustration output.

Documentation must distinguish one-shot wall time from prepared-view time and
must distinguish vector Fast HLR from browser raster HLR.

## Demo migration and user acceptance

HLR Lab migrates from handwritten Worker JSON to the generated typed HLR
client. Illustration Lab imports the production illustration and raster-HLR
package exports. Demo code retains only UI state, file selection, camera
control, presentation, downloads, and validation orchestration.

The packaged demos must continue to support bundled and uploaded STEP models,
all view and layer controls, exact/poly/Fast comparison, independent outline
selection, Fast options, illustration styling, SVG export, and raster camera
interaction. Automated browser gates run first. The final release gate then
stops and presents both packaged demo paths plus a concise checklist to the
user. `user-demo-signoff` cannot be inferred from automated tests and must be
set only after explicit approval.

## V1 validation

- Contract generation/freshness and governed vectors across C++, TypeScript,
  Rust, and Python.
- Direct semantic C++ tests for indexed meshes, preparation reuse, malformed
  inputs, limits, deterministic output, and existing STEP compatibility.
- Generic C ABI, native executable IPC, browser WASM, direct TypeScript, Worker,
  Python, and Rust round trips appropriate to each documented support lane.
- Byte or governed geometric equivalence across native and WASM Fast results.
- Regression vectors proving exact/poly defaults and old option behavior did
  not change.
- Package-consumer tests proving illustration works outside repository aliases
  and demos import production exports rather than copied algorithms.
- Browser tests for HLR Lab, Illustration Lab, uploads, downloads, controls,
  raster interaction, and packaged single-file closure.
- Current Fast benchmark corpus, including ABM8 where appropriate, with
  one-shot and prepared-view timings reported separately.
- Downstream compatibility snapshots for maintained Viz and documentation
  consumers before publication.

## Explicit post-V1 work

The following do not block the accepted first production version, but must
remain visible in durable limitations or follow-up work:

- general curve fitting beyond the current visibility-safe collinear joins;
- changing the default HLR backend to Fast;
- perspective vector-projection guarantees;
- multithreaded WASM visibility execution;
- a stable serialized prepared-model format or cross-process resident session;
- a native C++ illustration renderer and native illustration operation;
- screen-space raster line quads, full dynamic smooth silhouettes, transparent
  occlusion policy, and broader GPU/browser qualification; and
- application-specific PCB, documentation, or visualizer styling policy.

## Closeout

After explicit demo approval and release validation, move settled decisions
into ADRs, requirements, generated contracts, interface docs, compatibility
records, tests, examples, and release notes. Remove both completed temporary
plans according to repository policy.
