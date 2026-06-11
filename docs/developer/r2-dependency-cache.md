# R2 Dependency Cache Setup

Geometer can restore prebuilt dependency install trees from the Wavenumber R2
dependency cache. This avoids expensive local or CI source builds for
dependencies such as OCCT.

The cache is an optimization. Restored archives are still treated as generated
local state under `.deps/`, and each archive is validated by manifest and
SHA-256 before use.

## Access Model

Use read-only credentials for developer machines.

Developer credentials should be allowed to:

- list objects under the Wavenumber dependency-cache prefix
- read objects under the Wavenumber dependency-cache prefix

Developer credentials should not be allowed to:

- write objects
- delete objects
- change bucket policy

Trusted GitHub producer workflows use separate upload-capable credentials.

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
deps/v1/geometer/occt/V8_0_0/native/windows-x64/<cache-key>/
deps/v1/geometer/occt/V8_0_0/native/linux-x64/<cache-key>/
deps/v1/geometer/occt/V8_0_0/native/linux-arm64/<cache-key>/
deps/v1/geometer/occt/V8_0_0/native/macos-arm64/<cache-key>/
deps/v1/geometer/occt/V8_0_0/wasm/wasm-emscripten/<cache-key>/
```

## Local Environment

Create a local `.env` file in the repository root or set equivalent shell
environment variables. Do not commit `.env`.

```text
R2_BUCKET=<bucket-name>
R2_ENDPOINT_URL=<r2-s3-api-endpoint>
R2_ACCESS_KEY_ID=<read-only-access-key-id>
R2_SECRET_ACCESS_KEY=<read-only-secret-access-key>
AWS_DEFAULT_REGION=auto
GEOMETER_OCCT_BINARY=auto
```

For Wavenumber-managed developer machines, `R2_BUCKET` is `wn-build-deps`.
Use a read-only R2 token for normal local builds.

Root `.env` is a local convenience only. Move or remove it before release
signoff because the Wavenumber standards check requires no root `.env` file to
be present.

`GEOMETER_OCCT_BINARY` modes:

- `auto`: use R2 when available and fall back to source build on cache miss
- `only`: require R2 and fail on cache miss
- `off`: ignore R2 and build from source

Use `only` when checking that a developer machine has cache access without
accidentally starting a long source build.

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

- If the command reports that the cache is not configured, check the R2 env
  vars or `.env` file.
- If `only` mode reports a cache miss, the requested dependency/platform/version
  has not been uploaded yet or the prefix does not match.
- If SHA-256 validation fails, delete the local `.deps/` install tree and retry.
  Do not upload replacement objects from a developer machine.
- If source builds start unexpectedly, confirm `GEOMETER_OCCT_BINARY=only` while
  testing cache access.
