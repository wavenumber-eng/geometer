+++
type = "plan_log"
id = "phase-a-foundation-2026-09-20"
plan_id = "release-cycle-time-and-artifact-pipeline"
step_id = "candidate-root-contract"
created = "2026-09-20T09:05:57-04:00"
+++

# Log: candidate-root-contract

The reviewed candidate-root contract is implemented as strict canonical JSON
binding source, release version/date/ABI/tag, workflow repository/revision/file,
policy, OCCT lock, and ordered lane recipe/toolchain digests. Creation is
atomic and refuses replacement. Date-version serials remain consistent through
release inventory and static-SDK metadata.

The same low-risk slice adds a fail-closed canonical execution ledger and a
pure non-mutating promotion planner. The current release workflow runs the
orchestration fixtures before build matrices, records major native and WASM
build/test/package commands, uploads machine-readable ledgers even on failure,
and excludes those evidence artifacts from the unchanged release payload.

Independent review found no blocker after remediation. Focused tests, the
Python Rack stratum, L99 release signoff, Pyright, Ruff, and wn-dev-std 2026.9.8
pass. Production promotion still requires channel-specific target identity and
asset projections; the current planner is preflight-only.
