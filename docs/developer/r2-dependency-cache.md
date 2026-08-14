# Public Dependency Cache Setup

Geometer can restore prebuilt dependency install trees from the Wavenumber
artifact cache. This avoids expensive local or CI source builds for dependencies
such as OCCT.

The cache is an optimization. Restored archives are still treated as generated
local state under `.deps/`, and each archive is validated by manifest and
SHA-256 before use.

## Access Model

Normal developer machines and CI jobs use public HTTPS reads from:

```text
https://artifacts.wavenumber.net
```

No R2 credentials are required for ordinary cache restore. The public hostname
is backed by the Wavenumber Cloudflare R2 bucket and exposes immutable artifact
prefixes only.

R2 credentials are reserved for private fallback testing and producer workflows.
If credentials are issued, read-only credentials should be allowed to:

- list objects under the Wavenumber dependency-cache prefix
- read objects under the Wavenumber dependency-cache prefix

Read-only credentials should not be allowed to:

- write objects
- delete objects
- change bucket policy

Trusted GitHub producer workflows use separate upload-capable credentials.

When the OCCT recipe changes, run the GitHub `OCCT Dependency Cache` workflow
with `target=all` to build and upload the current platform keys before relying
on fast fresh CI restores.

## Object Layout

The Wavenumber-wide cache bucket is `wn-build-deps`. Objects are stored under
this layout:

```text
deps/v1/<project>/<dependency>/<dependency-version>/<target-kind>/<platform-tag>/<cache-key>/
  manifest.json
  <archive-name>.zip
  <archive-name>.zip.sha256
```

For Geometer's OCCT cache:

```text
deps/v1/geometer/occt/V8_0_1/native/windows-x64/<cache-key>/
deps/v1/geometer/occt/V8_0_1/native/linux-x64/<cache-key>/
deps/v1/geometer/occt/V8_0_1/native/linux-arm64/<cache-key>/
deps/v1/geometer/occt/V8_0_1/native/macos-arm64/<cache-key>/
deps/v1/geometer/occt/V8_0_1/wasm/wasm-emscripten/<cache-key>/
```

## Local Environment

No local `.env` is required for normal cache restore. `scripts/build_occt.py`
and `scripts/build_wasm.py` default to the public artifact domain.

```text
GEOMETER_OCCT_BINARY=auto
GEOMETER_OCCT_CACHE_PUBLIC_BASE_URL=https://artifacts.wavenumber.net
```

`GEOMETER_OCCT_CACHE_PUBLIC_BASE_URL` is optional and only needed when testing a
different artifact hostname. Set `GEOMETER_OCCT_PUBLIC_CACHE=off` to force
source builds or signed R2-only testing.

Create a local `.env` file only for upload-capable producer work or explicit
private fallback testing. Do not commit `.env`.

```text
R2_BUCKET=wn-build-deps
R2_ENDPOINT_URL=<r2-s3-api-endpoint>
R2_ACCESS_KEY_ID=<access-key-id>
R2_SECRET_ACCESS_KEY=<secret-access-key>
AWS_DEFAULT_REGION=auto
```

Use the account-level R2 S3 endpoint for `R2_ENDPOINT_URL`; do not include the
bucket name in the URL path. The build scripts defensively strip a trailing
`/wn-build-deps` segment, but producer secrets should still be kept canonical.

Root `.env` is a local convenience only. Move or remove it before release
signoff because the Wavenumber standards check requires no root `.env` file to
be present.

`GEOMETER_OCCT_BINARY` modes:

- `auto`: use the public cache, then any configured signed R2 fallback, and
  fall back to source build on cache miss
- `only`: require a binary cache hit and fail on cache miss
- `off`: ignore binary caches and build from source

Use `only` when checking that the artifact domain has the expected cache object
without accidentally starting a long source build.

## PowerShell Verification

```powershell
$env:GEOMETER_OCCT_BINARY = "only"
python scripts\build_occt.py --print-binary-cache-key
python scripts\build_occt.py
```

For WASM:

```powershell
$env:GEOMETER_OCCT_BINARY = "only"
python scripts\build_wasm.py --print-occt-binary-cache-key
python scripts\build_wasm.py --occt-only
```

## WSL/Linux Verification

```bash
export GEOMETER_OCCT_BINARY=only
python scripts/build_occt.py --print-binary-cache-key
python scripts/build_occt.py
```

When running WSL in the same checkout as Windows, keep uv environments separate
so Windows and Linux do not replace each other's `.venv`:

```bash
export UV_PROJECT_ENVIRONMENT=.venv-wsl
```

For WASM:

```bash
export GEOMETER_OCCT_BINARY=only
python scripts/build_wasm.py --print-occt-binary-cache-key
python scripts/build_wasm.py --occt-only
```

## Troubleshooting

- If the command reports a cache miss in `only` mode, check that the requested
  key exists under `https://artifacts.wavenumber.net/deps/v1/geometer/occt/`.
  The requested dependency/platform/version may not have been uploaded yet, or
  the prefix may not match.
- If SHA-256 validation fails, delete the local `.deps/` install tree and retry.
  Do not upload replacement objects from a developer machine.
- If source builds start unexpectedly, confirm `GEOMETER_OCCT_BINARY=only` while
  testing cache access.
- If testing signed R2 fallback, confirm `GEOMETER_OCCT_PUBLIC_CACHE=off` before
  assuming the signed credentials are being exercised.
- If producer uploads appear under
  `https://artifacts.wavenumber.net/wn-build-deps/deps/...`, remove the bucket
  path from the configured R2 endpoint and rerun the producer workflow.
