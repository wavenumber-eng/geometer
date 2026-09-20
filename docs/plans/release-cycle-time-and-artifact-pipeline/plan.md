+++
type = "plan"
id = "release-cycle-time-and-artifact-pipeline"
status = "active"
created = "2026-09-20"

[[steps]]
id = "audit-baseline"
title = "Record release-attempt, job-step, cache, storage, and local-hardware baselines"
status = "done"

[[steps]]
id = "target-architecture"
title = "Approve the build-once candidate and promotion architecture"
status = "active"
depends_on = ["audit-baseline"]

[[steps]]
id = "occt-lock"
title = "Replace derived OCCT cache discovery with an explicit immutable dependency lock"
status = "pending"
depends_on = ["target-architecture"]

[[steps]]
id = "local-builder"
title = "Qualify Windows x64 plus direct WSL2 Linux x64 and WASM builders on the AMD workstation"
status = "pending"
depends_on = ["target-architecture", "occt-lock"]

[[steps]]
id = "arm-default-qualification"
title = "Keep GitHub-hosted ARM64 builders as default and qualify the MacBook as a sequential compatibility fallback"
status = "pending"
depends_on = ["target-architecture", "occt-lock"]

[[steps]]
id = "mac-cloud-options"
title = "Benchmark faster macOS ARM64 clouds against the GitHub-hosted default"
status = "pending"
depends_on = ["audit-baseline", "target-architecture"]

[[steps]]
id = "build-graph-consolidation"
title = "Compile each source, profile, platform, and commit once per candidate"
status = "pending"
depends_on = ["target-architecture"]

[[steps]]
id = "test-lane-consolidation"
title = "Remove duplicate client and packaging validation while preserving release coverage"
status = "pending"
depends_on = ["target-architecture"]

[[steps]]
id = "candidate-workflow"
title = "Produce one immutable candidate from local x64/WASM and hosted ARM64 builders"
status = "pending"
depends_on = ["local-builder", "arm-default-qualification", "build-graph-consolidation", "test-lane-consolidation"]

[[steps]]
id = "r2-release-store"
title = "Add immutable R2 candidate storage and exact-byte release mirroring"
status = "pending"
depends_on = ["candidate-workflow"]

[[steps]]
id = "promotion-workflow"
title = "Publish one qualified inventory to PyPI, GitHub Releases, and R2 without rebuilding"
status = "pending"
depends_on = ["candidate-workflow", "r2-release-store"]

[[steps]]
id = "failure-and-security-tests"
title = "Prove resumability, immutability, secret isolation, and fail-closed behavior"
status = "pending"
depends_on = ["promotion-workflow"]

[[steps]]
id = "performance-qualification"
title = "Meet measured clean, warm, candidate, and promotion cycle-time budgets"
status = "pending"
depends_on = ["failure-and-security-tests"]

[[steps]]
id = "design-doc-intent-audit"
title = "Update and audit ADRs, release policy, developer guidance, and artifact governance"
status = "pending"
depends_on = ["performance-qualification"]

[[steps]]
id = "test-runtime-impact-audit"
title = "Audit test-lane coverage, duplication, runtime, and release critical path"
status = "pending"
depends_on = ["performance-qualification"]

[[steps]]
id = "external-review"
title = "Obtain independent build, release, security, and supply-chain review"
status = "pending"
depends_on = ["design-doc-intent-audit", "test-runtime-impact-audit"]

[[steps]]
id = "cutover"
title = "Run one shadow candidate and one production release through the new pipeline"
status = "pending"
depends_on = ["external-review"]

[[exit_criteria]]
id = "single-build"
title = "No release channel recompiles an artifact already present in the qualified candidate inventory"
status = "pending"

[[exit_criteria]]
id = "promotion-time"
title = "Promotion completes in five minutes or less and failed promotion recovery performs zero compilation"
status = "pending"

[[exit_criteria]]
id = "local-cycle"
title = "Windows, Linux x64, and WASM qualify on the AMD workstation, with the constrained MacBook proven as a sequential ARM compatibility fallback"
status = "pending"

[[exit_criteria]]
id = "candidate-cycle"
title = "The complete parallel candidate matrix finishes within fifteen wall-clock minutes"
status = "pending"

[[exit_criteria]]
id = "occt-immutable"
title = "Every OCCT consumer resolves one reviewed immutable lock entry and can never rebuild implicitly"
status = "pending"

[[exit_criteria]]
id = "channel-identity"
title = "R2, GitHub Releases, and PyPI publish bytes whose digests match the one signed candidate inventory"
status = "pending"

[[exit_criteria]]
id = "consumer-continuity"
title = "Existing PyPI and GitHub Release consumers continue to work during the distribution transition"
status = "pending"

[[exit_criteria]]
id = "security"
title = "Self-hosted builders cannot publish releases or access PyPI and production R2 credentials"
status = "pending"

[[exit_criteria]]
id = "no-hidden-duplication"
title = "The workflow reports compilation, validation, upload, and promotion time by artifact and rejects duplicate builds"
status = "pending"

