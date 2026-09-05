+++
type = "plan"
id = "geometer-documentation-cleanup"
status = "active"
created = "2026-09-05"

[[steps]]
id = "baseline-inventory"
title = "Freeze the documentation, interface, contract-authority, and demo baseline"
status = "done"

[[steps]]
id = "documentation-taxonomy"
title = "Define the durable documentation taxonomy and disposition every current design document"
status = "pending"
depends_on = ["baseline-inventory"]

[[steps]]
id = "readme-refresh"
title = "Rewrite the root README around supported capabilities, maturity, and primary entry points"
status = "pending"
depends_on = ["documentation-taxonomy"]

[[steps]]
id = "ipc-consumer-guide"
title = "Create a concise executable IPC consumer guide and correct the implemented protocol reference"
status = "pending"
depends_on = ["documentation-taxonomy"]

[[steps]]
id = "contract-authority-matrix"
title = "Document TypeSpec coverage and the authority of every logical and packed contract"
status = "pending"
depends_on = ["baseline-inventory"]

[[steps]]
id = "alx-generation-assessment"
title = "Assess ALX HTML generation, shared presentation, and Geometer generation gaps"
status = "done"
depends_on = ["baseline-inventory"]

[[steps]]
id = "all-operation-typespec-roadmap"
title = "Specify migration waves for every operation, including logical contracts, packed codecs, and executable adapters"
status = "pending"
depends_on = ["contract-authority-matrix", "alx-generation-assessment"]

[[steps]]
id = "shared-html-presentation"
title = "Align Geometer documentation with the ALX shared stylesheet and reusable page components"
status = "pending"
depends_on = ["documentation-taxonomy", "alx-generation-assessment"]

[[steps]]
id = "generated-html-expansion"
title = "Generate operation coverage, packet references, relationships, and documentation indexes from governed sources"
status = "pending"
depends_on = ["contract-authority-matrix", "shared-html-presentation", "all-operation-typespec-roadmap"]

[[steps]]
id = "design-doc-cleanup"
title = "Keep interface specifications in design and relocate research, evidence, and maintainer procedures"
status = "pending"
depends_on = ["documentation-taxonomy", "contract-authority-matrix"]

[[steps]]
id = "demo-runtime-audit"
title = "Run the smallest current validation for every demo and record reproducible status"
status = "pending"
depends_on = ["baseline-inventory"]

[[steps]]
id = "demo-disposition-review"
title = "Approve keep, repair, archive, or remove dispositions for every demo"
status = "pending"
depends_on = ["demo-runtime-audit"]

[[steps]]
id = "demo-cleanup"
title = "Apply approved demo documentation, moves, repairs, and pruning"
status = "pending"
depends_on = ["demo-disposition-review", "readme-refresh"]

[[steps]]
id = "documentation-drift-gates"
title = "Add lightweight checks for links, interface inventory freshness, maturity labels, and demo registration"
status = "pending"
depends_on = ["design-doc-cleanup", "ipc-consumer-guide", "demo-cleanup", "generated-html-expansion"]

[[steps]]
id = "closeout"
title = "Update durable docs, record release-facing changes, and remove this completed plan"
status = "pending"
depends_on = ["documentation-drift-gates"]

[[exit_criteria]]
id = "readme"
title = "A new consumer can select the correct supported interface and reach its canonical documentation from the root README"
status = "pending"

[[exit_criteria]]
id = "design-directory"
title = "Every document under docs/design primarily specifies a current interface, format, or architecture boundary"
status = "pending"

[[exit_criteria]]
id = "ipc"
title = "Executable IPC has a tested quick start, current operation inventory, client guidance, and separate complete wire reference"
status = "pending"

[[exit_criteria]]
id = "contract-authority"
title = "Every callable operation and packet has one documented authority and an explicit TypeSpec, handwritten, or separately governed classification"
status = "pending"

