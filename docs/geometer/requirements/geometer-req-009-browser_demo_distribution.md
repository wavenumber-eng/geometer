+++
type = "requirement"
id = "geometer-req-009"
domain = "geometer"
status = "implemented"
title = "Browser Demo Distribution"
created = "2026-08-22"

[[verification_refs]]
kind = "local_file"
target = "tests/python/test_package_single_html_site.py"

[[verification_refs]]
kind = "local_file"
target = "tests/typescript/hlr_static_site_validation.mjs"

[[verification_refs]]
kind = "local_file"
target = "tests/wasm/test_hlr_static_site.py"
+++

# REQ-009: Browser Demo Distribution

## Summary

Geometer provides reusable, locally reviewable packaging for browser demos that
can be deployed unchanged after explicit publication approval.

## Requirements

1. Keep maintained browser-demo source and generic TypeScript demo tooling under
   `examples/wasm/`.
2. Put distributable demo outputs under `dist/wasm/demos/`; do not recreate
   root-level `dist` artifacts.
3. A self-contained hosted demo must declare `index.html` as its only runtime
   file and must not depend on a network CDN, external stylesheet, import map,
   or external script.
4. Generate deterministic SHA-256 closure metadata and Cloudflare-compatible
   security/cache headers for the hosted directory.
5. Keep build/package operations local. Do not publish from a build script or CI
   validation job.
6. Validate the packaged closure statically and exercise its primary workflow
   in a real browser with no unexpected external request or uncaught exception.
7. Keep uploaded user files local to the browser unless a future requirement
   explicitly adds an upload service and its security/privacy policy.
8. Keep reusable panel, input, and presentation behavior independent of Viz,
   board-specific policy, and Geometer core geometry semantics.
9. Require HLR and Illustration Labs to import production package APIs and use
   the generic governed HLR operation. A packaged demo must not carry a copied
   authoritative HLR, illustration, or raster algorithm.
