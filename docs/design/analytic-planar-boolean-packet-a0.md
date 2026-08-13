# Analytic Planar Boolean Packet A0

## Status

Proposed for joint review. This separately governed binary projection carries
the logical models in [Analytic Planar Boolean A0 Design](analytic-planar-boolean-a0.md).
It is not generated merely by choosing a TypeSpec emitter. Once accepted, any
incompatible change requires a new packet generation and magic.

Every numeric enum, flag, role, status, event, diagnostic, and path token is
assigned in the machine-readable governed catalog
`docs/contracts/analytic-planar-boolean-a0-catalog.toml`. This document names
the fields and the catalog supplies their exact wire values; drift tests require
the two to remain aligned.

The packet travels through named attachments on the generic operation C ABI and
executable IPC. No operation-specific C symbol is added.

## Attachments And Envelope

The operation request JSON contains only generated attachment metadata:

```json
{
  "schema": "geometry.analytic_planar_boolean_batch.request.a0",
  "packet": {
    "attachment": "analytic_planar_boolean_request",
    "format": "geometry.analytic_planar_boolean.packet.a0"
  }
}
```

The required request attachment is:

- name: `analytic_planar_boolean_request`
- media type:
  `application/vnd.wavenumber.geometer.analytic-planar-boolean-request`
- packet magic: `GMABRQ01`

The successful response envelope references:

- name: `analytic_planar_boolean_result`
- media type:
  `application/vnd.wavenumber.geometer.analytic-planar-boolean-result`
- packet magic: `GMABRS01`

Small noncanonical call telemetry may appear in the response JSON. It is not in
the canonical result attachment or its digest.

## Scalar Rules

- All integers are little-endian.
- `u8`, `u16`, `u32`, `u64`, and `i64` have their normal exact widths.
- Booleans are `u8` with only `0` and `1` accepted.
- There are no pointers, platform-sized integers, floating-point values,
  strings, or NUL-terminated fields in A0 packets.
- Coordinates and lengths are integer nanometers. A0 packets contain no angle
  or trigonometric fields.
- Every packet offset is relative to byte zero of that packet.
- Every table offset and byte length is a multiple of eight. Padding bytes and
  reserved fields must be zero.
- Decoders read fields explicitly; they do not cast packet memory to host
  structs.

## Fixed Header

Every request and result begins with this 64-byte header:

| Offset | Size | Type | Field |
| ---: | ---: | --- | --- |
| 0 | 8 | bytes | request or result magic |
| 8 | 2 | `u16` | packet generation, exactly `1` |
| 10 | 2 | `u16` | header bytes, exactly `64` |
| 12 | 4 | `u32` | flags, zero in A0 |
| 16 | 8 | `u64` | total packet bytes |
| 24 | 8 | `u64` | directory offset, exactly `64` |
| 32 | 4 | `u32` | directory entry count |
| 36 | 4 | `u32` | job or job-result count |
| 40 | 4 | `u32` | relationship query or result count |
| 44 | 4 | `u32` | reserved, zero |
| 48 | 8 | `u64` | canonical payload bytes after directory/padding |
| 56 | 8 | `u64` | reserved, zero |

`total packet bytes` must equal the attachment length. `canonical payload
bytes` is the sum of declared table byte lengths, excluding the header,
directory, and zero padding. It supports accounting but is not trusted until
every range is validated.

## Table Directory

The directory immediately follows the header. Entries are 32 bytes and sorted
by ascending table kind:

| Offset | Size | Type | Field |
| ---: | ---: | --- | --- |
| 0 | 2 | `u16` | table kind |
| 2 | 2 | `u16` | record version, exactly `1` |
| 4 | 4 | `u32` | fixed record bytes |
| 8 | 8 | `u64` | table offset |
| 16 | 8 | `u64` | table byte length |
| 24 | 8 | `u64` | record count |

For every fixed table, `record count * record bytes == table byte length` with
overflow-safe arithmetic. A request directory contains exactly kinds 1 through
13 and a result directory contains exactly kinds 101 through 114, each exactly
once, even when its record count is zero. Consequently the A0 request directory
has exactly 13 entries and the result directory has exactly 14. Unknown,
missing, or duplicate kinds are rejected.

