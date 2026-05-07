#pragma once

#include "status.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace geometer
{

struct PlanarSolvePoint
{
    double x = 0.0;
    double y = 0.0;
};

using PlanarSolvePath = std::vector<PlanarSolvePoint>;
using PlanarSolveRing = PlanarSolvePath;

struct PlanarSolveRegion
{
    PlanarSolveRing outline;
    std::vector<PlanarSolveRing> holes;
};

enum class PlanarSolveJoinType
{
    Miter = 0,
    Round = 1,
    Bevel = 2,
    Square = 3,
};

enum class PlanarSolveEndType
{
    Round = 0,
    Square = 1,
    Butt = 2,
    Joined = 3,
};

struct PlanarSolveStrokeGroup
{
    double radius_mm = 0.0;
    double miter_limit = 2.0;
    double arc_tolerance_mm = 0.0;
    PlanarSolveJoinType join_type = PlanarSolveJoinType::Miter;
    PlanarSolveEndType end_type = PlanarSolveEndType::Round;
    std::vector<PlanarSolvePath> paths;
};

struct PlanarSolveJob
{
    std::vector<PlanarSolveRing> subject_rings;
    std::vector<PlanarSolveRing> subtract_rings;
    std::vector<PlanarSolveStrokeGroup> stroke_groups;
    bool subtract_common_rings = true;
    bool filter_common_subtract_by_bounds = true;
    bool clip_to_final_rings = true;
    double common_subtract_filter_margin_mm = 0.0;
};

struct PlanarBatchSolveOptions
{
    int decimal_precision = 6;
    double cleanup_radius_mm = 0.0;
    double cleanup_miter_limit = 2.0;
    double cleanup_arc_tolerance_mm = 0.0;
};

struct PlanarSolveJobResult
{
    std::vector<PlanarSolveRegion> regions;
    double area_mm2 = 0.0;
};

struct PlanarBatchSolveInput
{
    PlanarBatchSolveOptions options;
    std::vector<PlanarSolveRing> common_subtract_rings;
    std::vector<PlanarSolveRing> final_clip_rings;
    std::vector<PlanarSolveJob> jobs;
};

struct PlanarBatchSolveResult
{
    std::vector<PlanarSolveJobResult> jobs;
};

int solve_planar_batch(const PlanarBatchSolveInput& input, PlanarBatchSolveResult* result,
                       Status* status = nullptr);

int solve_planar_batch_from_bytes(const unsigned char* request_data, std::size_t request_size,
                                  std::vector<unsigned char>* response_bytes,
                                  Status* status = nullptr);

} // namespace geometer