[[exit_criteria]]
id = "mac-cloud-options"
title = "GitHub and shortlisted macOS ARM64 clouds have comparable three-run time, cost, environment, and artifact evidence"
status = "pending"

[[exit_criteria]]
id = "design-doc-intent-audit"
title = "Durable ADRs, design docs, developer guidance, and governance match the implemented pipeline"
status = "pending"

[[exit_criteria]]
id = "test-runtime-impact-audit"
title = "Every test has one justified lane and measured critical-path impact"
status = "pending"

[[exit_criteria]]
id = "external-review"
title = "Independent build, release, security, and supply-chain review has no unresolved blockers"
status = "pending"
+++

# Release Cycle-Time And Artifact Pipeline

## Priority and relationship to active work

This is Geometer's top-priority infrastructure plan. It blocks the next ordinary
feature release after `2026.9.19`; an emergency correction may use the existing
manual release workflow only with an explicit recorded exception.

The plan changes how the outputs of the static-SDK and clipping plans are built,
qualified, stored, and promoted. It does not change their public geometry or
operation contracts. Before this plan closes, amend ADR-018 and the distribution
docs so they describe an immutable multi-channel candidate rather than a GitHub-
Release-only authority. Do not leave that change only in this temporary plan.

## Outcome

Build a release candidate once, on the fastest trusted builder appropriate for
each platform, and promote those exact bytes everywhere. A PyPI or GitHub
publication failure must be recoverable by rerunning only the failed promotion
step. OCCT is a locked binary dependency for ordinary builds: a missing or
mismatched artifact is an immediate error, never permission to spend an hour
rebuilding it.

The target flow is:

```text
reviewed source SHA + dependency lock
        |
        +-- AMD workstation / Windows --------- Windows CLI/wheel/SDK
        +-- AMD workstation / WSL2 ------------ Linux x64 CLI/wheel/SDK
        +-- AMD workstation / WSL2 ------------ WASM/npm/demo payload
        +-- GitHub-hosted Ubuntu ARM64 --------- Linux ARM64 CLI/wheel/SDK
        +-- GitHub-hosted macOS ARM64 ---------- macOS ARM64 CLI/wheel/SDK
        |
        v
canonical candidate inventory + validation records + attestations
        |
        +-- immutable R2 candidate/release objects
        +-- PyPI trusted publication of the inventoried wheels
        +-- GitHub Release compatibility mirror and human-facing catalog
```

The preferred initial target builds Windows, Linux x64, and WASM on the AMD
workstation and keeps GitHub-hosted runners as the default for Linux ARM64 and
macOS ARM64. Faster macOS clouds and the constrained local MacBook are measured
alternatives, not assumptions. The platform list remains the current four
native targets plus WASM until a
downstream and support-policy audit explicitly removes a target. Earlier intent
to emphasize Windows and macOS is not sufficient to silently remove Linux:
published Python wheels and Alexandria's native manifest currently depend on
the broader matrix.

## Measured audit baseline

### Release attempts

The `2026.9.19` release required five workflow attempts. GitHub API timestamps
and job records give the following baseline:

| Workflow run | Result | Workflow wall time | Aggregate job time |
| --- | --- | ---: | ---: |
| `35474146471` | cancelled | 34.02 min | 112.92 min |
| `35475710489` | failed | 57.07 min | 193.18 min |
| `35480219384` | failed | 8.67 min | 23.18 min |
| `35480891798` | failed | 21.77 min | 61.05 min |
| `35482169778` | artifacts and PyPI succeeded; final GitHub promotion step failed | 22.70 min | 66.92 min |
| **Total** | | **144.23 min** | **457.25 min** |

Elapsed time from the first dispatch to final manual publication was about 3
hours 25 minutes, excluding local diagnosis and repair. The final failure was a
one-line directory-creation defect after the expensive bytes had already been
built and published to PyPI. The old workflow design made earlier failures pay
for substantial work again.

### Successful-run critical work

| Lane or step | Observed time |
| --- | ---: |
| Windows native job | 17.33 min |
| Linux x64 native job | 12.93 min |
| Linux ARM64 native job | 11.65 min |
| macOS ARM64 native job | 10.30 min |
| WASM job | 9.70 min |
| Windows static SDK build | 4.17 min |
| Windows relocated SDK validation | 6.50 min |
| Windows production native build/test | 5.08 min |
| WASM compilation | 6.92 min |
| inventory, draft, PyPI, and GitHub promotion steps individually | 0.47-1.50 min |

The release globally forced `CMAKE_BUILD_PARALLEL_LEVEL=2` and
`CARGO_BUILD_JOBS=1`. GitHub's public Windows and Linux runners expose four
CPUs, while the local Ryzen 9 9950X exposes 16 cores/32 threads and 61.6 GiB of
RAM.

### Local clean measurements

Both local measurements used a new build directory, the existing locked OCCT
install, MSVC 19.44, Ninja, Release configuration, and parallelism 16:

