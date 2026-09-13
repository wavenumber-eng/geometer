+++
type = "plan"
id = "geometer-model-illustration-a0"
status = "active"
created = "2026-09-13"

[[steps]]
id = "adr"
title = "Amend ADR 016 or adopt a successor ADR for combined model illustration"
status = "pending"

[[steps]]
id = "contract"
title = "Author and generate the model-illustration and analytic-scene TypeSpec contracts"
status = "pending"
depends_on = ["adr"]

[[steps]]
id = "native-model"
title = "Implement the one-pass native model illustration pipeline"
status = "pending"
depends_on = ["contract"]

[[steps]]
id = "analytic"
title = "Implement bounded analytic extrusion, cylinder, sphere, definition and occurrence support"
status = "pending"
depends_on = ["contract"]

[[steps]]
id = "clients"
title = "Expose generated and ergonomic C++, Python, Rust and TypeScript clients"
status = "pending"
depends_on = ["native-model", "analytic"]

[[steps]]
id = "demos-docs"
title = "Update the native, browser and custom-renderer demos and algorithm documentation"
status = "pending"
depends_on = ["clients"]

[[steps]]
id = "acr-integration"
title = "Integrate the candidate Geometer build into ACR Toon and qualify representative boards"
status = "pending"
depends_on = ["clients"]

[[steps]]
id = "stress-performance"
title = "Qualify limits, determinism, memory and performance on difficult model and board fixtures"
status = "pending"
depends_on = ["demos-docs", "acr-integration"]

[[steps]]
id = "promotion"
title = "Promote all four roots and both operations to TypeSpec authority under ADR 010"
status = "pending"
depends_on = ["stress-performance"]

[[steps]]
id = "design-doc-intent-audit"
title = "Audit contracts, design docs, ADRs, requirements and runtime catalogs against implementation"
status = "pending"
depends_on = ["promotion"]

[[steps]]
id = "test-runtime-impact-audit"
title = "Audit new test runtime and retain only behavior-owning coverage"
status = "pending"
depends_on = ["promotion"]

[[steps]]
id = "external-review"
title = "Obtain independent implementation and release review"
status = "pending"
depends_on = ["design-doc-intent-audit", "test-runtime-impact-audit"]

[[steps]]
id = "release"
title = "Publish Geometer, update downstream pins, requalify ACR and publish ACR"
status = "pending"
depends_on = ["external-review"]

[[steps]]
id = "cleanup"
title = "Remove completed plan and task-specific worktrees, outputs and redundant build caches"
status = "pending"
depends_on = ["release"]

[[exit_criteria]]
id = "contracts"
title = "All new roots and operations are promoted to TypeSpec authority and every generated projection is current"
status = "pending"

[[exit_criteria]]
id = "one-pass"
title = "Eligible single-STEP and analytic illustration complete without an IPC mesh round trip"
status = "pending"

[[exit_criteria]]
id = "raw-geometry"
title = "Consumers can request renderer-neutral illustration geometry without SVG"
status = "pending"

[[exit_criteria]]
id = "analytic"
title = "Extrusions, cylinders, spheres and repeated occurrences meet the A0 semantics"
status = "pending"

[[exit_criteria]]
id = "qualification"
title = "Demos and ACR Toon pass visual, deterministic, limit and performance qualification"
status = "pending"

[[exit_criteria]]
id = "design-doc-intent-audit"
title = "Contracts, docs, ADRs and requirements match the released implementation"
status = "pending"

[[exit_criteria]]
id = "test-runtime-impact-audit"
title = "New test runtime is measured and justified"
status = "pending"

[[exit_criteria]]
id = "external-review"
title = "Independent review has no unresolved material findings"
status = "pending"

[[exit_criteria]]
id = "release"
title = "Geometer and ACR consume published compatible versions and release evidence is complete"
status = "pending"
+++

# Model illustration and analytic 2.5D boundary

## Outcome

Publish a new Geometer release that accepts either a model attachment or a
generic analytic scene and returns a finished illustration in one native
operation. The operation must offer both SVG and renderer-neutral drawing
geometry forms. STEP import, tessellation, Fast vector detail, Fast Mesh Shadow,
visibility, shading and same-color fusion remain inside Geometer; an
intermediate mesh collection must not cross executable IPC.

Use the candidate Geometer build from ACR Toon before publishing it. Once the
demos and ACR corpus are visually and mechanically accepted, publish Geometer,
update the Geometer dependency pin through Altium Monkey if its package boundary
requires it, consume that published dependency in Altium Cruncher, repeat the
focused qualification and then publish ACR. Do not publish either dependency or
consumer merely because the new operation compiles.

## Motivation and measured failure

The current complete STEP illustration path performs three independent
operations:

```text
model bytes
  -> geometry.model_tessellation.a0
  -> geometry.mesh_collection.a0 JSON attachment
  -> geometry.model_hlr_projection.a0
  -> geometry.mesh_illustration_geometry.a0 or geometry.mesh_illustration.a0
```

