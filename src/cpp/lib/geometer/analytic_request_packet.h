#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace geometer
{

inline constexpr std::size_t kAnalyticRequestTableCount = 13;

// Error classes map one-to-one to the governed contract diagnostics in
// docs/contracts/analytic-planar-boolean-a0-catalog.toml [contract_diagnostic].
// Every nonzero value rejects the whole batch before geometry execution.
enum class AnalyticRequestPacketError : std::uint8_t
{
    none = 0,
    invalid_packet = 1,
    invalid_id = 2,
    invalid_reference = 3,
    limit_exceeded = 4,
};

struct AnalyticRequestJobRecord
{
    std::uint64_t job_id = 0;
    std::uint32_t stage_begin = 0;
    std::uint32_t stage_count = 0;
};

struct AnalyticRequestStageRecord
{
    std::uint64_t stage_id = 0;
    std::uint8_t operation = 0;
    std::uint32_t operand_begin = 0;
    std::uint32_t operand_count = 0;
};

struct AnalyticRequestOperandRecord
{
    std::uint64_t operand_id = 0;
    std::uint16_t geometry_kind = 0;
    std::uint32_t geometry_index = 0;
};

struct AnalyticRequestPlanarRegionRecord
{
    std::uint64_t region_id = 0;
    std::uint32_t outer_ring = 0;
    std::uint32_t hole_reference_begin = 0;
    std::uint32_t hole_reference_count = 0;
};

struct AnalyticRequestRingRecord
{
    std::uint64_t id = 0;
    std::uint32_t vertex_begin = 0;
    std::uint32_t vertex_count = 0;
    std::uint32_t segment_begin = 0;
    std::uint32_t segment_count = 0;
    std::uint32_t flags = 0;
};

struct AnalyticRequestVertexRecord
{
    std::uint64_t id = 0;
    std::int64_t x_nm = 0;
    std::int64_t y_nm = 0;
};

struct AnalyticRequestSegmentRecord
{
    std::uint64_t id = 0;
    std::uint64_t curve_id = 0;
    std::uint8_t kind = 0;
    std::uint8_t direction = 0;
    bool major_arc = false;
    std::int64_t center_x_nm = 0;
    std::int64_t center_y_nm = 0;
    std::uint64_t radius_nm = 0;
};

struct AnalyticRequestDiskRecord
{
    std::uint64_t feature_id = 0;
    std::int64_t center_x_nm = 0;
    std::int64_t center_y_nm = 0;
    std::uint64_t radius_nm = 0;
};

struct AnalyticRequestAnnulusRecord
{
    std::uint64_t feature_id = 0;
    std::int64_t center_x_nm = 0;
    std::int64_t center_y_nm = 0;
    std::uint64_t inner_radius_nm = 0;
    std::uint64_t outer_radius_nm = 0;
};

struct AnalyticRequestCapsuleRecord
{
    std::uint64_t feature_id = 0;
    std::int64_t start_x_nm = 0;
    std::int64_t start_y_nm = 0;
    std::int64_t end_x_nm = 0;
    std::int64_t end_y_nm = 0;
    std::uint64_t width_nm = 0;
};

struct AnalyticRequestSweptPathRecord
{
    std::uint64_t feature_id = 0;
    std::uint32_t path_ring = 0;
    std::uint64_t width_nm = 0;
};

struct AnalyticRequestQueryRecord
{
    std::uint64_t query_id = 0;
    std::uint64_t left_job_id = 0;
    std::uint64_t right_job_id = 0;
};

struct AnalyticRequestPacketRecords
{
    std::vector<AnalyticRequestJobRecord> jobs;
    std::vector<AnalyticRequestStageRecord> stages;
    std::vector<AnalyticRequestOperandRecord> operands;
    std::vector<AnalyticRequestPlanarRegionRecord> planar_regions;
    std::vector<std::uint32_t> ring_references;
    std::vector<AnalyticRequestRingRecord> rings;
    std::vector<AnalyticRequestVertexRecord> vertices;
    std::vector<AnalyticRequestSegmentRecord> segments;
    std::vector<AnalyticRequestDiskRecord> disks;
    std::vector<AnalyticRequestAnnulusRecord> annuli;
    std::vector<AnalyticRequestCapsuleRecord> capsules;
    std::vector<AnalyticRequestSweptPathRecord> swept_paths;
    std::vector<AnalyticRequestQueryRecord> relationship_queries;
};

struct AnalyticRequestPacketRecordsResult
{
    AnalyticRequestPacketError error = AnalyticRequestPacketError::none;
    std::optional<AnalyticRequestPacketRecords> value;
};

struct AnalyticRequestPacketEncodeResult
{
    AnalyticRequestPacketError error = AnalyticRequestPacketError::none;
    std::optional<std::vector<std::uint8_t>> value;
};

// Validates the complete canonical structural graph of an already decoded or
// to-be-encoded record set: id spaces, sorted canonical orders, gapless owned
// range partitions, exactly-once geometry/ring ownership in owner-traversal
// order, cross-space references, and the governed A0 count/value limits.
// Job-local geometric validity (arc coherence, containment, distinct capsule
// endpoints, coordinate span) is deliberately not decided here; it fails the
// isolated job later rather than rejecting the batch.
[[nodiscard]] AnalyticRequestPacketError
validate_analytic_request_packet_records(const AnalyticRequestPacketRecords& records);

// Decodes and fully validates a GMABRQ01 request packet. Any defect rejects
// the whole batch with the mapped contract diagnostic class; no partial value
// is returned.
[[nodiscard]] AnalyticRequestPacketRecordsResult
decode_analytic_request_packet(const std::uint8_t* data, std::size_t size);

// Encodes the unique canonical byte representation. The records must already
// satisfy validate_analytic_request_packet_records; encoding validates first
// and fails closed without producing bytes.
[[nodiscard]] AnalyticRequestPacketEncodeResult
encode_analytic_request_packet(const AnalyticRequestPacketRecords& records);

} // namespace geometer
