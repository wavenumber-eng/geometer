# Native illustration API readiness

Native illustration API and consumer handoff for `2026.9.6`. The user accepted
the Windows Rust demo as proof of the workflow; further GUI polish is deferred.
Release qualification is separate from those earlier feature-build observations.

## Callable surfaces

| Capability | Public Python API | Rust executable client | Direct C++ / native process |
| --- | --- | --- | --- |
| Colored STEP tessellation | `GeometerClient.model_tessellation`; one-shot `geometer.model_tessellation` | `model_tessellation(ModelTessellationRequest)` | `model_tessellation_from_bytes`; IPC `geometry.model_tessellation.a0` |
| Mesh illustration | `GeometerClient.mesh_illustration`; one-shot `geometer.mesh_illustration` | `mesh_illustration(MeshIllustrationInputA0)` | `illustrate_mesh(input, result, status)`; IPC `geometry.mesh_illustration.a0` |
| Illustration with visible HLR | Either Python illustration entry point with `hlr_projection=` | `mesh_illustration_with_hlr(input, hlr)` | C++ overload with HLR result; same IPC operation plus optional governed HLR attachment |
| Independent HLR geometry | `GeometerClient.model_hlr_projection` / `mesh_hlr_projection` | Typed model/mesh HLR methods | Existing model/mesh operations and file CLI remain unchanged |

IPC runs headlessly in `geometer(.exe) serve --stdio`: no JS runtime, WASM,
GPU or UI toolkit is needed. Python is executable-backed, not a second renderer
or static-link binding. TypeSpec-generated contracts govern values, discovery
and attachment declarations. Wrappers do not maintain another protocol.
Rust desktop consumers may use the default launcher or transfer an already
contained process through `GeometerClient::from_process`; both paths use the
same negotiation, typed operations and lifecycle implementation.

Use the complete [STEP-to-SVG examples and limits](../design/mesh-illustration-native.md),
[Python API guide](../design/python-package.md), and [Rust guide](../design/rust-client.md).
Clients and executable catalogs must match. The release requires 2026.9.6 or
later compatible artifacts; released 2026.9.4 lacks these operations.

`fuse_surfaces` defaults to true. Native composition handles SVG layer order;
callers supply visible-only polyline HLR for the same millimeter model,
placement and view. Arbitrary 2D lines cannot authenticate those relationships.
Experimental AO remains unavailable, raw mesh lines remain diagnostic, and
the unrelated analytic planar solver has not been promoted.

## Historical feature-build package evidence

Windows x64 now passes:

- A real wheel installed in a fresh virtual environment, executing outside the
  source package with bundled-executable discovery and explicit override.
  Public tessellation, pure illustration, Fast-HLR composition, determinism,
  fusion default, limit recovery and both one-shot helpers are exercised.
  SOT-23 produces 220 rendered triangles and 13,085 SVG bytes. Existing legacy
  package/CLI examples also pass.
- A clean external Cargo consumer against the extracted packaged crate,
  running analytic checks and the complete public STEP/HLR/illustration example.
- Existing native/TypeScript parity (40 pure cases, 24 composed cases), typed
  IPC and error tests. Python source tests additionally prove owned one-shot
  processes exit after success and operation failure.

The installed check is wired into [package validation](../../scripts/validate_python_package.py)
through [the isolated illustration check](../../scripts/validate_illustration_package.py).
Packaged Rust coverage is in Rack `RUST_001`; Python coverage is `PY_020`/`PY_021`.
These lanes already run in the four-platform native/release matrix; their
presence does not claim those platform jobs have run for this feature.

## Release qualification

The [release PR](https://github.com/wavenumber-eng/geometer/pull/27) and
[Publish workflow](https://github.com/wavenumber-eng/geometer/actions/workflows/release.yml)
record authoritative hosted outcomes. Publication requires all four native
platforms, installed Python wheels, packaged Rust consumers, browser WASM and
cross-transport parity, plus L99, contracts and repository standards. Native
artifacts must bind their exact bytes to clean source and verified OCCT profile
provenance. Do not substitute the historical development wheel below.

Portability qualification added locale-independent decimal conversion without
raising the macOS 11 deployment floor or changing the pinned WASM toolchain.
Exact Node comparisons cover 6,611 numeric cases, including powers of two and
adjacent ULPs, alongside 40 complete renderer cases and 24 HLR compositions.
Client tests select the platform's built executable from the repository root,
independent of Cargo's working directory. GUI/Metal acceptance remains deferred.

## Historical platform gaps at the Windows feature checkpoint

This table records the pre-release feature assessment, not current release
support. Use the hosted release results above for subsequent qualification.

| Platform | Qualification |
| --- | --- |
| Windows x64 | Local native, installed-wheel and packaged-Rust API checks passed; clean release provenance and full release gates remain. |
| Linux x64 | Feature runtime/package qualification not run here. |
| Linux ARM64 | Feature runtime/package qualification not run here. |
| macOS ARM64 | Native runtime/package qualification remains; GUI/Metal testing is separately deferred. |

The staged wheel is **test evidence only**: it retains the development version
and bundles an executable whose attestation reports a dirty source tree and an
unverified cached OCCT profile. Do not publish it. The executable SHA-256 tested
is `04da2a7625131b73172e0137879082aef58477693b3154f8b38102a4cd0794e7`.

No tag, push, upload or PyPI release occurred in that feature readiness pass. Demo
packaging and the later C++ GPU viewer remain separate decisions.
