+++
type = "plan"
id = "release-cycle-time-and-artifact-pipeline"
status = "active"
created = "2026-09-20"

[[steps]]
id = "audit"
title = "Audit release time, duplicate work, dependency caching, and supported builders"
status = "done"

[[steps]]
id = "occt-lock"
title = "Make OCCT an explicit immutable dependency with no implicit rebuild"
status = "done"
depends_on = ["audit"]

[[steps]]
id = "candidate-command"
title = "Use one fixed candidate build and qualification command locally and in CI"
status = "done"
depends_on = ["audit", "occt-lock"]

[[steps]]
id = "artifact-handoff"
title = "Replace the R2 release store with the retained qualified GitHub Actions artifact"
status = "active"
depends_on = ["candidate-command"]

[[steps]]
id = "promotion"
title = "Promote exact candidate bytes to PyPI and GitHub without rebuilding"
status = "pending"
depends_on = ["artifact-handoff"]

[[steps]]
id = "test-runtime-impact-audit"
title = "Audit test ownership and remove proven duplicate release work"
status = "done"
depends_on = ["audit"]

[[steps]]
id = "design-doc-intent-audit"
title = "Align developer, design, governance, and generated documentation"
status = "pending"
depends_on = ["promotion"]

[[steps]]
id = "external-review"
title = "Resolve independent implementation and security review findings"
status = "pending"
depends_on = ["design-doc-intent-audit", "test-runtime-impact-audit"]

[[steps]]
id = "shadow-candidate"
title = "Run the complete candidate matrix from main and verify the retained handoff"
status = "pending"
depends_on = ["external-review"]

[[steps]]
id = "cutover"
title = "Enable protected environments and remove obsolete R2 release credentials"
status = "pending"
depends_on = ["shadow-candidate"]

[[exit_criteria]]
id = "build-once"
title = "Promotion performs no compilation or packaging and publishes inventoried candidate bytes"
status = "pending"

[[exit_criteria]]
id = "one-input"
title = "Promotion requires only a successful candidate workflow run ID"
status = "pending"

[[exit_criteria]]
id = "occt-only-r2"
title = "R2 is used only for locked OCCT dependency archives"
status = "pending"

[[exit_criteria]]
id = "supported-platforms"
title = "Windows x64, Linux x64, Linux ARM64, macOS ARM64, and WASM qualify once"
status = "pending"

[[exit_criteria]]
id = "exact-publication"
title = "PyPI and the immutable GitHub Release match the candidate inventory"
status = "pending"

[[exit_criteria]]
id = "resumable"
title = "A retry resumes publication without rebuilding or overwriting conflicting bytes"
status = "pending"

[[exit_criteria]]
id = "design-doc-intent-audit"
title = "Durable design and developer documentation matches the implemented pipeline"
status = "pending"

[[exit_criteria]]
id = "test-runtime-impact-audit"
title = "Every release test has one justified lane and no known redundant rebuild"
status = "pending"

[[exit_criteria]]
id = "external-review"
title = "Local gates, shadow candidate, and independent review have no unresolved blockers"
status = "pending"
+++

# Release Cycle-Time And Artifact Pipeline

## Outcome

Build each release artifact once, qualify it once, and publish those exact bytes.
Keep the release path small enough to understand from the workflow files.

```text
manual candidate dispatch on main
             |
             v
  four native lanes + one WASM lane
             |
             v
 inventory, validation, attestations
             |
             v
 qualified-release Actions artifact (30 days)
             |
             v
 promotion by candidate run ID
        /                 \
      PyPI       immutable GitHub Release
```

## Decisions

- Candidate dispatch has no parameters. Source and expected tag come from the
  selected `main` revision.
- The hosted matrix remains the release authority for Windows x64, Linux x64,
  Linux ARM64, macOS ARM64, and WASM. The Windows workstation and WSL2 builder
  use the same command for fast local qualification but hold no release
  credentials.
- The successful candidate run's `qualified-release` artifact is the only
  pre-promotion handoff. Promotion takes only its run ID.
- Promotion verifies workflow path, branch, conclusion, source, tag, inventory,
  and every asset digest before external mutation.
- PyPI and GitHub Releases are the product distribution channels. R2 stores
  only immutable OCCT dependency archives selected by
  `dependencies/occt-lock.json`.
- The protected `pypi` and `release-github-production` environments contain the
  only publication authority.
- Clean C, C++, and Rust SDK consumers run for every candidate. A downstream
  application trial is repeated only when the SDK interface, supported profile,
  packaging, or embedded runtime semantics change.

## Qualification and cutover

1. Run focused workflow, inventory, provenance, and promotion tests.
2. Run the Python stratum, L99 release gate, generated-doc checks, and repository
   standards checks.
3. Obtain an independent review of the final workflows and credential boundary.
4. Merge the implementation to `main`.
5. Dispatch one complete shadow candidate and verify every platform lane, the
   aggregate inventory, attestations, artifact retention, and reported identity.
6. Restrict the OCCT R2 writer credentials to its protected environment and
   remove obsolete release-R2 environments and repository-level credentials.

The already-published `2026.9.19` release is not rebuilt or version-bumped for
this infrastructure-only cutover. The next product change will be the first
release promoted through this path.

## Explicit non-goals

- No R2 product candidate store, release mirror, or tag alias.
- No recipe fingerprint discovery or fallback cache keys.
- No self-hosted release publication.
- No alternate macOS cloud or MacBook release builder in this cutover.
- No automatic push, pull-request, tag, or schedule triggers.
