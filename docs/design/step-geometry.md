# STEP Geometry Interfaces

STEP is currently the only supported source model format for Geometer's generic
model-input APIs. Preferred new entry points use `model_*` names and
`format="step"`. Existing STEP-specific names remain compatibility wrappers.

## Model Bounds

Defined in `src/cpp/lib/geometer/model_bounds.h`.

```cpp
enum class ModelFormat {
    Step
};

struct ModelBoundsOptions {
    ModelFormat format = ModelFormat::Step;
    std::array<double, 16> model_transform = identity;
};

struct ModelBoundsResult {
    std::string schema = "geometry.model_bounds.a0";
    std::string units = "mm";
    std::string source_format = "step";
    std::string source_hash;
    std::array<double, 3> min;
    std::array<double, 3> max;
    std::array<double, 3> size;
    std::array<double, 3> center;
};

int model_bounds_from_bytes(
    const unsigned char* model_data,
    std::size_t model_size,
    const ModelBoundsOptions& options,
    ModelBoundsResult* result,
    Status* status = nullptr
);

int write_model_bounds_json(
    const ModelBoundsResult& result,
    std::string* json,
    Status* status = nullptr
);
```

`model_bounds_from_bytes` imports the source model, applies
`model_transform`, and returns transformed axis-aligned 3D bounds in
millimeters. The first implementation accepts only `ModelFormat::Step`.

## STEP To GLB

Defined in `src/cpp/lib/geometer/step_to_glb.h`.

```cpp
struct StepToGlbOptions {
    double linear_deflection = 0.1;
    double angular_deflection = 0.5;
};

int step_to_glb(
    const std::string& step_path,
    const std::string& glb_path,
    const StepToGlbOptions& options = {}
);

int step_to_glb_from_bytes(
    const unsigned char* step_data,
    std::size_t step_size,
    const StepToGlbOptions& options,
    std::vector<unsigned char>* glb_bytes,
    Status* status = nullptr
);
```

`step_to_glb` is the file-based converter. `step_to_glb_from_bytes` accepts STEP
bytes and fills owned GLB bytes for browser/downstream consumers that cannot
depend on local files.

Preferred Python, CLI, and batch callers should use the generic model names
`model_to_glb`, `model-to-glb`, and `model_to_glb` with `format="step"` where
available. The C++ STEP names remain the current compiled API for this release.
## HLR Projection

Defined in `src/cpp/lib/geometer/projection.h`.

```cpp
enum class ProjectionCurveMode {
    NativeArcs,
    Polyline
};

enum class ProjectionAlgorithm {
    Poly,
    Exact,
    Fast
};

enum class ProjectionOutlineAlgorithm {
    HlrClosedEdges,
    MeshShadow,
    FastMeshShadow
};

struct FastHlrLimits {
    std::size_t max_vertices = 2'000'000;
    std::size_t max_triangles = 2'000'000;
    std::size_t max_edges = 4'000'000;
    std::size_t max_grid_references = 64'000'000;
    std::size_t max_candidate_pairs = 100'000'000;
    std::size_t max_fragments = 8'000'000;
    std::size_t max_output_segments = 4'000'000;
};

struct FastHlrOptions {
    bool include_boundaries = true;
    bool include_creases = true;
    bool include_silhouettes = true;
    bool include_hidden = false;
    double crease_angle_rad = 0.5235987755982988;
    double weld_tolerance = 1.0e-7;
    double projected_tolerance = 1.0e-8;
    double depth_tolerance = 1.0e-7;
    FastHlrLimits limits;
};

struct ProjectionViewSpec {
    std::string id = "top";
    std::array<double, 3> direction = {0.0, 0.0, 1.0};
    std::array<double, 3> up = {0.0, 1.0, 0.0};
};

struct HlrProjectionOptions {
    std::vector<ProjectionViewSpec> views;
    bool output_outline = true;
    bool output_detail = true;
    bool output_bbox = true;
    FastHlrOptions fast;
    std::array<double, 16> model_transform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    };
    bool strip_root_placement = false;
    ProjectionCurveMode curve_mode = ProjectionCurveMode::NativeArcs;
    int samples_per_curve = 24;
    int round_digits = 3;
    bool edge_v_sharp = true;
    bool edge_v_outline = true;
    bool edge_v_smooth = false;
    bool edge_v_sewn = false;
    bool edge_v_iso = false;
    bool edge_h_sharp = false;
    bool edge_h_outline = false;
    bool edge_h_smooth = false;
    bool edge_h_sewn = false;
    bool edge_h_iso = false;
    bool union_outline_polygons = true;
    ProjectionAlgorithm projection_algorithm = ProjectionAlgorithm::Poly;
    double mesh_linear_deflection = 0.01;
    double mesh_angular_deflection = 0.5;
    bool mesh_relative = false;
    MeshDeflectionMode mesh_deflection_mode = MeshDeflectionMode::BboxRelative;
    double mesh_deflection_coefficient = 0.004;
    ProjectionOutlineAlgorithm outline_algorithm = ProjectionOutlineAlgorithm::HlrClosedEdges;
    double hlr_angle_tolerance = 0.0174533;
};
```

