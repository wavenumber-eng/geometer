+++
type = "plan_log"
id = "occt-lock-audit-2026-09-20"
plan_id = "release-cycle-time-and-artifact-pipeline"
step_id = "occt-lock"
created = "2026-09-20T09:27:44-04:00"
+++

# Log: occt-lock audit

Read-only R2 and upstream inspection established the exact migration inventory
before a lock schema or consumer cutover is accepted. No R2 object was written,
no bucket policy was changed, and no consumer was switched.

The OCCT `V8_0_1` tag is annotated. Its tag-object SHA is
`c5605924864829ce8c1e1477f976ffb3880538a8`; the source commit is the peeled
`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`. The codeload archive addressed by
the peeled commit is 45,131,814 bytes with SHA-256
`dba62b81078dd43cec23feba89432be301582341001edad1b93342ad8bda35ea`.
It contains root `LICENSE_LGPL_21.txt` and `OCCT_LGPL_EXCEPTION.txt`.

The six existing public archives were verified against their public manifest,
checksum sidecar, and content length:

| Profile | Archive SHA-256 | Bytes | Recipe |
| --- | --- | ---: | --- |
| Linux ARM64 GCC 11 | `b0365beeb38e24902f0986437c0544935309fce52e25af7808386e83a1215086` | 66,156,117 | `27635f141bd6348796cc4a13b6b2d56b248a05c2dc00a209bcae14760eb0926b` |
| Linux x64 GCC 11 | `944288a4095d15badbd6f2589c23a46e925efb35d9358f1f3d69bca6b50d4fd8` | 63,696,694 | `2f6f9279d66a98f1ca4fd464299d50fd6d1e2989c5fd208d97b096e93469d10b` |
| macOS ARM64 Apple Clang 17 | `0eca21e61fa621725b3d735dfdf619b9d11d8045ba0df6821a5ee108d81da5f8` | 50,468,138 | `8776f0c80db8373283aaaeeb8f8b3a9718c4ac19348ff62bec5598ff8eb21976` |
| Windows x64 MSVC v143 `/MD` | `940d35de8c47d38fe2ce9f7e2b263d7aa58c8fa7deae3ac79ed27850bee968be` | 188,372,259 | `afddffde43bb0288a99269de68220cc973eb3212551045a0f779e964a912231b` |
| Windows x64 MSVC v143 `/MT` | `6ffd26db0f67d874a47d4aeb2df421ca36b576eeda77e3e4c33e5a343ad7c9ad` | 220,161,765 | `38db7a0a99c3d23f9cf3adfb23bdd599377c067826e7204473172a18398cdc9c` |
| WASM Emscripten 3.1.56 | `44fe6d6294c7a26ac77cfa17e1fd4a312578638a5669b7032c227750d032614e` | 57,317,147 | `a15818c33b508d24f66702e3834be2d25fce89031a00a58f6391c0d702bb95f4` |

The five native manifests identify packaging run `35477956256` at Geometer SHA
`53f2d7f112bae5bf396a1c2adf15d2cf2ac26a0c`; only its Windows `/MT` lane
performed a source build. The other native lanes restored GitHub cache before
repackaging. The legacy WASM manifest has no GitHub repository, run, or source
SHA and uses recipe `a15818c3...`; its currently reviewed consumer recipe is
`c48157a4...`. A final lock must record actual artifact provenance separately
from an explicit compatibility decision, or replace the WASM bytes with a clean
current-recipe build.

Independent review rejected the first uncommitted schema draft. The replacement
must:

- keep legacy v1 source keys in a one-time migration manifest, never the
  permanent consumer lock;
- record and verify the canonical internal install-profile manifest digest;
- distinguish artifact recipe/build provenance from consumer compatibility;
- bind exact tool versions and a recoverable digest-addressed build-recipe,
  patch, license, and relink-evidence bundle;
- use the peeled OCCT source commit and corresponding-source digest above;
- keep schema validation generic while a separate governed policy checks the
  current supported matrix; and
- make `dependency_versions.py` derive from the lock or prove equality until
  cutover.

Next implementation action: enhance the producer evidence contract and create
the separate migration manifest. Rebuild WASM once under the current recipe (or
approve explicit compatibility evidence), review the resulting lock, then copy
the exact five native and qualified WASM/source bytes to conditional-create,
content-addressed R2 keys before changing any consumer.
