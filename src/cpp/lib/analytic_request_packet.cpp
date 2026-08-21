#include "geometer/analytic_request_packet.h"

#include <algorithm>
#include <array>
#include <exception>
#include <limits>
#include <stdexcept>

namespace geometer
{
namespace
{

constexpr std::array<std::uint8_t, 8> kMagic{'G', 'M', 'A', 'B', 'R', 'Q', '0', '1'};
constexpr std::uint64_t kHeaderBytes = 64;
constexpr std::uint64_t kDirectoryEntryBytes = 32;
constexpr std::uint64_t kDirectoryBytes = kDirectoryEntryBytes * kAnalyticRequestTableCount;
constexpr std::uint64_t kPayloadStart = kHeaderBytes + kDirectoryBytes;
constexpr std::uint64_t kMaximumPacketBytes = 268'435'456;
constexpr std::uint64_t kMaximumJobs = 65'535;
constexpr std::uint64_t kMaximumStages = 1'048'576;
constexpr std::uint64_t kMaximumOperands = 4'194'304;
constexpr std::uint64_t kMaximumQueries = 1'048'576;
constexpr std::uint64_t kMaximumRingVertices = 131'072;
constexpr std::uint64_t kMaximumPathSegments = 131'072;
constexpr std::uint64_t kMaximumRegionHoles = 131'071;
constexpr std::uint64_t kMaximumLengthNm = 1'000'000'000'000;
constexpr std::array<std::uint32_t, kAnalyticRequestTableCount> kRecordBytes{
    24, 32, 24, 32, 4, 32, 24, 40, 32, 40, 48, 32, 24,
};

AnalyticRequestPacketRecordsResult decode_failure(AnalyticRequestPacketError error)
{
    return {error, std::nullopt};
}

AnalyticRequestPacketEncodeResult encode_failure(AnalyticRequestPacketError error)
{
    return {error, std::nullopt};
}

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error("analytic request packet addition overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("analytic request packet multiplication overflow");
    return left * right;
}

std::uint64_t align_eight(std::uint64_t value)
{
    return checked_add(value, 7U) & ~std::uint64_t{7};
}

std::uint16_t read_u16(const std::uint8_t* data)
{
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[0]) |
                                      (static_cast<std::uint16_t>(data[1]) << 8U));
}

std::uint32_t read_u32(const std::uint8_t* data)
{
    std::uint32_t value = 0;
    for (std::uint32_t index = 0; index < 4; ++index)
        value |= static_cast<std::uint32_t>(data[index]) << (index * 8U);
    return value;
}

std::uint64_t read_u64(const std::uint8_t* data)
{
    std::uint64_t value = 0;
    for (std::uint32_t index = 0; index < 8; ++index)
        value |= static_cast<std::uint64_t>(data[index]) << (index * 8U);
    return value;
}

std::int64_t read_i64(const std::uint8_t* data)
{
    return static_cast<std::int64_t>(read_u64(data));
}

void write_u16(std::vector<std::uint8_t>& output, std::size_t offset, std::uint16_t value)
{
    for (std::uint32_t index = 0; index < 2; ++index)
        output[offset + index] = static_cast<std::uint8_t>(value >> (index * 8U));
}

void write_u32(std::vector<std::uint8_t>& output, std::size_t offset, std::uint32_t value)
{
    for (std::uint32_t index = 0; index < 4; ++index)
        output[offset + index] = static_cast<std::uint8_t>(value >> (index * 8U));
}

void write_u64(std::vector<std::uint8_t>& output, std::size_t offset, std::uint64_t value)
{
    for (std::uint32_t index = 0; index < 8; ++index)
        output[offset + index] = static_cast<std::uint8_t>(value >> (index * 8U));
}

void write_i64(std::vector<std::uint8_t>& output, std::size_t offset, std::int64_t value)
{
    write_u64(output, offset, static_cast<std::uint64_t>(value));
}

bool zero_range(const std::uint8_t* data, std::uint64_t begin, std::uint64_t end)
{
    for (std::uint64_t index = begin; index < end; ++index)
        if (data[index] != 0)
            return false;
    return true;
}

} // namespace