This makes the consumer coordinate matching model placement, view and style
across calls. More significantly, every tessellated coordinate, index, normal
and material crosses IPC as JSON before Geometer reads it back into the
illustration engine. Same-color fusion occurs only after that transfer. OV Tech
PiMX8 U16 demonstrated the mismatch: its transferred mesh exceeded the former
JSON limit even though native fusion reduced the final drawing to four
surfaces.

The 2026.9.12 transport raised governed JSON envelopes to 32 MiB and attachment
ceilings remain 256 MiB, but larger envelopes do not remove the redundant
serialization. The new boundary removes it. The existing mesh operations stay
available for callers that already own triangle data.

## Scope

This plan includes:

- one model-illustration source union covering a file attachment and analytic
  scene;
- separate SVG and drawing-geometry operations sharing that source contract;
- STEP as the only A0 file media type, with media-type dispatch that can accept
  other model formats in a later generation;
- analytic planar extrusions, Z-axis cylinders and spheres;
- reusable analytic definitions and occurrence transforms;
- internal Fast detail and Fast Mesh Shadow generation from the same prepared
  geometry used for illustration;
- partial STEP tessellation by default, with ordered warnings and usable faces;
- generated C++, TypeScript, Rust, Python, JSON Schema and HTML contract
  projections;
- direct C++ values, executable IPC, full WASM and client conveniences;
- native/browser/Python demos and ACR Toon candidate integration;
- evidence-based resource limits, difficult-board stress runs and release
  cleanup.

This plan does not include:

- Toon isometric PCB views;
- a whole-PCB multi-model scene or exact inter-component occlusion;
- dynamic or multiple model attachments in one A0 request;
- Parasolid/X_T import;
- a public packed mesh or GPU render-cache format;
- application-specific PCB, Altium or KiCad fields in Geometer;
- changes to OCCT meshing algorithms;
- PNG/raster output.

## Architectural decisions

### One logical source union, two output operations

Use one discriminated source model for file-backed and analytic input. The two
operations differ only in their output target:

- `geometry.model_illustration.a0` returns deterministic SVG inline;
- `geometry.model_illustration_geometry.a0` returns the existing
  `geometry.mesh_illustration.geometry.a0` as a bounded output attachment.

This mirrors the existing mesh illustration boundary and keeps raw drawing
geometry available to consumers that own their renderer. It avoids an optional
SVG/geometry result object with invalid cross-field states.

The operation catalog declares `model` as an optional attachment because the
analytic source uses no attachment. Semantic validation makes the relationship
strict: `kind: "model"` requires exactly one supported `model` attachment, while
`kind: "analytic"` rejects a `model` attachment. No operation reads a local path
or fetches a URL.

A0 intentionally accepts at most one model attachment. ACR may use the new
boundary for one STEP body or one analytic scene containing multiple analytic
bodies. Components containing multiple STEP bodies or mixed STEP and analytic
bodies retain the existing combined-mesh path so their cross-body visibility,
outline and paint ordering do not regress. A later multi-model scene operation
requires a separate dynamic-attachment or packed-container design.

### A0 file support and future formats

The A0 catalog advertises `application/step` and `model/step`. The actual
attachment media type selects the importer; no field is named `step_path` or
otherwise makes the logical DTO STEP-specific. A future format requires an
advertised media type, importer, capability evidence and compatibility review.
Unsupported media such as X_T produces a stable operation diagnostic and can be
handled as a warning by a partial-rendering application such as Toon.

### Keep analytic public input analytic

Analytic bodies remain analytic on the public boundary. Native code may lower a
profile to triangles or another private visibility representation, but it must
not create a temporary STEP file, invoke an OCCT model round trip or serialize a
mesh collection. This retains the existing illustration and Fast HLR engines
without exposing their working representation as a new contract.

The A0 analytic scene has reusable definitions and occurrences. Repeated
footprints or component bodies send the local definition once and reference it
from bounded occurrence transforms. Separate bodies remain separate geometry;
there is no CAD union. Illustration surface fusion remains the existing optional
paint optimization and HLR retains visible body boundaries.

Every declared definition must be reached by at least one occurrence. Reject
unreferenced definitions so canonical inputs and resource charging cannot
disagree about ignored data.

### Reuse existing geometry vocabulary deliberately

Reuse the established illustration vectors, matrices, materials, view,
preparation, style, SVG options, render statistics, Fast HLR controls and
drawing-geometry output types.

The experimental analytic Boolean `PlanarRing` cannot be reused directly on the
wire: it carries solver-specific authored IDs and signed nanometer coordinates.
The model boundary uses a small millimeter profile DTO with the same line and
circular-arc topology. Its C++ lowering should reuse the existing planar ring,
arc reconstruction, triangulation and Clipper2 helpers where their semantics
match. Do not modify the analytic Boolean A0 wire shape or make illustration
depend on its experimental solver lifecycle.

### Fast linework is part of the one-pass operation

