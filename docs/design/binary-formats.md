# Binary Formats

## Indexed Triangle Mesh A0

`geometry.indexed_triangle_mesh.packet.a0` is the canonical input attachment
for synthesized Fast HLR geometry. Its media type is
`application/vnd.wavenumber.geometer.indexed-triangle-mesh`. Coordinates are
IEEE 754 binary64 millimeters, indices are little-endian unsigned 32-bit
integers, and all reserved or alignment bytes are zero.

The fixed 64-byte little-endian header is:

```text
bytes[8] magic = "GMIMSH01"
u16 version = 1
u16 header_bytes = 64
u32 flags                         // bit 0: source-face table present
u64 packet_bytes                  // exact attachment length
u32 vertex_count                  // 1..2,000,000
u32 triangle_count                // 1..2,000,000
u64 positions_offset              // exactly 64
u64 triangles_offset              // immediately after positions
u64 source_faces_offset           // zero if absent; otherwise next 8-byte boundary
u64 reserved0 = 0
```

Positions contain `vertex_count` tightly packed records of three finite
binary64 values `(x, y, z)`. Triangles contain `triangle_count` tightly packed
records of three `u32` vertex indices. Each triangle must reference three
distinct in-range indices. The optional source-face table contains one `u32`
per triangle; equal values identify triangles tessellated from the same smooth
source face, and `0xffffffff` means unspecified. If every value is unspecified,
the canonical encoder omits the table and clears flag bit 0.

The packet ends on an 8-byte boundary, is at most 268,435,456 bytes, and has no
unreferenced ranges or trailing data. Decoders reject non-finite coordinates,
unknown flags, noncanonical offsets, nonzero padding, malformed indices, empty
meshes, and limit violations before Fast HLR preparation. Surface normals are
derived rather than transported. Materials, colors, application object IDs,
and PCB policy are deliberately outside this geometry contract.

## Planar Batch Byte Format

`solve_planar_batch_from_bytes`, `geometer_planar_batch_solve_bytes`, and the
matching WASM exports use a little-endian binary packet. Version 2 request
packets start with:

```text
bytes[8] magic = "GMPBRQ01"
u32 version = 2
u32 flags = 0
u32 decimal_precision
u32 job_count
f64 cleanup_radius_mm
f64 cleanup_miter_limit
f64 cleanup_arc_tolerance_mm
u32 common_subtract_ring_count
u32 final_clip_ring_count
u32 reserved0
u32 reserved1
```

Then all common subtract rings, then all final clip rings, then each job.

Ring and path encoding:

```text
u32 point_count
repeat point_count:
  f64 x
  f64 y
```

Job encoding:

```text
u32 flags
f64 common_subtract_filter_margin_mm
u32 subject_ring_count
u32 local_subtract_ring_count
u32 stroke_group_count
u32 reserved
subject rings...
local subtract rings...
stroke groups...
```

Job flags:

- `1`: subtract common rings.
- `2`: filter common subtract rings by expanded subject bounds.
- `4`: clip result to final clip rings.

Stroke group encoding:

```text
f64 radius_mm
f64 miter_limit
f64 arc_tolerance_mm
u32 join_type      // 0=miter, 1=round, 2=bevel, 3=square
u32 end_type       // 0=round, 1=square, 2=butt, 3=joined
u32 path_count
u32 reserved
open paths...
```

Response packets start with:

```text
bytes[8] magic = "GMPBRS01"
u32 version = 2
u32 job_count
u32 total_region_count
u32 total_ring_count
u32 total_point_count
u32 reserved
```

Each job then encodes:

```text
u32 region_count
u32 ring_count
u32 point_count
u32 source_subject_ring_count
f64 area_mm2
u32 raw_subject_ring_count
u32 stroke_path_count
u32 stroke_region_count
u32 local_subtract_ring_count
u32 common_subtract_ring_count
u32 reserved
regions...
```

Each region then encodes:

```text
u32 hole_count
u32 reserved
outline ring
hole rings...
```
