# Geometer Contracts

Geometer's maintained interface contracts are currently documented in:

- `docs/design/json-formats.md`
- `docs/design/binary-formats.md`
- `docs/design/c-abi.md`
- `docs/design/cli.md`
- `docs/design/python-package.md`
- `docs/design/wasm.md`

The TypeSpec contract program is governed by
[ADR-010](../adr/010_typespec_contract_authority_and_promotion.md). Its current
implementation-backed baseline and promotion state are:

- [current interface inventory](current-interface-inventory.md); and
- [`promotion-manifest.toml`](promotion-manifest.toml).

The manifest does not make an inventoried contract authoritative. A contract
becomes TypeSpec-authoritative only after its status and required evidence are
promoted under ADR-010. Generated JSON Schemas and contract references will
live under this tree as individual public wire contracts are promoted.
