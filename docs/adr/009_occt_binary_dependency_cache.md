# ADR-009: OCCT Binary Dependency Cache In R2

## Status

Accepted.

## Context

Geometer builds OCCT as generated dependency state under `.deps/` because OCCT
does not work correctly as an in-tree CMake subdirectory. A cold OCCT build is
expensive on every supported target, especially in GitHub Actions where each
platform runner can spend roughly tens of minutes compiling OCCT before
Geometer itself is validated.

GitHub Actions cache helps when a matching cache entry is available, but cache
entries are branch-scoped, key-sensitive, and not durable dependency artifacts.
We need a more predictable second-tier cache for CI and developer machines.

## Decision

Geometer may restore OCCT install trees from immutable binary archives stored in
the Wavenumber Cloudflare R2 dependency cache. The archives are treated as
generated dependency state, not as source and not as committed repository
artifacts.

The R2 bucket is a Wavenumber-wide dependency cache. OCCT is the first Geometer
dependency using it, but object layout must allow multiple projects,
dependencies, versions, target kinds, and platforms.

The cache consumer path is:

1. Reuse an existing local `.deps/` install tree.
2. Restore GitHub Actions cache when running in CI.
3. Download a verified R2 OCCT binary archive when credentials are configured.
4. Build OCCT from source as the fallback unless binary-only mode is requested.

The producer path is a separate trusted workflow:

- `.github/workflows/occt-deps.yml`
- manual `workflow_dispatch`
- target runner builds or restores OCCT for its platform
- packages `occt-install`
- writes `manifest.json` and `occt-install.zip.sha256`
- uploads immutable objects to R2

Normal CI and release workflows consume the cache but do not publish dependency
artifacts.

Developer machines should use read-only R2 credentials. Upload-capable
credentials are reserved for trusted producer workflows.

## Cache Layout

New dependency cache objects use:

```text
deps/v1/<project>/<dependency>/<dependency-version>/<target-kind>/<platform-tag>/<cache-key>/
  manifest.json
  <archive-name>.zip
  <archive-name>.zip.sha256
```

For Geometer OCCT 8:

```text
deps/v1/geometer/occt/V8_0_0/native/windows-x64/<cache-key>/
deps/v1/geometer/occt/V8_0_0/native/linux-x64/<cache-key>/
deps/v1/geometer/occt/V8_0_0/native/linux-arm64/<cache-key>/
deps/v1/geometer/occt/V8_0_0/native/macos-arm64/<cache-key>/
deps/v1/geometer/occt/V8_0_0/wasm/wasm-emscripten/<cache-key>/
```

The legacy OCCT prefix `geometer/occt/<target-kind>/<platform-tag>/<cache-key>/`
may be checked for compatibility with existing OCCT 7.8.1 cache objects.

## Cache Identity

Each archive key includes:

- native or WASM target kind
- platform tag
- OCCT tag
- build configuration
- OCCT library type
- macOS deployment target when relevant
- Emscripten version for WASM
- a recipe hash derived from the build scripts and vendored RapidJSON inputs

Changing the OCCT recipe produces a new object key rather than replacing an
existing dependency silently.

## Configuration

Local and CI consumers use these environment variables:

- `R2_BUCKET` or `GEOMETER_OCCT_CACHE_BUCKET`
- `R2_ENDPOINT_URL` or `GEOMETER_OCCT_CACHE_ENDPOINT_URL`
- `R2_ACCESS_KEY_ID` or `GEOMETER_OCCT_CACHE_ACCESS_KEY_ID`
- `R2_SECRET_ACCESS_KEY` or `GEOMETER_OCCT_CACHE_SECRET_ACCESS_KEY`
- `AWS_DEFAULT_REGION` or `GEOMETER_OCCT_CACHE_REGION`, default `auto`
- `GEOMETER_OCCT_CACHE_PREFIX`, default `deps/v1/geometer/occt`

Cache mode is controlled with `GEOMETER_OCCT_BINARY`:

- `auto`: try R2 when configured and fall back to source
- `off`: ignore R2 and build from source
- `only`: require an R2 hit

## Consequences

- Fresh CI and developer setup can avoid repeated OCCT source builds.
- R2 upload credentials are limited to the dependency producer workflow.
- Consumers still validate archive manifests and SHA-256 checksums before use.
- Source builds remain available for new platforms, cache misses, and debugging.
- Binary archives must be rebuilt when toolchain, OCCT, deployment target, or
  recipe inputs change.
