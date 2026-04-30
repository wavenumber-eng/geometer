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

struct ProjectionViewSpec
{
    std::string id = "top";
    std::array<double, 3> direction = {0.0, 0.0, 1.0};
    std::array<double, 3> up = {0.0, 1.0, 0.0};
};

struct HlrProjectionOptions
{
    std::vector<ProjectionViewSpec> views;
    ProjectionCurveMode curve_mode = ProjectionCurveMode::NativeArcs;
    int samples_per_curve = 24;
    int round_digits = 3;
    bool include_visible = true;
    bool include_outline = true;
    bool union_simple_polygons = true;
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

struct HlrProjectionResult
{
    std::string schema = "wn.geometry.projection.a0";
    std::string units = "mm";
    std::string source_hash;
    std::vector<ProjectedViewGeometry> views;
};

int step_hlr_projection_from_bytes(const unsigned char* step_data, std::size_t step_size,
                                   const HlrProjectionOptions& options, HlrProjectionResult* result,
                                   Status* status = nullptr);

int write_hlr_projection_json(const HlrProjectionResult& result, std::string* json,
                              Status* status = nullptr);

int write_hlr_projection_svg(const HlrProjectionResult& result, const std::string& view_id,
                             const std::string& mode, std::string* svg, Status* status = nullptr);

} // namespace geometer
