#include "geometer/analytic_result_packet_canonical.h"
#include "geometer/analytic_result_packet_records.h"
#include "geometer/analytic_result_packet_standalone.h"
#include "geometer/sha256.h"

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

std::uint32_t read_u32(const std::vector<std::uint8_t>& bytes, std::size_t offset)
{
    std::uint32_t value = 0;
    for (std::uint32_t index = 0; index < 4; ++index)
        value |= static_cast<std::uint32_t>(bytes[offset + index]) << (index * 8U);
    return value;
}

void write_u64(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint64_t value)
{
    for (std::uint32_t index = 0; index < 8; ++index)
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
        {AnalyticSourceKind::authored_segment_curve, AnalyticSourceRole::authored_line, 7,
         std::numeric_limits<std::uint64_t>::max() - 4,
         std::numeric_limits<std::uint64_t>::max() - 5},
    };
    records.operand_events = {
        {7, AnalyticOperandOutcomeKind::contributes_final_material, 0, 1, 1},
    };
    records.relationship_results = {
        {std::numeric_limits<std::uint64_t>::max(), 0, 3, 0, 1},
    };
    records.relationship_pairs = {{1, 1, 3, true, true, true}};
    records.source_reference_indices = {0};
    return records;
}

AnalyticResultPacketRecords deeply_nested_records(std::uint32_t ring_count)
{
    AnalyticResultPacketRecords records;
    records.vertices.reserve(static_cast<std::size_t>(ring_count) * 4);
    records.fragments.reserve(static_cast<std::size_t>(ring_count) * 4);
    records.rings.reserve(ring_count);
    records.fragment_references.reserve(static_cast<std::size_t>(ring_count) * 4);
    records.regions.reserve((ring_count + 1) / 2);
    for (std::uint32_t ring = 0; ring < ring_count; ++ring)
    {
        const std::int64_t radius = static_cast<std::int64_t>(ring_count - ring);
        const std::uint32_t vertex_begin = static_cast<std::uint32_t>(records.vertices.size());
        const bool hole = ring % 2 != 0;
        const std::pair<std::int64_t, std::int64_t> points[] = {
            {-radius, -radius},
            {hole ? -radius : radius, hole ? radius : -radius},
            {radius, radius},
            {hole ? radius : -radius, hole ? -radius : radius}};
        for (const auto& [x, y] : points)
            records.vertices.push_back(
                {static_cast<std::uint64_t>(records.vertices.size()) + 1, x, y, 0, 0});
        const std::uint32_t fragment_begin = static_cast<std::uint32_t>(records.fragments.size());
        for (std::uint32_t offset = 0; offset < 4; ++offset)
        {
            records.fragments.push_back({static_cast<std::uint64_t>(records.fragments.size()) + 1,
                                         vertex_begin + offset, vertex_begin + (offset + 1) % 4, 1,
                                         0, false, 0, 1, 0});
            records.fragment_references.push_back(fragment_begin + offset);
        }
        records.rings.push_back({static_cast<std::uint64_t>(ring) + 1, fragment_begin, 4,
                                 ring == 0 ? std::numeric_limits<std::uint32_t>::max() : ring - 1,
                                 ring, hole ? 1U : 0U});
        if (!hole)
            records.regions.push_back(
                {static_cast<std::uint64_t>(records.regions.size()) + 1, ring, 1});
    }
    records.job_results = {
        {1, 0, 0, 0, 0, static_cast<std::uint32_t>(records.regions.size()), 0, 0}};
    records.source_sets = {{0, 1}};
    records.source_references = {
        {AnalyticSourceKind::authored_segment_curve, AnalyticSourceRole::authored_line, 1, 1, 1}};
    records.source_reference_indices = {0};
    return records;
}

AnalyticResultPacketRecords scrambled_records()
{
    AnalyticResultPacketRecords records = rich_records();
    const std::uint32_t vertex_order[]{2, 0, 3, 1};
    std::vector<std::uint32_t> vertex_map(4);
    std::vector<AnalyticResultVertexRecord> vertices;
    for (std::uint32_t index = 0; index < 4; ++index)
    {
        vertex_map[vertex_order[index]] = index;
        auto value = records.vertices[vertex_order[index]];
        value.id = static_cast<std::uint64_t>(index) + 1;
        vertices.push_back(value);
    }
    records.vertices = std::move(vertices);
    for (auto& fragment : records.fragments)
    {
        fragment.start_vertex = vertex_map[fragment.start_vertex];
        fragment.end_vertex = vertex_map[fragment.end_vertex];
    }
    const std::uint32_t fragment_order[]{2, 0, 3, 1};
    std::vector<std::uint32_t> fragment_map(4);
    std::vector<AnalyticDirectedFragmentRecord> fragments;
    for (std::uint32_t index = 0; index < 4; ++index)
    {
        fragment_map[fragment_order[index]] = index;
        auto value = records.fragments[fragment_order[index]];
        value.id = static_cast<std::uint64_t>(index) + 1;
        fragments.push_back(value);
    }
    records.fragments = std::move(fragments);
    records.fragment_references = {fragment_map[2], fragment_map[3], fragment_map[0],
                                   fragment_map[1]};
    return records;
}