| Work | Local | Hosted release | Local speedup |
| --- | ---: | ---: | ---: |
| Production Windows configure/build/20 CTests | 54.39 sec | 5.08 min | 5.6x |
| Windows `/MT` SDK library and link probe | 19.35 sec | 4.17 min | 12.9x |

These are compile/test measurements, not estimates. They prove that using this
machine for Windows candidate production can materially reduce wall time. They
do not yet prove the end-to-end SDK packaging/relocation or WASM budgets; those
remain explicit implementation measurements.

The first native audit artifact reported `promotion_attested=false` because the
developer checkout contains local state such as the root `.env`. This is a
useful constraint, not a reason to reject local building: release candidates
must use a clean isolated worktree or disposable VM, and credentials must live
outside it.

### Current local capacity

- Windows 11 Pro, Ryzen 9 9950X, 16 cores/32 threads, 61.6 GiB RAM.
- WSL2 Ubuntu 24.04 is installed; Docker is not currently installed.
- The available Apple-silicon MacBook has 8 GiB RAM and a 512 GB SSD. It can
  provide native macOS ARM64 and hardware-virtualized Linux ARM64 execution,
  but those lanes must run sequentially, use conservative parallelism, and
  aggressively remove disposable build/dependency state. It is a compatibility
  and outage fallback until measurements prove otherwise.
- A dedicated Ubuntu 22.04 environment is required for the current
  `manylinux_2_35` promise. A binary built directly on Ubuntu 24.04 can acquire
  a newer glibc requirement and is not an acceptable substitute.
- Current generated state is large: `.deps/` is about 27.6 GiB and 506,000
  files, while build/output trees add roughly another 10 GiB. Isolation and
  cleanup policy must account for disk and antivirus/indexing costs.

### GitHub cache inventory

The repository currently has 65 Actions cache entries totaling about 6.31 GiB:

| Family | Entries | Size |
| --- | ---: | ---: |
| OCCT | 22 | 3.60 GiB |
| emsdk | 6 | 2.30 GiB |
| WASM build | 9 | 0.18 GiB |
| Node | 4 | 0.20 GiB |
| uv | 24 | 0.04 GiB |

Branch, pull-request, and tag scopes duplicate the same OCCT content. GitHub
evicts unused caches and applies a repository storage quota, so Actions cache
cannot be the durable authority for a dependency that is intended never to
change implicitly. In the final successful release, OCCT restoration took
seconds; the hours were not caused by an OCCT source build. Earlier misses did
rebuild OCCT and show that the fallback behavior remains too fragile.

### Release payload

The `2026.9.19` GitHub Release contains 22 assets totaling roughly 462 MiB.
The largest objects are the Windows static SDK (about 157 MiB), Linux ARM64 SDK
(55 MiB), Linux x64 SDK (53 MiB), macOS SDK (40 MiB), and native/wheel/WASM
payloads. This fits comfortably within R2's current free storage allowance for
a small number of retained releases; R2 egress is not billed. Retention still
needs an explicit policy so release history cannot grow without bounds.

## Root-cause findings

The audit ranks the current problems as follows:

1. **Qualification and publication are coupled.** The release workflow builds
   from an existing tag, so a late orchestration or channel failure encourages
   another complete release run.
2. **The same sources compile more than once per platform.** Static SDK and
   production CLI/wheel use separate CMake graphs. Non-Windows builds largely
   duplicate `geometer_lib`; Windows deliberately uses `/MT` for the SDK and
   `/MD` for the executable, creating a second Geometer and OCCT profile.
3. **Hosted parallelism is artificially constrained.** Two CMake jobs and one
   Cargo job undersubscribe both hosted machines and the local workstation.
4. **The release native build includes unrelated work.** SDL3/ImGui preview
   sources are built in the release-native graph even when the deliverable and
   production tests do not require the interactive preview.
5. **Client validation is duplicated.** Linux x64 native validation already
   runs Python/Rust/TypeScript integration, while the manual CI workflow also
   has separate language jobs that repeat them.
6. **OCCT lookup has too many identities and fallbacks.** Derived recipe keys,
   legacy prefixes, accepted aliases, local-install migrations, GitHub cache,
   and R2 fallbacks obscure which exact binary is authoritative.
7. **R2 objects are not protected from same-key overwrite.** Current upload
   code writes archive, checksum, and manifest to the derived key without a
   conditional create. R2 is strongly consistent, but last-writer-wins is not
   immutability.
8. **Orchestration defects are discovered after compilation.** Inventory,
   directory layout, shell behavior, and promotion flow lack a small synthetic
   dry run that could fail in under a minute.
9. **Logs do not make duplicated effort a first-class regression.** There is
   no machine-readable compilation ledger proving that an artifact was built
   once and only promoted afterward.

## Decisions

### 1. Separate candidate production from promotion

Introduce two workflows and one shared local command:

- `build-release-candidate` accepts an immutable source commit SHA and candidate
  identifier. It builds only missing platform payloads, validates them, and
  emits a canonical inventory. It never publishes to PyPI or a public GitHub
  Release.
