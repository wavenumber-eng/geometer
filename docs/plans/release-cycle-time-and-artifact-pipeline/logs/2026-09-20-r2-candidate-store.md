+++
type = "plan_log"
id = "r2-candidate-store-2026-09-20"
plan_id = "release-cycle-time-and-artifact-pipeline"
step_id = "r2-release-store"
created = "2026-09-20T19:00:00-04:00"
+++

# Log: immutable R2 candidate-store primitive

The existing R2 SigV4 and `If-None-Match: *` implementation is now one generic
module used by OCCT and release-candidate storage. OCCT's 27 focused dependency
tests pass unchanged after extraction, preserving its locked dependency policy.

The release candidate publisher accepts only a complete B0 inventory bound to
the requested source revision. It re-runs artifact, wheel/native pairing, SDK,
and WASM validation, then writes assets beneath the direct
`releases/candidates/<source-sha>/<inventory-sha256>/` identity. The inventory
is written last as the completion record. Retries conditionally create each
object and accept an occupied key only when the downloaded bytes are identical.

The tool is intentionally not wired to R2 credentials yet. Candidate builders
must remain secret-free, and a credentialed job must not execute code from the
candidate checkout. The plan step stays pending until a protected hosted
ingestion workflow downloads the untrusted candidate payload, runs trusted
default-branch validation code, publishes the namespace, and records evidence.
