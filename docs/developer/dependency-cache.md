# Locked OCCT Dependency

Geometer consumes OCCT from one canonical checked-in lock:
`dependencies/occt-lock.json`. The extracted install remains generated state
under `.deps/`; neither archives nor installs are committed.

## Consumer contract

Each supported target has one stable profile ID and one exact archive record:

- explicit target and ABI facts;
- immutable R2 object key;
- archive byte count and SHA-256;
- internal producer-profile SHA-256; and
- qualification evidence.

The archive SHA-256 is the identity. A consumer selects a profile by the
explicit target selector, accepts a local install only when its lock marker
matches, or downloads exactly the recorded object from
`https://artifacts.wavenumber.net`. It verifies the archive and extracted
install before replacing local state.

There is no consumer-side key derivation, legacy prefix search, alias,
nearest-match lookup, signed fallback, GitHub OCCT cache, or source-build
fallback. Missing or mismatched state is an error.

The source archive is also locked by peeled upstream commit, byte count, digest,
license-file inventory, and immutable object key.

## Object layout

```text
dependencies/occt/<stable-profile-id>/<archive-sha256>/occt-install.zip
dependencies/occt/source/<source-commit>/<archive-sha256>/occt-source.tar.gz
```

An object key is created once. Publication uses `If-None-Match: *`; a producer
cannot overwrite an occupied key.

`scripts/r2_store.py` is the one SigV4 and conditional-create transport used by
immutable R2 publishers. OCCT keeps its dependency-specific validation and key
policy in `occt_producer.py`; it does not carry a second signing implementation.

## Producer contract

`.github/workflows/occt-deps.yml` is the credentialed, manual-only producer. It
always builds from source and publishes a new digest-addressed candidate. It
builds and packages without credentials, hands the short-lived archive to a
separate hosted publication job, and only that protected-environment job can
write R2. The publisher checks out the same canonical default-branch commit,
validates the handoff as untrusted data, and conditionally creates the R2
object. Build jobs never receive production R2 credentials.

The dispatch input selects either `all` or one exact lock profile, so a failed
platform can be retried without rebuilding unrelated profiles. Before upload,
the producer verifies the checkout's tag object and peeled commit against the
source identity in the lock.

Producer recipe hashes and toolchain details are evidence about how candidate
bytes were made. They do not select consumer bytes. After review, update the
lock with the candidate object key, byte count, archive digest, internal marker
digest, and qualification evidence. Consumers cannot use the candidate before
that reviewed lock change.

R2 producer credentials use `R2_BUCKET`, `R2_ENDPOINT_URL`,
`R2_ACCESS_KEY_ID`, and `R2_SECRET_ACCESS_KEY`. Normal consumers require no R2
credentials. `WN_ARTIFACTS_BASE_URL` may override the public host for an
isolated test.

## Explicit source builds

Source builds are producer or debugging operations, never an automatic miss
path:

```powershell
python scripts\build_occt.py --binary-cache off
python scripts\build_wasm.py --occt-only --occt-binary-cache off
```
