+++
type = "plan_log"
id = "geometer-documentation-cleanup-typespec-html-assessment"
plan_id = "geometer-documentation-cleanup"
step_id = "alx-generation-assessment"
created = "2026-09-05"
+++

# TypeSpec Coverage And ALX HTML Generation Assessment

Assessment date: 2026-09-05. Geometer baseline: `v2026-09-04` plus ADR-017.
ALX reference: the local `appz/data_models` checkout at appz revision
`27a6c567d69c3a61f367774aed5dcab6fc09164a`. The inspected ALX generator and
shared stylesheet have no local changes. This is an assessment for the active
[documentation cleanup plan](plan.md).

## Findings From ALX

ALX has multiple HTML generation paths, with different source authorities:

| Reference source in `appz/data_models` | Output and useful pattern |
| --- | --- |
| `scripts/generate-alx-contract-docs.mjs` | `docs/generated/alx_contracts/`: normalized TypeSpec catalog joined with promotion waves; root fields, reachable declarations, source/schema links, deterministic inventory and `--check`. |
| `scripts/generate_model_reference_docs.py` | `docs/generated/models/alx/`: discovered model references, composition and reference relationships, documentation, examples and optional runtime roundtrip evidence. |
| `scripts/generate_model_ops_reference_docs.py` | `docs/generated/model_ops/`: registry-derived operation documentation and coverage. |
| `scripts/generate_contract_spec_docs.py` | Contract specification pages derived from schemas and linked authored design intent. |
| `scripts/generate_domain_current_state_docs.py` and `scripts/generate_docs_indexes.py` | Model/contract inventories, domain navigation, and documentation indexes. |
| `docs/core/design/styles.css` | Shared Wavenumber colors, dark headers, tables, panels, navigation, watermark, and responsive layout used by the broader ALX documentation. |

The TypeSpec-specific ALX generator currently embeds a small system-font style
block. The broader ALX model and design pages link the shared stylesheet. Use
the broader documentation's shared presentation as the visual target, while
adopting catalog discovery and deterministic checks from the TypeSpec generator.

ALX's Python model introspection is useful as a design reference, but Geometer
should discover its structural fields and type relationships from its own
normalized TypeSpec catalog. Re-inspecting generated Python DTOs would create
an unnecessary dependency on one language projection. Keep Geometer generation
self-contained; record the upstream revision and vendor reusable assets with
their provenance rather than reading sibling files during builds.

## What Geometer Already Generates

`scripts/generate-contract-docs.mjs` already generates
`docs/generated/contracts/` from the normalized TypeSpec catalog and
`docs/contracts/promotion-manifest.toml`. It includes root and operation pages,
fields, attachment declarations, source/schema navigation, local assets,
catalog digests, and deterministic freshness checks. `npm run generate:docs`
and `npm run check:docs` are existing entry points.

Extend this generator instead of replacing it. Two observed gaps belong in the
first slice:

- Operation HTML shows `native_runtime_available` directly as native executable
  availability. That flag denotes additional native-only exposure: portable
  operations also work natively. Derive the effective value from portable OR
  native-only availability and check it against the executable catalog.
- The generator reads lifecycle `status` but does not display the analytic
  `maturity` added by ADR-017. Render maturity separately from structural
  promotion and runtime availability on operation, contract, and index pages.

## Target Meaning Of TypeSpec-Generated Operations

For each public operation, TypeSpec should own request/result structure,
identity, attachment declarations, constraints, and supported projection
metadata. The normalized catalog should drive DTOs, structural codecs,
operation metadata, client wrappers where mechanical, and HTML reference pages.
Geometer's C++ implementations continue to own geometry algorithms and semantic
validation such as topology validity. Runtime wiring needs an implementation
adapter; adding a TypeSpec declaration alone does not expose a solver via IPC.

Count operation coverage and payload coverage separately. A generated envelope
around an opaque packet is operation coverage but leaves that packet's internal
layout and codec handwritten. Model all public operations, including retained
compatibility entry points, without treating every CLI alias as a new semantic
operation. Browser-only illustration has generated contracts but executes in
TypeScript; document that boundary rather than promising executable support.

## Proposed Migration Waves

These waves specify follow-on implementation work. The cleanup plan delivers
the detailed roadmap and generated coverage reporting, not every migration.

