#pragma once

#include "geometer/analytic_operand_outcome.h"
#include "geometer/analytic_result_packet_layout.h"
#include "geometer/exact_boolean_provenance.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace geometer
{

struct AnalyticJobResultRecord
{
    std::uint64_t job_id = 0;
    std::uint8_t status = 0;
    std::uint32_t diagnostic_begin = 0;
    std::uint32_t diagnostic_count = 0;
    std::uint32_t result_region_begin = 0;
    std::uint32_t result_region_count = 0;
    std::uint32_t operand_event_begin = 0;
    std::uint32_t operand_event_count = 0;
};

struct AnalyticDiagnosticRecord
{
    std::uint32_t code = 0;
    std::uint8_t severity = 0;
    std::uint16_t presence_flags = 0;
    std::uint64_t job_id = 0;
    std::uint64_t stage_id = 0;
    std::uint64_t operand_id = 0;
    std::uint64_t geometry_source_id = 0;
    std::uint32_t path_token = 0;
};

struct AnalyticResultVertexRecord
{
    std::uint64_t id = 0;
    std::int64_t x_nm = 0;
    std::int64_t y_nm = 0;
    std::uint32_t intersection_source_set = 0;
    std::uint32_t flags = 0;
};

struct AnalyticDirectedFragmentRecord
{
    std::uint64_t id = 0;
    std::uint32_t start_vertex = 0;
    std::uint32_t end_vertex = 0;
    std::uint8_t kind = 0;
    std::uint8_t direction = 0;
    bool major_arc = false;
    std::uint64_t radius_nm = 0;
    std::uint32_t positive_source_set = 0;
    std::uint32_t subtraction_source_set = 0;
};

struct AnalyticResultRingRecord
{
    std::uint64_t id = 0;
    std::uint32_t fragment_reference_begin = 0;
    std::uint32_t fragment_reference_count = 0;
    std::uint32_t parent_ring = 0;
    std::uint32_t depth = 0;
    std::uint32_t flags = 0;
};

struct AnalyticResultRegionRecord
{
    std::uint64_t id = 0;
    std::uint32_t outer_ring = 0;
    std::uint32_t positive_source_set = 0;
};

struct AnalyticPacketSourceSetRecord
{
    std::uint32_t source_reference_index_begin = 0;
    std::uint32_t source_reference_index_count = 0;
};

struct AnalyticOperandEventRecord
{
    std::uint64_t operand_id = 0;
    AnalyticOperandOutcomeKind kind = AnalyticOperandOutcomeKind::no_effect;
    std::uint32_t result_reference_begin = 0;
    std::uint32_t result_reference_count = 0;
    std::uint32_t source_set = 0;
};

struct AnalyticRelationshipResultRecord
{
    std::uint64_t query_id = 0;
    std::uint8_t status = 0;
    std::uint8_t aggregate_dimension = 0;
    std::uint32_t pair_begin = 0;
    std::uint32_t pair_count = 0;
};

struct AnalyticRelationshipPairRecord
{
    std::uint64_t left_result_region_id = 0;
    std::uint64_t right_result_region_id = 0;
    std::uint8_t dimension = 0;
    bool equality = false;
    bool left_contains_right = false;
    bool right_contains_left = false;
};

struct AnalyticResultPacketRecords
{
    std::vector<AnalyticJobResultRecord> job_results;
    std::vector<AnalyticDiagnosticRecord> diagnostics;
    std::vector<AnalyticResultVertexRecord> vertices;
    std::vector<AnalyticDirectedFragmentRecord> fragments;
    std::vector<AnalyticResultRingRecord> rings;
    std::vector<std::uint32_t> fragment_references;
    std::vector<AnalyticResultRegionRecord> regions;
    std::vector<std::uint64_t> ring_region_references;
    std::vector<AnalyticPacketSourceSetRecord> source_sets;
    std::vector<exact::ExactSourceReference> source_references;
    std::vector<AnalyticOperandEventRecord> operand_events;
    std::vector<AnalyticRelationshipResultRecord> relationship_results;
    std::vector<AnalyticRelationshipPairRecord> relationship_pairs;
    std::vector<std::uint32_t> source_reference_indices;
};

struct AnalyticResultPacketRecordsResult
{
    AnalyticResultPacketLayoutError error = AnalyticResultPacketLayoutError::none;
    std::optional<AnalyticResultPacketRecords> value;
};

[[nodiscard]] AnalyticResultPacketLayoutError
validate_analytic_result_packet_records(const AnalyticResultPacketRecords& records);

[[nodiscard]] AnalyticResultPacketRecordsResult
decode_analytic_result_packet_records(const std::uint8_t* data, std::size_t size);

[[nodiscard]] AnalyticResultPacketEncodeResult
encode_analytic_result_packet_records(const AnalyticResultPacketRecords& records);

} // namespace geometer