- `promote-release-candidate` accepts the candidate inventory digest and final
  tag. It proves that the tag resolves to the inventoried source SHA, retrieves
  the exact bytes, rechecks every digest, and publishes them. It has no compiler
  setup, OCCT restore, CMake configure, Cargo build, or Emscripten build step.
- `scripts/build_release_candidate.py` is the shared entry point used by local
  developers and workflow runners. The workflow must not contain a separate
  implementation of artifact selection or naming.

Candidate identity is independent of a date tag. A reviewed commit may be fully
qualified before its final release date is chosen. Promotion records the tag as
an immutable alias of that candidate; it does not rename or repackage contents.

Every channel operation is idempotent. If PyPI already contains the exact
inventoried wheel, that wheel is verified and skipped. If a GitHub or R2 asset
already exists with the exact size and digest, it is verified and skipped. A
different byte sequence at an occupied release identity is a hard error.

### 2. Use local compute without trusting it with publication secrets

Use the AMD workstation for Windows x64, Linux x64, and WASM candidate jobs
after qualification. Linux x64 and WASM run in a dedicated Ubuntu 22.04 WSL2
environment so the wheel is built and tested against the governed glibc
baseline.

Use direct WSL2 as the performance baseline and preferred builder. Docker
Desktop's normal Linux-container backend itself uses WSL2, so Docker is not a
separate faster virtualization path and may add image, overlay-filesystem,
volume, and container-start overhead. Keep the checkout, build tree, and
dependency state in the Linux filesystem rather than `/mnt/c`; WSL's cross-OS
filesystem access would otherwise dominate the result.

Obtain isolation with a dedicated Ubuntu 22.04 WSL distribution created from a
versioned root filesystem and reproducible provisioning manifest. Export a
verified clean base or recreate it from the pinned inputs; do not reuse the
interactive Ubuntu 24.04 distribution. Docker installation and comparison are
optional follow-up work only if the dedicated distro proves hard to reproduce
or clean. If compared later, both layouts must invoke the same shared candidate
command and use the same CPU/memory limits and OCCT lock.
Run them through a dedicated, manual-only, self-hosted GitHub Actions runner so
the existing workflow identity, log, artifact upload, and attestation path is
retained. The runner must be isolated from the interactive developer checkout:

- exact reviewed commit, detached clean worktree, and no root `.env`;
- dedicated runner account and work directory, preferably on the larger data
  volume;
- no pull-request, push, or fork-triggered jobs;
- exact runner labels and environment allow-list;
- no PyPI trusted-publisher environment and no production R2 credentials;
- no inherited user secrets, SSH agent, cloud CLI sessions, or writable source
  checkout outside the job;
- destroy the worktree and unregister or reset the runner after each candidate
  until a disposable VM/snapshot runner is available.

The self-hosted job uploads candidate files to GitHub workflow artifacts. A
small hosted ingestion job verifies the candidate inventory and is the only job
allowed to write the immutable R2 candidate store. This prevents a build runner
from becoming a release publisher.

GitHub explicitly warns about self-hosted runners on public repositories. This
design relies on manual dispatch, trusted exact refs, no untrusted workflow
events, secret separation, and ephemeral cleanup. If those controls cannot be
proven, use the local command outside Actions and submit its signed inventory to
a hosted ingestion workflow, or keep the affected target hosted.

### 3. Keep GitHub-hosted ARM64 as the initial default

Continue using native GitHub-hosted `ubuntu-22.04-arm` and `macos-15` runners for
Linux ARM64 and macOS ARM64 candidates. They already execute the full build,
CTest, wheel-install, and relocated-SDK qualification on the target architecture
and require no new trusted build control plane.

Qualify the 8 GiB/512 GB Apple-silicon MacBook only as a compatibility and
outage fallback. Run macOS ARM64 on the host and Linux ARM64 in a dedicated
Ubuntu 22.04 ARM64 VM, sequentially rather than concurrently. Use conservative
parallelism derived from observed memory pressure; restore exact OCCT binaries
from R2 instead of compiling OCCT; place disposable state under one bounded
root; and prove cleanup leaves enough disk for the next lane. A Docker ARM64
container remains optional and is not expected to improve speed.

Also benchmark Linux ARM64 under QEMU/binfmt on the AMD workstation, but treat
it as a disaster-recovery option unless it meets the full candidate budget.
Cross-compilation alone is insufficient because the candidate's CTests, bundled
executable, wheel, and relocated SDK must execute as ARM64. A complete ARM64
guest under QEMU or an ARM64 container using binfmt can do that, but emulated
compilation is likely slower and more fragile than the MacBook's virtualized
ARM guest.

Run three clean and three warm MacBook fallback comparisons against the standard
GitHub-hosted Linux ARM64 and macOS ARM64 runners. GitHub remains the default
regardless of a successful fallback qualification until the separate cloud or
local adoption criteria are approved. An ephemeral DigitalOcean ARM64 builder
remains a final fallback; adopt one only if measured outage recovery justifies
another credentialed control plane.

### 4. Research faster macOS ARM64 cloud capacity