When the illustration style requests HLR outline or detail, the model operation
computes both from its prepared geometry:

- detail uses `projection_algorithm: fast` semantics;
- outline uses `outline_algorithm: fast-mesh-shadow` semantics;
- hidden edges remain disabled;
- the request has one view, used by fills and linework;
- linework is visibility-filtered before composition.

The full model operation does not expose the older poly/exact/HLR-close choices.
Callers needing those products continue to use the independent HLR operations.
The nested `fast` controls are reused for bounded expert tuning.

Before contract promotion, amend ADR 016 or accept a focused successor ADR. It
must record that HLR and mesh illustration remain independently callable while
the new convenience operation composes the two existing algorithms over one
shared prepared source. Contract promotion depends on that decision; it is not
left as a conditional documentation cleanup.

### Binary mesh stays deferred

The Appz Technical Render Lab research favors definition-local geometry,
occurrence transforms, contiguous typed arrays and independently keyed geometry
and semantic bindings. That direction is compatible with the analytic
definition/occurrence contract here. It solves a broader GPU cache, reload and
picking problem.

This release does not define that binary container. The model-illustration
operation removes the immediate mesh transfer, so adding a packed mesh would
increase scope without improving its hot path. The existing mesh collection
operation remains JSON for compatibility. Preserve source-local coordinates and
occurrence transforms so a later packed format does not require a semantic
redesign.

## Proposed TypeSpec structures

The following is the planned logical shape. Names may receive mechanical
adjustments during TypeSpec compilation, but discriminator values, units,
relationships and operation identities are acceptance requirements.

```typespec
namespace Wavenumber.Geometer.Contracts.ModelIllustrationA0;

@minItems(2)
@maxItems(2)
model IllustrationVector2Mm is float64[];

enum PlanarArcSweep { cw, ccw }

model IllustrationProfileLineA0 {
  kind: "line";
}

model IllustrationProfileCircularArcA0 {
  kind: "circular_arc";
  center_mm: IllustrationVector2Mm;
  sweep: PlanarArcSweep;
}

@oneOf
union IllustrationProfileSegmentA0 {
  line: IllustrationProfileLineA0,
  circular_arc: IllustrationProfileCircularArcA0,
}

model IllustrationProfileRingA0 {
  // segment i connects points_mm[i] to points_mm[(i + 1) % count]
  @minItems(3) @maxItems(131072) points_mm: IllustrationVector2Mm[];
  @minItems(3) @maxItems(131072) segments: IllustrationProfileSegmentA0[];
}

model IllustrationProfileRegionA0 {
  outer: IllustrationProfileRingA0;
  @maxItems(131071) holes?: IllustrationProfileRingA0[];
}

model AnalyticExtrusionA0 {
  kind: "extrusion";
  @minLength(1) @maxLength(1024) id: string;
  @minItems(1) @maxItems(65536) regions: IllustrationProfileRegionA0[];
  z_min_mm: float64;
  z_max_mm: float64;
  material: MeshIllustrationA0.MeshIllustrationMaterial;
}

model AnalyticCylinderA0 {
  kind: "cylinder";
  @minLength(1) @maxLength(1024) id: string;
  center_mm: IllustrationVector2Mm;
  @minValueExclusive(0) radius_mm: float64;
  z_min_mm: float64;
  z_max_mm: float64;
  material: MeshIllustrationA0.MeshIllustrationMaterial;
}

model AnalyticSphereA0 {
  kind: "sphere";
  @minLength(1) @maxLength(1024) id: string;
  center_mm: MeshIllustrationA0.IllustrationVector3;
  @minValueExclusive(0) radius_mm: float64;
  material: MeshIllustrationA0.MeshIllustrationMaterial;
}

@oneOf
union AnalyticPrimitiveA0 {
  extrusion: AnalyticExtrusionA0,
  cylinder: AnalyticCylinderA0,
  sphere: AnalyticSphereA0,
}

model AnalyticDefinitionA0 {
  @minLength(1) @maxLength(1024) id: string;
  @minItems(1) @maxItems(65536) primitives: AnalyticPrimitiveA0[];
}

model AnalyticOccurrenceA0 {
  @minLength(1) @maxLength(1024) id: string;
  @minLength(1) @maxLength(1024) definition_id: string;
  // Omitted is identity; column-major affine transform in millimeters.
  transform?: MeshIllustrationA0.IllustrationMatrix4x4;
}

model AnalyticSceneA0 {
  @minItems(1) @maxItems(65536) definitions: AnalyticDefinitionA0[];
  @minItems(1) @maxItems(65536) occurrences: AnalyticOccurrenceA0[];
}

model ModelAttachmentIllustrationSourceA0 {
  kind: "model";
  attachment: "model";
  // Applies after source-root placement normalization.
  transform?: MeshIllustrationA0.IllustrationMatrix4x4;
  material_override?: MeshIllustrationA0.MeshIllustrationMaterial;
  tessellation?: ModelTessellationA0.ModelTessellationOptionsA0;
}

model AnalyticIllustrationSourceA0 {
  kind: "analytic";
  scene: AnalyticSceneA0;
  lowering?: AnalyticLoweringOptionsA0;
}

@oneOf
union ModelIllustrationSourceA0 {
  model: ModelAttachmentIllustrationSourceA0,
  analytic: AnalyticIllustrationSourceA0,
}

model AnalyticIllustrationLimitsA0 {
  @minValue(1) @maxValue(65536) max_reached_definitions?: uint32 = 4096;
  @minValue(1) @maxValue(65536) max_reached_occurrences?: uint32 = 65536;
  @minValue(1) @maxValue(65536) max_reached_primitives?: uint32 = 65536;
  @minValue(1) @maxValue(262144) max_reached_rings?: uint32 = 262144;
  @minValue(1) @maxValue(2000000) max_reached_points?: uint32 = 2000000;
  @minValue(1) @maxValue(2000000) max_generated_curve_samples?: uint32 = 2000000;
  @minValue(1) @maxValue(2000000) max_definition_triangles?: uint32 = 750000;
}

model AnalyticLoweringOptionsA0 {
  @minValueExclusive(0) linear_deflection_mm?: float64 = 0.1;
  @minValueExclusive(0) angular_deflection_rad?: float64 = 0.5;
  limits?: AnalyticIllustrationLimitsA0;
}

model ModelIllustrationWorkLimitsA0 {
  @minValue(1) @maxValue(100000000)
  max_visibility_candidate_pairs?: uint32 = 100000000;
  @minValue(1) @maxValue(2000000)
  max_drawing_commands?: uint32 = 2000000;
}

model ModelIllustrationLineworkOptionsA0 {
  fast?: HlrProjectionA0.FastHlrOptionsA0;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_illustration.request:a0")
@contractIdentity("geometry.model_illustration.request.a0")
model ModelIllustrationRequestA0 {
  schema: "geometry.model_illustration.request.a0";
  source: ModelIllustrationSourceA0;
  view: MeshIllustrationA0.MeshIllustrationView;
  prepare?: MeshIllustrationA0.MeshIllustrationPrepareOptions;
  linework?: ModelIllustrationLineworkOptionsA0;
  style?: MeshIllustrationA0.MeshIllustrationStyleA0;
  svg?: MeshIllustrationA0.MeshIllustrationSvgOptions;
  work_limits?: ModelIllustrationWorkLimitsA0;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_illustration_geometry.request:a0")
@contractIdentity("geometry.model_illustration_geometry.request.a0")
model ModelIllustrationGeometryRequestA0 {
  schema: "geometry.model_illustration_geometry.request.a0";
  source: ModelIllustrationSourceA0;
  view: MeshIllustrationA0.MeshIllustrationView;
  prepare?: MeshIllustrationA0.MeshIllustrationPrepareOptions;
  linework?: ModelIllustrationLineworkOptionsA0;
  style?: MeshIllustrationA0.MeshIllustrationStyleA0;
  work_limits?: ModelIllustrationWorkLimitsA0;
}
```

