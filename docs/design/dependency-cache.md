# Dependency Cache

Geometer may restore generated dependency install trees from the Wavenumber
artifact cache. The cache is not a source dependency and is not a release
artifact. It is a verified shortcut for rebuilding large dependencies such as
OCCT.

## Scope

The public artifact hostname is `https://artifacts.wavenumber.net`. It is backed
by the Wavenumber R2 bucket, which is intended to serve all Wavenumber projects.
Project separation is handled by object prefixes. Separate buckets are reserved
for cases where access policy requires hard isolation.

## Object Layout

Canonical prefix layout:

```text
deps/v1/<project>/<dependency>/<dependency-version>/<target-kind>/<platform-tag>/<cache-key>/
  manifest.json
  <archive-name>.zip
  <archive-name>.zip.sha256
```

For Geometer OCCT:

```text
deps/v1/geometer/occt/V8_0_0/native/windows-x64/<cache-key>/
deps/v1/geometer/occt/V8_0_0/native/linux-x64/<cache-key>/
deps/v1/geometer/occt/V8_0_0/native/linux-arm64/<cache-key>/
deps/v1/geometer/occt/V8_0_0/native/macos-arm64/<cache-key>/
deps/v1/geometer/occt/V8_0_0/wasm/wasm-emscripten/<cache-key>/
```

The Geometer OCCT cache also checks the legacy prefix
`geometer/occt/<target-kind>/<platform-tag>/<cache-key>/` while the OCCT 7.8.1
cache remains useful for rollback or older branches.

## Cache Identity

The cache key must be immutable and specific enough that restoring an archive is
equivalent to rebuilding that dependency locally. For OCCT, the key includes:

- dependency name
- OCCT tag
- native or WASM target kind
- platform tag
- build configuration
- library type
- macOS deployment target when relevant
- Emscripten version for WASM
- recipe hash from build scripts and vendored inputs

Changing any of those compatibility inputs creates a new key.

## Manifest

New dependency cache manifests use schema
`wavenumber.dependency_cache_manifest.a1`.

Required concepts:

- project
- dependency name, version, and source repository
- target kind, platform tag, and toolchain when applicable
- build configuration and library type
- recipe hash
- archive name, size, and SHA-256
- producer repository, commit, workflow run, and UTC timestamp

Legacy OCCT manifests using `geometry.occt_binary_cache_manifest.a0` are accepted
only for existing OCCT cache compatibility.

## Access

Developer machines and normal CI workflows use public HTTPS reads. Trusted
producer workflows may use upload-capable R2 credentials. Consumers must
validate `manifest.json` and archive SHA-256 before extracting an archive into
`.deps/`.

## Local State

Restored archives extract under `.deps/`. They must not be committed. Source
build fallback remains available for new platforms, cache misses, and debugging.