namespace
{

bool unique_ids(std::vector<std::uint64_t>& ids)
{
    std::sort(ids.begin(), ids.end());
    return std::adjacent_find(ids.begin(), ids.end()) == ids.end();
}

AnalyticRequestPacketError validate_jobs(const AnalyticRequestPacketRecords& records,
                                         std::vector<std::uint64_t>& job_ids)
{
    job_ids.reserve(records.jobs.size());
    std::uint64_t previous_job_id = 0;
    std::uint64_t stage_cursor = 0;
    for (const AnalyticRequestJobRecord& job : records.jobs)
    {
        if (job.job_id == 0)
            return AnalyticRequestPacketError::invalid_id;
        job_ids.push_back(job.job_id);
        if (job.job_id < previous_job_id)
            return AnalyticRequestPacketError::invalid_packet;
        previous_job_id = job.job_id;
        if (job.stage_begin != stage_cursor)
            return AnalyticRequestPacketError::invalid_reference;
        stage_cursor += job.stage_count;
        if (stage_cursor > records.stages.size())
            return AnalyticRequestPacketError::invalid_reference;
    }
    if (stage_cursor != records.stages.size())
        return AnalyticRequestPacketError::invalid_reference;
    return unique_ids(job_ids) ? AnalyticRequestPacketError::none
                               : AnalyticRequestPacketError::invalid_id;
}

AnalyticRequestPacketError validate_stages(const AnalyticRequestPacketRecords& records)
{
    std::vector<std::uint64_t> stage_ids;
    stage_ids.reserve(records.stages.size());
    std::uint64_t operand_cursor = 0;
    for (const AnalyticRequestStageRecord& stage : records.stages)
    {
        if (stage.stage_id == 0)
            return AnalyticRequestPacketError::invalid_id;
        stage_ids.push_back(stage.stage_id);
        if (stage.operation != 1 && stage.operation != 2)
            return AnalyticRequestPacketError::invalid_packet;
        if (stage.operand_begin != operand_cursor)
            return AnalyticRequestPacketError::invalid_reference;
        operand_cursor += stage.operand_count;
        if (operand_cursor > records.operands.size())
            return AnalyticRequestPacketError::invalid_reference;
    }
    if (operand_cursor != records.operands.size())
        return AnalyticRequestPacketError::invalid_reference;
    return unique_ids(stage_ids) ? AnalyticRequestPacketError::none
                                 : AnalyticRequestPacketError::invalid_id;
}

AnalyticRequestPacketError validate_operands(const AnalyticRequestPacketRecords& records)
{
    std::vector<std::uint64_t> operand_ids;
    operand_ids.reserve(records.operands.size());
    std::array<std::uint64_t, 5> geometry_cursor{};
    const std::array<std::uint64_t, 5> geometry_sizes{
        records.planar_regions.size(), records.disks.size(),       records.annuli.size(),
        records.capsules.size(),       records.swept_paths.size(),
    };
    for (const AnalyticRequestOperandRecord& operand : records.operands)
    {
        if (operand.operand_id == 0)
            return AnalyticRequestPacketError::invalid_id;
        operand_ids.push_back(operand.operand_id);
        if (operand.geometry_kind < 1 || operand.geometry_kind > 5)
            return AnalyticRequestPacketError::invalid_packet;
        const std::size_t kind = static_cast<std::size_t>(operand.geometry_kind - 1U);
        if (operand.geometry_index != geometry_cursor[kind] ||
            operand.geometry_index >= geometry_sizes[kind])
            return AnalyticRequestPacketError::invalid_reference;
        ++geometry_cursor[kind];
    }
    for (std::size_t kind = 0; kind < geometry_sizes.size(); ++kind)
        if (geometry_cursor[kind] != geometry_sizes[kind])
            return AnalyticRequestPacketError::invalid_reference;
    if (!unique_ids(operand_ids))
        return AnalyticRequestPacketError::invalid_id;
    for (const AnalyticRequestStageRecord& stage : records.stages)
        for (std::uint32_t index = 1; index < stage.operand_count; ++index)
            if (records.operands[stage.operand_begin + index - 1U].operand_id >
                records.operands[stage.operand_begin + index].operand_id)
                return AnalyticRequestPacketError::invalid_packet;
    return AnalyticRequestPacketError::none;
}

AnalyticRequestPacketError validate_regions(const AnalyticRequestPacketRecords& records)
{
    std::vector<std::uint64_t> region_ids;
    region_ids.reserve(records.planar_regions.size());
    std::uint64_t hole_reference_cursor = 0;
    for (const AnalyticRequestPlanarRegionRecord& region : records.planar_regions)
    {
        if (region.region_id == 0)
            return AnalyticRequestPacketError::invalid_id;
        region_ids.push_back(region.region_id);
        if (region.hole_reference_count > kMaximumRegionHoles)
            return AnalyticRequestPacketError::limit_exceeded;
        if (region.hole_reference_begin != hole_reference_cursor)
            return AnalyticRequestPacketError::invalid_reference;
        hole_reference_cursor += region.hole_reference_count;
        if (hole_reference_cursor > records.ring_references.size())
            return AnalyticRequestPacketError::invalid_reference;
    }
    if (hole_reference_cursor != records.ring_references.size())
        return AnalyticRequestPacketError::invalid_reference;
    return unique_ids(region_ids) ? AnalyticRequestPacketError::none
                                  : AnalyticRequestPacketError::invalid_id;
}

AnalyticRequestPacketError validate_ring_ownership(const AnalyticRequestPacketRecords& records)
{
    std::uint64_t ring_cursor = 0;
    for (const AnalyticRequestOperandRecord& operand : records.operands)
    {
        if (operand.geometry_kind == 1)
        {
            const AnalyticRequestPlanarRegionRecord& region =
                records.planar_regions[operand.geometry_index];
            if (ring_cursor >= records.rings.size() || region.outer_ring != ring_cursor ||
                (records.rings[region.outer_ring].flags & 1U) != 0)
                return AnalyticRequestPacketError::invalid_reference;
            ++ring_cursor;
            for (std::uint32_t index = 0; index < region.hole_reference_count; ++index)
            {
                const std::uint32_t reference =
                    records.ring_references[region.hole_reference_begin + index];
                if (ring_cursor >= records.rings.size() || reference != ring_cursor ||
                    (records.rings[reference].flags & 1U) != 0)
                    return AnalyticRequestPacketError::invalid_reference;
                ++ring_cursor;
            }
        }
        else if (operand.geometry_kind == 5)
        {
            const AnalyticRequestSweptPathRecord& swept =
                records.swept_paths[operand.geometry_index];
            if (ring_cursor >= records.rings.size() || swept.path_ring != ring_cursor ||
                (records.rings[swept.path_ring].flags & 1U) == 0)
                return AnalyticRequestPacketError::invalid_reference;
            ++ring_cursor;
        }
    }
    return ring_cursor == records.rings.size() ? AnalyticRequestPacketError::none
                                               : AnalyticRequestPacketError::invalid_reference;
}

AnalyticRequestPacketError validate_rings(const AnalyticRequestPacketRecords& records)
{
    std::vector<std::uint64_t> ring_ids;
    std::vector<std::uint64_t> path_ids;
    ring_ids.reserve(records.rings.size());
    path_ids.reserve(records.rings.size());
    std::uint64_t vertex_cursor = 0;
    std::uint64_t segment_cursor = 0;
    for (const AnalyticRequestRingRecord& ring : records.rings)
    {
        if ((ring.flags & ~1U) != 0)
            return AnalyticRequestPacketError::invalid_packet;
        const bool open_path = (ring.flags & 1U) != 0;
        if (ring.id == 0)
            return AnalyticRequestPacketError::invalid_id;
        (open_path ? path_ids : ring_ids).push_back(ring.id);
        if (ring.vertex_begin != vertex_cursor)
            return AnalyticRequestPacketError::invalid_reference;
        vertex_cursor += ring.vertex_count;
        if (vertex_cursor > records.vertices.size())
            return AnalyticRequestPacketError::invalid_reference;
        if (ring.segment_begin != segment_cursor)
            return AnalyticRequestPacketError::invalid_reference;
        segment_cursor += ring.segment_count;
        if (segment_cursor > records.segments.size())
            return AnalyticRequestPacketError::invalid_reference;
        if (open_path)
        {
            for (std::uint64_t index = ring.segment_begin; index < segment_cursor; ++index)
                if (records.segments[index].kind == 3)
                    return AnalyticRequestPacketError::invalid_packet;
            if (ring.segment_count == 0 ||
                static_cast<std::uint64_t>(ring.vertex_count) != ring.segment_count + 1ULL)
                return AnalyticRequestPacketError::invalid_packet;
            if (ring.segment_count > kMaximumPathSegments)
                return AnalyticRequestPacketError::limit_exceeded;
        }
        else
        {
            if (ring.vertex_count < 2 || ring.vertex_count != ring.segment_count)
                return AnalyticRequestPacketError::invalid_packet;
            if (ring.vertex_count > kMaximumRingVertices)
                return AnalyticRequestPacketError::limit_exceeded;
        }
    }
    if (vertex_cursor != records.vertices.size() || segment_cursor != records.segments.size())
        return AnalyticRequestPacketError::invalid_reference;
    return unique_ids(ring_ids) && unique_ids(path_ids) ? AnalyticRequestPacketError::none
                                                        : AnalyticRequestPacketError::invalid_id;
}

AnalyticRequestPacketError
validate_vertices_and_segments(const AnalyticRequestPacketRecords& records)
{
    std::vector<std::uint64_t> vertex_ids;
    vertex_ids.reserve(records.vertices.size());
    for (const AnalyticRequestVertexRecord& vertex : records.vertices)
    {
        if (vertex.id == 0)
            return AnalyticRequestPacketError::invalid_id;
        vertex_ids.push_back(vertex.id);
    }
    if (!unique_ids(vertex_ids))
        return AnalyticRequestPacketError::invalid_id;

    std::vector<std::uint64_t> segment_ids;
    segment_ids.reserve(records.segments.size());
    for (const AnalyticRequestSegmentRecord& segment : records.segments)
    {
        if (segment.id == 0 || segment.curve_id == 0)
            return AnalyticRequestPacketError::invalid_id;
        segment_ids.push_back(segment.id);
        if (segment.kind == 1)
        {
            if (segment.direction != 0 || segment.major_arc || segment.center_x_nm != 0 ||
                segment.center_y_nm != 0 || segment.radius_nm != 0)
                return AnalyticRequestPacketError::invalid_packet;
        }
        else if (segment.kind == 2)
        {
            if ((segment.direction != 1 && segment.direction != 2) || segment.radius_nm != 0)
                return AnalyticRequestPacketError::invalid_packet;
        }
        else if (segment.kind == 3)
        {
            if ((segment.direction != 1 && segment.direction != 2) || segment.center_x_nm != 0 ||
                segment.center_y_nm != 0 || segment.radius_nm == 0)
                return AnalyticRequestPacketError::invalid_packet;
            if (segment.radius_nm > kMaximumLengthNm)
                return AnalyticRequestPacketError::limit_exceeded;
        }
        else
        {
            return AnalyticRequestPacketError::invalid_packet;
        }
    }
    return unique_ids(segment_ids) ? AnalyticRequestPacketError::none
                                   : AnalyticRequestPacketError::invalid_id;
}

AnalyticRequestPacketError validate_features(const AnalyticRequestPacketRecords& records)
{
    std::vector<std::uint64_t> feature_ids;
    feature_ids.reserve(records.disks.size() + records.annuli.size() + records.capsules.size() +
                        records.swept_paths.size());
    for (const AnalyticRequestDiskRecord& disk : records.disks)
    {
        if (disk.feature_id == 0)
            return AnalyticRequestPacketError::invalid_id;
        feature_ids.push_back(disk.feature_id);
        if (disk.radius_nm == 0)
            return AnalyticRequestPacketError::invalid_packet;
        if (disk.radius_nm > kMaximumLengthNm)
            return AnalyticRequestPacketError::limit_exceeded;
    }
    for (const AnalyticRequestAnnulusRecord& annulus : records.annuli)
    {
        if (annulus.feature_id == 0)
            return AnalyticRequestPacketError::invalid_id;
        feature_ids.push_back(annulus.feature_id);
        if (annulus.inner_radius_nm == 0 || annulus.inner_radius_nm >= annulus.outer_radius_nm)
            return AnalyticRequestPacketError::invalid_packet;
        if (annulus.outer_radius_nm > kMaximumLengthNm)
            return AnalyticRequestPacketError::limit_exceeded;
    }
    for (const AnalyticRequestCapsuleRecord& capsule : records.capsules)
    {
        if (capsule.feature_id == 0)
            return AnalyticRequestPacketError::invalid_id;
        feature_ids.push_back(capsule.feature_id);
        if (capsule.width_nm == 0)
            return AnalyticRequestPacketError::invalid_packet;
        if (capsule.width_nm > kMaximumLengthNm)
            return AnalyticRequestPacketError::limit_exceeded;
    }
    for (const AnalyticRequestSweptPathRecord& swept : records.swept_paths)
    {
        if (swept.feature_id == 0)
            return AnalyticRequestPacketError::invalid_id;
        feature_ids.push_back(swept.feature_id);
        if (swept.width_nm == 0)
            return AnalyticRequestPacketError::invalid_packet;
        if (swept.width_nm > kMaximumLengthNm)
            return AnalyticRequestPacketError::limit_exceeded;
    }
    return unique_ids(feature_ids) ? AnalyticRequestPacketError::none
                                   : AnalyticRequestPacketError::invalid_id;
}

AnalyticRequestPacketError validate_queries(const AnalyticRequestPacketRecords& records,
                                            const std::vector<std::uint64_t>& job_ids)
{
    std::uint64_t previous_query_id = 0;
    for (const AnalyticRequestQueryRecord& query : records.relationship_queries)
    {
        if (query.query_id == 0 || query.query_id == previous_query_id)
            return AnalyticRequestPacketError::invalid_id;
        if (query.query_id < previous_query_id)
            return AnalyticRequestPacketError::invalid_packet;
        previous_query_id = query.query_id;
        if (!std::binary_search(job_ids.begin(), job_ids.end(), query.left_job_id) ||
            !std::binary_search(job_ids.begin(), job_ids.end(), query.right_job_id))
            return AnalyticRequestPacketError::invalid_reference;
    }
    return AnalyticRequestPacketError::none;
}

} // namespace

