# Generated Contract Reference

## Purpose and authority

Geometer generates browsable HTML contract reference documentation from the
normalized TypeSpec catalog. The pages are review and consumer-navigation
artifacts; authored TypeSpec and the normalized catalog remain structural
authority, and focused design documents remain intent authority.

Generated pages live under `docs/generated/contracts/` and are committed with
the schemas and catalog they describe. They are never hand edited.

## Wavenumber visual system

The generated site uses the same Wavenumber presentation system as
`C:/eli/wn-hw/appz/data_models`:

- shared square-corner layout and Wavenumber color variables;
- Berkeley Mono with the same fallback stack;
- dark headers and section summaries;
- table, panel, tag, callout, code, navigation, and responsive rules;
- the light-background Wavenumber watermark; and
- relative links and local assets so the site works offline.

Geometer will vendor a reviewed copy of the stylesheet and its required
font/logo assets under `docs/design/`. A generation or documentation command
must not read from the sibling `data_models` checkout. The initial source is
the `appz` workspace revision
`27548a16e23a8bc225fc81c047bbef8c325fb4ae`; its asset snapshot is:

| Asset | Source SHA-256 |
| --- | --- |
| `styles.css` | `b0452e403db12c3fca581866b0953dbca45d751bcc83c137f0da16674859d151` |
| Berkeley Mono regular | `16e05cba507907e4a5156c6199b0c7b8752dc22ea2c43e81a4f2e61a393a2a62` |
| Berkeley Mono bold | `7d180b17f42dcbce0d63808fca7a7a3e3fd8bfdcce56560b629012c482438041` |
| Wavenumber light watermark | `87e16b5b2453ad1f9263d92953d5741a30780db02eeea0d59d61f10967c4537b` |

A deliberate style refresh records the upstream workspace revision and new
digests. Normal contract generation never silently refreshes presentation
assets.

## Site structure

The generated landing page links:

- contract/model roots;
- operation request, response, diagnostic, and attachment types;
- JSON Schemas;
- C++, TypeScript, Rust, and Python generated package references;
- native, browser/WASM, and executable IPC transport documentation; and
- compatibility and migration notes.

Each contract page shows identity, lifecycle/promotion status, description,
fields, requiredness, nullability, default intent, presence/emission behavior,
constraints, nested types, diagnostics, aliases/adapters, and source links.
Each operation page shows request/result roots, named attachments and media
types, supported transports, capability identity, and runnable examples.

Generated HTML includes stable `data-*` markers for generator identity,
contract identity, promotion status, and source catalog digest. It uses
`data-doc-status="generated"`, `data-generated="true"`, and
`data-wn-watermark="true"` on the document body.

## Generation and verification

One pinned command generates schemas, catalog, HTML, and language projections.
Its `--check` mode fails on stale, missing, or unexpected pages/assets.

Verification covers:

- deterministic bytes from the same inputs;
- catalog-complete contract and operation navigation;
- valid relative stylesheet, asset, schema, and source links;
- required metadata and generated warnings;
- no authored-authority claims in generated pages;
- offline rendering without CDN or sibling-repository access; and
- browser smoke at desktop and narrow viewport sizes.
