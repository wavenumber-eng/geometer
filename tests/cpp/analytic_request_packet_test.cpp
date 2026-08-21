#include "geometer/analytic_request_packet.h"

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

std::string hex(const std::vector<std::uint8_t>& bytes)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (const std::uint8_t byte : bytes)
        stream << std::setw(2) << static_cast<unsigned>(byte);
    return stream.str();
}

std::uint64_t read_u64(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    std::uint64_t value = 0;
    for (std::uint32_t index = 0; index < 8; ++index)
        value |= static_cast<std::uint64_t>(bytes[offset + index]) << (index * 8U);
    return value;
}

void write_u32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value)
{
    for (std::uint32_t index = 0; index < 4; ++index)
        bytes[offset + index] = static_cast<std::uint8_t>(value >> (index * 8U));
}

void write_u64(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint64_t value)
{
    for (std::uint32_t index = 0; index < 8; ++index)
        bytes[offset + index] = static_cast<std::uint8_t>(value >> (index * 8U));
}

void require_decode_error(const std::vector<std::uint8_t>& bytes,
                          AnalyticRequestPacketError expected, const std::string& message)
{
    AnalyticRequestPacketRecordsResult result =
        decode_analytic_request_packet(bytes.data(), bytes.size());
    require(result.error == expected && !result.value, message);
}

void require_encode_error(const AnalyticRequestPacketRecords& records,
                          AnalyticRequestPacketError expected, const std::string& message)
{
    require(validate_analytic_request_packet_records(records) == expected, message);
    AnalyticRequestPacketEncodeResult encoded = encode_analytic_request_packet(records);
    require(encoded.error == expected && !encoded.value, message + " (encode)");
}

// One batch exercising every request table: a region with one hole, two
// disks, an annulus, a capsule, a swept open path with a shared-carrier arc
// hole, and two relationship queries across two jobs.
AnalyticRequestPacketRecords build_exemplar()
{
    AnalyticRequestPacketRecords records;
    records.jobs = {{10, 0, 2}, {20, 2, 1}};
    records.stages = {{100, 1, 0, 3}, {101, 2, 3, 2}, {102, 1, 5, 1}};
    records.operands = {{1000, 1, 0}, {1001, 2, 0}, {1002, 3, 0},
                        {1003, 4, 0}, {1004, 5, 0}, {1005, 2, 1}};
    records.planar_regions = {{500, 0, 0, 1}};
    records.ring_references = {1};
    records.rings = {{600, 0, 4, 0, 4, 0}, {601, 4, 2, 4, 2, 0}, {602, 6, 2, 6, 1, 1}};
    records.vertices = {{700, 0, 0},         {701, 10000, 0},    {702, 10000, 10000},
                        {703, 0, 10000},     {704, 4000, 5000},  {705, 6000, 5000},
                        {706, 20000, 20000}, {707, 30000, 20000}};
    records.segments = {{800, 900, 1, 0, false, 0, 0},       {801, 901, 1, 0, false, 0, 0},
                        {802, 902, 1, 0, false, 0, 0},       {803, 903, 1, 0, false, 0, 0},
                        {804, 904, 2, 1, false, 5000, 5000}, {805, 904, 2, 1, false, 5000, 5000},
                        {806, 906, 1, 0, false, 0, 0}};
    records.disks = {{300, 1, 2, 250}, {301, -5, -6, 1'000'000'000'000}};
    records.annuli = {{310, 0, 0, 100, 200}};
    records.capsules = {{320, 0, 0, 1000, 0, 50}};
    records.swept_paths = {{330, 2, 40}};
    records.relationship_queries = {{5000, 10, 20}, {5001, 20, 10}};
    return records;
}

} // namespace