[[exit_criteria]]
id = "typespec-migration-roadmap"
title = "Every operation has a concrete TypeSpec migration wave, compatibility gate, and generated-code boundary; packed codec gaps are explicit"
status = "pending"

[[exit_criteria]]
id = "generated-html"
title = "Generatable reference and index pages derive from governed inputs, show runtime and maturity accurately, and share the ALX presentation system"
status = "pending"

[[exit_criteria]]
id = "demos"
title = "Every retained demo has an owner, maturity, build command, output, automated check, and last-verified release"
status = "pending"

[[exit_criteria]]
id = "pruning"
title = "No demo or research document is deleted without an approved disposition and removal of its build, test, manifest, and distribution references"
status = "pending"

[[exit_criteria]]
id = "freshness"
title = "Documentation and generated-contract checks fail on stale links, stale inventories, or undocumented public surfaces"
status = "pending"
+++

# Geometer Documentation Cleanup Plan

## Objective

Make Geometer's documentation accurately answer six questions without
requiring repository archaeology:

1. What does Geometer support, and at what maturity?
2. How should a consumer call `geometer(.exe)` through persistent stdio IPC?
3. Which schemas and packets are governed by TypeSpec, and which are not?
4. Which examples and demos are current, experimental, historical, or safe to
   remove?
5. How can every operation acquire TypeSpec-generated structural contracts,
   client bindings, and reference documentation?
6. Which documentation can be generated automatically with the same shared
   HTML presentation as ALX?

This effort includes documentation cleanup, generated HTML and shared styling,
and assessment of the migration to TypeSpec for all operations. The migration
assessment must produce concrete implementation waves and acceptance gates.
Changing solver behavior or implementing all contract migrations is follow-on
work; it is not implied by completing this documentation plan.

The [TypeSpec and ALX generation assessment](typespec-html-assessment.md)
records the inspected source paths, migration proposal, HTML generation
opportunities, and stylesheet comparison. Its recommendations are incorporated
into the steps and exit criteria above.

## Baseline

The baseline is `origin/main` at release `v2026-09-04`, plus the documentation
decision in ADR-017 that retains the analytic planar Boolean solver as
experimental. The initial audit found:

- `docs/design/` contains 43 Markdown files, including 16 STEP-topology files
  whose titles or status explicitly describe research, evidence, handoff, or an
  experimental candidate.
- `docs/design/analytic-planar-boolean-a0.md` is 1,849 lines and mixes public
  interface semantics with historical feasibility results, solver internals,
  qualification evidence, and an abandoned MATZ promotion sequence.
- `docs/contracts/current-interface-inventory.md` is a 2026-08-12 snapshot whose
  current-sounding name and text incorrectly say that `serve --stdio` does not
  exist.
- `docs/design/executable-ipc-a0.md` still calls the implemented executable-pipe
  contract “proposed.” It is a useful wire specification, but it is not an easy
  consumer introduction.
- `docs/design/cli.md` calls model bounds the initial live IPC operation even
  though the generated catalog now advertises model bounds, model/mesh HLR, and
  the experimental analytic operation, with additional native-only experimental
  STEP-topology operations.
- The Python examples README still recommends `wn-geometer==2026.6.10` even
  though the audited release is `2026.9.4`.
- Demo maturity language is inconsistent. For example, the illustration code
  is called a production package module while the example index correctly says
  the application is not itself a supported production renderer.

These are documentation defects. Their correction does not require rebuilding
the native solver.

## Documentation Taxonomy

Use a small number of explicit document roles:

