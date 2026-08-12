# Analytic Planar Boolean Packet A0

## Status

Proposed for joint review. This separately governed binary projection carries
the logical models in [Analytic Planar Boolean A0 Design](analytic-planar-boolean-a0.md).
It is not generated merely by choosing a TypeSpec emitter. Once accepted, any
incompatible change requires a new packet generation and magic.

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
- Coordinates and lengths are integer nanometers. Compact authored angles are
  signed integer microdegrees.
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
overflow-safe arithmetic. A required kind appears exactly once. Unknown kinds
are rejected in generation 1. Ranges may not overlap the header, directory, or
another table. Tables are packed in directory order with only the minimum
zero-filled alignment between them.

Indices are zero-based `u32` indices into the named table. `(first, count)` is
valid only when `first + count` does not overflow and is within the target
record count. An empty range uses `first == 0` and `count == 0`.

## Request Tables

| Kind | Name | Record bytes |
| ---: | --- | ---: |
| 1 | jobs | 24 |
| 2 | stages | 32 |
| 3 | operands | 32 |
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

### Operand record, 32 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | operand id |
| 8 | `u16` | geometry kind |
| 10 | `u16` | flags, zero in A0 |
| 12 | `u32` | geometry-table index |
| 16 | `u64` | compact feature id, or zero for planar region |
| 24 | `u64` | reserved, zero |

Geometry kinds are `1` planar region, `2` disk, `3` annulus, `4` capsule, and
`5` swept path. The geometry-table index addresses the table implied by the
kind. The referenced geometry record may be owned by only one operand.

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
| 104 | normalized curves | 40 |
| 105 | directed fragments | 48 |
| 106 | result rings | 32 |
| 107 | fragment references | 4 |
| 108 | result regions | 32 |
| 109 | ring/region references | 8 |
| 110 | source sets | 16 |
| 111 | source references | 32 |
| 112 | operand outcome events | 48 |
| 113 | relationship results | 32 |
| 114 | relationship region pairs | 32 |

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
| 5 | `u8` | scope: `1` job, `2` query |
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

### Result-vertex record, 32 bytes

`result vertex id u64`, `x i64`, `y i64`, `intersection source-set index u32`,
and `flags u32`.

### Normalized-curve record, 40 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | generated curve id |
| 8 | `u8` | kind: `1` line, `2` circle |
| 9 | 7 bytes | reserved, zero |
| 16 | `i64` | circle center x, zero for line |
| 24 | `i64` | circle center y, zero for line |
| 32 | `u64` | circle radius, zero for line |

Lines are defined by fragment endpoints rather than duplicate coefficients.

### Directed-fragment record, 48 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u64` | generated fragment id |
| 8 | `u32` | normalized curve index |
| 12 | `u32` | start vertex index |
| 16 | `u32` | end vertex index |
| 20 | `u8` | direction: `0` line, `1` CCW, `2` CW |
| 21 | `u8` | major arc boolean |
| 22 | 2 bytes | reserved, zero |
| 24 | `u32` | coincident positive source-set index |
| 28 | `u32` | surviving subtraction source-set index |
| 32 | `u64` | reserved, zero |
| 40 | `u64` | reserved, zero |

An empty source set has index zero; nonempty source-set indices are one-based so
zero remains the empty sentinel.

### Result-ring record, 32 bytes

`ring id u64`, first fragment-reference `u32`, fragment-reference count `u32`,
parent ring index `u32` or `UINT32_MAX`, depth `u32`, flags `u32` (bit 0 hole),
and reserved `u32`. Fragment-reference records are `u32` fragment indices.

### Result-region record, 32 bytes

`result region id u64`, outer ring index `u32`, first child/hole reference
`u32`, reference count `u32`, positive contributor source-set index `u32`, flags
`u32`, and reserved `u32`. Ring/region references are `u64`: high 32 bits are
kind (`1` ring, `2` child region) and low 32 bits are the table index.

### Source-set and source-reference records

A source-set record is: first source-reference index `u32`, count `u32`, and a
canonical 64-bit content key used only for ordering/collision-checked lookup.

A 32-byte source reference is:

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u16` | source kind |
| 2 | `u16` | canonical feature/boundary role |
| 4 | `u32` | flags, zero in A0 |
| 8 | `u64` | operand id |
| 16 | `u64` | authored segment/curve/feature id |
| 24 | `u64` | secondary authored id or zero |

Kinds distinguish authored curve/segment, compact feature role, and subtractive
operand effect. References sort by their full tuple. Content keys never replace
full equality checks.

### Operand-outcome-event record, 40 bytes

`operand id u64`, event code `u16`, flags `u16`, first result-reference `u32`,
result-reference count `u32`, source-set index `u32`, two reserved `u32`
fields, and two reserved `u64` fields. Result references use the
ring/region-reference table.
Several records may exist for one operand and sort by operand/event/reference
key.

### Relationship-result record, 32 bytes

`query id u64`, status `u8` (`0` success, `1` skipped dependency failed),
aggregate dimension `u8` (`0` disjoint, `1` point, `2` curve, `3` area), flags
`u16`, first pair index `u32`, pair count `u32`, reserved `u32`, and reserved
`u64`.

A 32-byte relationship pair is: left result-region id `u64`, right
result-region id `u64`, dimension `u8`, equality `u8`, left-contains-right
`u8`, right-contains-left `u8`, four reserved zero bytes, and reserved `u64`.

## Diagnostic Codes

The packet uses these proposed `u32` assignments, mapped exactly to generated
string identities:

| Code | Generated identity |
| ---: | --- |
| `0x00010001` | `geometry.analytic_planar_boolean.invalid_id` |
| `0x00010002` | `geometry.analytic_planar_boolean.invalid_reference` |
| `0x00010003` | `geometry.analytic_planar_boolean.invalid_topology` |
| `0x00010004` | `geometry.analytic_planar_boolean.invalid_arc` |
| `0x00010005` | `geometry.analytic_planar_boolean.unsupported_geometry` |
| `0x00010006` | `geometry.analytic_planar_boolean.normalization_ambiguous_tie` |
| `0x00010007` | `geometry.analytic_planar_boolean.normalization_error_exceeded` |
| `0x00010008` | `geometry.analytic_planar_boolean.normalization_topology_collapse` |
| `0x00010009` | `geometry.analytic_planar_boolean.nonanalytic_result` |
| `0x0001000a` | `geometry.analytic_planar_boolean.solver_failed` |
| `0x0001000b` | `geometry.analytic_planar_boolean.resource_limit_exceeded` |

Packet framing/table failures occur before trustworthy job isolation and use
the generic invocation/attachment failure response rather than a partial result
packet.

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
Tables contain no telemetry. A canonical job-result digest is SHA-256 over a
standalone canonical subpacket made from that job's owned result records with
indices rebased to zero; enclosing batch layout and queries cannot affect it.

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

Implementations advertise smaller effective limits when required by available
memory. Every multiplication, addition, alignment, index/range, signed-origin
subtraction, and native-size conversion is checked before allocation. The
decoder validates the full structural graph before invoking OCCT.

## Generated Codec Tests

Before freeze, C++, TypeScript, and Python codecs must share vectors for:

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
