#pragma once

#include "geometer/analytic_filtered_normalization.h"
#include "geometer/analytic_result_packet_standalone.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace geometer
{

inline constexpr std::uint32_t kNoAnalyticFilteredPacketIndex =
    std::numeric_limits<std::uint32_t>::max();

enum class AnalyticFilteredPacketError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    resource_limit_exceeded = 2,
    encoding_failed = 3,
};

struct AnalyticFilteredPacketTopologyMaps
{
    std::vector<std::uint32_t> arrangement_vertex_to_packet_vertex;
    std::vector<std::uint32_t> boundary_to_packet_fragment;
    std::vector<std::uint32_t> ring_to_packet_ring;
    std::vector<std::uint32_t> region_to_packet_region;
};

struct AnalyticFilteredPacketTelemetry
{
    std::uint64_t normalization_work_units = 0;
    std::uint64_t normalization_peak_working_memory_bytes = 0;
    std::uint64_t source_range_visits = 0;
    std::uint64_t source_memberships = 0;
    std::uint64_t unique_sources = 0;
    std::uint64_t unique_source_sets = 0;
    std::uint64_t sequence_nodes = 0;
    std::uint64_t sequence_table_probes = 0;
    std::uint64_t canonical_sort_work_units = 0;
    std::uint64_t emitted_vertices = 0;
    std::uint64_t emitted_fragments = 0;
    std::uint64_t emitted_rings = 0;
    std::uint64_t emitted_regions = 0;
    std::uint64_t emitted_events = 0;
    std::uint64_t emitted_packet_bytes = 0;
    std::uint64_t retained_records_bytes = 0;
    std::uint64_t required_working_memory_bytes = 0;
    std::uint64_t encoding_peak_working_memory_bytes = 0;
    std::uint64_t reserved_packet_work_units = 0;
    std::uint64_t reserved_packet_memory_bytes = 0;
    std::uint64_t packet_work_units = 0;
    std::uint64_t predicate_calls = 0;
    std::uint64_t peak_working_memory_bytes = 0;
    std::uint64_t algebraic_fallback_calls = 0;
};

struct AnalyticFilteredJobPacketResult
{
    AnalyticFilteredPacketError error = AnalyticFilteredPacketError::none;
    AnalyticFilteredNormalizationError normalization_error =
        AnalyticFilteredNormalizationError::none;
    std::optional<AnalyticStandaloneJob> standalone;
    AnalyticFilteredPacketTopologyMaps maps;
    AnalyticFilteredPacketTelemetry telemetry;
};

struct AnalyticFilteredJobRecordsResult
{
    AnalyticFilteredPacketError error = AnalyticFilteredPacketError::none;
    AnalyticFilteredNormalizationError normalization_error =
        AnalyticFilteredNormalizationError::none;
    std::optional<AnalyticResultPacketRecords> records;
    AnalyticFilteredPacketTopologyMaps maps;
    AnalyticFilteredPacketTelemetry telemetry;
};

// Records-only owned publication path used by the batch orchestrator. It
// performs the same normalization, provenance projection, source interning,
// and canonical record construction as the standalone path, but deliberately
// does not serialize or hash a per-job packet.
[[nodiscard]] AnalyticFilteredJobRecordsResult build_analytic_filtered_job_records(
    const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
    const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& candidate_pairs,
    const AnalyticSolverLimits& limits = {});

// Owned production publication stage. It invokes the complete filtered
// normalization pipeline, projects lineage/outcomes through the explicit
// topology maps, interns source sets, assigns canonical one-based packet ids,
// and emits one standalone packet plus its SHA-256 digest. It never accepts a
// caller-constructed normalized topology and never invokes the algebraic
// solver or its result normalization.
[[nodiscard]] AnalyticFilteredJobPacketResult build_analytic_filtered_job_packet(
    const AnalyticRequestPacketRecords& records, std::uint32_t job_index,
    const AnalyticFilteredGeometry& geometry, const std::vector<AnalyticCurvePair>& candidate_pairs,
    const AnalyticSolverLimits& limits = {});

} // namespace geometer
