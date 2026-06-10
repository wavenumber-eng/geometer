# Planar Geometry Interfaces

## Planar Contours

Defined in `src/cpp/lib/geometer/planar_contours.h`.

```cpp
struct PlanarContourPoint {
    double x = 0.0;
    double y = 0.0;
};

struct PlanarContourSegment {
    PlanarContourPoint start;
    PlanarContourPoint end;
};

struct PlanarContourRing {
    std::vector<PlanarContourPoint> points;
    bool hole = false;
    int nesting_depth = 0;
    double signed_area = 0.0;
};

struct PlanarContourOptions {
    int round_digits = 3;
    bool union_polygons = true;
    double area_epsilon = 1.0e-9;
};

struct PlanarContourResult {
    std::vector<PlanarContourRing> rings;
    std::vector<PlanarContourSegment> segments;
};

int build_planar_contours(
    const std::vector<PlanarContourSegment>& segments,
    const PlanarContourOptions& options,
    PlanarContourResult* result,
    Status* status = nullptr
);
```

This helper turns planar line segments into closed rings and output contour
segments. The HLR implementation uses it to generate simplified projection
geometry.
## Planar Batch Solve

Defined in `src/cpp/lib/geometer/planar_solve.h`.

The planar batch solve API performs generic 2D filled-geometry work:

1. starts with closed subject rings;
2. offsets open stroke paths into filled regions;
3. unions the subject and stroked fills;
4. subtracts local and optional common rings;
5. optionally clips each job to final clip rings;
6. returns outline/hole regions suitable for meshing or rendering.

It intentionally has no PCB, EDA, visualizer, or file-format policy.

```cpp
struct PlanarSolvePoint {
    double x = 0.0;
    double y = 0.0;
};

using PlanarSolvePath = std::vector<PlanarSolvePoint>;
using PlanarSolveRing = PlanarSolvePath;

struct PlanarSolveRegion {
    PlanarSolveRing outline;
    std::vector<PlanarSolveRing> holes;
};

enum class PlanarSolveJoinType { Miter, Round, Bevel, Square };
enum class PlanarSolveEndType { Round, Square, Butt, Joined };

struct PlanarSolveStrokeGroup {
    double radius_mm = 0.0;
    double miter_limit = 2.0;
    double arc_tolerance_mm = 0.0;
    PlanarSolveJoinType join_type = PlanarSolveJoinType::Miter;
    PlanarSolveEndType end_type = PlanarSolveEndType::Round;
    std::vector<PlanarSolvePath> paths;
};

struct PlanarSolveJob {
    std::vector<PlanarSolveRing> subject_rings;
    std::vector<PlanarSolveRing> subtract_rings;
    std::vector<PlanarSolveStrokeGroup> stroke_groups;
    bool subtract_common_rings = true;
    bool filter_common_subtract_by_bounds = true;
    bool clip_to_final_rings = true;
    double common_subtract_filter_margin_mm = 0.0;
};

struct PlanarBatchSolveOptions {
    int decimal_precision = 6;
    double cleanup_radius_mm = 0.0;
    double cleanup_miter_limit = 2.0;
    double cleanup_arc_tolerance_mm = 0.0;
};

struct PlanarSolveJobResult {
    std::vector<PlanarSolveRegion> regions;
    double area_mm2 = 0.0;
    std::uint32_t source_subject_ring_count = 0;
    std::uint32_t raw_subject_ring_count = 0;
    std::uint32_t stroke_path_count = 0;
    std::uint32_t stroke_region_count = 0;
    std::uint32_t local_subtract_ring_count = 0;
    std::uint32_t common_subtract_ring_count = 0;
};

struct PlanarBatchSolveInput {
    PlanarBatchSolveOptions options;
    std::vector<PlanarSolveRing> common_subtract_rings;
    std::vector<PlanarSolveRing> final_clip_rings;
    std::vector<PlanarSolveJob> jobs;
};

struct PlanarBatchSolveResult {
    std::vector<PlanarSolveJobResult> jobs;
};

int solve_planar_batch(
    const PlanarBatchSolveInput& input,
    PlanarBatchSolveResult* result,
    Status* status = nullptr
);

int solve_planar_batch_from_bytes(
    const unsigned char* request_data,
    std::size_t request_size,
    std::vector<unsigned char>* response_bytes,
    Status* status = nullptr
);

int write_planar_batch_solve_json(
    const PlanarBatchSolveResult& result,
    std::string* json,
    Status* status = nullptr
);

int solve_planar_batch_json_from_bytes(
    const unsigned char* request_data,
    std::size_t request_size,
    std::string* response_json,
    Status* status = nullptr
);
```

Closed rings should be supplied without a duplicate closing point. Output
outlines are oriented positive and holes are oriented negative. Inputs that
contain duplicate closing points are tolerated and cleaned.

The CLI exposes the JSON ring form with
`planar-batch-solve request.bin rings.json --format json` or
`--return-rings true`. Python exposes the same fused regions as
`geometer.planar_batch_solve(...)`, returning jobs with `regions[].outer` and
`regions[].holes` in millimeters.
