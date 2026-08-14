#include "geometer/analytic_result_packet_layout.h"

#include <algorithm>
#include <array>
#include <exception>
#include <limits>
#include <stdexcept>

namespace geometer
{
namespace
{

constexpr std::array<std::uint8_t, 8> kMagic{'G', 'M', 'A', 'B', 'R', 'S', '0', '1'};
constexpr std::uint64_t kHeaderBytes = 64;
constexpr std::uint64_t kDirectoryEntryBytes = 32;
constexpr std::uint64_t kDirectoryBytes = kDirectoryEntryBytes * kAnalyticResultTableCount;
constexpr std::uint64_t kPayloadStart = kHeaderBytes + kDirectoryBytes;
constexpr std::uint64_t kMaximumPacketBytes = 268'435'456;
constexpr std::uint32_t kMaximumJobResults = 65'535;
constexpr std::uint32_t kMaximumRelationshipResults = 1'048'576;
constexpr std::array<std::uint32_t, kAnalyticResultTableCount> kRecordBytes{
    48, 56, 32, 48, 32, 4, 24, 8, 8, 32, 48, 32, 32, 4,
};

AnalyticResultPacketLayoutResult decode_failure(AnalyticResultPacketLayoutError error)
{
    return {error, std::nullopt};
}

AnalyticResultPacketEncodeResult encode_failure(AnalyticResultPacketLayoutError error)
{
    return {error, std::nullopt};
}

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
        throw std::overflow_error("analytic result packet addition overflow");
    return left + right;
}

std::uint64_t checked_multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
        throw std::overflow_error("analytic result packet multiplication overflow");
    return left * right;
}

std::uint64_t align_eight(std::uint64_t value)
{
    return checked_add(value, 7U) & ~std::uint64_t{7};
}

std::uint16_t read_u16(const std::uint8_t* data)
{
    return static_cast<std::uint16_t>(data[0]) | (static_cast<std::uint16_t>(data[1]) << 8U);
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

bool zero_range(const std::uint8_t* data, std::uint64_t begin, std::uint64_t end)
{
    for (std::uint64_t index = begin; index < end; ++index)
        if (data[index] != 0)
            return false;
    return true;
}

} // namespace

AnalyticResultPacketLayoutResult decode_analytic_result_packet_layout(const std::uint8_t* data,
                                                                      std::size_t size)
{
    try
    {
        if (data == nullptr || size < kPayloadStart)
            return decode_failure(AnalyticResultPacketLayoutError::invalid_packet);
        if (size > kMaximumPacketBytes)
            return decode_failure(AnalyticResultPacketLayoutError::limit_exceeded);
        if (!std::equal(kMagic.begin(), kMagic.end(), data) || read_u16(data + 8) != 1 ||
            read_u16(data + 10) != kHeaderBytes || read_u32(data + 12) != 0 ||
            read_u64(data + 16) != size || read_u64(data + 24) != kHeaderBytes ||
            read_u32(data + 32) != kAnalyticResultTableCount || read_u32(data + 44) != 0 ||
            read_u64(data + 56) != 0)
            return decode_failure(AnalyticResultPacketLayoutError::invalid_packet);

        AnalyticResultPacketLayout layout;
        layout.job_result_count = read_u32(data + 36);
        layout.relationship_result_count = read_u32(data + 40);
        layout.canonical_payload_bytes = read_u64(data + 48);
        if (layout.job_result_count > kMaximumJobResults ||
            layout.relationship_result_count > kMaximumRelationshipResults)
            return decode_failure(AnalyticResultPacketLayoutError::limit_exceeded);
        std::uint64_t cursor = kPayloadStart;
        std::uint64_t payload_sum = 0;
        for (std::size_t index = 0; index < kAnalyticResultTableCount; ++index)
        {
            const std::uint64_t directory_offset = kHeaderBytes + index * kDirectoryEntryBytes;
            const std::uint8_t* entry = data + directory_offset;
            AnalyticResultTableView view;
            view.kind = read_u16(entry);
            const std::uint16_t version = read_u16(entry + 2);
            view.record_bytes = read_u32(entry + 4);
            view.offset = read_u64(entry + 8);
            view.byte_length = read_u64(entry + 16);
            view.record_count = read_u64(entry + 24);
            const std::uint64_t expected_offset = align_eight(cursor);
            const std::uint64_t expected_length =
                checked_multiply(view.record_count, view.record_bytes);
            const std::uint64_t range_end = checked_add(view.offset, view.byte_length);
            if (view.kind != 101U + index || version != 1 ||
                view.record_bytes != kRecordBytes[index] || view.offset != expected_offset ||
                view.offset % 8U != 0 || view.byte_length != expected_length || range_end > size ||
                !zero_range(data, cursor, view.offset))
                return decode_failure(AnalyticResultPacketLayoutError::invalid_packet);
            payload_sum = checked_add(payload_sum, view.byte_length);
            cursor = range_end;
            layout.tables[index] = view;
        }
        if (cursor != size || payload_sum != layout.canonical_payload_bytes ||
            layout.tables[0].record_count != layout.job_result_count ||
            layout.tables[11].record_count != layout.relationship_result_count)
            return decode_failure(AnalyticResultPacketLayoutError::invalid_packet);
        return {AnalyticResultPacketLayoutError::none, layout};
    }
    catch (const std::overflow_error&)
    {
        return decode_failure(AnalyticResultPacketLayoutError::limit_exceeded);
    }
    catch (const std::exception&)
    {
        return decode_failure(AnalyticResultPacketLayoutError::invalid_packet);
    }
}