| Role | Location | Content rule |
| --- | --- | --- |
| Consumer overview | `README.md` | Supported capabilities, maturity, installation, interface selection, and links only. |
| Interface and architecture record | `docs/design/` | Current callable behavior, operation semantics, transport boundaries, and wire formats. |
| Contract authority | `docs/contracts/` and `docs/generated/contracts/` | Lifecycle, source-of-truth matrix, compatibility snapshots, schemas, and generated references. |
| Developer procedure | `docs/developer/` | Build, generation, packaging, qualification, and troubleshooting instructions. |
| Research and evidence | `docs/research/` | Experiments, feasibility results, fixture observations, rejected directions, and handoffs. |
| Durable decision and requirement | `docs/geometer/adr/`, `docs/geometer/requirements/` | Accepted decisions and supported behavioral requirements. |
| Temporary execution plan | `docs/plans/<plan-id>/` | Active steps and logs; delete after durable closeout. |

Moving a document does not change its maturity. Preserve Git history with
`git mv`, repair inbound links, and avoid duplicating normative text in the old
and new locations.

## Initial Design-Document Disposition

The implementation step must turn this initial grouping into a file-by-file
checklist before moving anything.

### Keep in `docs/design/`

- Interface policy and versioning.
- STEP, planar geometry, HLR, C ABI, generic operation C ABI, CLI, Python, WASM,
  TypeScript, and Rust interfaces.
- JSON, binary, contract-semantics, distribution, and executable IPC formats.
- The concise normative portions of the analytic A0 logical and packet
  contracts.

### Move to developer or contract documentation

- `dependency-cache.md` to developer procedures.
- `browser-demos.md` to developer/demo maintenance documentation.
- `typespec-toolchain.md` to contract-authoring/developer documentation, leaving
  a short authority link in the design index.
- `generated-contract-reference.md` to contract documentation.
- Keep one authored source per guide; generate its HTML presentation and shared
  navigation where practical. Generated reference pages must remain visibly
  distinct from authored intent and historical research.
- `model-bounds-contract-compatibility.md`,
  `geom-a0-contract-alignment.md`, and `transport-design-review.md` to dated
  compatibility or historical review records.

### Split or move to research

- Split `analytic-planar-boolean-a0.md`: retain concise logical contract and
  failure semantics in design; move feasibility, qualification, historic MATZ
  gates, and solver-development evidence to an analytic research record.
- Move `exact-real-algebraic-a0.md` to analytic research while keeping ADR-012,
  ADR-013, and ADR-017 as the durable decision trail.
- Move the 16 `step-topology*.md` documents into a coherent
  `docs/research/step-topology/` group, except any section proven to be the sole
  normative definition of a currently advertised experimental contract. Such
  sections must first be reduced into a concise experimental interface page or
  TypeSpec documentation.

The asset directory remains in place during the first cleanup. Build scripts,
promotion metadata, and generated-doc tests refer to its current paths; moving
assets is a separate mechanical change with little consumer benefit.

## Root README Refresh

Rewrite the README in this order:

1. One-paragraph purpose and non-goals.
2. Capability table with `supported`, `pilot`, and `experimental` labels.
3. “Choose an interface” table for persistent IPC, Python, CLI files, browser
   WASM, C++, and the C ABI.
4. Installation and a single smallest supported example.
5. Persistent IPC quick-start link.
6. Documentation map.
7. Example/demo index with maturity labels.
8. Short contributor and release links instead of duplicated build detail.

Every version, platform, command, operation name, and maturity claim must be
checked against source or a generated catalog. Avoid words such as “current,”
“first,” and “proposed” unless the claim is mechanically checked or dated.

## Executable IPC Documentation

Keep two deliberately different documents:

- A consumer guide, tentatively `docs/design/executable-ipc.md`, explaining
  when to use persistent IPC, how to locate/start the executable, recommended
  Python/Rust/TypeScript clients, handshake and operation discovery, request and
  attachment flow, diagnostics, timeouts, cancellation, shutdown, and process
  recovery. Include one runnable model-bounds example and one attachment-heavy
  example.
- `executable-ipc-a0.md` as the normative framing reference for implementers.
  Correct its implemented status, link every generated control DTO, and keep
  byte layout, limits, state machine, and failure behavior there.