Contract semantics beyond structural TypeSpec validation:

- every vector and matrix member is finite;
- matrices use `m[column * 4 + row]` and transform a point as
  `p_world = M * [x, y, z, 1]^T`; only `m[12..14]` carry millimeter
  translation and the last row `m[3], m[7], m[11], m[15]` must equal
  `[0, 0, 0, 1]`;
- the upper-left 3x3 may contain rotation, reflection, nonuniform scale and
  shear. Compute its singular values with a scale-normalized decomposition;
  require finite `sigma_max > 0` and `sigma_min / sigma_max >= 1e-12`. This
  accepts uniformly small or large scales while rejecting singular and severely
  ill-conditioned transforms;
- normals use a scale-normalized inverse transpose and negative determinants
  reverse winding. Reject a transform if any prepared or transformed vertex,
  normal, analytic extent or derived bound is nonfinite, including overflow
  caused by finite source and matrix members;
- model placement is `world = source_transform * normalized_source_point`;
  analytic placement is `world = occurrence_transform * primitive_local_point`;
- ring point and segment counts are equal and segments form the implicit closed
  cycle;
- a circular arc uses its start/end points, center and `sweep` to select the
  unique clockwise or counterclockwise branch, including sweeps greater than
  180 degrees; coincident endpoints and full-circle segments are rejected and
  adapters must split full circles into at least two arcs;
- zero-length segments, self-intersecting rings and zero-area rings are
  rejected; outer rings normalize counterclockwise and holes clockwise;
- holes must be strictly contained, nonintersecting and non-touching; regions
  in one extrusion must have disjoint interiors, while touching or overlapping
  separately painted bodies remain separate primitives;
- `z_max_mm > z_min_mm` for extrusions and cylinders;
- definition IDs, primitive IDs and occurrence IDs are unique in their scopes;
- every occurrence references one existing definition;
- every definition is reached by at least one occurrence;
- authored and reached definition, occurrence, primitive, ring and point totals
  are checked with 64-bit arithmetic before allocation against the declared
  analytic limits;