Zero-count tables have byte length zero and use the current minimum-aligned
payload cursor as their offset; they do not advance it. Nonempty ranges may not
overlap the header, directory, or another nonempty table. Tables are packed in
directory order with only the minimum zero-filled alignment between them. This
rule gives an empty/minimal logical model one unique directory and offset
encoding.

Indices are zero-based `u32` indices into the named table. `(first, count)` is
valid only when `first + count` does not overflow and is within the target
record count. An empty range uses `first == 0` and `count == 0`.

## Request Tables

| Kind | Name | Record bytes |
| ---: | --- | ---: |
| 1 | jobs | 24 |
| 2 | stages | 32 |
| 3 | operands | 24 |
| 4 | planar regions | 32 |
| 5 | ring references | 4 |
| 6 | rings | 32 |
| 7 | authored vertices | 24 |
| 8 | authored segments | 40 |
| 9 | disks | 32 |
| 10 | annuli | 40 |
| 11 | capsules | 48 |
| 12 | swept paths | 32 |
| 13 | relationship queries | 24 |

### Job record, 24 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | job id |
| 8 | `u32` | first stage index |
| 12 | `u32` | stage count |
| 16 | `u64` | reserved, zero |

Jobs sort by job id in the packet. Stage ranges retain authored stage order.
The job stage ranges are pairwise disjoint and, in job-table order, form a
gapless complete partition of the stage table. Every stage has exactly one job
owner; no unowned stage record is permitted.

### Stage record, 32 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | stage id |
| 8 | `u8` | operation: `1` union, `2` difference |
| 9 | 7 bytes | reserved, zero |
| 16 | `u32` | first operand index |
| 20 | `u32` | operand count |
| 24 | `u64` | reserved, zero |

Operands within a stage sort by operand id.
A stage range may be empty; this encodes the governed zero-operand no-op
semantics. A job may contain zero stages and then succeeds with an empty result.
The stage operand ranges are pairwise disjoint and, in stage-table order, form
a gapless complete partition of the operand table. Every operand has exactly
one stage owner; no unowned operand record is permitted.

### Operand record, 24 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | operand id |
| 8 | `u16` | geometry kind |
| 10 | `u16` | flags, zero in A0 |
| 12 | `u32` | geometry-table index |
| 16 | `u64` | reserved, zero |

Geometry kinds are `1` planar region, `2` disk, `3` annulus, `4` capsule, and
`5` swept path. The geometry-table index addresses the table implied by the
kind. The referenced geometry record may be owned by only one operand and is
the sole authority for its region/feature id.

### Planar-region record, 32 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | authored region id |
| 8 | `u32` | outer ring index |
| 12 | `u32` | first hole-ring-reference index |
| 16 | `u32` | hole-ring-reference count |
| 20 | `u32` | reserved, zero |
| 24 | `u64` | reserved, zero |

Each ring-reference record is one `u32` ring-table index. Hole references are
unique and may not name the outer ring.
Planar-region hole-reference ranges are pairwise disjoint and form a gapless
complete partition of the ring-reference table. Across all planar-region outer
indices, hole references, and swept-path indices, every ring-table record has
exactly one owner and is referenced exactly once. A ring cannot be both a
region ring and a swept path.

### Ring record, 32 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | ring or path id |
| 8 | `u32` | first vertex index |
| 12 | `u32` | vertex count |
| 16 | `u32` | first segment index |
| 20 | `u32` | segment count |
| 24 | `u32` | flags: bit 0 means open path |
| 28 | `u32` | reserved, zero |

Closed rings require equal vertex/segment counts. Open paths require exactly
one more vertex than segment. Ranges are owned and non-overlapping.
Taken in ring-table order, the vertex ranges form a gapless complete partition
of the authored-vertex table and the segment ranges form a gapless complete
partition of the authored-segment table. Every authored vertex and segment has
exactly one ring/path owner.

### Authored-vertex record, 24 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | vertex id |
| 8 | `i64` | x nanometers |
| 16 | `i64` | y nanometers |

### Authored-segment record, 40 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | segment id |
| 8 | `u64` | curve id |
| 16 | `u8` | kind: `1` line, `2` circular arc |
| 17 | `u8` | direction: `0` line, `1` CCW, `2` CW |
| 18 | `u8` | major arc boolean |
| 19 | 5 bytes | reserved, zero |
| 24 | `i64` | circle center x, zero for line |
| 32 | `i64` | circle center y, zero for line |