The guide must explain that stdout is binary protocol data, stderr is logging,
operations are discovered from `welcome`, execution is serialized, timeout is
local, active work is not cancellable in A0, and forced process termination
fails all outstanding calls. It must not tell application authors to hand-build
frames when a maintained client exists.

Add an operation matrix generated from or checked against the normalized
catalog. It must distinguish portable runtime, native-only experimental, and
structural-only operations rather than presenting all TypeSpec declarations as
callable everywhere.

## TypeSpec And Packet Authority Audit

TypeSpec is implemented but does not govern every Geometer byte format. The
audited normalized catalog contains 16 operations:

- 4 portable runtime operations: model bounds, model HLR, mesh HLR, and the
  experimental analytic planar Boolean operation;
- 9 native-only experimental STEP-topology operations; and
- 3 structural-only STEP-topology operations.

The promotion manifest separately inventories six older operation families not
represented as generated TypeSpec operations: model-to-GLB, planar STEP,
planar batch solve, planar triangulation, Clipper2 Boolean, and Clipper2 open
inflate. The handwritten JSON batch request/response and older projection
contracts also remain inventoried.

Produce one authority matrix with these columns:

| Field | Meaning |
| --- | --- |
| Operation identity | Stable callable identity, if one exists. |
| Maturity/runtime | Promoted, pilot, experimental, inventory-only; portable, native-only, or unavailable. |
| Logical request/result authority | TypeSpec source, handwritten source, or none. |
| Transport | Persistent IPC, file CLI, C ABI, WASM, Python wrapper, or direct C++. |
| Attachment/packet authority | Separate packet spec and magic/version where applicable. |
| Generated projections | C++, TypeScript, Rust, Python, JSON Schema, and HTML status. |
| Canonical documentation | One consumer page and one normative reference. |

Explicitly document the layered exceptions:

- TypeSpec governs IPC control/envelope JSON DTOs; `GMIPCA01` framing remains a
  separately specified binary transport.
- TypeSpec governs analytic logical DTOs; `GMABRQ01`/`GMABRS01` remain a
  separately governed packed projection.
- The indexed-mesh packet is handwritten binary input attached to a TypeSpec
  HLR operation.
- Planar batch, triangulation, and Clipper2 packets are handwritten and
  independently versioned.
- Legacy JSON batch and file-oriented CLI formats are not made TypeSpec-owned by
  being callable from the same executable.

These classifications describe the migration starting point. The target is
TypeSpec-generated structural contracts and operation metadata for every public
operation, with the remaining packed codec generation assessed explicitly.
Follow the migration waves in the linked assessment; do not count a generic
JSON object or opaque byte attachment as complete payload coverage. Preserve
the existing packet bytes and legacy client behavior during migration, or
record an intentional versioned contract change.

The HTML expansion can proceed before runtime migration. Generate coverage and
gap pages for inventoried operations, clearly identifying those that are not
yet TypeSpec-backed or callable through IPC. Extend Geometer's existing HTML
generator and ALX-derived styling rather than introducing a parallel reference
site with independent operation lists.

Audit whether `src/tsp/geometer/analytic-candidate.tsp` still has a distinct
purpose now that `main.tsp` imports the analytic declarations. Remove it only if
its focused check and promotion evidence can be replaced without losing a
governed boundary.

## Demo And Example Baseline

“Automated coverage” below means a test or validation path is registered in the
repository; it does not mean the demo was rerun during this planning audit.