- cylinders and spheres derive angular sampling from
  `theta = min(angular_deflection_rad,
  2*acos(clamp(1-linear_deflection_mm/radius_mm,-1,1)))`; cylinder longitude is
  `clamp(ceil(2*pi/theta),12,4096)`, sphere latitude is
  `clamp(ceil(pi/theta),6,2048)` and sphere longitude uses the cylinder rule;
  generated samples are charged before allocation;
- definition-local triangles are charged to `max_definition_triangles`; checked
  occurrence expansion is independently charged to the existing
  `prepare.max_triangles`; neither option overrides the other;
- visibility candidate pairs and emitted drawing commands are admitted against
  `work_limits` before their corresponding allocations;
- a model source and attachment must agree as described above;
- model `tessellation` controls only imported models; analytic `lowering`
  controls only analytic sampling; shared `prepare` and `work_limits` apply
  after either source is prepared;
- model `allow_partial` defaults to true and local model-face failures produce
  ordered warnings rather than discarding usable geometry;
- warnings preserve source order; at most 256 are returned, with the first 255
  followed by `N additional warnings omitted` when truncation is required.

## Proposed operations and IPC projection

```typespec
namespace Wavenumber.Geometer.Contracts.Operations;

@operationIdentity("geometry.model_illustration.a0")
@inputAttachment(
  "model",
  false,
  "application/step,model/step",
  268435456
)
op modelIllustration(
  request: ModelIllustrationA0.ModelIllustrationRequestA0
): ModelIllustrationA0.ModelIllustrationResultA0;

@operationIdentity("geometry.model_illustration_geometry.a0")
@inputAttachment(
  "model",
  false,
  "application/step,model/step",
  268435456
)
@outputAttachment(
  "illustration_geometry",
  true,
  "application/vnd.wavenumber.geometer.illustration-geometry+json",
  268435456
)
op modelIllustrationGeometry(
  request: ModelIllustrationA0.ModelIllustrationGeometryRequestA0
): ModelIllustrationA0.ModelIllustrationGeometryResultA0;
```

The result roots are:

```typespec
enum ModelAttachmentMediaTypeA0 {
  application_step: "application/step",
  model_step: "model/step",
}

model ModelAttachmentSourceSummaryA0 {
  kind: "model";
  media_type: ModelAttachmentMediaTypeA0;
  @minLength(64) @maxLength(64) source_sha256: string;
  @minValue(0) meshes: uint32;
  @minValue(0) triangles: uint32;
}

model AnalyticSourceSummaryA0 {
  kind: "analytic";
  @minValue(0) definitions: uint32;
  @minValue(0) occurrences: uint32;
  @minValue(0) primitives: uint32;
  @minValue(0) triangles: uint32;
}

@oneOf
union ModelIllustrationSourceSummaryA0 {
  model: ModelAttachmentSourceSummaryA0,
  analytic: AnalyticSourceSummaryA0,
}

model ModelIllustrationTimingsA0 {
  @minValue(0) import_ms: float64;
  @minValue(0) lowering_ms: float64;
  @minValue(0) tessellation_ms: float64;
  @minValue(0) linework_ms: float64;
  @minValue(0) illustration_ms: float64;
  @minValue(0) serialization_ms: float64;
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_illustration.result:a0")
@contractIdentity("geometry.model_illustration.result.a0")
model ModelIllustrationResultA0 {
  schema: "geometry.model_illustration.result.a0";
  svg: string;
  source: ModelIllustrationSourceSummaryA0;
  stats: MeshIllustrationA0.MeshIllustrationRenderStats;
  timings: ModelIllustrationTimingsA0;
  @maxItems(256) warnings: string[];
}

@jsonSchema
@id("urn:wavenumber:schema:geometer:geometry.model_illustration_geometry.result:a0")
@contractIdentity("geometry.model_illustration_geometry.result.a0")
model ModelIllustrationGeometryResultA0 {
  schema: "geometry.model_illustration_geometry.result.a0";
  geometry: MeshIllustrationGeometryA0.IllustrationGeometryAttachment;
  source: ModelIllustrationSourceSummaryA0;
  stats: MeshIllustrationA0.MeshIllustrationRenderStats;
  timings: ModelIllustrationTimingsA0;
  @maxItems(256) warnings: string[];
}
```

Timings are diagnostic and excluded from semantic equivalence by an explicit
contract projection. Model source digest is SHA-256 of the attachment bytes.
The A0 analytic summary deliberately has no digest; introducing one would first
require a cross-language canonical JSON/numeric representation and conformance
vectors. The drawing attachment reuses the current geometry schema and paint
order; a new drawing format is not introduced.

