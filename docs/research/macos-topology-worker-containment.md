# macOS topology-worker containment feasibility

This record narrows the hard-memory question in
[issue 25](https://github.com/wavenumber-eng/geometer/issues/25). It does not
change the support boundary: the experimental Python topology-worker supervisor
continues to reject macOS until a tested, unprivileged, OS-enforced primitive
satisfies every acceptance test below.

## Kernel constraints

The relevant macOS mechanisms are not interchangeable:

- XNU defines `RLIMIT_RSS` as a source-compatibility alias for `RLIMIT_AS`, not
  as a separate resident-memory ceiling. See the pinned XNU
  [`resource.h`](https://github.com/apple-oss-distributions/xnu/blob/f6217f891ac0bb64f3d375211650a4c1ff8ca1ea/bsd/sys/resource.h).
- `setrlimit(RLIMIT_AS)` calls `vm_map_set_size_limit` for the caller's current
  map and returns `EINVAL` if the requested value is already below current map
  usage. See
  [`kern_resource.c`](https://github.com/apple-oss-distributions/xnu/blob/f6217f891ac0bb64f3d375211650a4c1ff8ca1ea/bsd/kern/kern_resource.c).
  This explains the release-gate result from a Python/dyld-mapped launcher; it
  is not an incidental CPython error.
- XNU has a physical-footprint ledger limiter, but its public Mach routine
  calls `proc_check_footprint_priv()`. That check requires
  `PRIV_VM_FOOTPRINT_LIMIT`, granted to root or an applicable MAC policy. See
  [`task.c`](https://github.com/apple-oss-distributions/xnu/blob/f6217f891ac0bb64f3d375211650a4c1ff8ca1ea/osfmk/kern/task.c)
  and
  [`kern_priv.c`](https://github.com/apple-oss-distributions/xnu/blob/f6217f891ac0bb64f3d375211650a4c1ff8ca1ea/bsd/kern/kern_priv.c).
  A normal signed/notarized desktop application cannot treat that privileged
  routine as its containment API.
- Jetsam/memorystatus controls are kernel implementation mechanisms, not a
  supported unprivileged macOS application contract. They cannot be the public
  SDK solution.

Consequently, a smaller native executable is only a hypothesis. It still has a
dyld-created address map before `main`; it must be proven able to install the
requested absolute limit on the oldest supported macOS runner. Raising the
limit to “current virtual size plus budget” would change the meaning of the
configured ceiling and may permit allocations far beyond it after `exec`, so it
is not accepted without a separate kernel-accounting proof.

## Feasibility experiment

The spike must run on macOS ARM64 outside Python and report distinct outcomes
for:

1. launcher/bootstrap setup, including the inherited limits, current mapped
   size, requested limit, `setrlimit` result, and `errno`;
2. successful `exec` of the real OCCT-linked containment test worker;
3. a below-ceiling control allocation that exits normally;
4. an above-ceiling allocation that reaches the worker and fails specifically
   with the worker's containment status, not the launcher's setup status;
5. a real topology session plus deliberately hanging descendant, killed as one
   process group on deadline and explicit cancellation; and
6. private temporary-directory cleanup and successful next-generation
   replacement.

Launcher setup and worker containment must have disjoint exit statuses and
diagnostics. The existing test's “any nonzero exit” assertion is insufficient.
The allocation test records the process tree and verifies that no descendant
survives.

## Decision rule

The issue can close only if an ordinary, non-root, hardened-runtime-compatible
process can enforce the configured ceiling before untrusted OCCT work and all
six experiments pass on the signed macOS ARM64 artifact. Advisory polling and
kill-after-observation do not count as a hard ceiling because an allocation can
overshoot between samples or destabilize the host first.

If the minimal launcher still receives `EINVAL`, and no supported unprivileged
primitive is found, the correct result is a documented platform limitation:
keep the experimental supervisor fail-closed on macOS and do not claim issue 25
complete. Core native topology operations, static SDK calls, HLR, illustration,
and the KiCad Cruncher static-link trial remain macOS-supported because they do
not claim this separate hard process-memory guarantee.