The owning ring/path supplies endpoints by topology. Full-circle arc segments,
zero-length segments, and incoherent circle branches are invalid.

### Disk record, 32 bytes

`feature id u64`, `center x i64`, `center y i64`, `radius u64`.

### Annulus record, 40 bytes

`feature id u64`, `center x i64`, `center y i64`, `inner radius u64`, `outer
radius u64`.

### Capsule record, 48 bytes

`feature id u64`, start `x/y i64`, end `x/y i64`, and `width u64`.

### Swept-path record, 32 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | feature id |
| 8 | `u32` | path record index in the ring table |
| 12 | `u32` | reserved, zero |
| 16 | `u64` | width nanometers |
| 24 | `u64` | A0 cap/join policy, zero means round/round |

The ring-table record must have the open-path flag. Width is positive.

### Relationship-query record, 24 bytes

`query id u64`, `left job id u64`, `right job id u64`. Query records sort by
query id.

## Result Tables

| Kind | Name | Record bytes |
| ---: | --- | ---: |
| 101 | job results | 48 |
| 102 | diagnostics | 56 |
| 103 | result vertices | 32 |
| 104 | directed fragments | 48 |
| 105 | result rings | 32 |
| 106 | fragment references | 4 |
| 107 | result regions | 24 |
| 108 | ring/region references | 8 |
| 109 | source sets | 8 |
| 110 | source references | 32 |
| 111 | operand outcome events | 48 |
| 112 | relationship results | 32 |
| 113 | relationship region pairs | 32 |
| 114 | source-reference indices | 4 |

All generated result ids are deterministic nonzero one-based `u64` ordinals in
their own result spaces after canonical sorting.

### Job-result record, 48 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | caller job id |
| 8 | `u8` | status: `0` success, `1` failure |
| 9 | 7 bytes | reserved, zero |
| 16 | `u32` | first diagnostic index |
| 20 | `u32` | diagnostic count |
| 24 | `u32` | first result-region index |
| 28 | `u32` | result-region count |
| 32 | `u32` | first operand-event index |
| 36 | `u32` | operand-event count |
| 40 | `u64` | reserved, zero |

Failed jobs own no geometry/result-region records. Successful empty jobs own a
zero-length result-region range.

### Diagnostic record, 56 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u32` | governed diagnostic code |
| 4 | `u8` | severity: `1` error, `2` warning |
| 5 | `u8` | scope: exactly `1` job |
| 6 | `u16` | trusted-id presence flags |
| 8 | `u64` | job id or zero |
| 16 | `u64` | stage id or zero |
| 24 | `u64` | operand id or zero |
| 32 | `u64` | geometry/source id or zero |
| 40 | `u32` | generated path token |
| 44 | `u32` | detail token, zero in A0 |
| 48 | `u64` | reserved, zero |

Path tokens are generated catalog integers mapped to documented logical paths;
A0 carries no arbitrary strings in the hot packet.
The exhaustive mapping is the numeric catalog's
`path_token_logical_pattern` table. Packed decoders expand the selected pattern
with the record's trusted request indexes into the optional RFC 6901 logical
`path`; packed encoders accept only a unique canonical inverse mapping. Token
zero maps to an absent path.

### Result-vertex record, 32 bytes

`result vertex id u64`, `x i64`, `y i64`, `intersection source-set handle u32`,
and `flags u32`.

### Directed-fragment record, 48 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | generated fragment id |
| 8 | `u32` | start vertex index |
| 12 | `u32` | end vertex index |
| 16 | `u8` | kind: line or circular arc |
| 17 | `u8` | direction: line, CCW, or CW |
| 18 | `u8` | major arc boolean |
| 19 | 5 bytes | reserved, zero |
| 24 | `u64` | radius nanometers, zero for line |
| 32 | `u32` | coincident positive source-set handle |
| 36 | `u32` | surviving subtraction source-set handle |
| 40 | `u64` | reserved, zero |

