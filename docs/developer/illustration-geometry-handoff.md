# Illustration geometry release handoff

The illustration optimization plans are complete. Version `2026.9.11`, tag
`v2026-09-11`, packages the reviewed work on `perf/illustration-geometry`.
Publication status is authoritative on the GitHub release and PyPI; the
qualification below records local evidence before hosted release validation.

Native integer formatting, shared preparation/fusion, material/vertex-key
reuse and Python numeric-array validation avoid repeated work. Owned Fast HLR
and mesh-shadow paths preserve deterministic ordering while reducing search
bookkeeping. The additive [drawing geometry API](../design/mesh-illustration-geometry.md)
returns shaded surfaces and linework without producing SVG.

## Qualification

The [measurement record](../research/illustration-performance.md) contains
paired native, IPC and whole-board results, including scope and memory limits.
All 398 captured poses retain exact illustration and full-precision Fast
HLR/shadow results. Paired four-worker uncached board runs retained all 12 SVGs
exactly: Loz 52.30/36.95 seconds, RT 9.95/10.04, NXP 122.61/102.91 and OV
51.20/42.65. Independent reviews found no further demonstrated low-risk
optimization that warrants expanding this release.

The versioned Windows build passed all 71 CTest cases. A clean installed wheel
passed tessellation, SVG, drawing geometry and the headless Python example.
Full browser and Node-test WASM plus the planar-only runtime were rebuilt
against the existing OCCT cache. Browser demos passed Chrome smoke checks;
native Node examples, Python Canvas and optional PyVista off-screen rendering
passed. The C++ preview and Rust Lab were rebuilt with the current code. See
the [demo audit](demo-status.md) for retained interactive-review limitations.

Artifact hashes and build provenance live beside the grouped native/WASM
distributions. Local attestations identify local development source; hosted
release jobs independently rebuild and validate Windows, Linux x64/ARM64,
macOS ARM64 and WASM before publishing their artifacts and wheels.

The Linux x64/ARM64 and macOS ARM64 runtime and C++ preview copies were
refreshed from successful jobs in CI run `34618554713`, with executable hashes
verified against their clean-source attestations. The user visually accepted
the refreshed Windows Rust Lab and Illustration HTML demo.

## Consumption

Install the published `wn-geometer==2026.9.11` package once release jobs finish.
Use matching generated Python/Rust/TypeScript clients and native/WASM runtimes:
an older client rejects the expanded operation catalog during handshake.
Replacing only its executable is insufficient. Altium Cruncher should consume
the published package with its ordinary dependency pin and lockfile, without
source-path or executable overrides.

The existing SVG API remains available. Drawing geometry can be larger and
slower over IPC than compact fused SVG; it is intended for custom renderers.
Planar-only WASM has no generic illustration operation. Experimental AO remains
outside the native interface. PNG optimization and Altium Monkey's separate
changes remain outside this Geometer release.

Independent reviews covered contracts, attachment limits, typed clients,
numerical/render parity and benchmark methodology. Camera snapshot ownership,
finite extent enforcement and merged-style validation findings were fixed.
Temporary working plans were removed after their conclusions were transferred
to the maintained design and measurement documents.
