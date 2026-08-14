#include "geometer/analytic_result_packet_records.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>

namespace
{

using namespace geometer;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void write_u32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value)
{
    for (std::uint32_t index = 0; index < 4; ++index)
        bytes[offset + index] = static_cast<std::uint8_t>(value >> (index * 8U));
}

std::string hex(const std::vector<std::uint8_t>& bytes)
{
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (std::uint8_t byte : bytes)
        out << std::setw(2) << static_cast<unsigned>(byte);
    return out.str();
}

AnalyticResultPacketRecords rich_records()
{
    AnalyticResultPacketRecords records;
    records.job_results = {
        {1, 0, 0, 0, 0, 1, 0, 1},
        {std::numeric_limits<std::uint64_t>::max(), 1, 0, 1, 0, 0, 0, 0},
    };
    records.diagnostics = {{65'547, 1, 15, std::numeric_limits<std::uint64_t>::max(),
                            std::numeric_limits<std::uint64_t>::max() - 1,
                            std::numeric_limits<std::uint64_t>::max() - 2,
                            std::numeric_limits<std::uint64_t>::max() - 3, 26}};
    records.vertices = {
        {1, -10, 0, 0, 0},
        {2, 10, 0, 0, 0},
        {3, 10, 10, 0, 0},
        {4, -10, 10, 0, 0},
    };
    records.fragments = {
        {1, 0, 1, 1, 0, false, 0, 1, 0},
        {2, 1, 2, 1, 0, false, 0, 1, 0},
        {3, 2, 3, 1, 0, false, 0, 1, 0},
        {4, 3, 0, 1, 0, false, 0, 1, 0},
    };
    records.rings = {{1, 0, 4, std::numeric_limits<std::uint32_t>::max(), 0, 0}};
    records.fragment_references = {0, 1, 2, 3};
    records.regions = {{1, 0, 1}};
    records.ring_region_references = {(std::uint64_t{2} << 32U)};
    records.source_sets = {{0, 1}};
    records.source_references = {
        {exact::ExactSourceKind::authored_segment_curve, exact::ExactSourceRole::authored_line, 7,
         std::numeric_limits<std::uint64_t>::max() - 4,
         std::numeric_limits<std::uint64_t>::max() - 5},
    };
    records.operand_events = {
        {7, exact::ExactOperandOutcomeKind::contributes_final_material, 0, 1, 1},
    };
    records.relationship_results = {
        {std::numeric_limits<std::uint64_t>::max(), 0, 3, 0, 1},
    };
    records.relationship_pairs = {{1, 1, 3, true, true, true}};
    records.source_reference_indices = {0};
    return records;
}

void require_decode_failure(const std::vector<std::uint8_t>& bytes, const std::string& message)
{
    AnalyticResultPacketRecordsResult decoded =
        decode_analytic_result_packet_records(bytes.data(), bytes.size());
    require(decoded.error == AnalyticResultPacketLayoutError::invalid_packet && !decoded.value,
            message);
}

} // namespace

