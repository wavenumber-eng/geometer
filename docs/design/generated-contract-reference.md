# Generated Contract Reference

## Implementation status

Deterministic generation, complete navigation, relative-link validation,
offline resource validation, catalog coverage, responsive CSS rules, metadata,
and vendored-asset digest checks are implemented. Real-Chrome smoke passes at
desktop and an emulated 390 px viewport. That pass caught and closed a narrow
generated-identifier overflow that static resource tests did not expose.

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
- Cousine with a conventional monospace fallback stack;
- dark headers and section summaries;
- table, panel, tag, callout, code, navigation, and responsive rules;
- the light-background Wavenumber watermark; and
- relative links and local assets so the site works offline.

Geometer vendors a reviewed copy of the stylesheet and its required
font/logo assets under `docs/design/`. A generation or documentation command
must not read from the sibling `data_models` checkout. The initial source is
the `appz` workspace revision
`27548a16e23a8bc225fc81c047bbef8c325fb4ae`; its asset snapshot is:

| Asset | Vendored SHA-256 |
| --- | --- |
| `styles.css` | `39b9b7fd997a1707e9743ebcff6a21200e1d35ad6f629c8a5799d56192ab019d` |
| `Cousine-Regular.ttf` | `1da22250675fc4c42fcf3a9736c44bc0570516105331443b663fd5cfbd1412fe` |
| `Cousine-Bold.ttf` | `17c8a7245156d2253531c9e529474937b09d9f641c5ae7695c5e33f22822eef4` |
| Cousine `OFL.txt` | `b81c4d4dc0a9f72c9155e78187316e016e2012a8102468804173dc61468b906d` |
| `wn_logo_w_text__for_light.svg` | `87e16b5b2453ad1f9263d92953d5741a30780db02eeea0d59d61f10967c4537b` |

The stylesheet preserves the reviewed upstream layout and component rules but
replaces its Berkeley Mono declarations with the vendored OFL Cousine files
and adds narrow-viewport wrapping for generated identifiers. The wrapping rule
was added after real-browser smoke exposed horizontal overflow that static
resource checks could not detect.
The manifest records the unchanged upstream stylesheet digest
`b0452e403db12c3fca581866b0953dbca45d751bcc83c137f0da16674859d151`
and the adapted vendored digest above. A deliberate style refresh records the
upstream workspace revision and new digests. Normal contract generation never
silently refreshes presentation assets.

The machine-readable asset lock is the `documentation.assets` table in
`docs/contracts/promotion-manifest.toml`. Each entry records its upstream
source, repository destination, SHA-256 digest, and lifecycle status. Every
asset is `vendored`; tests require each destination to exist and match exactly.
This makes the entire asset set, not only the stylesheet, part of the
generated-reference completion gate.

Font redistribution is a separate prerequisite. Although Wavenumber purchased
Berkeley Mono for internal use, Geometer does not treat that purchase as public
redistribution authority and will not commit those font binaries. The public
reference instead uses [Cousine](https://fonts.google.com/specimen/Cousine),
pinned to Google Fonts revision
`038b637da7b3fd956a4ed93ffc607c3d5e4ce172` under the
[SIL Open Font License 1.1](https://github.com/google/fonts/blob/038b637da7b3fd956a4ed93ffc607c3d5e4ce172/ofl/cousine/OFL.txt).
The regular, bold, and license files are vendored together. A font can remain
`vendored` only while its `redistribution_status` is `approved_open_license`
and its `license_evidence` names the committed OFL file.

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

The pinned `npm run generate:contracts` command generates schemas, catalog, and
HTML. It will add language projections as those slices land. Its `--check` mode
fails on stale, missing, unexpected, unlinked, or externally dependent
pages/assets.

Verification covers:

- deterministic bytes from the same inputs;
- catalog-complete contract and operation navigation;
- valid relative stylesheet, asset, schema, and source links;
- required metadata and generated warnings;
- no authored-authority claims in generated pages;
- offline rendering without CDN or sibling-repository access; and
- browser smoke at desktop and narrow viewport sizes.