For the raw operation, root `stats` and `warnings` exactly equal those encoded
in `geometry.mesh_illustration.geometry.a0`. A producer must emit the same
deterministically truncated warning set in both locations. Clients reject a
mismatch as a malformed success response. Ergonomic client methods return a
composite `{ result, geometry }`, retaining source summary and timings alongside
the validated decoded attachment.

The generated operation catalog must advertise both operations, their optional
model input, the geometry output attachment, request/result roots and the
existing generic IPC limits. `IpcRequestValueA0` and
`OperationResultValueA0` gain explicit variants. Runtime dispatch remains
`logical_dto`.

Representative wire request:

```json
{
  "operation": "geometry.model_illustration_geometry.a0",
  "request": {
    "schema": "geometry.model_illustration_geometry.request.a0",
    "source": {
      "kind": "model",
      "attachment": "model",
      "transform": [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1],
      "tessellation": {
        "linear_deflection_mm": 0.01,
        "angular_deflection_rad": 0.5,
        "root_placement": "preserve",
        "allow_partial": true
      }
    },
    "view": {
      "direction": [0, 0, 1],
      "up": [0, 1, 0],
      "mirror_x": false
    },
    "style": {
      "shading": "toon",
      "fuse_surfaces": true,
      "show_hlr_outline": true,
      "show_hlr_detail": true,
      "show_outlines": false,
      "show_creases": false
    }
  }
}
```

The frame carries one attachment named `model` with STEP bytes and media type
`application/step` or `model/step`. A successful response has the generated
result DTO in its JSON outcome and one `illustration_geometry` attachment. The
consumer never receives `mesh_collection` in this path.

## Native implementation slices

### Contract authority first

1. Add the model-illustration TypeSpec source and operation declarations.
2. Extract reusable tessellation options without changing the current
   `geometry.model_tessellation.request.a0` JSON shape.
3. Register all four new contract roots and two operations as candidates while
   their generated and runtime implementations are built.
4. Import the roots from `main.tsp`, add the IPC request/result union variants
   and regenerate the normalized catalog, schemas, C++ DTO/codecs, TypeScript,
   Rust, Python and styled HTML reference.
5. Add strict and semantic contract vectors for source discrimination,
   attachment relationships, analytic references/topology, numeric limits,
   unknown fields and operation/outcome pairing.
6. Before release, promote all four roots and both operations under ADR 010.
   Record schema/field parity, generator ownership, strict and semantic vectors,
   runtime dispatch evidence, client projection evidence and compatibility
   disposition in the promotion manifest. Candidate registration alone does not
   satisfy this plan's TypeSpec-authority exit criterion.

Do not handwrite parallel DTOs in any language. Handwritten code may provide
ergonomic adapters around generated types but cannot redefine their wire shape.

### One-pass model pipeline

Add small focused C++ modules for source preparation, analytic lowering,
linework composition and operation transport. Reuse the current model importer,
colored tessellator, Fast HLR preparation, Fast Mesh Shadow and illustration
renderer in memory. Do not serialize and decode `MeshCollectionA0` between
stages.

Parse and validate the logical request before importing the attachment. Apply
root-placement policy and source transform once. Prepare one canonical mesh
scene shared by Fast linework and illustration so view, winding, normals,
materials and placement cannot diverge. Preserve the existing deterministic
warning and drawing order.

Allow partial model tessellation by default. A failed face is omitted with a
stable model/face diagnostic; a completely unusable source fails its isolated
request. Keep source model bytes immutable.

### Analytic lowering

Lower each reached definition once. Apply occurrence transforms after local
geometry preparation and before scene visibility. Cache definition-local
lowering for the request and retain a seam between definition geometry and
occurrence placement for future persistent caching.

- Extrusion: validate rings and arcs, triangulate caps and generate side
  surfaces directly from the profile and Z interval.
- Cylinder: generate bounded caps and side representation directly from center,
  radius and Z interval; choose subdivision from the same deflection policy as
  model tessellation rather than a client-owned fixed segment count.
- Sphere: accept an exact center/radius contract. Initially lower to a bounded
  private representation compatible with the existing visibility and toon
  shading engine. An analytic projected-disc optimization may replace that
  lowering only if it preserves occlusion, banding, outlines and deterministic
  output. No sphere mesh is exposed or transferred.

Apply resource admission before large allocations. Report invalid topology,
non-finite transforms, bad extents, unresolved occurrence references and limit
exhaustion as stable operation diagnostics.

Every admission boundary gets a positive exact-limit vector and negative
`limit + 1` vectors. Include multiplicative cases where individually valid
definitions and occurrences exceed expanded-triangle or visibility-pair limits,
and a high-subdivision sphere that is rejected before allocating its sampled
vertices. Transform vectors include a valid small uniform scale, a singular
matrix, an ill-conditioned nonuniform scale and finite matrix/source values
whose transformed product overflows.

## KiCad compatibility research

The 2026-09-13 inspection of current KiCad upstream master found its unreleased
`EXTRUDED_3D_BODY` representation in `pcbnew/footprint.h` and S-expression
reader/writer. It contains:

- a 2D outline selected from courtyard, fab, silk or pad bounding box;
- overall height and board standoff;
- material and optional RGBA color;
- XYZ offset, scale and rotation;
- footprint-side placement;
- optional metallic through-hole pin extrusions derived separately from pads.

KiCad's renderer triangulates the outline, creates top/bottom caps and contour
walls, and applies the transform. The proposed profile, extrusion Z interval,
material and occurrence matrix cover this shape. KiCad source-layer selection,
fallback outline policy and pin derivation belong in KiCad Cruncher; Geometer
receives resolved generic geometry. No KiCad-specific field belongs in this A0
contract.

## Clients and public API

Expose both operations through:

- direct C++ owning-value functions;
- generic C ABI and full WASM operation dispatch;
- persistent and one-shot Python methods;
- asynchronous Rust client methods;
- TypeScript generated DTOs/codecs and operation metadata;
- the executable operation catalog and `serve --stdio` transport.

Client helpers accept bytes plus generated request/options for a model source,
or a generated analytic request with no bytes. They construct attachment frames
but do not materialize an intermediate mesh. Advanced consumers retain the
existing tessellation and mesh-illustration methods.

The raw-geometry method returns the decoded
`MeshIllustrationGeometryA0` value after validating attachment name, media type,
length, digest and schema. The SVG method returns the generated result DTO.
Transport and operation failures retain the current distinction and process
lifecycle behavior.

## Demo and documentation updates

Update every illustration example that claims to demonstrate the complete
model pipeline:

- the browser/WASM Illustration Lab uses the model operation for uploaded and
  bundled STEP files;
- the native Rust app uses the model operation for its illustration pane while
  retaining separate tessellation only where its interactive 3D pane requires
  mesh data;
- the Python Canvas example requests model illustration geometry directly;
- the Rust command-line example requests the single SVG operation;
- add an analytic example containing overlapping differently colored
  extrusions, a cylinder, a sphere and repeated occurrences.

Rebuild committed native/WASM/sample artifacts according to repository policy.
Open the browser Illustration Lab and launch the native Rust app for human
acceptance before release.

Add or update durable design documentation for:

- the model-illustration operation and its source/attachment semantics;
- the analytic 2.5D profile and occurrence model;
- raw drawing geometry and SVG output choices;
- executable IPC, Python, Rust, TypeScript, C++ and WASM usage;
- model import/tessellation and partial-failure behavior;
- the Fast HLR detail, Fast Mesh Shadow and illustration algorithms, including
  preparation, visibility, fusion, resource limits and determinism;
- demo commands and release validation.

Use the existing generated Wavenumber-styled contract pages. Add authored
algorithm/design pages to the design index rather than maintaining separate
handwritten contract tables that can drift from TypeSpec. The mandatory ADR 016
amendment or successor is accepted before contract implementation and referenced
by the promotion evidence.

## ACR Toon integration and qualification

Use a local candidate wheel/executable or direct workspace pin before publishing
Geometer. Do not require a temporary public release merely to test the consumer.

Select the new raw `model_illustration_geometry` call only when the complete
component can cross A0 without losing body composition:

- a component with one STEP body uses one model request;
- a component with one or more analytic bodies uses one analytic scene request;
- a component with multiple STEP bodies, or mixed STEP and analytic bodies,
  retains ACR's existing combined-mesh path for A0.

Do not split a mixed or multi-STEP component into independently illustrated SVG
fragments: that would lose cross-body visibility and ordering. Emit diagnostic
progress that identifies the chosen path. Qualify each cardinality explicitly
so unsupported composition never silently lowers fidelity. Preserve Toon
warning aggregation, disk-cache behavior, SVG metadata and friendly
component/group names. Cache by source digest or analytic definition content,
complete local pose, side, material override, view and illustration settings.
Absolute component placement remains in ACR's SVG composition.

Send Altium analytic bodies through the analytic source:

- model type 0 as one or more planar extrusions with holes and exact Z extents;
- model type 2 as cylinders;
- model type 3 as spheres;
- separate body colors/opacities and Z extents remain separate primitives;
- repeated local analytic geometry reuses definitions/occurrences and the ACR
  disk cache;
- a footprint with no 3D body remains omitted as today;
- unsupported external model formats warn and do not fail the board.

Audit the final SVG hierarchy while integrating: view, virtual layer,
component and symbol definitions must retain stable IDs, data attributes,
human-readable labels and Inkscape layer names. The new operation must not make
native implementation names part of the SVG metadata contract.

Visual and deterministic qualification includes:

- RT Super C1 for a small mix of STEP and extruded bodies;
- OV Tech PiMX8 for U16 and its formerly excessive mesh JSON;
- NXP FRDM i.MX93 for complex models;
- TDM Monkey for tolerant model-import warnings;
- the private MRX corpus board as the largest stress case, without copying it
  into the public repository;
- a focused fixture covering overlapping extrusions at different Z extents,
  same-color overlapping bodies, cylinder, sphere, holes, a greater-than-180
  degree arc, distinct colors, repeated occurrences and a reflected bottom-side
  occurrence;