Every fragment is an independently replayable analytic record. Its endpoints
are the exact shared result vertices. A line is the exact segment between them.
A circular arc is the endpoint/radius/direction/major-branch solution; its
center is derived and is not serialized. Full circles use the governed two-half
arc decomposition. Source curve identity is carried through source references,
not a competing normalized result-curve table.

Every source-set field is a handle rather than a raw zero-based table index:
zero denotes the empty set and nonzero value `n` addresses source-set table
record `n - 1`. The table contains nonempty sets only. This convention applies
uniformly to vertices, fragments, regions, and operand events.

### Result-ring record, 32 bytes

`ring id u64`, first fragment-reference `u32`, fragment-reference count `u32`,
parent ring index `u32` or `UINT32_MAX`, depth `u32`, flags `u32` (bit 0 hole),
and reserved `u32`. Fragment-reference records are `u32` fragment indices.

The ring hierarchy is the sole containment authority. A root has parent
`UINT32_MAX` and depth zero. Every other ring names exactly one parent whose
depth is one less. Parent links are acyclic, and the parent must be the
geometrically smallest ring that strictly contains the child without crossing
it. The hole flag is set exactly for odd depth. Even-depth rings are CCW and
odd-depth rings are CW.

### Result-region record, 24 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | generated result-region id |
| 8 | `u32` | outer ring index |
| 12 | `u32` | positive contributor source-set handle |
| 16 | `u32` | flags, zero in A0 |
| 20 | `u32` | reserved, zero |

Every even-depth ring is named as the outer ring of exactly one result region;
no odd-depth ring is named by a result region. Thus an island nested inside a
hole is a separate connected result region while retaining its ring parent.
The former child/hole range is deliberately absent, so no second hierarchy can
disagree with the ring parent/depth authority.

Ring/region-reference records remain only for operand outcome references. They
are `u64`: high 32 bits are kind (`1` ring, `2` result region) and low 32 bits
are the table index.

### Source-set, source-reference, and source-reference-index records

A source-set record is: first source-reference-index-table index `u32` and
count `u32`. Each source-reference-index record is one `u32` zero-based index
into the source-reference table. The index range for a set lists its members in
strictly increasing source-reference tuple order. The source-reference-index
ranges owned by source sets are pairwise disjoint and, in source-set order,
form a gapless complete partition of the source-reference-index table. Empty
sets have no source-set record and use handle zero as defined above.

The source-reference table contains each referenced canonical source-reference
tuple exactly once and sorts globally by that tuple. Sets sort
lexicographically by their complete member tuple sequence and are interned only
after full equality comparison. The indirection permits overlapping sets such
as `{A, B}` and `{A, C}` without duplicating `A` or imposing incompatible
contiguous ranges. An implementation may use a private hash, but no hash is
serialized or participates in canonical ordering.

