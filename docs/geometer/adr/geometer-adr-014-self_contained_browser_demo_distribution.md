+++
type = "adr"
id = "geometer-adr-014"
domain = "geometer"
status = "accepted"
title = "Distribute Browser Demos As Self-Contained Review Sites"
created = "2026-08-22"
+++

# ADR-014: Self-Contained Browser Demo Distribution

## Status

Accepted.

## Context

Geometer browser demos serve two purposes: they exercise the real browser/WASM
integration and they provide small public tools that can be reviewed before
being placed on the Wavenumber site. A demo assembled from CDN scripts, separate
WASM files, models, Workers, and styles is easy to run from a repository checkout
but harder to review, cache, move between static hosts, and preserve as one
versioned artifact.

Future demos also need shared UI behavior without importing application policy
from Viz or growing one monolithic demo script.

## Decision

Geometer supports self-contained browser demos with these boundaries:

1. Maintained source stays under `examples/wasm/`. Generic interaction and UI
   primitives stay in strict TypeScript under `examples/wasm/demo-tooling/`.
2. A demo-specific builder may bundle JavaScript, Worker source, WASM, fonts,
   licenses, images, and fixtures into one self-contained HTML artifact under
   `dist/wasm/demos/`.
3. `scripts/package_single_html_site.py` turns such an artifact into a
   deploy-unchanged static directory. `index.html` is its only runtime file;
   `_headers` and `asset-manifest.json` are host and verification metadata.
4. The packager rejects external runtime references and module/import-map
   dependencies. It injects a CSP, Cloudflare-compatible response headers, and
   deterministic SHA-256 closure metadata.
5. Build and packaging scripts only create local artifacts. Publication is a
   separate, explicitly authorized step after human review.
6. A hosted demo must have static closure validation and a real-browser gate.
   The browser gate must exercise the primary workflow, reject unexpected
   external requests and uncaught exceptions, and cover responsive layout where
   appropriate.
7. Demo UI policy remains outside the Geometer C++ library and public geometry
   contracts. Reusable panels and controls are browser-example tooling, not a
   new core geometry API.

The HLR Lab is the first single-runtime-file hosted demo. It embeds Three.js,
its license, Geometer browser WASM, the projection Worker, local example models,
and all presentation assets. Local STEP uploads remain inside the browser.

## Consequences

- A reviewed demo directory can be copied to Cloudflare Pages or another static
  host without URL rewriting or dependency installation.
- The single HTML can be large because WASM and fixtures are embedded. This is
  an intentional portability tradeoff; metadata records its exact bytes.
- External CDNs and runtime package resolution are unavailable by design.
- CSP changes, new runtime assets, and new browser permissions require explicit
  packaging and test updates.
- Future demos can reuse the panel system and site packager while retaining
  their own domain-specific controls and Worker protocol.
