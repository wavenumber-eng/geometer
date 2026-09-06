# Documentation Maintenance

Keep interface semantics in `docs/design/`, contract ownership and generation
in `docs/contracts/`, maintainer procedures here, and measured experiments,
rejected approaches and historical reviews in `docs/research/`. Accepted
decisions and requirements remain in `docs/geometer/`. Active plans/logs are
temporary and must be removed after their durable content has been transferred.

The [document disposition map](documentation-map.json) records every design
Markdown file inspected during the 2026-09-05 cleanup, its destination and
rationale. Retained experimental topology value/packet contracts are not
production promises. Evidence moves must not strand their sole interface
definitions. Asset paths under `docs/design/assets/` stay fixed.

## Source Authority

Edit authored Markdown for intent; edit TypeSpec for generated structure;
edit the promotion manifest for lifecycle evidence and maturity. Do not edit
generated HTML, schemas or language projections. Inventory-only contracts
remain handwritten until their reviewed migration actually lands.

Documentation styling uses the shared ALX teal palette and local Wavenumber
layout. Geometer retains OFL Cousine, not internally licensed Berkeley Mono,
and long-identifier wrapping. Refresh the vendored stylesheet hash explicitly
in the promotion manifest. Builds must not read a sibling appz checkout.

## Focused Checks

```powershell
npm run generate:docs
npm run check:docs
uv run pytest tests/python/test_contract_promotion_manifest.py -q
git diff --check
```

Run contract freshness when its governed inputs change. No geometry rebuild is
needed for a documentation-only edit. Browser visual checks must cover desktop
and narrow layouts; static link checks alone cannot prove readable rendering.

`check:docs` also enforces the [public-source review boundary](../contracts/public-entrypoints.md).
If it changes, inspect the public interfaces and then explicitly refresh its
lock. Generation never silently approves an API/documentation mismatch.

Example verification is recorded in the [demo audit](demo-status.md); artifact
presence, test registration and a successful runtime test are different facts.
Removing a demo needs explicit disposition approval and a complete audit of
scripts, tests, manifests, packaging and committed outputs.

The link check also requires audit source links for top-level browser HTML,
Python `.py`, C++ `.cpp`, and Node `.ts`/`.mjs` example entrypoints. Browser
workers and support modules are not separate demos. If a new demo uses a
different directory or entrypoint convention, extend this registration check.
