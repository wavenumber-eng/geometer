# Binary Formats

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