AnalyticRequestPacketError
validate_analytic_request_packet_records(const AnalyticRequestPacketRecords& records)
{
    if (records.jobs.size() > kMaximumJobs || records.stages.size() > kMaximumStages ||
        records.operands.size() > kMaximumOperands ||
        records.relationship_queries.size() > kMaximumQueries)
        return AnalyticRequestPacketError::limit_exceeded;

    std::vector<std::uint64_t> job_ids;
    AnalyticRequestPacketError error = validate_jobs(records, job_ids);
    if (error != AnalyticRequestPacketError::none)
        return error;
    error = validate_stages(records);
    if (error != AnalyticRequestPacketError::none)
        return error;
    error = validate_operands(records);
    if (error != AnalyticRequestPacketError::none)
        return error;
    error = validate_regions(records);
    if (error != AnalyticRequestPacketError::none)
        return error;
    error = validate_ring_ownership(records);
    if (error != AnalyticRequestPacketError::none)
        return error;
    error = validate_rings(records);
    if (error != AnalyticRequestPacketError::none)
        return error;
    error = validate_vertices_and_segments(records);
    if (error != AnalyticRequestPacketError::none)
        return error;
    error = validate_features(records);
    if (error != AnalyticRequestPacketError::none)
        return error;
    return validate_queries(records, job_ids);
}