The three output modes are independently selectable. Their result members stay
present when disabled but contain no primitives. In particular, an
outline-only mesh-shadow request does not run detail HLR, and a detail-only
request does not construct an outline. A combined request never merges outline
and detail geometry, allowing downstream renderers to composite them with
different presentation choices.

`Fast` is an additive evaluation backend. It tessellates through OCCT, builds a
central C++ triangle/edge incidence graph, activates boundary, crease, and
view-dependent silhouette candidates, and classifies visibility against a
projected-triangle spatial index. Its provisional controls are isolated in the
`fast` member rather than reinterpreting exact/poly edge flags. It currently
emits straight segments and joins only exact-collinear, unbranched fragments
that share topology, visibility, edge category, and source-face provenance.
Occlusion-created endpoints are never eligible joins. The existing
line-and-circular-arc result schema is retained while tolerance-based
simplification and bounded arc fitting are evaluated. `max_fragments` bounds
pre-reconstruction work independently from the final `max_output_segments`
limit.

Fast-option behavior is intentionally explicit during evaluation:

| Option group | `fast` behavior |
|---|---|
| `views`, `model_transform`, `strip_root_placement`, `round_digits` | Supported common behavior |
| `output_outline`, `output_detail`, `output_bbox` | Supported as independent composable layers |
| mesh deflection controls | Supported; they define the prepared triangle mesh |
| nested `fast` controls and limits | Supported by the fast detail and fast mesh-shadow engines |
| `outline_algorithm=mesh-shadow` | Delegated to the independent Clipper2 triangle-union outline |
| `outline_algorithm=fast-mesh-shadow` | Reconstructs projected CAD-face loops when possible, falls back to per-face triangle unions, then unions the reduced contours |
| `outline_algorithm=hlr-close` | Delegated to the existing poly HLR-close outline path |
| exact/poly `edge_*` flags | Not interpreted by fast detail; nested `fast` candidate flags apply instead |
| `curve_mode`, `samples_per_curve`, native-arc behavior | Fast detail emits segments only and does not fit arcs |
| `union_outline_polygons`, `hlr_angle_tolerance` | Apply only in the delegated outline path where applicable |

The retained WebGL depth-pass experiment remains a deliberately independent
comparator. It derives candidates from the display GLB and is not yet
semantically centralized with the C++ prepared mesh. A versioned prepared-data
transport is required before GPU/vector parity can be claimed.

Projection output:

```cpp
struct ProjectedSegment {
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
};

struct ProjectedArc {
    std::array<double, 2> start = {0.0, 0.0};
    std::array<double, 2> end = {0.0, 0.0};
    std::array<double, 2> center = {0.0, 0.0};
    double radius = 0.0;
    double extent_rad = 0.0;
    bool ccw = true;
    bool full_circle = false;
};

struct ProjectedModeGeometry {
    std::vector<ProjectedSegment> segments;
    std::vector<ProjectedArc> arcs;
};

struct ProjectedViewGeometry {
    ProjectionViewSpec view;
    ProjectedModeGeometry outline;
    ProjectedModeGeometry detail;
    ProjectedModeGeometry bbox;
};

struct HlrProjectionTimings {
    double step_read_ms = 0.0;
    double mesh_ms = 0.0;
    double hlr_ms = 0.0;
    double extract_ms = 0.0;
};

struct HlrProjectionResult {
    std::string schema = "geometry.projection.b0";
    std::string units = "mm";
    std::string source_hash;
    std::vector<ProjectedViewGeometry> views;
    HlrProjectionTimings timings;
};
```

Functions:

```cpp
int step_hlr_projection_from_bytes(
    const unsigned char* step_data,
    std::size_t step_size,
    const HlrProjectionOptions& options,
    HlrProjectionResult* result,
    Status* status = nullptr
);

int write_hlr_projection_json(
    const HlrProjectionResult& result,
    std::string* json,
    Status* status = nullptr
);

int write_hlr_projection_svg(
    const HlrProjectionResult& result,
    const std::string& view_id,
    const std::string& mode,
    std::string* svg,
    Status* status = nullptr
);
```

`step_hlr_projection_from_bytes` parses STEP bytes, runs projection for each
requested view, and fills `detail`, `outline`, and `bbox` geometry. `detail`
is the configured HLR edge-category output. `outline` is the assembly
projection silhouette, using either tessellated mesh shadow or closed HLR edge
contours. `bbox` is the projected 3D shape bounding box. `write_hlr_projection_json`
emits `geometry.projection.b0` JSON. `write_hlr_projection_svg` emits a quick
inspection SVG for one view and mode.

Preferred Python, CLI, and batch callers should use the generic model names
`project_model_hlr`, `model-project-hlr`, `model_hlr_projection_json`, and
`model_hlr_projection_svg` with `format="step"` where available. The raw C++
projection function remains STEP-specific in this release.

`model_transform` is an optional row-major 4x4 affine transform applied to the
loaded source shape before projection. Translation lives in the final column and
the final row must be `[0, 0, 0, 1]`. The transform is generic source-model
normalization; it does not imply PCB side, screen mirroring, or SVG/canvas
Y-down policy.

`strip_root_placement` optionally matches STEP-to-GLB's definition-local frame
by removing only placements on free-shape roots before projection. It preserves
assembly-child placements and defaults to `false` for backward compatibility.