int main()
{
    AnalyticResultPacketRecords empty_records;
    AnalyticResultPacketEncodeResult empty = encode_analytic_result_packet_records(empty_records);
    require(empty.error == AnalyticResultPacketLayoutError::none && empty.value &&
                empty.value->size() == 512,
            "typed empty result packet failed");
    AnalyticResultPacketRecordsResult decoded_empty =
        decode_analytic_result_packet_records(empty.value->data(), empty.value->size());
    require(decoded_empty.error == AnalyticResultPacketLayoutError::none && decoded_empty.value,
            "typed empty result packet did not round trip");

    AnalyticResultPacketRecords records = rich_records();
    AnalyticResultPacketEncodeResult encoded = encode_analytic_result_packet_records(records);
    require(encoded.error == AnalyticResultPacketLayoutError::none && encoded.value,
            "typed rich result packet failed to encode");
    AnalyticResultPacketRecordsResult decoded =
        decode_analytic_result_packet_records(encoded.value->data(), encoded.value->size());
    require(decoded.error == AnalyticResultPacketLayoutError::none && decoded.value,
            "typed rich result packet failed to decode");
    AnalyticResultPacketEncodeResult reencoded =
        encode_analytic_result_packet_records(*decoded.value);
    require(reencoded.error == AnalyticResultPacketLayoutError::none && reencoded.value &&
                *reencoded.value == *encoded.value,
            "typed result packet did not re-encode byte exactly");

    AnalyticResultPacketLayoutResult layout =
        decode_analytic_result_packet_layout(encoded.value->data(), encoded.value->size());
    require(layout.error == AnalyticResultPacketLayoutError::none && layout.value,
            "typed fixture layout missing");
    const std::pair<std::size_t, std::uint64_t> reserved_bytes[]{
        {0, 9}, {1, 44}, {3, 19}, {4, 28}, {6, 16}, {9, 4}, {10, 10}, {11, 10}, {12, 20},
    };
    for (const auto& [table, offset] : reserved_bytes)
    {
        std::vector<std::uint8_t> reserved = *encoded.value;
        reserved[layout.value->tables[table].offset + offset] = 1;
        require_decode_failure(reserved, "nonzero typed-record reserved byte was accepted");
    }
    std::vector<std::uint8_t> malformed = *encoded.value;
    malformed = *encoded.value;
    write_u32(malformed, layout.value->tables[2].offset, 2);
    require_decode_failure(malformed, "noncanonical generated vertex id was accepted");
    malformed = *encoded.value;
    write_u32(malformed, layout.value->tables[3].offset + 32, 2);
    require_decode_failure(malformed, "out-of-range fragment source handle was accepted");
    malformed = *encoded.value;
    write_u32(malformed, layout.value->tables[4].offset + 16, 0);
    require_decode_failure(malformed, "self-parented root ring was accepted");
    malformed = *encoded.value;
    malformed[layout.value->tables[9].offset + 2] = 0;
    require_decode_failure(malformed, "cross-kind source role was accepted");
    malformed = *encoded.value;
    malformed[layout.value->tables[12].offset + 17] = 2;
    require_decode_failure(malformed, "non-boolean relationship equality was accepted");

    AnalyticResultPacketRecords missing_error = rich_records();
    missing_error.diagnostics[0].severity = 2;
    require(validate_analytic_result_packet_records(missing_error) ==
                AnalyticResultPacketLayoutError::invalid_packet,
            "failed job without an error diagnostic was accepted");
    AnalyticResultPacketRecords impossible_arc = rich_records();
    impossible_arc.fragments[0].kind = 2;
    impossible_arc.fragments[0].direction = 1;
    impossible_arc.fragments[0].radius_nm = 4;
    require(validate_analytic_result_packet_records(impossible_arc) ==
                AnalyticResultPacketLayoutError::invalid_packet,
            "arc with chord longer than its diameter was accepted");
    impossible_arc.fragments[0].radius_nm = 10;
    impossible_arc.fragments[0].major_arc = true;
    require(validate_analytic_result_packet_records(impossible_arc) ==
                AnalyticResultPacketLayoutError::invalid_packet,
            "major arc with a diameter chord was accepted");
    AnalyticResultPacketRecords cross_job_reference = rich_records();
    cross_job_reference.job_results[0].operand_event_count = 0;
    cross_job_reference.job_results[1].operand_event_count = 1;
    require(validate_analytic_result_packet_records(cross_job_reference) ==
                AnalyticResultPacketLayoutError::invalid_packet,
            "operand event was allowed to reference another job's result");
    AnalyticResultPacketRecords disconnected_ring = rich_records();
    std::swap(disconnected_ring.fragment_references[1], disconnected_ring.fragment_references[2]);
    require(validate_analytic_result_packet_records(disconnected_ring) ==
                AnalyticResultPacketLayoutError::invalid_packet,
            "disconnected ring fragment sequence was accepted");

    std::cout << "ANALYTIC_RESULT_PACKET_RECORD_VECTOR=" << hex(*encoded.value) << '\n';
    return 0;
}