AnalyticResultPacketEncodeResult
encode_analytic_result_packet_layout(const AnalyticResultTableBytes& tables)
{
    try
    {
        std::array<std::uint64_t, kAnalyticResultTableCount> offsets{};
        std::array<std::uint64_t, kAnalyticResultTableCount> counts{};
        std::uint64_t cursor = kPayloadStart;
        std::uint64_t payload_sum = 0;
        for (std::size_t index = 0; index < kAnalyticResultTableCount; ++index)
        {
            if (tables[index].size() % kRecordBytes[index] != 0)
                return encode_failure(AnalyticResultPacketLayoutError::invalid_packet);
            cursor = align_eight(cursor);
            offsets[index] = cursor;
            counts[index] = tables[index].size() / kRecordBytes[index];
            cursor = checked_add(cursor, tables[index].size());
            payload_sum = checked_add(payload_sum, tables[index].size());
        }
        if (cursor > kMaximumPacketBytes || counts[0] > kMaximumJobResults ||
            counts[11] > kMaximumRelationshipResults ||
            cursor > std::numeric_limits<std::size_t>::max())
            return encode_failure(AnalyticResultPacketLayoutError::limit_exceeded);

        std::vector<std::uint8_t> output(static_cast<std::size_t>(cursor), 0);
        std::copy(kMagic.begin(), kMagic.end(), output.begin());
        write_u16(output, 8, 1);
        write_u16(output, 10, static_cast<std::uint16_t>(kHeaderBytes));
        write_u64(output, 16, cursor);
        write_u64(output, 24, kHeaderBytes);
        write_u32(output, 32, static_cast<std::uint32_t>(kAnalyticResultTableCount));
        write_u32(output, 36, static_cast<std::uint32_t>(counts[0]));
        write_u32(output, 40, static_cast<std::uint32_t>(counts[11]));
        write_u64(output, 48, payload_sum);
        for (std::size_t index = 0; index < kAnalyticResultTableCount; ++index)
        {
            const std::size_t entry =
                static_cast<std::size_t>(kHeaderBytes + index * kDirectoryEntryBytes);
            write_u16(output, entry, static_cast<std::uint16_t>(101U + index));
            write_u16(output, entry + 2, 1);
            write_u32(output, entry + 4, kRecordBytes[index]);
            write_u64(output, entry + 8, offsets[index]);
            write_u64(output, entry + 16, tables[index].size());
            write_u64(output, entry + 24, counts[index]);
            std::copy(tables[index].begin(), tables[index].end(),
                      output.begin() + static_cast<std::size_t>(offsets[index]));
        }
        return {AnalyticResultPacketLayoutError::none, std::move(output)};
    }
    catch (const std::exception&)
    {
        return encode_failure(AnalyticResultPacketLayoutError::limit_exceeded);
    }
}

} // namespace geometer