GitHub's standard public macOS ARM64 runner remains the control: M1, 3 CPU,
7 GiB RAM, free for this public repository, and 10.30 minutes observed for the
`2026.9.19` macOS candidate job. Compare the exact shared candidate command and
locked OCCT input on these alternatives:

| Priority | Provider/profile | Current published capacity and price | Initial disposition |
| ---: | --- | --- | --- |
| Control | GitHub standard macOS | M1, 3 CPU, 7 GiB; free for this public repository | Default until displaced by evidence |
| Control | GitHub XLarge macOS | M2 Pro, 5 CPU, 14 GiB; $0.102/min and requires an eligible organization plan | Separates hardware gain from provider migration |
| 1 | Buildkite M4 Medium/Large | 6 CPU/28 GiB at $0.12/min or 12 CPU/56 GiB at $0.24/min, metered to the second | First external speed benchmark |
| 2 | Codemagic M4 | 10-core M4, 16 GiB at $0.114/min; personal plan currently includes 500 M2 minutes/month | Second external speed/cost benchmark |
| 3 | CircleCI M4 Pro | 6 CPU/28 GiB or 12 CPU/56 GiB; 200 or 400 credits/min | Benchmark after normalizing plan and credit cost |
| 4 | Cirrus CI Apple silicon | Ephemeral Tart macOS VMs; open-source projects advertise a free tier | Evaluate availability/current hardware; older published M1 data may not beat GitHub materially |
| 5 | Scaleway dedicated M2 | 8-core M2, 16 GiB, 256 GB at EUR 0.17/hour with a 24-hour minimum | About EUR 4.08 minimum; useful dedicated fallback, not first burst choice |
| 6 | AWS EC2 Mac | M2/M2 Pro/M4 bare metal with 24-48 GiB; 24-hour Dedicated Host minimum | Operationally capable but poor fit for occasional ten-minute builds |
| 7 | MacStadium | Dedicated M4 10-core/16 GiB begins at $149/month; larger monthly profiles available | Consider only if release frequency justifies an always-on host |

For the top three external candidates, run the same reviewed source SHA at
least three times with:

- the same exact R2 OCCT lock and no dependency source builds;
- equivalent clean checkout and no compiler-object cache for the clean result;
- the same candidate command, test selection, SDK relocation, packaging, and
  upload destination;
- measured queue, machine startup, checkout, dependency restore, compile, test,
  package, and upload durations;
- reported CPU model/allocation, memory peak, swap, disk, Xcode/Clang, and
  macOS deployment target;
- candidate byte digests and validation-ledger equivalence; and
- actual per-candidate cost including platform/base-plan charges.

Adopt a non-GitHub default only if it reduces macOS candidate wall time by at
least 30 percent across three runs, remains below $3 per candidate at current
release frequency, supports clean ephemeral state, and hands the exact bytes to
the hosted R2-ingestion/promotion boundary without publication credentials.
Separate CI configuration must remain a thin invocation of the repository's
shared candidate command; do not fork release behavior into Buildkite,
Codemagic, CircleCI, or another provider.

### 5. Lock OCCT instead of discovering it

Replace ordinary consumer key derivation with a checked-in canonical OCCT lock,
for example `dependencies/occt-lock.json`. One reviewed entry per supported
binary profile contains:

- OCCT repository, exact tag, and source commit;
- platform, architecture, compiler family/major, C++ ABI, CRT, library type,
  and deployment baseline;
- build recipe identity and tool versions;
- immutable R2 object key;
- archive SHA-256 and byte count; and
- canonical profile-manifest SHA-256.

Ordinary native, SDK, WASM, local, and CI consumers perform only:

1. find the exact profile in the lock;
2. accept a local install only when its marker matches the complete lock entry;
3. otherwise download the one exact R2 object;
4. verify byte count, archive digest, and internal profile; and
5. fail within 30 seconds if any object or identity is absent or wrong.

There is no source-build fallback, legacy prefix, nearest-key restore, accepted
alias, or local marker migration in a consumer path. GitHub Actions cache is
removed from OCCT consumption after cutover; a verified extracted local install
may remain a machine-local acceleration, but R2 is the only remote dependency
authority.

Only the separately dispatched `publish-occt-dependency` producer may compile
OCCT. It requires an explicit profile and human-reviewed lock update, uploads
to a new content-addressed key with conditional create (`If-None-Match: *`),
downloads and verifies it, and then changes the lock in a normal reviewed
commit. Enable an indefinite R2 bucket lock for the OCCT immutable prefix so
even privileged accidental overwrite or deletion is rejected. Publishing a new
OCCT build creates a new object and lock digest; it never mutates an existing
one.

### 6. Consolidate the Geometer build graph

For Linux and macOS, configure one release CMake graph per platform/profile that
produces the library, CLI, production tests, SDK link probe/install tree, native
archive, and wheel input. Package different deliverables from the same compiled
objects.

For Windows, run a bounded decision spike:

- preferred: move the release CLI and bundled Python executable to the supported
  `/MT` profile so CLI, wheel, tests, and SDK share one OCCT and Geometer graph;