- focused source-cardinality fixtures for one STEP, many analytic bodies,
  multiple STEP bodies and mixed STEP/analytic bodies.

Compare top/bottom SVGs and warnings against the accepted current behavior.
Intentional newly supported analytic bodies require visual approval and focused
golden assertions; unrelated solder mask, copper, silk, drill, designator and
variant output must remain unchanged.

## Limits and performance evidence

Measure, per source and per whole board:

- STEP attachment bytes;
- former mesh-collection JSON bytes;
- final illustration-geometry or SVG bytes;
- triangle/surface/line counts;
- import, lowering, tessellation, linework, illustration and serialization time;
- peak process RSS;
- cache hits and native worker count;
- cold and warm ACR wall time.

The new path should eliminate the former mesh attachment and its JSON encode,
validation, transfer and decode time. Final geometry must still use existing
same-color fusion when enabled.

Exercise OV U16, NXP and MRX under the current 32 MiB JSON, 256 MiB
per-attachment and 512 MiB native-frame ceilings plus the exact analytic and
work ceilings declared above.
If a valid final result reaches a ceiling, record the measured size and memory
cost, choose an operation-specific bounded increase, update TypeSpec/catalog,
all transport implementations and negative limit vectors together, then rerun
the corpus. A resource failure remains bounded and diagnostic; it must never
become an unbounded allocation.

Run worker-count experiments only after the single-request pipeline is stable.
Preserve deterministic output and source-ordered warnings at every supported
count.

## Test strategy and runtime ownership

Add tests after the implementation shape settles. Retain tests that own a
contract, geometry invariant, failure branch or previous regression:

- TypeSpec strict and semantic vectors for every source/attachment union edge;
- C++ value tests for model parity, analytic geometry, overlap/Z ordering,
  partial tessellation, transforms and every exact resource limit;
- executable IPC tests proving one model attachment in and no mesh attachment
  out;
- Python/Rust/TypeScript client tests using generated DTOs;
- native/WASM equivalence for representative model and analytic fixtures;
- same-color fusion and Fast linework presence/parity;
- ACR focused integration tests for formerly failing U16, analytic bodies and
  all four source-cardinality paths without silent fidelity loss;
- demo smoke tests and release artifact provenance.

Do not duplicate generated field validation in handwritten unit tests. Record
new test counts and wall time, and split or consolidate fixtures if the release
gate grows materially.

## Release sequence

1. Complete Geometer focused checks, native tests, contract generation and demo
   builds on the candidate branch.
2. Run ACR Toon against that candidate without publishing it.
3. Obtain visual acceptance of browser/native Geometer demos and the ACR board
   set.
4. Run Geometer release-facing Rack, native, WASM, package, provenance and
   supported-platform CI gates.
5. Obtain independent implementation/release review and remediate findings.
6. Publish the next date-versioned Geometer tag/package/artifacts.
7. Update Altium Monkey's Geometer pin and publish its compatible patch only if
   that package owns the transitive version boundary needed by ACR.
8. Pin ACR to published compatible dependencies, rerun the focused Toon corpus
   and packaging/install smoke, then publish ACR.
9. Verify clean installs on Windows and macOS, with Linux covered by hosted CI.

Do not pin a release to an unpublished branch or leave a final consumer release
depending on a Git commit.

## Cleanup and closeout

Before plan closeout:

- promote lasting decisions and algorithm descriptions from this plan into
  ADR/design/requirement docs;
- remove this completed temporary plan with `wn-dev-std plan close`;
- remove task-created worktrees and stale branches after their changes are
  merged;
- remove task-specific profiling output, temporary extracted models and demo
  output;
- audit Cargo target directories and remove redundant task-local caches while
  preserving the canonical build needed for release evidence;
- preserve `.deps` OCCT/emsdk caches unless intentionally refreshing a
  dependency, because rebuilding them is outside this plan;
- verify each repository is clean and published tags resolve to the qualified
  commits.

## Exit evidence

The plan is complete only when:

- TypeSpec owns every new DTO and operation and all generated projections pass
  freshness checks;
- the one-pass model operation uses Fast detail and Fast Mesh Shadow by default;
- raw illustration geometry is independently consumable without SVG;
- model and analytic sources obey strict attachment and topology semantics;
- analytic definitions lower once and repeated occurrences preserve placement,
  color, outline and Z ordering;
- U16, NXP and MRX complete without the former mesh-transfer limit failure;
- model import warnings remain partial and actionable;
- browser, native Rust and Python demos exercise the released operation;
- ACR output has accepted visuals, stable grouping/metadata and no unrelated 2D
  regressions;
- performance and memory evidence show the eliminated mesh round trip and no
  new unbounded work;
- supported release gates and independent review pass;
- Geometer and ACR releases consume published, mutually compatible versions;
- temporary plans, worktrees and task-only caches are cleaned up.