A 32-byte source reference is:

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u16` | source kind |
| 2 | `u16` | canonical feature/boundary role |
| 4 | `u32` | flags, zero in A0 |
| 8 | `u64` | operand id |
| 16 | `u64` | primary id governed by source kind |
| 24 | `u64` | secondary id governed by source kind |

The mapping is exhaustive:

| Source kind | Primary id | Secondary id | Role |
| --- | --- | --- | --- |
| authored segment/curve | authored segment id | authored curve id | `authored_line` or `authored_circular_arc` |
| compact feature role | compact feature id | boundary-occurrence key below | one compatible compact role |
| subtractive operand effect | stage id | zero | `none` |

For disks, annuli, and capsules, the compact role uniquely identifies the
boundary and the occurrence key is zero. Swept offset occurrences use the
one-based centerline segment ordinal in the high 32 bits and zero in the low
32 bits. A round join uses the incoming one-based segment ordinal in the high
half and outgoing ordinal in the low half. Start and end caps use respectively
vertex ordinal 1 and the final centerline vertex ordinal in the high half, with
zero low halves. Swept roles distinguish left/right line offsets, left/right
circular-arc offsets, round joins, and start/end caps. Splitting one occurrence
at intersections repeats the same source reference on every surviving
fragment. Any source-kind/id/role combination outside the catalog mapping is a
contract error. References sort by their full tuple.

### Operand-outcome-event record, 48 bytes

`operand id u64`, event code `u16`, flags `u16`, first result-reference `u32`,
result-reference count `u32`, source-set handle `u32`, two reserved `u32`
fields, and two reserved `u64` fields. Result references use the
ring/region-reference table.
Several records may exist for one operand and sort by operand/event/reference
key.

### Relationship-result record, 32 bytes

`query id u64`, status `u8` (`0` success, `1` skipped dependency failed),
aggregate dimension `u8` (`0` disjoint, `1` point, `2` curve, `3` area), flags
`u16`, first pair index `u32`, pair count `u32`, reserved `u32`, and reserved
`u64`.

For `skipped_dependency_failed`, aggregate dimension is exactly `disjoint`
(`0`), flags are zero, and the pair range is exactly `first pair index = 0`,
`pair count = 0`. All reserved fields remain zero. A successful disjoint query
uses those same aggregate/range values and is distinguished only by status.

A 32-byte relationship pair is: left result-region id `u64`, right
result-region id `u64`, dimension `u8`, equality `u8`, left-contains-right
`u8`, right-contains-left `u8`, four reserved zero bytes, and reserved `u64`.

## Diagnostic Codes

The packet uses these `u32` assignments, mapped exactly to generated string
identities and duplicated in the governed numeric catalog:

| Code | Generated identity |
| ---: | --- |
| `0x00010003` | `geometer.operation.analytic_planar_boolean.invalid_topology` |
| `0x00010004` | `geometer.operation.analytic_planar_boolean.invalid_arc` |
| `0x00010005` | `geometer.operation.analytic_planar_boolean.unsupported_geometry` |
| `0x00010007` | `geometer.operation.analytic_planar_boolean.normalization_error_exceeded` |
| `0x00010008` | `geometer.operation.analytic_planar_boolean.normalization_topology_collapse` |
| `0x00010009` | `geometer.operation.analytic_planar_boolean.nonanalytic_result` |
| `0x0001000a` | `geometer.operation.analytic_planar_boolean.solver_failed` |
| `0x0001000b` | `geometer.operation.analytic_planar_boolean.resource_limit_exceeded` |

Code `0x00010006`, formerly proposed for an ambiguous normalization tie, is
reserved in A0 and must not be emitted or accepted as a known diagnostic. Exact
comparison now resolves side/tie or produces `resource_limit_exceeded`.

Contract diagnostics for packet/id/reference defects use the governed
`geometer.contract.analytic_planar_boolean_packet.*` identities in the response
JSON and reject the batch without a result attachment. They are not entries in
the result-packet operation diagnostic table. Raw foreign-memory/framing
failures retain the generic transport behavior.

## Canonical Encoding

Canonical request packets:

- sort jobs and queries by id;
- preserve stage order within a job;
- sort same-stage operands by id;
- emit owned topology ranges once in their owner order;
- emit directory/table kinds in ascending order;
- use minimum alignment and zero padding; and
- reject unused records and unreferenced ids.

Canonical result packets follow the ordering rules in the logical design.
The complete directed-fragment key includes direction between kind and
major-arc branch; every other canonical key likewise contains every semantic
field before generated ids are assigned.

The total result-table keys are:

- job results: caller job id;
- diagnostics: `(job id, severity, code, presence flags, stage id, operand id,
  geometry/source id, path token, detail token)`;
- vertices: the complete logical vertex key from the design;
- fragments: `(start vertex key, end vertex key, kind, direction, majorArc,
  radius, complete positive source-set tuple, complete subtraction source-set
  tuple)`;
- rings: `(depth, canonical rotated directed-fragment-key sequence, parent
  ring key)` after winding normalization;
- result regions: `(outer ring key, complete positive-contributor source-set
  tuple)`;
- source references: their complete `(kind, role, flags, operand id, primary
  id, secondary id)` tuple;
- source sets: their complete sorted source-reference tuple sequence;
- source-reference indices: the referenced source-reference tuple sequence in
  their owning source-set order;
- operand events: `(operand id, event code, complete sorted result-reference
  tuple, complete source-set tuple)`;
- relationship pairs: `(left result-region id, right result-region id,
  dimension, equality, leftContainsRight, rightContainsLeft)`; and
- relationship results: query id, with each owned pair range already sorted by
  the preceding key.

Fragment-reference and ring/region-reference tables retain the canonical owner
sequence. Generated result ids are assigned only after these semantic sort
keys have been sorted. If two records have identical complete semantic keys,
they must be interned where the model permits one record or retained in their
governed owner range where multiplicity is semantic; allocator/traversal order
is never a tiebreaker.
Tables contain no telemetry. A canonical job-result digest is SHA-256 over a
standalone result packet with zero relationship results and exactly one
job-result record. Its closure and rebasing are normative:

1. begin with the job's diagnostic, result-region, and operand-event ranges;
2. follow regions to their outer rings and contributor source sets, then
   repeatedly include every ring whose parent is already included;
3. follow all included rings to fragment references and fragments;
4. follow fragments to endpoint vertices and both source sets;
5. follow vertices to intersection source sets;
6. follow source sets through source-reference indices to source references;
7. follow operand events to every ring/region reference and source set; and
8. reject any record reached from two job closures unless it is immutable
   source content deliberately duplicated into each standalone subpacket.

For each table, selected records sort by the normal complete canonical key,
receive dense local indices from zero, and every reference is rewritten to the
new local index. Generated nonzero ids are recomputed as one-based local
ordinals. Directory entries and offsets are regenerated with minimum alignment.
No original batch index, offset, unused record, query, padding choice, or
telemetry survives. Enclosing batch layout and queries therefore cannot affect
the digest.

The digest is a deterministic derived field of each logical successful or
failed job result. It is not present in the 48-byte job-result record or any
other result table. Decoders compute it from the standalone bytes above;
encoders verify a supplied lowercase hexadecimal digest against those bytes.

## Limits

The generic transport already limits an individual attachment to 256 MiB and
aggregate WASM input/output attachments to 256 MiB. This packet never advertises
larger limits.

Additional A0 maxima are:

| Item | Maximum |
| --- | ---: |
| Jobs | 65,535 |
| Stages per batch | 1,048,576 |
| Operands per batch | 4,194,304, additionally packet-size limited |
| Relationship queries | 1,048,576 |
| Records in any one table | `UINT32_MAX` and packet-size limited |
| Job-local coordinate span per axis | `1,000,000,000,000 nm` |
| Job-local positive radius or width | `1,000,000,000,000 nm` |
| Analytic boundary occurrences per job | 131,072 |
| Curve pairs examined after conservative pruning per job | 8,388,608 |
| Exact intersections per job | 1,048,576 |
| Arrangement vertices per job | 1,048,576 |
| Arrangement half-edges per job | 2,097,152 |
| Arrangement faces per job | 1,048,576 |
| Live real-algebraic scalars per job | 4,194,304 |
| Defining-polynomial degree | 64 |
| Bits in any algebraic polynomial coefficient | 16,384 |
| Algebraic integer/coefficient storage per job | 256 MiB |
| Provenance source references per job | 8,388,608 |
| Source-reference-index memberships per job | 8,388,608 |
| Exact predicate calls per job | 100,000,000 |
| Total interval-refinement steps per job | 100,000,000 |
| Solver working memory per job | 1 GiB |

Implementations advertise smaller effective limits when required by available
memory. Every multiplication, addition, alignment, index/range, signed-origin
subtraction, and native-size conversion is checked before allocation. The
decoder validates the full structural graph before invoking OCCT.
The authoritative arrangement charges each counter before performing the work
or allocation that would exceed it. Hitting any solver counter is the stable
job-local `resource_limit_exceeded` outcome. Native and WASM use the same hard
counts; an advertised effective limit may only be smaller, and clients target
the minimum negotiated capability when cross-runtime parity is required.

## Generated Codec Tests

Before freeze, C++, TypeScript, Rust, and Python codecs must share vectors for:

- empty/minimal packets and every record kind;
- exact uint64 maximum handling and JavaScript `bigint` enforcement;
- duplicate ids and invalid cross-table references;
- truncated headers/directories/tables, overlaps, misalignment, nonzero
  padding/reserved bits, count multiplication overflow, and oversized packets;
- logical encode/decode equivalence for every portable MATZ case;
- canonical request byte equality across languages;
- canonical native/WASM result byte equality;
- standalone versus mixed-batch canonical job-result digests; and
- unknown magic/generation/kind/code behavior.

Raw-byte goldens are created only after this layout and the logical TypeSpec
model receive joint and independent implementation review.