int main()
{
    // Unique canonical empty batch: header + 13 directory entries, no payload.
    AnalyticRequestPacketRecords empty_records;
    AnalyticRequestPacketEncodeResult empty = encode_analytic_request_packet(empty_records);
    require(empty.error == AnalyticRequestPacketError::none && empty.value &&
                empty.value->size() == 480,
            "unique empty request packet encoding failed");
    for (std::size_t index = 0; index < kAnalyticRequestTableCount; ++index)
    {
        const std::size_t entry = 64 + index * 32;
        require((*empty.value)[entry] == index + 1 && (*empty.value)[entry + 2] == 1 &&
                    read_u64(*empty.value, entry + 8) == 480 &&
                    read_u64(*empty.value, entry + 16) == 0 &&
                    read_u64(*empty.value, entry + 24) == 0,
                "empty request directory entry is not canonical");
    }
    AnalyticRequestPacketRecordsResult decoded_empty =
        decode_analytic_request_packet(empty.value->data(), empty.value->size());
    require(decoded_empty.error == AnalyticRequestPacketError::none && decoded_empty.value &&
                decoded_empty.value->jobs.empty() && decoded_empty.value->segments.empty() &&
                decoded_empty.value->relationship_queries.empty(),
            "unique empty request packet decoding failed");
    AnalyticRequestPacketEncodeResult empty_again =
        encode_analytic_request_packet(*decoded_empty.value);
    require(empty_again.value && *empty_again.value == *empty.value,
            "empty request round trip is not byte identical");

    // Full exemplar round trip.
    const AnalyticRequestPacketRecords exemplar = build_exemplar();
    require(validate_analytic_request_packet_records(exemplar) == AnalyticRequestPacketError::none,
            "exemplar records failed structural validation");
    AnalyticRequestPacketEncodeResult encoded = encode_analytic_request_packet(exemplar);
    require(encoded.error == AnalyticRequestPacketError::none && encoded.value &&
                encoded.value->size() == 1608,
            "exemplar request packet encoding failed");
    require(read_u64(*encoded.value, 64 + 4 * 32 + 8) == 800 &&
                read_u64(*encoded.value, 64 + 5 * 32 + 8) == 808 && (*encoded.value)[804] == 0 &&
                (*encoded.value)[805] == 0 && (*encoded.value)[806] == 0 &&
                (*encoded.value)[807] == 0,
            "odd 4-byte ring reference table did not use minimum zero padding");
    AnalyticRequestPacketRecordsResult decoded =
        decode_analytic_request_packet(encoded.value->data(), encoded.value->size());
    require(decoded.error == AnalyticRequestPacketError::none && decoded.value,
            "exemplar request packet decoding failed");
    const AnalyticRequestPacketRecords& round = *decoded.value;
    require(round.jobs.size() == 2 && round.jobs[1].job_id == 20 &&
                round.jobs[1].stage_begin == 2 && round.stages.size() == 3 &&
                round.stages[1].operation == 2 && round.operands.size() == 6 &&
                round.operands[5].geometry_kind == 2 && round.operands[5].geometry_index == 1,
            "exemplar job graph did not round trip");
    require(round.planar_regions.size() == 1 && round.planar_regions[0].hole_reference_count == 1 &&
                round.ring_references.size() == 1 && round.ring_references[0] == 1 &&
                round.rings.size() == 3 && round.rings[2].flags == 1 &&
                round.vertices.size() == 8 && round.vertices[2].y_nm == 10000,
            "exemplar topology did not round trip");
    require(round.segments.size() == 7 && round.segments[4].kind == 2 &&
                round.segments[4].direction == 1 && round.segments[4].curve_id == 904 &&
                round.segments[4].center_x_nm == 5000 && !round.segments[4].major_arc,
            "exemplar segments did not round trip");
    require(round.disks.size() == 2 && round.disks[1].center_x_nm == -5 &&
                round.disks[1].radius_nm == 1'000'000'000'000 && round.annuli.size() == 1 &&
                round.annuli[0].inner_radius_nm == 100 && round.capsules.size() == 1 &&
                round.capsules[0].width_nm == 50 && round.swept_paths.size() == 1 &&
                round.swept_paths[0].path_ring == 2 && round.relationship_queries.size() == 2 &&
                round.relationship_queries[1].right_job_id == 10,
            "exemplar geometry did not round trip");
    AnalyticRequestPacketEncodeResult reencoded = encode_analytic_request_packet(round);
    require(reencoded.value && *reencoded.value == *encoded.value,
            "exemplar request round trip is not byte identical");

    // The pre-release A0 row retains center-form bytes and uses kind 3 with a
    // radius at offset 24 for endpoint/radius authored arcs.
    AnalyticRequestPacketRecords radius_records = exemplar;
    for (std::size_t index : {std::size_t{4}, std::size_t{5}})
    {
        radius_records.segments[index].kind = 3;
        radius_records.segments[index].center_x_nm = 0;
        radius_records.segments[index].center_y_nm = 0;
        radius_records.segments[index].radius_nm = 1000;
    }
    AnalyticRequestPacketEncodeResult radius_encoded =
        encode_analytic_request_packet(radius_records);
    require(radius_encoded.error == AnalyticRequestPacketError::none && radius_encoded.value,
            "endpoint/radius request packet encoding failed");
    AnalyticRequestPacketRecordsResult radius_decoded =
        decode_analytic_request_packet(radius_encoded.value->data(), radius_encoded.value->size());
    require(radius_decoded.error == AnalyticRequestPacketError::none && radius_decoded.value &&
                radius_decoded.value->segments[4].kind == 3 &&
                radius_decoded.value->segments[4].radius_nm == 1000 &&
                radius_decoded.value->segments[4].center_x_nm == 0 &&
                radius_decoded.value->segments[4].center_y_nm == 0,
            "endpoint/radius request packet did not round trip");
    AnalyticRequestPacketEncodeResult radius_reencoded =
        encode_analytic_request_packet(*radius_decoded.value);
    require(radius_reencoded.value && *radius_reencoded.value == *radius_encoded.value,
            "endpoint/radius request packet round trip is not byte identical");
    const std::size_t radius_segment_table =
        static_cast<std::size_t>(read_u64(*radius_encoded.value, 64 + 7 * 32 + 8));
    std::vector<std::uint8_t> malformed_radius = *radius_encoded.value;
    write_u64(malformed_radius, radius_segment_table + 4 * 40 + 24, 0);
    require_decode_error(malformed_radius, AnalyticRequestPacketError::invalid_packet,
                         "zero endpoint/radius arc radius was accepted");
    malformed_radius = *radius_encoded.value;
    malformed_radius[radius_segment_table + 4 * 40 + 32] = 1;
    require_decode_error(malformed_radius, AnalyticRequestPacketError::invalid_packet,
                         "nonzero endpoint/radius reserved bytes were accepted");

    AnalyticRequestPacketRecords path_radius_records = exemplar;
    path_radius_records.segments[6].kind = 3;
    path_radius_records.segments[6].direction = 1;
    path_radius_records.segments[6].radius_nm = 1000;
    require_encode_error(path_radius_records, AnalyticRequestPacketError::invalid_packet,
                         "endpoint/radius arc was accepted in an open swept path");
    std::vector<std::uint8_t> malformed_path_radius = *encoded.value;
    malformed_path_radius[radius_segment_table + 6 * 40 + 16] = 3;
    malformed_path_radius[radius_segment_table + 6 * 40 + 17] = 1;
    write_u64(malformed_path_radius, radius_segment_table + 6 * 40 + 24, 1000);
    require_decode_error(malformed_path_radius, AnalyticRequestPacketError::invalid_packet,
                         "packet endpoint/radius arc was accepted in an open swept path");

    // Header and directory defects reject the batch as invalid_packet.
    std::vector<std::uint8_t> malformed = *encoded.value;
    malformed[0] = 0;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "bad request magic was accepted");
    malformed = *encoded.value;
    malformed[8] = 2;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "bad request generation was accepted");
    malformed = *encoded.value;
    malformed[12] = 1;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "nonzero request header flags were accepted");
    malformed = *encoded.value;
    write_u64(malformed, 16, malformed.size() - 1);
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "bad request total length was accepted");
    malformed = *encoded.value;
    write_u32(malformed, 36, 3);
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "bad header job count was accepted");
    malformed = *encoded.value;
    write_u32(malformed, 40, 1);
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "bad header query count was accepted");
    malformed = *encoded.value;
    malformed[44] = 1;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "nonzero reserved header field was accepted");
    malformed = *encoded.value;
    write_u64(malformed, 48, 1);
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "bad payload byte sum was accepted");
    malformed = *encoded.value;
    malformed[64] = 2;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "bad request table kind was accepted");
    malformed = *encoded.value;
    malformed[66] = 2;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "bad request record version was accepted");
    malformed = *encoded.value;
    malformed[68] = 23;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "bad fixed record size was accepted");
    malformed = *encoded.value;
    write_u64(malformed, 72, 488);
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "noncanonical table offset was accepted");
    malformed = *encoded.value;
    malformed[804] = 1;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "nonzero alignment padding was accepted");
    malformed = *encoded.value;
    malformed.pop_back();
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "truncated request packet was accepted");
    std::vector<std::uint8_t> tiny(479, 0);
    require_decode_error(tiny, AnalyticRequestPacketError::invalid_packet,
                         "under-sized request packet was accepted");
    AnalyticRequestPacketRecordsResult oversized =
        decode_analytic_request_packet(encoded.value->data(), 268'435'457);
    require(oversized.error == AnalyticRequestPacketError::limit_exceeded && !oversized.value,
            "over-limit request packet size was accepted");

    // Record-level reserved and enumeration defects.
    malformed = *encoded.value;
    malformed[496] = 1;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "nonzero job reserved bytes were accepted");
    malformed = *encoded.value;
    malformed[537] = 1;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "nonzero stage reserved bytes were accepted");
    malformed = *encoded.value;
    malformed[536] = 3;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "unknown stage operation was accepted");
    malformed = *encoded.value;
    malformed[634] = 1;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "nonzero operand flags were accepted");
    malformed = *encoded.value;
    malformed[832] = 2;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "unknown ring flag bit was accepted");
    malformed = *encoded.value;
    malformed[1114] = 2;
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "non-boolean major arc byte was accepted");

    // Id-space defects reject the batch as invalid_id.
    malformed = *encoded.value;
    write_u64(malformed, 480, 20);
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_id,
                         "duplicate job id was accepted");
    malformed = *encoded.value;
    write_u64(malformed, 480, 0);
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_id,
                         "zero job id was accepted");
    malformed = *encoded.value;
    write_u64(malformed, 480, 30);
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "descending job order was accepted");

    // Reference defects reject the batch as invalid_reference.
    malformed = *encoded.value;
    write_u32(malformed, 660, 1);
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_reference,
                         "out-of-order geometry ownership was accepted");
    malformed = *encoded.value;
    write_u64(malformed, 1568, 999);
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_reference,
                         "relationship query naming an unknown job was accepted");

    // Value defects: zero magnitudes are packet defects, oversized magnitudes
    // are limit defects.
    malformed = *encoded.value;
    write_u64(malformed, 1400, 0);
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "zero disk radius was accepted");
    malformed = *encoded.value;
    write_u64(malformed, 1432, 1'000'000'000'001);
    require_decode_error(malformed, AnalyticRequestPacketError::limit_exceeded,
                         "over-limit disk radius was accepted");
    malformed = *encoded.value;
    write_u64(malformed, 1464, 200);
    require_decode_error(malformed, AnalyticRequestPacketError::invalid_packet,
                         "degenerate annulus radii were accepted");

    // Records-level validation and fail-closed encoding.
    AnalyticRequestPacketRecords mutated = exemplar;
    mutated.jobs.assign(65'536, AnalyticRequestJobRecord{});
    require_encode_error(mutated, AnalyticRequestPacketError::limit_exceeded,
                         "over-limit job count was accepted");
    mutated = exemplar;
    mutated.planar_regions[0].hole_reference_count = 131'072;
    require_encode_error(mutated, AnalyticRequestPacketError::limit_exceeded,
                         "over-limit region hole count was accepted");
    mutated = exemplar;
    mutated.disks.push_back({999, 0, 0, 1});
    require_encode_error(mutated, AnalyticRequestPacketError::invalid_reference,
                         "unused disk record was accepted");
    mutated = exemplar;
    mutated.rings[0].flags = 1;
    require_encode_error(mutated, AnalyticRequestPacketError::invalid_reference,
                         "open outer boundary ring was accepted");
    mutated = exemplar;
    mutated.rings[2].flags = 0;
    require_encode_error(mutated, AnalyticRequestPacketError::invalid_reference,
                         "closed swept path centerline was accepted");
    mutated = exemplar;
    mutated.segments[0].direction = 1;
    require_encode_error(mutated, AnalyticRequestPacketError::invalid_packet,
                         "line segment with arc direction was accepted");
    mutated = exemplar;
    mutated.segments[4].curve_id = 0;
    require_encode_error(mutated, AnalyticRequestPacketError::invalid_id,
                         "zero curve id was accepted");
    mutated = exemplar;
    mutated.capsules[0].feature_id = 300;
    require_encode_error(mutated, AnalyticRequestPacketError::invalid_id,
                         "duplicate compact feature id across tables was accepted");
    mutated = exemplar;
    mutated.stages[0].operand_count = 2;
    require_encode_error(mutated, AnalyticRequestPacketError::invalid_reference,
                         "gapped operand partition was accepted");

    std::cout << "ANALYTIC_REQUEST_PACKET_EMPTY_VECTOR=" << hex(*empty.value) << '\n';
    std::cout << "ANALYTIC_REQUEST_PACKET_EXEMPLAR_VECTOR=" << hex(*encoded.value) << '\n';
    std::cout << "analytic request packet codec tests passed" << '\n';
    return 0;
}
