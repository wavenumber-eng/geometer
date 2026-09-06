# Geometer Contracts

Start with the [current interface inventory](current-interface-inventory.md),
[public entry-point reconciliation](public-entrypoints.md),
[generated coverage](../generated/contracts/coverage.html),
[TypeSpec toolchain](typespec-toolchain.md), and
[all-operation migration roadmap](typespec-coverage-assessment.md).
The [HTML generator guide](generated-contract-reference.md) explains local
generation, source authority, asset licensing and deterministic checks.

Geometer's maintained interface contracts are currently documented in:

- `docs/design/json-formats.md`
- `docs/design/binary-formats.md`
- `docs/design/c-abi.md`
- `docs/design/cli.md`
- `docs/design/python-package.md`
- `docs/design/wasm.md`

The TypeSpec contract program is governed by
[ADR-010](../geometer/adr/geometer-adr-010-typespec_contract_authority_and_promotion.md). Its current
implementation-backed baseline and promotion state are:

- [current interface inventory](current-interface-inventory.md); and
- [`promotion-manifest.toml`](promotion-manifest.toml).

Named downstream compatibility snapshots live under `compatibility/`. The
current Viz snapshot is
[`viz-2026.6.10.toml`](compatibility/viz-2026.6.10.toml).

The manifest does not make an inventoried contract authoritative. A contract
becomes TypeSpec-authoritative only after its status and required evidence are
promoted under ADR-010. Generated JSON Schemas and contract references live
under this tree. `geometry.model_bounds.a0` is the first promoted operation;
its manifest entry links the accepted cross-language and hosted evidence.

Contract authority and runtime maturity are separate. The frozen candidate
contracts for `geometry.analytic_planar_boolean_batch.a0` remain available, but
the solver is experimental and not production-ready under
[ADR-017](../geometer/adr/geometer-adr-017-retain_analytic_planar_boolean_as_experimental.md).
