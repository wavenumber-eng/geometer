+++
type = "plan_log"
id = "geometer-documentation-cleanup-execution-checkpoint"
plan_id = "geometer-documentation-cleanup"
step_id = "demo-runtime-audit"
created = "2026-09-05"
+++

# Documentation Implementation Checkpoint

Worktree: `docs/documentation-cleanup-plan`, based on reviewed plan commit
`64782847b32403642b6103586c3974e35d3959f1`. The primary feature checkout was
not changed. No native/OCCT/WASM rebuild or runtime-code migration was performed.

## Delivered

- Refreshed README and tested executable IPC introduction with a Node bounds
  example; corrected implemented transport and topology availability claims.
- Dispositioned the original 43 design Markdown files plus the new IPC guide.
  Moved 13 documents to their proper roles; retained experimental normative
  topology definitions in design. Split the analytic contract from its full
  preserved historical research, with legacy heading links and repaired hashes.
- Reconciled public operation families, aliases, native helpers and client
  exports. Documented TypeSpec/handwritten packet authority and implementation
  migration waves. These are an assessment, not completed runtime migrations.
- Extended the existing generator with authored-guide HTML, operation/packet
  coverage, nested type links, separate maturity and correct effective native
  availability. Adopted ALX teal styling with existing openly licensed Cousine
  assets; builds do not access sibling appz files.
- Added explicit source-review freshness for 141 public-source files,
  documentation link/disposition checks, and registration checks for 11 demo
  entrypoints. Normal generation cannot silently refresh the source-review lock.
- Recorded reproducible demo evidence and remaining limitations in the
  [durable audit](../../developer/demo-status.md). No demos were deleted.

## Focused Validation And Runtime Impact

New `tests/typescript/documentation_reference_validation.mjs` covers runtime,
maturity, handwritten operation gaps, binary packet inventory and authored-guide
presentation. It is registered in the existing TypeScript validation pytest
list and Rack objective. Existing promotion tests were adjusted for moved
authorities and historical evidence, not weakened; their governed digest was
refreshed. `docs/test-strategy.html` distinguishes these fast checks from the
existing browser lanes. No native test or solver build was added.

Observed focused runs:

- Documentation/promotion selection: 9 passed, 28 deselected, 0.62 seconds.
- Four existing Chrome demo tests: 4 passed, 25.62 seconds; no new slow tests.
- Worker-client bounds, native Node IPC quick start, topology replay, Python
  headless JSON/SVG/GLB, and optional PyVista off-screen smoke passed.
- Standalone planar ring solved with committed runtime 2026.9.4 / ABI 20260904.
- Generated-reference freshness, full contract generation freshness, authored
  file links, source review lock, Ruff, Biome and whitespace checks passed.

Generated coverage and IPC guide pages were checked in headless Chrome at
1440 and 390 pixels, including local font loading and absence of horizontal
page overflow. Screenshots exposed a wide-table issue and a white-on-light
guide title; both were fixed and rechecked. The browser skill connection failed
before browser setup (sandboxPolicy metadata missing); these observations use
project-style headless Chrome, not the in-app browser. Scratch outputs are
ignored under `out/docs-cleanup/` and are not release artifacts.

The docs generator adds pinned dev dependency `markdown-it` 15.0.1. npm audit
also reported an existing high-severity transitive `fast-uri` advisory; version
3.1.5 is identical in HEAD and the working lock. No unrelated dependency update
or automatic audit fix was applied.

## Independent Implementation Checkpoint Review

Reviewer: independent AI agent `/root/independent_plan_review`, read-only;
this is not external human approval or the final plan closeout review.

Findings addressed:

1. IPC shutdown rejects queued work; only active work completes within the
   shared 30-second shutdown deadline. Corrected consumer wording.
2. Generated coverage now includes all manifest binary-format records,
   including magic/version/limits, rather than only operation/DTO records.
3. Roadmap distinguishes pre-cleanup presentation differences from adopted
   teal styling.
4. Public-source gate now includes the complete focused CLI boundary, native
   topology dispatch, and handwritten JSON-options parsers.

Reviewer approved the corrected implementation checkpoint and independently
confirmed 141 source files, 281 links and 11 demo registrations. No missing
high-level operation family was found. Final follow-up also approved guide-title
wrapping, the durable assessment rename and this checkpoint record, confirming
the plan catalog, documentation regression, 286 links, 141 source files,
11 demo registrations and whitespace checks. Guide-title styling was visually
rechecked at both widths and included in the focused regression test.

## Remaining Before Closeout

Subsequent update: the user approved [retaining all demos](demo-retention.md).
This resolves the disposition decision below, not the verification gaps.

Native C++ preview still needs an interactive desktop smoke assessment; its
committed executable and packaging registration are not runtime proof. Browser
bounds page controls and Python GUI interactive controls were not separately
tested, although their underlying Worker/off-screen paths passed. Demo keep,
repair or pruning dispositions remain a user decision; all sources and outputs
are retained meanwhile. Final design-intent, test-runtime and independent
closeout audits remain pending this decision/verification.

Keep this active plan until those gates are resolved. Its presence continues
to block the repository's no-plans release-hygiene gate. Do not claim L99 or
release signoff, and do not delete the plan merely to make that check pass.