| Example | Current evidence | Initial disposition |
| --- | --- | --- |
| Browser model bounds | Generated TypeScript client pilot; source page plus compiled JS; exercised by TypeScript/WASM client validation. | Keep as the minimal generated-contract example; update maturity and launch instructions. |
| Browser HLR Lab (`embedded_model_viewer`) | Updated 2026-09-03; hosted site and single-file output; Chrome integration coverage. | Keep and present as the primary browser HLR demo. |
| Browser Illustration Lab | Updated 2026-09-03; hosted site and single-file output; Chrome integration coverage. | Keep provisionally; clearly separate supported package modules from demo-application maturity. |
| Analytic polygon pour | Source last changed 2026-08-18; hosted and single-file outputs; Worker/WASM browser coverage; owning solver is experimental. | Mark experimental immediately; candidate for research archive or removal after confirming no release gate requires it. |
| Interactive PCB polygon pour | Source last changed 2026-08-18; hosted and single-file outputs; substantial browser interaction coverage; built on the experimental analytic solver. | Candidate for removal or archival because it demonstrates the abandoned application direction. |
| Planar ring solver | Handwritten browser page last changed 2026-06-09; standalone builder; no dedicated Rack subtest found. | Strong prune/archive candidate after one compatibility run and reference audit. |
| Native C++ HLR preview | Optional CMake target, copied to native distribution, checked by native packaging validation; source last changed 2026-06-09. | Run and assess maintenance cost; keep only if it remains the useful direct-C++ example. |
| Python headless HLR/SVG | Run by package validation; source last changed 2026-06-09. | Keep, update release-independent install instructions, and use as the primary Python example. |
| Python PyVista/Qt viewer | Optional locked environment and off-screen validation command; source and documentation last changed 2026-06-09; README pin is stale. | Repair and validate, or explicitly archive as an unsupported GUI experiment. |
| Node STEP-topology reference | Built by contract generation and exercised by TypeScript integration; experimental native topology workflow. | Move with STEP-topology research unless retained as an explicitly experimental IPC reference. Do not use it as the basic IPC quick start. |

For each item, record:

- owner and intended audience;
- supported/pilot/experimental/historical maturity;
- source entry point and generated outputs;
- exact build and launch command;
- required fixture and platform constraints;
- registered automated coverage and last successful run;
- release/distribution inclusion; and
- keep, repair, archive, or remove decision.

## Pruning Gate

Pruning is a separate reviewed step. Before removal, inventory and update:

- source HTML/TypeScript/JavaScript/Python/C++ files;
- dedicated build and site-packaging scripts;
- `dist/` hosted and single-file artifacts;
- promotion-manifest demo entries and governed hashes;
- Rack strata, browser tests, workflow path filters, and release validation;
- root, example, developer, design, and distribution documentation; and
- reusable code that is incorrectly coupled to the demo name.

Prefer deletion over a permanent in-tree archive when Git history is sufficient.
Use `docs/research/` only when the artifact remains useful evidence or a runnable
experiment. Obtain an explicit disposition review before deleting any demo or
committed distribution artifact.

## Validation Strategy

Documentation-only phases use the smallest applicable checks:

```powershell
git diff --check
npm run check:docs
uv run pytest tests/python/test_contract_promotion_manifest.py -q
```

Add a repository-relative Markdown link check covering authored docs and
examples. Run `npm run check:contracts` only when TypeSpec, promotion metadata,
generated references, or package documentation changes.

The demo audit uses committed release artifacts first:

- focused TypeScript validation for model bounds and package consumers;
- focused Chrome tests for HLR and Illustration Labs;
- focused analytic/PCB browser tests only while those demos remain candidates;
- package validation for the headless Python example;
- PyVista `--off-screen-validate` for the optional GUI example;
- native validation plus a documented smoke run for the C++ preview; and
- the focused Node native-process test for the topology reference.

Do not rebuild OCCT or the native solver for documentation edits. Rebuild a
demo artifact only when its source/output freshness check requires it. A prune
may remove generated demo artifacts and their registrations without rebuilding
unrelated native or WASM outputs.

## Closeout

The cleanup is complete only when durable indexes, requirements, ADRs, contract
metadata, and release notes reflect the final state; all retained examples pass
their declared focused checks; and the full changed-link closure is valid.
Delete `docs/plans/documentation-cleanup/` in the closing change so completed
execution logs do not become another documentation archive.
