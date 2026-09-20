+++
type = "plan_log"
id = "shared-candidate-command-2026-09-20"
plan_id = "release-cycle-time-and-artifact-pipeline"
step_id = "candidate-workflow"
created = "2026-09-20T18:10:00-04:00"
+++

# Log: shared candidate lane command

The release workflow's native and WASM jobs now invoke one shared local command,
`scripts/build_release_candidate.py`. The workflow retains only toolchain setup,
the command invocation, artifact upload, and ledger upload. Build order, test
ownership, packaging, and output names no longer have a second YAML
implementation.

The implementation deliberately has no recipe fingerprint, change detector,
cache-key generator, alias lookup, or partial-output merge. A lane is selected
explicitly as `native --platform <platform>` or `wasm`. It requires a clean Git
checkout, requires a new output directory, executes a fixed ordered task list,
and stops at the first failure while retaining the execution ledger. A retry
uses a fresh worktree or a new explicit output directory.

The former WASM `hashFiles(...)` build-cache fingerprint and broad restore
prefixes were removed. Hosted retries may reuse `build-wasm` only at the exact
Git commit. Emscripten caches use the pinned SDK version without legacy aliases.
These keys are direct immutable identities, not inferred recipes.

This completes the shared-command portion of the candidate workflow. The plan
step remains pending until the four native lanes and WASM are combined into the
canonical candidate-root-bound inventory and stored without publication.
