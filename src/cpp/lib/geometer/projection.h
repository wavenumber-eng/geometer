#pragma once

#include "status.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace geometer
{

enum class ProjectionCurveMode
{
    NativeArcs,
    Polyline
};

enum class ProjectionAlgorithm
{
    Poly,
    Exact
};

struct ProjectionViewSpec
{
    std::string id = "top";
    std::array<double, 3> direction = {0.0, 0.0, 1.0};
    std::array<double, 3> up = {0.0, 1.0, 0.0};
};

struct HlrProjectionOptions
{
    std::vector<ProjectionViewSpec> views;
    // Row-major 4x4 affine transform applied to the source shape before
    // projection. Translation lives in the final column. The final row must be
    // [0, 0, 0, 1].
    std::array<double, 16> model_transform = {
        1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
    };
    ProjectionCurveMode curve_mode = ProjectionCurveMode::NativeArcs;
    int samples_per_curve = 24;
    int round_digits = 3;
    // OCCT HLR edge categories merged into the "detail" extraction.
    // Each maps to a corresponding `HLRBRep_HLRToShape::<X>Compound()` shape.
    // Defaults reproduce the historical behavior (visible sharp + visible
    // outline). The poly extractor (HLRBRep_PolyHLRToShape) only supports
    // V/H Compound + OutLine; the smooth/sewn/iso flags are no-ops in poly.
    bool edge_v_sharp = true;    // VCompound          (visible, sharp edges)
    bool edge_v_outline = true;  // OutLineVCompound   (visible, silhouette)
    bool edge_v_smooth = false;  // Rg1LineVCompound   (visible, smooth/tangential)
    bool edge_v_sewn = false;    // RgNLineVCompound   (visible, sewn/manifold)
    bool edge_v_iso = false;     // IsoLineVCompound   (visible, parametric isolines)
    bool edge_h_sharp = false;   // HCompound          (hidden, sharp edges)
    bool edge_h_outline = false; // OutLineHCompound   (hidden, silhouette)
    bool edge_h_smooth = false;  // Rg1LineHCompound   (hidden, smooth/tangential)
    bool edge_h_sewn = false;    // RgNLineHCompound   (hidden, sewn/manifold)
    bool edge_h_iso = false;     // IsoLineHCompound   (hidden, parametric isolines)

    bool union_simple_polygons = true;

    // 1.1: poly-algo path defaults. Exact path is selected explicitly.
    ProjectionAlgorithm projection_algorithm = ProjectionAlgorithm::Poly;
    double mesh_linear_deflection = 0.01; // mm
    double mesh_angular_deflection = 0.5; // rad (~28.6 deg)
    bool mesh_relative = false;
    double hlr_angle_tolerance = 0.0174533; // ~1 deg
};

struct ProjectedSegment
{
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
};

struct ProjectedArc
{
    std::array<double, 2> start = {0.0, 0.0};
    std::array<double, 2> end = {0.0, 0.0};
    std::array<double, 2> center = {0.0, 0.0};
    double radius = 0.0;
    double extent_rad = 0.0;
    bool ccw = true;
    bool full_circle = false;
};

struct ProjectedModeGeometry
{
    std::vector<ProjectedSegment> segments;
    std::vector<ProjectedArc> arcs;
};

struct ProjectedViewGeometry
{
    ProjectionViewSpec view;
    ProjectedModeGeometry simple;
    ProjectedModeGeometry detail;
};

struct HlrProjectionTimings
{
    double step_read_ms = 0.0;
    double mesh_ms = 0.0;
    double hlr_ms = 0.0;
    double extract_ms = 0.0;
};

struct HlrProjectionResult
{
    std::string schema = "geometry.projection.a0";
    std::string units = "mm";
    std::string source_hash;
    std::vector<ProjectedViewGeometry> views;
    HlrProjectionTimings timings;
};

int step_hlr_projection_from_bytes(const unsigned char* step_data, std::size_t step_size,
                                   const HlrProjectionOptions& options, HlrProjectionResult* result,
                                   Status* status = nullptr);

int write_hlr_projection_json(const HlrProjectionResult& result, std::string* json,
                              Status* status = nullptr);

int write_hlr_projection_svg(const HlrProjectionResult& result, const std::string& view_id,
                             const std::string& mode, std::string* svg, Status* status = nullptr);

} // namespace geometer
