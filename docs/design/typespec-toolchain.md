# TypeSpec Toolchain And Normalized Catalog

## Status and authority

This document fixes the implementation shape for Geometer's TypeSpec
foundation. ADR-010 remains the authority decision: authored TypeSpec owns the
structure of individually promoted contracts, while C++ owns geometry behavior
and the promotion manifest owns lifecycle/evidence state.

This foundation is prepared while ADR-011 awaits independent transport review.
It does not implement or authorize the generic C ABI or executable IPC server.
The toolchain remains `planned` until that gate opens.

## ALX baseline and Geometer differences

The design adapts the working ALX pattern from `appz/data_models`:

- a private, lockfile-pinned npm toolchain;
- TypeSpec source under `src/tsp`;
- JSON Schema plus one local normalized-catalog emitter;
- catalog-discovered roots rather than per-language root lists;
- generators that reject unsupported or lossy constructs; and
- committed outputs with deterministic `--check` behavior.

Geometer does not import ALX TypeSpec, its emitter, generated files, or sibling
scripts. The ALX catalog is schema-root focused and hard-codes the Alexandria
namespace. Geometer's in-repository emitter instead owns the Geometer namespace
and normalizes operations and raw attachment declarations as first-class
records alongside schema roots.

## Pinned build toolchain

The root gains a private `package.json` and committed `package-lock.json` with:

| Tool | Pinned version |
| --- | --- |
| Node.js CI/tooling generation | 24 |
| npm package manager declaration | 11.16.0 |
| `@typespec/compiler` | 1.14.0 |
| `@typespec/json-schema` | 1.14.0 |
| TypeScript | 5.9.3 |
| Biome | 2.5.7 |

The local emitter is a private file dependency. `npm ci` is the only supported
dependency restore in CI and release checks. Node and npm are build/test tools,
not native, WASM, executable, Rust-client, or Python-package runtime
dependencies. `@typespec/http` is not part of the foundation because Geometer's
governed transports are C ABI and framed stdio rather than HTTP.

Version upgrades are deliberate lockfile changes validated by regeneration and
all affected conformance gates. Geometer may adopt newer versions than this
baseline later, but must never float them during generation.

## Repository layout

| Purpose | Path |
| --- | --- |
| Authored TypeSpec entry point | `src/tsp/geometer/main.tsp` |
| Authored common declarations | `src/tsp/geometer/common.tsp` |
| Authored operation declarations | `src/tsp/geometer/operations/` |
| Local emitter/decorator package | `src/ts/wn-geometer-contract-emitter/` |
| TypeSpec configuration | `tspconfig.yaml` |
| Authored catalog-format schema | `contracts/geometer/catalog-schema.a0.json` |
| Normalized catalog | `contracts/geometer/generated/wn_geometer_contract_catalog.a0.json` |
| Generated JSON Schemas | `contracts/geometer/generated/schema/` |
| Generation/check driver | `scripts/generate-contracts.mjs` |
| Generated C++ DTO/codecs | `src/cpp/lib/geometer/generated/contracts/` |
| Generated TypeScript package source | `src/ts/geometer/generated/` |
| Generated Rust crate source | `src/rust/geometer-client/src/generated/` |
| Generated Python internals | `python/geometer/_generated/contracts/` |
| Generated HTML reference | `docs/generated/contracts/` |

The foundation slice creates only the toolchain, authored TypeSpec root,
catalog schema, normalized catalog, and JSON Schemas. Language projections and
HTML are added by their owning plan steps; the catalog records their eventual
output roots now so generators cannot invent competing layouts.

## Namespace and identities

All owned declarations are under `Wavenumber.Geometer.Contracts`. Operation
declarations are under `Wavenumber.Geometer.Contracts.Operations`; organized
subnamespaces such as `Common` and `ModelBoundsA0` are allowed below the owned
root.

Every schema root has both:

- the stable contract identity already governed by the promotion manifest,
  such as `geometry.model_bounds.options.a0`; and
- an explicit JSON Schema identifier using
  `urn:wavenumber:schema:geometer:<contract-identity-without-generation>:<generation>`,
  such as
  `urn:wavenumber:schema:geometer:geometry.model_bounds.options:a0`.

The emitter rejects missing or duplicate contract identities, schema
identifiers, operation identities, and declaration names. It also rejects a
TypeSpec root whose identity is absent from the promotion manifest. The
manifest may inventory handwritten contracts not yet present in TypeSpec, but
no TypeSpec contract may be untracked.

Catalog format identity is `wn.geometer.contract_catalog` generation `a0`.
This format generation is separate from contract identities, executable IPC
A0, packed format versions, releases, and the date-based C ABI generation.

## Catalog contents

The normalized catalog is the only input to language and HTML generators. It
contains, in deterministic order:

- catalog identity and generation;
- roots with declaration name, contract identity, schema identity, and kind;
- owned model, scalar, enum, and named-union declarations;
- documentation, constraints, defaults, requiredness, nullability, and
  presence/patch annotations;
- closed/open-object intent and reviewed extension-bucket annotations;
- discriminator and variant metadata;
- operations with stable identity plus request/result contract references;
- named input/output attachments with direction, requiredness, allowed media
  types, and byte limits; and
- generator-relevant logical annotations whose meaning is governed here or in
  a focused design document.

Promotion status, compatibility evidence, downstream snapshots, and review
approval do not move into TypeSpec. They remain in the promotion manifest and
are joined by identity during generation/checks.

Operations use Geometer-owned decorators implemented by the local package.
Those decorators attach operation identity and repeatable input/output
attachment records to TypeSpec operation declarations. Raw attachments are not
modeled as base64 strings or JSON properties. The operation request and result
references point to ordinary generated DTO roots; transport-specific envelopes
remain governed by ADR-011 and its specifications.

## Supported subset and failure policy

The first catalog generation supports named object/array/record models,
scalars, enums, named unions, literal variants, arrays, records, documented
defaults, and the constraints required by the pilot. It supports explicit
nullable unions while preserving optional-field absence separately.

Anonymous object models, anonymous unions, tuples, templates that remain
uninstantiated, unsupported intrinsic values, type-valued extension metadata,
unresolved references, ambiguous discriminators, and lossy number mappings are
hard errors. A new TypeSpec construct is enabled only with catalog-schema,
generator, documentation, and conformance coverage. Generators must not map an
unknown construct to `any`, `unknown`, a generic dictionary, or an omitted
declaration merely to complete generation.

Canonical objects are sealed by default. A record/open bucket requires the
reviewed reason and ownership metadata defined by contract semantics. Defaults
record intent but do not erase wire presence or eagerly populate patch DTOs.

## Deterministic generation and checks

The supported commands will be:

```powershell
npm ci
npm run generate:contracts
npm run check:contracts
```

`generate:contracts` compiles with warnings as errors into a staging directory,
validates the catalog against its authored format schema, verifies exact root
and operation agreement with the promotion manifest, and atomically updates
the governed generated paths.

`check:contracts` performs the same generation without modifying the worktree
and fails on stale, missing, or unexpected generated files. It also runs
formatter/type checks for the local emitter. Output uses UTF-8, LF endings,
stable key/order rules, and one trailing newline. Absolute paths, timestamps,
host details, and sibling-workspace paths are forbidden in generated bytes.

The TypeSpec/catalog foundation receives its own Rack stratum. Later language
and transport strata consume the same committed catalog and schemas; they do
not recompile a private subset or maintain handwritten root/operation lists.
