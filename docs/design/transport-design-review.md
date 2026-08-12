# Transport Design Review Packet

## Purpose

This packet is the independent-review entry point for ADR-011. It covers the
proposed generic operation C ABI and executable IPC A0 protocol only. No
transport implementation is authorized while the promotion manifest records
the review as `pending`.

The exact files and SHA-256 digests under review are locked in
`docs/contracts/promotion-manifest.toml`. Any normative change to ADR-011 or
either transport specification invalidates an approval and returns the record
to `pending` before implementation continues.

## Review scope

The reviewer should explicitly determine whether:

- the C descriptor has an implementable, asserted wasm32 and supported 64-bit
  layout;
- input borrowing, result ownership, output initialization, error cleanup,
  exception containment, accessor failure, and null behavior are complete;
- unadorned C function/type names correctly use the date-based C ABI generation
  rather than the contract/IPC `a0` identity;
- the operation-specific request DTO and generic outcome are consistent across
  C ABI, WASM, and IPC without duplicating operation identity in the C call;
- C calls have a conservative and implementable serialization policy;
- IPC framing can be bounded before variable allocation and parsed without
  implicit padding or resynchronization assumptions;
- connection states, request-id reuse, correlation, queue limits, atomic
  writes, broken pipes, and fatal versus correlated errors are unambiguous;
- queue-only cancellation and graceful/forced shutdown have one outcome for
  every race; and
- queued requests receive the specified terminal shutdown-rejection response,
  diagnostic, ordering, and request-id transition; the single grace deadline
  covers execution, bounded-queue submission, writes, and flush under a
  synchronized completion-versus-expiry decision; `activeRequestCompleted` is
  derived solely from whether a request was active at the atomic draining
  transition and subsequently flushed its terminal response; and
- the specifications preserve every frozen Viz and existing CLI/C ABI surface.

The review should separately label blocking findings, nonblocking follow-ups,
and accepted deferrals. Approval applies only when no blocking finding remains.

## Approval record

An approval update records all of the following in the promotion manifest:

- `status = "approved"`;
- reviewer identity and review date;
- the reviewed Git commit;
- unchanged digests for this packet, ADR-011, and both specifications; and
- `implementation_allowed = true`.

The same update changes ADR-011 from Proposed to Accepted and completes the
plan's `transport-design-review` step and exit criterion. A review comment or a
passing test alone does not open the implementation gate.
