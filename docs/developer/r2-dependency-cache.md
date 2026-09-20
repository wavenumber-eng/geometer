# R2 setup for locked OCCT dependencies

Geometer reads reviewed OCCT install archives from the public Wavenumber
artifact host:

```text
https://artifacts.wavenumber.net
```

Normal developer and CI consumers need no R2 credentials. They select one
profile from `dependencies/occt-lock.json` and either reuse its exact local
install marker or download its one immutable object. A missing object, wrong
byte count, wrong SHA-256, wrong internal marker, or wrong OCCT version fails
closed without compiling OCCT.

## Immutable layout

```text
dependencies/occt/<stable-profile-id>/<archive-sha256>/occt-install.zip
dependencies/occt/source/<source-commit>/<archive-sha256>/occt-source.tar.gz
```

The key contains the full content digest. Producer uploads use
`If-None-Match: *`; objects are never overwritten. Configure an indefinite R2
retention lock for the `dependencies/occt/` prefix as an additional service-side
guard.

## Consumer verification

PowerShell:

```powershell
python scripts\build_occt.py --print-binary-cache-key
python scripts\build_occt.py
python scripts\build_wasm.py --print-occt-binary-cache-key
python scripts\build_wasm.py --occt-only
```

WSL/Linux uses the same commands. When sharing a checkout with Windows, keep a
separate uv environment, for example `UV_PROJECT_ENVIRONMENT=.venv-wsl`.

`WN_ARTIFACTS_BASE_URL` may replace the public host for an isolated test. It
does not change the locked object key or expected bytes.

## Producer credentials

Only the manual `.github/workflows/occt-deps.yml` producer needs:

```text
R2_BUCKET=wn-build-deps
R2_ENDPOINT_URL=<account-level-r2-s3-endpoint>
R2_ACCESS_KEY_ID=<upload-key-id>
R2_SECRET_ACCESS_KEY=<upload-secret>
AWS_DEFAULT_REGION=auto
```

Do not include the bucket in `R2_ENDPOINT_URL`. Keep credentials in GitHub
secrets or a temporary ignored `.env`; remove the root `.env` before release
signoff.

Configure the GitHub `occt-dependency-production` environment to allow only the
repository default branch and require review before secrets are released. The
workflow itself rejects non-default-branch dispatches; the environment rule is
the independent control that prevents a modified workflow ref from removing
that check and receiving credentials.

The producer always builds from source and packages without secrets. A
one-day GitHub artifact hands the archive to a separate hosted publisher in the
protected `occt-dependency-production` environment. That job validates the
candidate without extracting or executing it, then creates the digest-addressed
R2 object. Build jobs never receive R2 credentials. The retained evidence is
`manifest.json` plus `occt-install.zip.sha256`; review those files and update the
lock in a normal commit. Producer recipe/tool details explain the candidate;
they never route consumers.

Dispatch `all` for a deliberate matrix refresh or choose one exact profile for
a targeted rebuild/retry. Publication verifies the actual source tag object and
peeled commit against `dependencies/occt-lock.json` before creating an object.

Manual local source builds are explicit:

```powershell
python scripts\build_occt.py --binary-cache off
python scripts\build_wasm.py --occt-only --occt-binary-cache off
```

## Troubleshooting

- A locked download failure means the selected immutable object is absent or
  inaccessible. Do not silently build or search another prefix.
- A digest or marker mismatch means the object or lock is wrong. Stop and audit
  the exact bytes; do not replace an existing object.
- If an intentional new build is needed, run the producer, review its evidence,
  qualify the result, and update the lock.