AnalyticRequestPacketRecordsResult decode_analytic_request_packet(const std::uint8_t* data,
                                                                  std::size_t size)
{
    try
    {
        if (data == nullptr || size < kPayloadStart)
            return decode_failure(AnalyticRequestPacketError::invalid_packet);
        if (size > kMaximumPacketBytes)
            return decode_failure(AnalyticRequestPacketError::limit_exceeded);
        if (!std::equal(kMagic.begin(), kMagic.end(), data) || read_u16(data + 8) != 1 ||
            read_u16(data + 10) != kHeaderBytes || read_u32(data + 12) != 0 ||
            read_u64(data + 16) != size || read_u64(data + 24) != kHeaderBytes ||
            read_u32(data + 32) != kAnalyticRequestTableCount || read_u32(data + 44) != 0 ||
            read_u64(data + 56) != 0)
            return decode_failure(AnalyticRequestPacketError::invalid_packet);

        std::array<std::uint64_t, kAnalyticRequestTableCount> offsets{};
        std::array<std::uint64_t, kAnalyticRequestTableCount> counts{};
        std::uint64_t cursor = kPayloadStart;
        std::uint64_t payload_sum = 0;
        for (std::size_t index = 0; index < kAnalyticRequestTableCount; ++index)
        {
            const std::uint8_t* entry = data + kHeaderBytes + index * kDirectoryEntryBytes;
            const std::uint16_t kind = read_u16(entry);
            const std::uint16_t version = read_u16(entry + 2);
            const std::uint32_t record_bytes = read_u32(entry + 4);
            const std::uint64_t offset = read_u64(entry + 8);
            const std::uint64_t byte_length = read_u64(entry + 16);
            const std::uint64_t record_count = read_u64(entry + 24);
            const std::uint64_t expected_offset = align_eight(cursor);
            const std::uint64_t expected_length = checked_multiply(record_count, record_bytes);
            const std::uint64_t range_end = checked_add(offset, byte_length);
            if (kind != 1U + index || version != 1 || record_bytes != kRecordBytes[index] ||
                offset != expected_offset || offset % 8U != 0 || byte_length != expected_length ||
                range_end > size || !zero_range(data, cursor, offset))
                return decode_failure(AnalyticRequestPacketError::invalid_packet);
            payload_sum = checked_add(payload_sum, byte_length);
            offsets[index] = offset;
            counts[index] = record_count;
            cursor = range_end;
        }
        if (cursor != size || payload_sum != read_u64(data + 48) ||
            counts[0] != read_u32(data + 36) || counts[12] != read_u32(data + 40))
            return decode_failure(AnalyticRequestPacketError::invalid_packet);

        AnalyticRequestPacketRecords records;
        records.jobs.reserve(static_cast<std::size_t>(counts[0]));
        for (std::uint64_t index = 0; index < counts[0]; ++index)
        {
            const std::uint8_t* record = data + offsets[0] + index * kRecordBytes[0];
            AnalyticRequestJobRecord job;
            job.job_id = read_u64(record);
            job.stage_begin = read_u32(record + 8);
            job.stage_count = read_u32(record + 12);
            if (read_u64(record + 16) != 0)
                return decode_failure(AnalyticRequestPacketError::invalid_packet);
            records.jobs.push_back(job);
        }
        records.stages.reserve(static_cast<std::size_t>(counts[1]));
        for (std::uint64_t index = 0; index < counts[1]; ++index)
        {
            const std::uint8_t* record = data + offsets[1] + index * kRecordBytes[1];
            AnalyticRequestStageRecord stage;
            stage.stage_id = read_u64(record);
            stage.operation = record[8];
            if (!zero_range(record, 9, 16) || read_u64(record + 24) != 0)
                return decode_failure(AnalyticRequestPacketError::invalid_packet);
            stage.operand_begin = read_u32(record + 16);
            stage.operand_count = read_u32(record + 20);
            records.stages.push_back(stage);
        }
        records.operands.reserve(static_cast<std::size_t>(counts[2]));
        for (std::uint64_t index = 0; index < counts[2]; ++index)
        {
            const std::uint8_t* record = data + offsets[2] + index * kRecordBytes[2];
            AnalyticRequestOperandRecord operand;
            operand.operand_id = read_u64(record);
            operand.geometry_kind = read_u16(record + 8);
            if (read_u16(record + 10) != 0 || read_u64(record + 16) != 0)
                return decode_failure(AnalyticRequestPacketError::invalid_packet);
            operand.geometry_index = read_u32(record + 12);
            records.operands.push_back(operand);
        }
        records.planar_regions.reserve(static_cast<std::size_t>(counts[3]));
        for (std::uint64_t index = 0; index < counts[3]; ++index)
        {
            const std::uint8_t* record = data + offsets[3] + index * kRecordBytes[3];
            AnalyticRequestPlanarRegionRecord region;
            region.region_id = read_u64(record);
            region.outer_ring = read_u32(record + 8);
            region.hole_reference_begin = read_u32(record + 12);
            region.hole_reference_count = read_u32(record + 16);
            if (read_u32(record + 20) != 0 || read_u64(record + 24) != 0)
                return decode_failure(AnalyticRequestPacketError::invalid_packet);
            records.planar_regions.push_back(region);
        }
        records.ring_references.reserve(static_cast<std::size_t>(counts[4]));
        for (std::uint64_t index = 0; index < counts[4]; ++index)
            records.ring_references.push_back(
                read_u32(data + offsets[4] + index * kRecordBytes[4]));
        records.rings.reserve(static_cast<std::size_t>(counts[5]));
        for (std::uint64_t index = 0; index < counts[5]; ++index)
        {
            const std::uint8_t* record = data + offsets[5] + index * kRecordBytes[5];
            AnalyticRequestRingRecord ring;
            ring.id = read_u64(record);
            ring.vertex_begin = read_u32(record + 8);
            ring.vertex_count = read_u32(record + 12);
            ring.segment_begin = read_u32(record + 16);
            ring.segment_count = read_u32(record + 20);
            ring.flags = read_u32(record + 24);
            if (ring.flags > 1U || read_u32(record + 28) != 0)
                return decode_failure(AnalyticRequestPacketError::invalid_packet);
            records.rings.push_back(ring);
        }
        records.vertices.reserve(static_cast<std::size_t>(counts[6]));
        for (std::uint64_t index = 0; index < counts[6]; ++index)
        {
            const std::uint8_t* record = data + offsets[6] + index * kRecordBytes[6];
            AnalyticRequestVertexRecord vertex;
            vertex.id = read_u64(record);
            vertex.x_nm = read_i64(record + 8);
            vertex.y_nm = read_i64(record + 16);
            records.vertices.push_back(vertex);
        }
        records.segments.reserve(static_cast<std::size_t>(counts[7]));
        for (std::uint64_t index = 0; index < counts[7]; ++index)
        {
            const std::uint8_t* record = data + offsets[7] + index * kRecordBytes[7];
            AnalyticRequestSegmentRecord segment;
            segment.id = read_u64(record);
            segment.curve_id = read_u64(record + 8);
            segment.kind = record[16];
            segment.direction = record[17];
            if (record[18] > 1U || !zero_range(record, 19, 24))
                return decode_failure(AnalyticRequestPacketError::invalid_packet);
            segment.major_arc = record[18] == 1U;
            if (segment.kind == 3)
            {
                segment.radius_nm = read_u64(record + 24);
                if (!zero_range(record, 32, 40))
                    return decode_failure(AnalyticRequestPacketError::invalid_packet);
            }
            else
            {
                segment.center_x_nm = read_i64(record + 24);
                segment.center_y_nm = read_i64(record + 32);
            }
            records.segments.push_back(segment);
        }
        records.disks.reserve(static_cast<std::size_t>(counts[8]));
        for (std::uint64_t index = 0; index < counts[8]; ++index)
        {
            const std::uint8_t* record = data + offsets[8] + index * kRecordBytes[8];
            AnalyticRequestDiskRecord disk;
            disk.feature_id = read_u64(record);
            disk.center_x_nm = read_i64(record + 8);
            disk.center_y_nm = read_i64(record + 16);
            disk.radius_nm = read_u64(record + 24);
            records.disks.push_back(disk);
        }
        records.annuli.reserve(static_cast<std::size_t>(counts[9]));
        for (std::uint64_t index = 0; index < counts[9]; ++index)
        {
            const std::uint8_t* record = data + offsets[9] + index * kRecordBytes[9];
            AnalyticRequestAnnulusRecord annulus;
            annulus.feature_id = read_u64(record);
            annulus.center_x_nm = read_i64(record + 8);
            annulus.center_y_nm = read_i64(record + 16);
            annulus.inner_radius_nm = read_u64(record + 24);
            annulus.outer_radius_nm = read_u64(record + 32);
            records.annuli.push_back(annulus);
        }
        records.capsules.reserve(static_cast<std::size_t>(counts[10]));
        for (std::uint64_t index = 0; index < counts[10]; ++index)
        {
            const std::uint8_t* record = data + offsets[10] + index * kRecordBytes[10];
            AnalyticRequestCapsuleRecord capsule;
            capsule.feature_id = read_u64(record);
            capsule.start_x_nm = read_i64(record + 8);
            capsule.start_y_nm = read_i64(record + 16);
            capsule.end_x_nm = read_i64(record + 24);
            capsule.end_y_nm = read_i64(record + 32);
            capsule.width_nm = read_u64(record + 40);
            records.capsules.push_back(capsule);
        }
        records.swept_paths.reserve(static_cast<std::size_t>(counts[11]));
        for (std::uint64_t index = 0; index < counts[11]; ++index)
        {
            const std::uint8_t* record = data + offsets[11] + index * kRecordBytes[11];
            AnalyticRequestSweptPathRecord swept;
            swept.feature_id = read_u64(record);
            swept.path_ring = read_u32(record + 8);
            if (read_u32(record + 12) != 0 || read_u64(record + 24) != 0)
                return decode_failure(AnalyticRequestPacketError::invalid_packet);
            swept.width_nm = read_u64(record + 16);
            records.swept_paths.push_back(swept);
        }
        records.relationship_queries.reserve(static_cast<std::size_t>(counts[12]));
        for (std::uint64_t index = 0; index < counts[12]; ++index)
        {
            const std::uint8_t* record = data + offsets[12] + index * kRecordBytes[12];
            AnalyticRequestQueryRecord query;
            query.query_id = read_u64(record);
            query.left_job_id = read_u64(record + 8);
            query.right_job_id = read_u64(record + 16);
            records.relationship_queries.push_back(query);
        }

        const AnalyticRequestPacketError validation =
            validate_analytic_request_packet_records(records);
        if (validation != AnalyticRequestPacketError::none)
            return decode_failure(validation);
        return {AnalyticRequestPacketError::none, std::move(records)};
    }
    catch (const std::overflow_error&)
    {
        return decode_failure(AnalyticRequestPacketError::limit_exceeded);
    }
    catch (const std::exception&)
    {
        return decode_failure(AnalyticRequestPacketError::invalid_packet);
    }
}