- fallback: retain separate `/MD` runtime and `/MT` SDK profiles, explicitly
  accepting two Windows compilations while eliminating every other duplicate.

The preferred option must pass executable/wheel import inspection, clean Python
installation, Rust static linking, and all existing runtime behavior. Do not
merge profiles merely to improve timing if it weakens CRT correctness.

Move SDL3/ImGui preview compilation into an interactive-demo qualification lane.
The preview should build when its source or packaging changes and before a demo
release, not as a hidden prerequisite for every headless CLI/wheel/SDK candidate.

Derive parallelism from available CPUs and a measured memory-per-compiler budget.
Do not impose repository-wide values of CMake 2 and Cargo 1. Record the chosen
values in each build record. Benchmark compiler caches only after graph
consolidation: prefer Ninja incremental reuse on isolated persistent dependency
volumes first; add `sccache` only if three-run evidence shows a material gain
without making correctness depend on cache availability.

### 7. Consolidate tests by responsibility

Keep one execution of each release assertion per source SHA and artifact:

- source-only standards, generated-file, contract, formatting, and packaging-
  policy checks run before native matrices;
- production C++ tests execute on every supported native candidate;
- Python, Rust, and TypeScript cross-language tests execute once against the
  canonical candidate platform, plus focused platform packaging/install smokes;
- wheel repair and clean install execute per wheel;
- SDK relocation and clean C/C++/Rust link tests execute per SDK;
- WASM Node/direct/Worker parity executes once from the one WASM candidate;
- experimental research qualification remains separately dispatched unless a
  change-class rule selects it;
- expensive downstream trials consume the already-built candidate bytes.

Add a generated test ledger to the candidate inventory. Each gate declares the
source SHA, input artifact digests, platform/profile, command identity, outcome,
duration, and log reference. Inventory validation rejects duplicate build
producers for one artifact identity and missing required tests.

Before any real platform build, run a sub-minute synthetic release dry run that
exercises artifact naming, directory creation, canonical ordering, inventory
generation, upload/download, draft/public release transitions in a test target,
and idempotent resume logic using tiny fixtures. Add workflow and shell linting
so path-creation and expression errors fail before matrix work.

### 8. Make R2 the immutable byte store, not the Python package manager

Use R2 for two distinct immutable namespaces or buckets:

```text
dependencies/occt/<profile>/<archive-sha256>/...
releases/candidates/<source-sha>/<inventory-sha256>/...
releases/tags/vYYYY-MM-DD/<inventory-sha256>.json
```

Candidate paths are content-addressed and bucket-locked. The tag object is a
small immutable signed alias to an existing candidate inventory. Store every
native archive, static SDK, WASM/npm/demo archive, wheel, checksum, internal
manifest, validation record, and provenance bundle referenced by the inventory.

Distribution roles are deliberately different:

| Channel | Role | Consumer behavior |
| --- | --- | --- |
| PyPI | Canonical Python index and wheel distribution | `pip`/`uv` installs a self-contained platform wheel with the Geometer executable; no R2 runtime fetch |
| R2 | Immutable large-byte origin, candidate handoff, dependency authority | Native/SDK/WASM tools may use digest-pinned URLs; promotion resumes from exact stored bytes |
| GitHub Releases | Human-facing release page, compatibility mirror, source/tag association | Existing direct-URL consumers keep working; every asset must match R2 and inventory digests |

Do not create a private R2 Python index or convert `wn-geometer` into a thin
downloader. That would move resolution, offline installation, integrity,
availability, and cache behavior out of normal PyPI tooling for no material
cycle-time benefit. Mirroring wheel bytes to R2 is useful for candidate recovery
and audit, but PyPI remains their public install channel.

GitHub Release assets may later become a smaller catalog if every downstream
consumer migrates to R2 and policy approves the compatibility break. During
this plan, upload the same inventoried bytes to both. In particular, update
Alexandria only after its existing GitHub native URLs have a tested R2 successor.

Use R2 lifecycle rules only for abandoned candidate prefixes after a generous
review window. Tagged release objects and their inventories are retained
indefinitely unless a separate retention ADR changes that policy. Collect
storage and request metrics so the initial low cost remains visible.

### 9. Preserve trusted PyPI publication

PyPI publication stays in a GitHub-hosted job using Trusted Publishing and the
protected release environment. The job downloads wheels from the immutable R2
candidate or the workflow artifact, proves their digests against the signed
inventory, and publishes those exact files. No local/self-hosted builder receives
the PyPI identity token or release-environment access.

Trusted Publishing establishes the GitHub repository, workflow, environment,
and commit identity of the publishing act. Candidate build provenance is a
separate attestation and must identify the self-hosted or hosted builder and
toolchain honestly; promotion must not imply that a hosted publisher compiled
bytes it only verified.

### 10. Make promotion resumable and channel-aware

Promotion state is a canonical record keyed by inventory digest with independent
states for R2, PyPI, draft GitHub Release, public GitHub Release, and final
download verification. Rerunning promotion:

