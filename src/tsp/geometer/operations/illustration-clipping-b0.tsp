import "@wavenumber/wn-geometer-contract-emitter";
import "./mesh-illustration-a0.tsp";

namespace Wavenumber.Geometer.Contracts.IllustrationClippingB0;

/** One world-space half-space. The kept side satisfies dot(normal, point) - distance_mm >= -tolerance_mm. */
model HalfSpacePlane {
  normal: Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3;
  distance_mm: float64;
  @minValue(0) tolerance_mm?: float64 = 0;
}

/** Independent hard work budgets for deterministic triangle clipping. */
model ClippingLimits {
  @minValue(1) @maxValue(2000000) max_output_triangles?: uint32 = 2000000;
  @minValue(1) @maxValue(6000000) max_generated_vertices?: uint32 = 6000000;
  @minValue(1) @maxValue(8000000) max_intersections?: uint32 = 8000000;
  @minValue(1) @maxValue(32000000) max_edge_plane_tests?: uint32 = 32000000;
}

/** Optional B0 clipping request. The first generation deliberately creates no section caps. */
model IllustrationClipping {
  @minItems(1) @maxItems(16) planes: HalfSpacePlane[];
  cap_policy: "none";
  limits?: ClippingLimits;
}

/** Canonical unit-normal plane returned in fragment metadata. */
model NormalizedHalfSpacePlane {
  normal: Wavenumber.Geometer.Contracts.MeshIllustrationA0.IllustrationVector3;
  distance_mm: float64;
  @minValue(0) tolerance_mm: float64;
}

/** Fully resolved clipping values that participate in semantic identity. */
model NormalizedClipping {
  @minItems(1) @maxItems(16) planes: NormalizedHalfSpacePlane[];
  cap_policy: "none";
  max_output_triangles: uint32;
  max_generated_vertices: uint32;
  max_intersections: uint32;
  max_edge_plane_tests: uint32;
}

/** Deterministic identity and size summary for the prepared world-space fragment. */
model FragmentMetadata {
  clipping?: NormalizedClipping;
  @maxValue(2000000) input_triangles: uint32;
  @maxValue(2000000) output_triangles: uint32;
  @minLength(64) @maxLength(64) fragment_sha256: string;
  @minLength(64) @maxLength(64) linework_geometry_sha256: string;
  @minLength(64) @maxLength(64) raw_attachment_sha256?: string;
}