AnalyticRequestPacketEncodeResult
encode_analytic_request_packet(const AnalyticRequestPacketRecords& records)
{
    try
    {
        const AnalyticRequestPacketError validation =
            validate_analytic_request_packet_records(records);
        if (validation != AnalyticRequestPacketError::none)
            return encode_failure(validation);

        const std::array<std::uint64_t, kAnalyticRequestTableCount> counts{
            records.jobs.size(),
            records.stages.size(),
            records.operands.size(),
            records.planar_regions.size(),
            records.ring_references.size(),
            records.rings.size(),
            records.vertices.size(),
            records.segments.size(),
            records.disks.size(),
            records.annuli.size(),
            records.capsules.size(),
            records.swept_paths.size(),
            records.relationship_queries.size(),
        };
        std::array<std::uint64_t, kAnalyticRequestTableCount> offsets{};
        std::uint64_t cursor = kPayloadStart;
        std::uint64_t payload_sum = 0;
        for (std::size_t index = 0; index < kAnalyticRequestTableCount; ++index)
        {
            const std::uint64_t byte_length = checked_multiply(counts[index], kRecordBytes[index]);
            cursor = align_eight(cursor);
            offsets[index] = cursor;
            cursor = checked_add(cursor, byte_length);
            payload_sum = checked_add(payload_sum, byte_length);
        }
        if (cursor > kMaximumPacketBytes)
            return encode_failure(AnalyticRequestPacketError::limit_exceeded);

        std::vector<std::uint8_t> output(static_cast<std::size_t>(cursor), 0);
        std::copy(kMagic.begin(), kMagic.end(), output.begin());
        write_u16(output, 8, 1);
        write_u16(output, 10, static_cast<std::uint16_t>(kHeaderBytes));
        write_u64(output, 16, cursor);
        write_u64(output, 24, kHeaderBytes);
        write_u32(output, 32, static_cast<std::uint32_t>(kAnalyticRequestTableCount));
        write_u32(output, 36, static_cast<std::uint32_t>(counts[0]));
        write_u32(output, 40, static_cast<std::uint32_t>(counts[12]));
        write_u64(output, 48, payload_sum);
        for (std::size_t index = 0; index < kAnalyticRequestTableCount; ++index)
        {
            const std::size_t entry =
                static_cast<std::size_t>(kHeaderBytes + index * kDirectoryEntryBytes);
            write_u16(output, entry, static_cast<std::uint16_t>(1U + index));
            write_u16(output, entry + 2, 1);
            write_u32(output, entry + 4, kRecordBytes[index]);
            write_u64(output, entry + 8, offsets[index]);
            write_u64(output, entry + 16, checked_multiply(counts[index], kRecordBytes[index]));
            write_u64(output, entry + 24, counts[index]);
        }

        std::size_t offset = static_cast<std::size_t>(offsets[0]);
        for (const AnalyticRequestJobRecord& job : records.jobs)
        {
            write_u64(output, offset, job.job_id);
            write_u32(output, offset + 8, job.stage_begin);
            write_u32(output, offset + 12, job.stage_count);
            offset += kRecordBytes[0];
        }
        offset = static_cast<std::size_t>(offsets[1]);
        for (const AnalyticRequestStageRecord& stage : records.stages)
        {
            write_u64(output, offset, stage.stage_id);
            output[offset + 8] = stage.operation;
            write_u32(output, offset + 16, stage.operand_begin);
            write_u32(output, offset + 20, stage.operand_count);
            offset += kRecordBytes[1];
        }
        offset = static_cast<std::size_t>(offsets[2]);
        for (const AnalyticRequestOperandRecord& operand : records.operands)
        {
            write_u64(output, offset, operand.operand_id);
            write_u16(output, offset + 8, operand.geometry_kind);
            write_u32(output, offset + 12, operand.geometry_index);
            offset += kRecordBytes[2];
        }
        offset = static_cast<std::size_t>(offsets[3]);
        for (const AnalyticRequestPlanarRegionRecord& region : records.planar_regions)
        {
            write_u64(output, offset, region.region_id);
            write_u32(output, offset + 8, region.outer_ring);
            write_u32(output, offset + 12, region.hole_reference_begin);
            write_u32(output, offset + 16, region.hole_reference_count);
            offset += kRecordBytes[3];
        }
        offset = static_cast<std::size_t>(offsets[4]);
        for (const std::uint32_t reference : records.ring_references)
        {
            write_u32(output, offset, reference);
            offset += kRecordBytes[4];
        }
        offset = static_cast<std::size_t>(offsets[5]);
        for (const AnalyticRequestRingRecord& ring : records.rings)
        {
            write_u64(output, offset, ring.id);
            write_u32(output, offset + 8, ring.vertex_begin);
            write_u32(output, offset + 12, ring.vertex_count);
            write_u32(output, offset + 16, ring.segment_begin);
            write_u32(output, offset + 20, ring.segment_count);
            write_u32(output, offset + 24, ring.flags);
            offset += kRecordBytes[5];
        }
        offset = static_cast<std::size_t>(offsets[6]);
        for (const AnalyticRequestVertexRecord& vertex : records.vertices)
        {
            write_u64(output, offset, vertex.id);
            write_i64(output, offset + 8, vertex.x_nm);
            write_i64(output, offset + 16, vertex.y_nm);
            offset += kRecordBytes[6];
        }
        offset = static_cast<std::size_t>(offsets[7]);
        for (const AnalyticRequestSegmentRecord& segment : records.segments)
        {
            write_u64(output, offset, segment.id);
            write_u64(output, offset + 8, segment.curve_id);
            output[offset + 16] = segment.kind;
            output[offset + 17] = segment.direction;
            output[offset + 18] = segment.major_arc ? 1U : 0U;
            if (segment.kind == 3)
                write_u64(output, offset + 24, segment.radius_nm);
            else
            {
                write_i64(output, offset + 24, segment.center_x_nm);
                write_i64(output, offset + 32, segment.center_y_nm);
            }
            offset += kRecordBytes[7];
        }
        offset = static_cast<std::size_t>(offsets[8]);
        for (const AnalyticRequestDiskRecord& disk : records.disks)
        {
            write_u64(output, offset, disk.feature_id);
            write_i64(output, offset + 8, disk.center_x_nm);
            write_i64(output, offset + 16, disk.center_y_nm);
            write_u64(output, offset + 24, disk.radius_nm);
            offset += kRecordBytes[8];
        }
        offset = static_cast<std::size_t>(offsets[9]);
        for (const AnalyticRequestAnnulusRecord& annulus : records.annuli)
        {
            write_u64(output, offset, annulus.feature_id);
            write_i64(output, offset + 8, annulus.center_x_nm);
            write_i64(output, offset + 16, annulus.center_y_nm);
            write_u64(output, offset + 24, annulus.inner_radius_nm);
            write_u64(output, offset + 32, annulus.outer_radius_nm);
            offset += kRecordBytes[9];
        }
        offset = static_cast<std::size_t>(offsets[10]);
        for (const AnalyticRequestCapsuleRecord& capsule : records.capsules)
        {
            write_u64(output, offset, capsule.feature_id);
            write_i64(output, offset + 8, capsule.start_x_nm);
            write_i64(output, offset + 16, capsule.start_y_nm);
            write_i64(output, offset + 24, capsule.end_x_nm);
            write_i64(output, offset + 32, capsule.end_y_nm);
            write_u64(output, offset + 40, capsule.width_nm);
            offset += kRecordBytes[10];
        }
        offset = static_cast<std::size_t>(offsets[11]);
        for (const AnalyticRequestSweptPathRecord& swept : records.swept_paths)
        {
            write_u64(output, offset, swept.feature_id);
            write_u32(output, offset + 8, swept.path_ring);
            write_u64(output, offset + 16, swept.width_nm);
            offset += kRecordBytes[11];
        }
        offset = static_cast<std::size_t>(offsets[12]);
        for (const AnalyticRequestQueryRecord& query : records.relationship_queries)
        {
            write_u64(output, offset, query.query_id);
            write_u64(output, offset + 8, query.left_job_id);
            write_u64(output, offset + 16, query.right_job_id);
            offset += kRecordBytes[12];
        }
        return {AnalyticRequestPacketError::none, std::move(output)};
    }
    catch (const std::exception&)
    {
        return encode_failure(AnalyticRequestPacketError::limit_exceeded);
    }
}

} // namespace geometer