| Wave | Work | Evidence required before declaring completion |
| --- | --- | --- |
| 0: Reconcile existing generated surfaces | Inventory direct C++, C ABI/WASM exports, CLI commands/jobs, IPC catalogs, Python/TS/Rust methods, and browser illustration; map aliases. Reconcile model bounds, HLR, analytic, IPC and topology lifecycle records with actual runtime exposure. | Every entry maps to one semantic operation, contract, or explicitly internal API; actual runtime catalogs and documentation agree. Experimental status is retained. |
| 1: Conversion and planar synthesis | Add model-to-GLB options/result attachment and planar-STEP request/result attachment contracts; wire generated structural validation and existing kernels to generic operation/IPC adapters. | Existing file CLI/Python callers retain defaults, diagnostics, and outputs; generated clients execute representative requests through IPC. |
| 2: Production polygon operations | Add logical request/result roots and attachment projections for Clipper2 Boolean, Clipper2 open inflate, planar batch solve, and planar triangulation. Preserve existing byte entry points and add generic IPC adapters where absent. | Codec goldens, invalid-input checks, existing native/WASM parity and one public IPC/client replay per operation; no geometry algorithm replacement. |
| 3: Legacy batch and projection boundaries | Model `geometer.batch.request.a0`/response and remaining handwritten projection/options contracts. Map canonical commands and legacy aliases to shared generated roots and explicit compatibility adapters. | Presence, defaults, unknown-field policy, aliases, errors and output shape remain compatible; stricter generated validation cannot silently break accepted legacy inputs. |
| 4: Packed codec generation | Pilot a catalog extension for binary scalar widths, endianness, header constants, offsets, alignment, counts, tables and constraints on the indexed-mesh packet. Assess planar, Clipper2 and analytic packet families after the pilot. | Generated codecs preserve exact existing wire bytes and reject malformed packets across the affected languages; memory/work behavior remains appropriate for large inputs. |

Wave 4 is the largest uncertainty. TypeSpec currently describes logical
structure; it does not itself generate Geometer's binary codecs. A custom
decorator/emitter extension could place layout metadata in the normalized
catalog and produce both layout tables and codecs. Define one authoritative
layout source. Avoid a second handwritten schema that can disagree with
TypeSpec. Prototype the simplest packet before committing to analytic tables,
canonical ordering, provenance validation, and standalone digest rules.

For analytic packets, semantic validation may remain handwritten even if byte
readers/writers are generated. Report that distinction precisely. Do not claim
all packets are generated based on header-only generation. Preserve framing
`GMIPCA01`, packet magics, versions, numeric values and alignment unless a
separate versioned change is deliberately required.

Each wave needs a field-level compatibility comparison before switching
structural authority, then generated-source freshness, focused cross-language
vectors, runtime adapter checks, and a recorded manifest transition. Native or
WASM adapter changes will require proportionate builds during implementation;
assessment and HTML generation do not require rebuilding geometry.

## HTML That Can Be Generated

| Page family | Authoritative input | Proposed output |
| --- | --- | --- |
| Operation/transport coverage | TypeSpec catalog plus governed inventory, alias mappings, runtime and maturity records | One searchable or filterable index of every operation, including unmigrated entries and explicit IPC gaps. Start with accessible static tables. |
| Contract/model reference | Catalog declarations, docstrings, constraints, defaults and schema identities | Extend existing contract pages with linked nested declarations and source links. |
| Type and attachment relationships | Catalog references and named attachments | Operation-to-request/result/attachment tables and model composition links. Semantic references require explicit annotations; field names alone are insufficient evidence. |
| Packet reference | Governed machine-readable layout definitions where available | Magic/version, scalar values, offsets, table layouts, limits and codec links. Keep authored specifications linked until layout coverage is complete. |
| Examples | Registered conformance vectors and example manifests | Valid request/result examples with asserted comparison policy; distinguish JSON from packed binary. Never invent a successful solver result from a schema. |
| Demo catalog | Maintained demo inventory and recorded verification evidence | Source/build/output links, maturity, supported hosts, last verification and disposition. File presence is not runtime success. |
| Navigation and authored pages | Documentation inventory, front matter and authored Markdown | HTML landing pages, guide/ADR/requirement rendering and backlinks using one template. Markdown remains the editable source. |

Engineering rationale, solver limitations, migration judgments and historical
research conclusions remain authored. Their HTML presentation can be generated
without attempting to infer those conclusions from code.

## Shared Stylesheet Adoption

Geometer already vendors an adapted ALX stylesheet at `docs/design/styles.css`.
The current ALX source SHA-256 is
`b0452e403db12c3fca581866b0953dbca45d751bcc83c137f0da16674859d151`,
matching Geometer's recorded upstream snapshot. The actual differences are:

- ALX teal `#006c67` / `#e5f3f0` versus Geometer orange
  `#b45309` / `#fff1e6` accents;
- Berkeley Mono versus Geometer's vendored OFL Cousine; and
- Geometer's extra narrow-screen wrapping for generated identifiers.

Adopt the ALX teal palette and common layout/component rules throughout the
Geometer documentation. Retain the existing public-distribution Cousine font
choice and identifier wrapping documented in `generated-contract-reference.md`.
Use one local stylesheet and shared page template; keep any small necessary
reference-specific rules explicit. This scope covers documentation styling;
interactive demo themes have their own audit and need not change with it.

Update asset provenance and hashes when the stylesheet changes. Validate the
result at desktop and narrow widths with long operation identities, tables,
code blocks, navigation and the watermark. Keep all assets local so a fresh
Geometer checkout can generate and view the pages independently of appz.

## Acceptance And Handoff

The extended cleanup is complete when the roadmap maps every public operation,
generated coverage exposes remaining migration gaps, the HTML generator shows
correct runtime/maturity information, and the generated site uses the agreed
ALX presentation. Freshness checks must detect added/removed operations,
unresolved references and stale outputs. Only rebuild the documentation for
documentation changes; use existing vectors and assets for focused checks.

Move enduring decisions and authoring instructions into contract/developer
docs before removing this temporary assessment with its completed parent plan.