AnalyticResultPacketRecords identical_two_job_records(bool reverse_job_topology)
{
    AnalyticResultPacketRecords records;
    std::uint32_t ring_for_job[2]{};
    const std::uint32_t first = reverse_job_topology ? 1 : 0;
    for (std::uint32_t iteration = 0; iteration < 2; ++iteration)
    {
        const std::uint32_t job = iteration == 0 ? first : 1 - first;
        const std::uint32_t vertex_begin = static_cast<std::uint32_t>(records.vertices.size());
        const std::pair<std::int64_t, std::int64_t> points[] = {
            {-10, 0}, {10, 0}, {10, 10}, {-10, 10}};
        for (const auto& [x, y] : points)
            records.vertices.push_back(
                {static_cast<std::uint64_t>(records.vertices.size()) + 1, x, y, 0, 0});
        const std::uint32_t fragment_begin = static_cast<std::uint32_t>(records.fragments.size());
        for (std::uint32_t offset = 0; offset < 4; ++offset)
        {
            records.fragments.push_back({static_cast<std::uint64_t>(records.fragments.size()) + 1,
                                         vertex_begin + offset, vertex_begin + (offset + 1) % 4, 1,
                                         0, false, 0, 1, 0});
            records.fragment_references.push_back(fragment_begin + offset);
        }
        ring_for_job[job] = static_cast<std::uint32_t>(records.rings.size());
        records.rings.push_back({static_cast<std::uint64_t>(records.rings.size()) + 1,
                                 fragment_begin, 4, std::numeric_limits<std::uint32_t>::max(), 0,
                                 0});
    }
    records.regions = {{1, ring_for_job[0], 1}, {2, ring_for_job[1], 1}};
    records.job_results = {{10, 0, 0, 0, 0, 1, 0, 0}, {20, 0, 0, 0, 1, 1, 0, 0}};
    records.source_sets = {{0, 1}};
    records.source_references = {
        {AnalyticSourceKind::authored_segment_curve, AnalyticSourceRole::authored_line, 1, 1, 1}};
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
    require(sha256_hex(nullptr, 0) ==
                "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            "SHA-256 empty NIST vector failed");
    const std::string abc = "abc";
    require(sha256_hex(reinterpret_cast<const std::uint8_t*>(abc.data()), abc.size()) ==
                "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            "SHA-256 abc NIST vector failed");
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
    AnalyticResultPacketRecords unused_source_set = rich_records();
    unused_source_set.source_sets.push_back({1, 1});
    unused_source_set.source_references.push_back(
        {AnalyticSourceKind::authored_segment_curve, AnalyticSourceRole::authored_line, 8, 1, 1});
    unused_source_set.source_reference_indices.push_back(1);
    require(validate_analytic_result_packet_records(unused_source_set) ==
                AnalyticResultPacketLayoutError::invalid_packet,
            "unreferenced source set was accepted into canonical bytes");
    require(validate_analytic_result_packet_records(deeply_nested_records(16'384)) ==
                AnalyticResultPacketLayoutError::none,
            "materially deep ring hierarchy failed bounded ownership validation");

    AnalyticResultPacketRecordsResult canonical =
        canonicalize_analytic_result_packet_records(records);
    AnalyticResultPacketRecordsResult canonical_scrambled =
        canonicalize_analytic_result_packet_records(scrambled_records());
    require(canonical.error == AnalyticResultPacketLayoutError::none && canonical.value &&
                canonical_scrambled.error == AnalyticResultPacketLayoutError::none &&
                canonical_scrambled.value,
            "semantic canonical projection failed");
    AnalyticResultPacketEncodeResult canonical_bytes =
        encode_analytic_result_packet_records(*canonical.value);
    AnalyticResultPacketEncodeResult canonical_scrambled_bytes =
        encode_analytic_result_packet_records(*canonical_scrambled.value);
    AnalyticResultPacketRecordsResult canonical_twice =
        canonicalize_analytic_result_packet_records(*canonical.value);
    AnalyticResultPacketEncodeResult canonical_twice_bytes =
        canonical_twice.value ? encode_analytic_result_packet_records(*canonical_twice.value)
                              : AnalyticResultPacketEncodeResult{};
    require(canonical_bytes.error == AnalyticResultPacketLayoutError::none &&
                canonical_bytes.value && canonical_scrambled_bytes.value &&
                *canonical_bytes.value == *canonical_scrambled_bytes.value &&
                canonical_twice.error == AnalyticResultPacketLayoutError::none &&
                canonical_twice_bytes.value &&
                *canonical_bytes.value == *canonical_twice_bytes.value,
            "semantic permutation changed canonical result bytes");
    AnalyticResultPacketRecordsResult jobs_forward =
        canonicalize_analytic_result_packet_records(identical_two_job_records(false));
    AnalyticResultPacketRecordsResult jobs_reverse =
        canonicalize_analytic_result_packet_records(identical_two_job_records(true));
    require(jobs_forward.error == AnalyticResultPacketLayoutError::none && jobs_forward.value &&
                jobs_reverse.error == AnalyticResultPacketLayoutError::none && jobs_reverse.value,
            "identical mixed-job canonical projection failed");
    AnalyticResultPacketEncodeResult jobs_forward_bytes =
        encode_analytic_result_packet_records(*jobs_forward.value);
    AnalyticResultPacketEncodeResult jobs_reverse_bytes =
        encode_analytic_result_packet_records(*jobs_reverse.value);
    require(jobs_forward_bytes.value && jobs_reverse_bytes.value &&
                *jobs_forward_bytes.value == *jobs_reverse_bytes.value &&
                jobs_reverse.value->regions[0].outer_ring == 0 &&
                jobs_reverse.value->regions[1].outer_ring == 1,
            "owner job id did not break identical batch geometry ties canonically");
    AnalyticStandaloneJobResult standalone_forward =
        build_analytic_standalone_job(identical_two_job_records(false), 10);
    AnalyticStandaloneJobResult standalone_reverse =
        build_analytic_standalone_job(identical_two_job_records(true), 10);
    require(standalone_forward.error == AnalyticResultPacketLayoutError::none &&
                standalone_forward.value &&
                standalone_reverse.error == AnalyticResultPacketLayoutError::none &&
                standalone_reverse.value &&
                standalone_forward.value->bytes == standalone_reverse.value->bytes &&
                standalone_forward.value->digest_sha256 ==
                    standalone_reverse.value->digest_sha256 &&
                standalone_forward.value->records.job_results.size() == 1 &&
                standalone_forward.value->records.relationship_results.empty(),
            "standalone job changed between mixed-batch layouts");
    AnalyticStandaloneJobResult standalone_alone =
        build_analytic_standalone_job(standalone_forward.value->records, 10);
    require(standalone_alone.error == AnalyticResultPacketLayoutError::none &&
                standalone_alone.value &&
                standalone_alone.value->bytes == standalone_forward.value->bytes &&
                standalone_alone.value->digest_sha256 == standalone_forward.value->digest_sha256,
            "standalone job bytes changed when extracted alone");
    AnalyticStandaloneJobResult standalone_failure =
        build_analytic_standalone_job(records, std::numeric_limits<std::uint64_t>::max());
    require(standalone_failure.error == AnalyticResultPacketLayoutError::none &&
                standalone_failure.value && standalone_failure.value->records.vertices.empty() &&
                standalone_failure.value->records.diagnostics.size() == 1 &&
                standalone_failure.value->digest_sha256.size() == 64,
            "failed standalone job did not close over its diagnostic-only result");

    AnalyticResultPacketLayoutResult canonical_layout = decode_analytic_result_packet_layout(
        canonical_bytes.value->data(), canonical_bytes.value->size());
    require(canonical_layout.error == AnalyticResultPacketLayoutError::none &&
                canonical_layout.value,
            "canonical vector layout missing");
    std::vector<std::uint8_t> alternate_order = *canonical_bytes.value;
    const std::size_t vertex_offset = canonical_layout.value->tables[2].offset;
    for (std::size_t byte = 0; byte < 32; ++byte)
        std::swap(alternate_order[vertex_offset + byte],
                  alternate_order[vertex_offset + 32 + byte]);
    write_u64(alternate_order, vertex_offset, 1);
    write_u64(alternate_order, vertex_offset + 32, 2);
    const std::size_t fragment_offset = canonical_layout.value->tables[3].offset;
    for (std::size_t fragment = 0; fragment < 4; ++fragment)
        for (const std::size_t field : {std::size_t{8}, std::size_t{12}})
        {
            const std::size_t at = fragment_offset + fragment * 48 + field;
            const std::uint32_t value = read_u32(alternate_order, at);
            if (value < 2)
                write_u32(alternate_order, at, 1 - value);
        }
    require_decode_failure(alternate_order,
                           "structurally valid alternate semantic order was accepted");

    std::cout << "ANALYTIC_RESULT_PACKET_RECORD_VECTOR=" << hex(*encoded.value) << '\n';
    std::cout << "ANALYTIC_RESULT_PACKET_CANONICAL_VECTOR=" << hex(*canonical_bytes.value) << '\n';
    std::cout << "ANALYTIC_RESULT_PACKET_STANDALONE_VECTOR=" << hex(standalone_forward.value->bytes)
              << '\n';
    std::cout << "ANALYTIC_RESULT_PACKET_STANDALONE_DIGEST="
              << standalone_forward.value->digest_sha256 << '\n';
    std::cout << "ANALYTIC_RESULT_PACKET_FAILED_STANDALONE_VECTOR="
              << hex(standalone_failure.value->bytes) << '\n';
    std::cout << "ANALYTIC_RESULT_PACKET_FAILED_STANDALONE_DIGEST="
              << standalone_failure.value->digest_sha256 << '\n';
    return 0;
}
