#pragma once

#include "geometer/analytic_filtered_regions.h"

#include <cstdint>
#include <vector>

namespace geometer
{

struct AnalyticFilteredSourceRange
{
    std::uint32_t begin = 0;
    std::uint32_t count = 0;
};

struct AnalyticFilteredBoundaryLineage
{
    std::uint32_t half_edge = 0;
    AnalyticFilteredSourceRange positive;
    AnalyticFilteredSourceRange subtraction;
};

struct AnalyticFilteredVertexLineage
{
    std::uint32_t arrangement_vertex = 0;
    AnalyticFilteredSourceRange intersection;
};

struct AnalyticFilteredRegionLineage
{
    std::uint32_t region = 0;
    AnalyticFilteredSourceRange positive_contributors;
};

enum class AnalyticFilteredLineageError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    resource_limit_exceeded = 2,
};

struct AnalyticFilteredLineageTelemetry
{
    std::uint64_t regions_work_units = 0;
    std::uint64_t regions_peak_working_memory_bytes = 0;
    std::uint64_t arrangement_work_units = 0;
    std::uint64_t coverage_node_visits = 0;
    std::uint64_t component_transition_visits = 0;
    std::uint64_t boundary_membership_visits = 0;
    std::uint64_t vertex_membership_visits = 0;
    std::uint64_t emitted_boundary_records = 0;
    std::uint64_t emitted_vertex_records = 0;
    std::uint64_t emitted_region_records = 0;
    std::uint64_t emitted_source_references = 0;
    std::uint64_t publication_capacity_records = 0;
    std::uint64_t sort_work_units = 0;
    std::uint64_t reserved_lineage_work_units = 0;
    std::uint64_t reserved_outcomes_work_units = 0;
    std::uint64_t lineage_work_units = 0;
    std::uint64_t predicate_calls = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t required_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
};

struct AnalyticFilteredLineageResult
{
    AnalyticFilteredLineageError error = AnalyticFilteredLineageError::none;
    AnalyticFilteredRegionsResult regions;
    std::vector<AnalyticFilteredBoundaryLineage> boundaries;
    std::vector<AnalyticFilteredVertexLineage> vertices;
    std::vector<AnalyticFilteredRegionLineage> region_lineage;
    // Ranges above address this flattened table. Each range is independently
    // sorted and unique by the complete source-reference tuple. Final packet
    // publication interns complete ranges once alongside outcome source sets.
    std::vector<AnalyticFilteredSourceReference> source_references;
    AnalyticFilteredLineageTelemetry telemetry;
};

// Internal production pipeline stage. It owns lowering-bound region selection
// and projects only coordinate-free lineage facts from the certified resolved
// topology. It performs no geometric predicate, coordinate repair, or second
// use of the 50 nm resolution envelope.
[[nodiscard]] AnalyticFilteredLineageResult
build_analytic_filtered_lineage(const AnalyticRequestPacketRecords& records,
                                std::uint32_t job_index, const AnalyticFilteredGeometry& geometry,
                                const std::vector<AnalyticCurvePair>& candidate_pairs,
                                const AnalyticSolverLimits& limits = {});

} // namespace geometer