- verifies completed channel objects instead of uploading them again;
- resumes the first incomplete channel;
- never rebuilds, repackages, resigns with different contents, or moves a tag;
- treats PyPI's immutable filename/version collision as success only when the
  published file digest is the expected digest; and
- retains enough candidate data to recover after workflow artifacts expire.

A final public verification job downloads from PyPI, R2, and GitHub, checks the
same inventory, and records channel URLs and digests. It may retry network reads;
it cannot alter candidate bytes.

## Implementation phases

### Phase A: low-risk immediate reductions

1. Add step-timing and compilation-ledger output to the current workflow.
2. Replace global CMake/Cargo throttles with measured per-runner values.
3. Run inventory/order/naming/shell dry-run tests before matrices.
4. Stop duplicating Python/Rust/TypeScript tests in `ci.yml`.
5. Exclude preview/example compilation from headless release production.
6. Preserve current publication behavior while measuring the reductions.

### Phase B: immutable dependencies and local builders

1. Create and review the OCCT lock schema and current-profile entries.
2. Publish existing verified OCCT archives to new content-addressed locked R2
   keys; download-verify before switching consumers.
3. Remove consumer source-build and legacy/alias fallbacks.
4. Qualify Windows plus direct WSL2 Linux x64 and WASM runners with a non-release
   candidate; defer Docker unless reproducibility evidence requires it.
5. Keep GitHub-hosted ARM64 as the default; qualify native macOS ARM64 and a
   sequential Ubuntu 22.04 ARM64 VM on the MacBook only as fallback evidence.
6. Benchmark Buildkite M4 and Codemagic M4 against GitHub macOS, then CircleCI
   M4 Pro if neither meets the adoption threshold.
7. Benchmark AMD-host QEMU ARM64 only as a disaster-recovery path.

### Phase C: build-once candidates

1. Implement the shared candidate command and canonical inventory.
2. Consolidate CMake graphs and choose the Windows CRT outcome.
3. Combine locally built Windows/Linux x64/WASM with the selected hosted ARM64
   payloads into one candidate.
4. Add hosted R2 ingestion and immutable candidate retention.
5. Run a complete shadow candidate beside the old release workflow and compare
   every artifact name, content policy, test result, and duration.

### Phase D: promotion-only release

1. Implement idempotent promotion from inventory digest.
2. Publish identical bytes to R2 and GitHub; publish inventoried wheels to PyPI.
3. Verify all public channels and downstream install URLs.
4. Amend ADR-018, distribution policy, developer docs, artifact governance, and
   release signoff to make the new authority durable.
5. Obtain independent review, execute one real release, and remove the old
   rebuild-on-tag path only after rollback evidence is complete.

## Validation and failure injection

The cutover is incomplete until automated tests demonstrate:

- missing OCCT lock entry, object, checksum, or profile fails without compiling;
- an attempted overwrite of an OCCT or tagged-release object fails;
- a dirty local checkout cannot produce a promotable attestation;
- untrusted refs and pull-request events cannot schedule the self-hosted runner;
- self-hosted jobs cannot access PyPI or production R2 credentials;
- an interrupted upload resumes and yields the original digest;
- failure immediately before and after PyPI publication resumes with no build;
- failure immediately before and after GitHub publication resumes with no build;
- an occupied asset/version with different bytes fails closed;
- an expired GitHub workflow artifact can be recovered from immutable R2;
- R2/GitHub/PyPI public downloads match the canonical inventory;
- release tag/source SHA/candidate SHA mismatches fail before publication;
- a runner loss leaves no mutable authoritative state only on that runner; and
- old GitHub Release and PyPI consumer paths remain valid.

## Performance budgets

Measure clean and warm time separately. Network outages are reported separately
from compute regressions, but they do not relax correctness.

- Clean local Windows native + SDK candidate: at most 5 minutes.
- Warm local Windows native + SDK candidate: at most 2 minutes.
- Clean local WASM candidate: at most 5 minutes.
- Warm local WASM candidate: at most 2 minutes.
- Complete AMD-workstation Windows + Linux x64 + WASM qualification: at most 10
  clean minutes and 5 warm minutes when its independent lanes run in parallel.
- Each GitHub-hosted ARM64 lane: at most 15 minutes.
- Any adopted external macOS cloud must be at least 30 percent faster than the
  three-run GitHub standard control and cost less than $3 per candidate.
- Each sequential MacBook fallback lane: at most 20 minutes without memory
  exhaustion or more than 50 GiB of disposable local state.
- Complete candidate wall time when all lanes run in parallel: at most 15
  minutes, excluding an explicitly approved downstream application trial.
- Promotion and public verification: at most 5 minutes with zero compilation.
- Recovery from a publication-only failure: at most 5 minutes with zero
  compilation.
- OCCT missing/mismatch failure: at most 30 seconds before platform compilation.
- One build producer per artifact identity and source/profile/platform tuple.

Record p50 and worst of three clean and three warm runs before accepting the
budgets. A cache hit is an optimization; correctness and artifact identity must
be identical after a cache miss.

## Documentation and governance changes

At implementation closeout, update at least:

- `docs/developer/ci-strategy.md` with the new lanes and measured budgets;
- `docs/developer/README.md` with local candidate, isolated runner, OCCT lock,
  and promotion commands;
- `docs/design/distribution.md` with PyPI/R2/GitHub roles;
- ADR-018's GitHub-Release-only authority and promotion language;
- `docs/governance/artifacts.toml` and `docs/governance/release.toml`;
- the OCCT qualification guide and dependency-cache workflow;
- downstream manifests that intentionally migrate from GitHub to R2;
- release troubleshooting and rollback guidance; and
- a durable audit report containing the before/after timings and costs.

Remove this plan after the new release pipeline has shipped and those records
are authoritative.

## Research sources

Primary sources used for architecture and cost assumptions:

- [GitHub Actions billing](https://docs.github.com/en/billing/concepts/product-billing/github-actions): standard GitHub-hosted runners are free in public repositories; self-hosted runner use is free.
- [GitHub-hosted runner reference](https://docs.github.com/en/actions/how-tos/write-workflows/choose-where-workflows-run/choose-the-runner-for-a-job): current standard public-runner resource classes.
- [Self-hosted runner reference](https://docs.github.com/en/actions/reference/runners/self-hosted-runners): routing, lifecycle, and ephemeral-runner behavior.
- [Adding self-hosted runners](https://docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/add-runners): GitHub's warning about public-repository fork risk.
- [GitHub dependency caching](https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching): cache scope, quota, and eviction behavior.
- [GitHub artifact attestations](https://docs.github.com/en/actions/concepts/security/artifact-attestations): workflow-bound build provenance.
- [PyPI Trusted Publishing security model](https://docs.pypi.org/trusted-publishers/security-model/): publisher identity and short-lived credentials.
- [Cloudflare R2 pricing](https://developers.cloudflare.com/r2/pricing/): storage, operations, and egress pricing.
- [Cloudflare R2 consistency](https://developers.cloudflare.com/r2/reference/consistency/): strong consistency and last-writer behavior.
- [Cloudflare R2 bucket locks](https://developers.cloudflare.com/r2/buckets/bucket-locks/): retention and overwrite/delete protection.
- [Cloudflare R2 S3 compatibility](https://developers.cloudflare.com/r2/api/s3/api/): conditional `PutObject` support.
- [PyPA manylinux](https://github.com/pypa/manylinux): glibc compatibility policy and build images.
- [DigitalOcean Droplet pricing](https://www.digitalocean.com/pricing/droplets): current dedicated-CPU comparison pricing.
- [Docker Desktop WSL2 backend](https://docs.docker.com/desktop/features/wsl/): Docker's Windows Linux-container backend, resource behavior, integration, and data-location guidance.
- [Microsoft WSL2 architecture](https://learn.microsoft.com/en-us/windows/wsl/wsl2-about): WSL2's managed VM, Linux kernel, and filesystem-performance guidance.
- [GitHub hosted-runner specifications](https://docs.github.com/en/actions/reference/runners/github-hosted-runners): standard macOS M1 capacity.
- [GitHub larger-runner specifications](https://docs.github.com/en/actions/reference/runners/larger-runners): M2 Pro ARM64 profile and limitations.
- [GitHub Actions runner pricing](https://docs.github.com/en/billing/reference/actions-runner-pricing): current standard and larger-runner rates.
- [Buildkite hosted-agent pricing](https://www2.buildkite.com/pricing/): M4 macOS shapes and per-second billing.
- [Codemagic pricing](https://codemagic.io/pricing/): M2/M4 per-minute rates, free allowance, and concurrency.
- [CircleCI macOS execution environment](https://circleci.com/docs/guides/execution-managed/using-macos/): M4 Pro resource shapes and supported environments.
- [CircleCI price list](https://circleci.com/pricing/price-list/): current macOS credit rates.
- [Cirrus CI macOS VMs](https://cirrus-ci.org/guide/macOS/): ephemeral Apple-silicon Tart VM support.
- [Scaleway Apple silicon pricing](https://www.scaleway.com/en/pricing/apple-silicon/): dedicated M2 capacity and hourly price.
- [AWS EC2 Mac FAQ](https://aws.amazon.com/ec2/faqs/): Apple-silicon profiles and 24-hour Dedicated Host minimum.
- [MacStadium pricing](https://macstadium.com/pricing): current dedicated Apple-silicon monthly profiles.

## Closure order

1. Approve the target architecture and budgets in this plan.
2. Implement the immediate low-risk reductions and measure them.
3. Replace OCCT discovery/fallback with the immutable lock and locked R2 objects.
4. Qualify Windows, Linux x64, and WASM on the AMD workstation; retain GitHub as
   the ARM64 default, qualify the MacBook fallback, and complete the macOS cloud
   comparison.
5. Consolidate build/test graphs and produce one complete shadow candidate.
6. Implement R2 ingestion and promotion-only PyPI/GitHub publication.
7. Run failure injection, security review, and independent review.
8. Cut one real release, publish the before/after audit, update durable docs,
   retire the old workflow, and close this temporary plan.
