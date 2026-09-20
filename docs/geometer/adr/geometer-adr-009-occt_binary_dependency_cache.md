+++
type = "adr"
id = "geometer-adr-009"
domain = "geometer"
status = "accepted"
title = "Cache OCCT Binary Dependencies In R2"
created = "2026-08-18"
+++

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
the Wavenumber Cloudflare R2 dependency cache and served through
`https://artifacts.wavenumber.net`. The archives are treated as generated
dependency state, not as source and not as committed repository artifacts.

The R2 bucket is a Wavenumber-wide dependency cache. The public artifact
hostname is the default read path for normal CI and developer machines. OCCT is
the first Geometer dependency using it, but object layout must allow multiple
projects, dependencies, versions, target kinds, and platforms.

The dependency consumer path is:

1. Select one explicit profile from `dependencies/occt-lock.json`.
2. Reuse an existing local `.deps/` install only when its lock marker matches.
3. Otherwise download the profile's one exact public R2 object.
4. Verify its byte count, archive SHA-256, internal profile-marker SHA-256, and
   installed OCCT version.
5. Fail closed if any identity or object is absent or wrong.

Normal consumers do not use GitHub Actions cache for OCCT, signed-cache
fallbacks, legacy paths, aliases, nearest matches, recipe-derived keys, or
source-build fallback. A developer may request a source build only with the
explicit producer/debug option `--binary-cache off` (or
`--occt-binary-cache off` for WASM).

The producer path is a separate trusted workflow:

- `.github/workflows/occt-deps.yml`
- manual `workflow_dispatch`
- target runner always builds OCCT from source for its platform
- packages `occt-install`
- writes candidate `manifest.json` and `occt-install.zip.sha256` evidence
- conditionally creates an archive-digest-addressed R2 object
- requires a reviewed lock update before consumers select the new archive

Normal CI and release workflows consume the public cache but do not receive R2
credentials and do not publish dependency artifacts.

Developer machines do not need R2 credentials for normal cache restore.
Upload-capable credentials are reserved for trusted producer workflows.

## Cache Layout

Locked dependency objects use:

```text
dependencies/occt/<stable-profile-id>/<archive-sha256>/occt-install.zip
dependencies/occt/source/<source-commit>/<archive-sha256>/occt-source.tar.gz
```

For Geometer OCCT 8:

```text
dependencies/occt/windows-x64-msvc-v143-md-static/<sha256>/occt-install.zip
dependencies/occt/linux-x64-gcc11-static/<sha256>/occt-install.zip
dependencies/occt/linux-arm64-gcc11-static/<sha256>/occt-install.zip
dependencies/occt/macos-arm64-appleclang17-static/<sha256>/occt-install.zip
dependencies/occt/wasm-emscripten-3.1.56-static/<sha256>/occt-install.zip
```

Legacy OCCT prefixes are migration evidence only and are never searched by a
consumer.

## Cache Identity

The archive SHA-256 is the consumer identity. The checked-in lock maps an
explicit selector to a stable profile ID and records the platform/ABI facts,
immutable object key, archive byte count and digest, internal marker digest,
source identity, and qualification evidence. `scripts/dependency_versions.py`
reads OCCT identity from the same lock.

Build recipes, exact producer tools, and patches remain producer evidence.
They do not participate in consumer selection, so changes to local scripts or
compiler patch releases cannot cause an unexpected cache miss or silently
select a nearby archive.

## Configuration

Local and CI consumers may override only the public artifact host:

- `WN_ARTIFACTS_BASE_URL`, default `https://artifacts.wavenumber.net`

Signed R2 fallback and producer uploads use these environment variables:

- `R2_BUCKET` or `GEOMETER_OCCT_CACHE_BUCKET`
- `R2_ENDPOINT_URL` or `GEOMETER_OCCT_CACHE_ENDPOINT_URL`
- `R2_ACCESS_KEY_ID` or `GEOMETER_OCCT_CACHE_ACCESS_KEY_ID`
- `R2_SECRET_ACCESS_KEY` or `GEOMETER_OCCT_CACHE_SECRET_ACCESS_KEY`
- `AWS_DEFAULT_REGION` or `GEOMETER_OCCT_CACHE_REGION`, default `auto`

The R2 endpoint should be the account-level S3 API endpoint. It should not
include the bucket name in the URL path.

`GEOMETER_OCCT_BINARY=off` is an explicit source-producer/debug request.
`auto` and `only` both use the fail-closed lock path; `auto` no longer means
"compile after a miss."

## Consequences

- Fresh CI and developer setup avoid repeated OCCT source builds.
- R2 upload credentials are limited to the dependency producer workflow.
- Consumers verify one explicit lock entry and never search or fall back.
- GitHub cache capacity and invalidation no longer affect OCCT consumption.
- Source builds remain explicit producer/debug operations for reviewed changes.
- A new binary becomes consumable only after immutable upload and lock review.
