#include "geometer/ipc_a0_frame.h"

#include <array>
#include <cstring>
#include <limits>
#include <unordered_set>

namespace geometer::ipc_a0
{
namespace
{

constexpr std::array<unsigned char, 8> kMagic = {'G', 'M', 'I', 'P', 'C', 'A', '0', '1'};

std::uint16_t load_u16(const unsigned char* data)
{
    return static_cast<std::uint16_t>(data[0]) | (static_cast<std::uint16_t>(data[1]) << 8U);
}

std::uint32_t load_u32(const unsigned char* data)
{
    return static_cast<std::uint32_t>(data[0]) | (static_cast<std::uint32_t>(data[1]) << 8U) |
           (static_cast<std::uint32_t>(data[2]) << 16U) |
           (static_cast<std::uint32_t>(data[3]) << 24U);
}

std::uint64_t load_u64(const unsigned char* data)
{
    std::uint64_t value = 0;
    for (unsigned int index = 0; index < 8U; ++index)
    {
        value |= static_cast<std::uint64_t>(data[index]) << (index * 8U);
    }
    return value;
}

void store_u16(unsigned char* data, std::uint16_t value)
{
    data[0] = static_cast<unsigned char>(value & 0xffU);
    data[1] = static_cast<unsigned char>((value >> 8U) & 0xffU);
}

void store_u32(unsigned char* data, std::uint32_t value)
{
    for (unsigned int index = 0; index < 4U; ++index)
    {
        data[index] = static_cast<unsigned char>((value >> (index * 8U)) & 0xffU);
    }
}

void store_u64(unsigned char* data, std::uint64_t value)
{
    for (unsigned int index = 0; index < 8U; ++index)
    {
        data[index] = static_cast<unsigned char>((value >> (index * 8U)) & 0xffU);
    }
}

bool add_size(std::size_t value, std::size_t addition, std::size_t* result)
{
    if (value > std::numeric_limits<std::size_t>::max() - addition)
    {
        return false;
    }
    *result = value + addition;
    return true;
}

bool read_exact(std::FILE* stream, unsigned char* data, std::size_t size, std::size_t* read)
{
    *read = 0;
    while (*read < size)
    {
        const std::size_t count = std::fread(data + *read, 1, size - *read, stream);
        if (count == 0)
        {
            return false;
        }
        *read += count;
    }
    return true;
}

bool write_exact(std::FILE* stream, const unsigned char* data, std::size_t size)
{
    std::size_t written = 0;
    while (written < size)
    {
        const std::size_t count = std::fwrite(data + written, 1, size - written, stream);
        if (count == 0)
        {
            return false;
        }
        written += count;
    }
    return true;
}

bool valid_kind(std::uint16_t value)
{
    return value >= static_cast<std::uint16_t>(FrameKind::hello) &&
           value <= static_cast<std::uint16_t>(FrameKind::protocol_error);
}

bool valid_utf8(const std::string& value)
{
    const auto* bytes = reinterpret_cast<const unsigned char*>(value.data());
    std::size_t index = 0;
    while (index < value.size())
    {
        const unsigned char first = bytes[index++];
        if (first <= 0x7fU)
        {
            continue;
        }
        std::size_t continuation_count = 0;
        unsigned char second_min = 0x80U;
        unsigned char second_max = 0xbfU;
        if (first >= 0xc2U && first <= 0xdfU)
        {
            continuation_count = 1;
        }
        else if (first >= 0xe0U && first <= 0xefU)
        {
            continuation_count = 2;
            second_min = first == 0xe0U ? 0xa0U : 0x80U;
            second_max = first == 0xedU ? 0x9fU : 0xbfU;
        }
        else if (first >= 0xf0U && first <= 0xf4U)
        {
            continuation_count = 3;
            second_min = first == 0xf0U ? 0x90U : 0x80U;
            second_max = first == 0xf4U ? 0x8fU : 0xbfU;
        }
        else
        {
            return false;
        }
        if (value.size() - index < continuation_count || bytes[index] < second_min ||
            bytes[index] > second_max)
        {
            return false;
        }
        ++index;
        for (std::size_t continuation = 1; continuation < continuation_count; ++continuation)
        {
            if (bytes[index] < 0x80U || bytes[index] > 0xbfU)
            {
                return false;
            }
            ++index;
        }
    }
    return true;
}

ReadStatus fail(std::string* error, const char* message)
{
    if (error != nullptr)
    {
        *error = message;
    }
    return ReadStatus::error;
}

} // namespace

std::size_t encoded_size(const Frame& frame)
{
    std::size_t size = kHeaderSize;
    if (!add_size(size, frame.json.size(), &size))
    {
        return std::numeric_limits<std::size_t>::max();
    }
    for (const auto& attachment : frame.attachments)
    {
        if (!add_size(size, 16U, &size) || !add_size(size, attachment.name.size(), &size) ||
            !add_size(size, attachment.media_type.size(), &size) ||
            !add_size(size, attachment.data.size(), &size))
        {
            return std::numeric_limits<std::size_t>::max();
        }
    }
    return size;
}

ReadStatus read_frame(std::FILE* stream, Frame* frame, std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }
    std::array<unsigned char, kHeaderSize> header{};
    std::size_t header_read = 0;
    if (!read_exact(stream, header.data(), header.size(), &header_read))
    {
        if (header_read == 0 && std::feof(stream) != 0)
        {
            return ReadStatus::end_of_stream;
        }
        return fail(error, "Unexpected EOF or read error within an IPC frame header.");
    }
    if (!std::equal(kMagic.begin(), kMagic.end(), header.begin()))
    {
        return fail(error, "IPC frame magic is invalid.");
    }
    const std::uint16_t header_size = load_u16(header.data() + 8U);
    const std::uint16_t generation = load_u16(header.data() + 10U);
    const std::uint16_t kind = load_u16(header.data() + 12U);
    const std::uint16_t flags = load_u16(header.data() + 14U);
    const std::uint64_t request_id = load_u64(header.data() + 16U);
    const std::uint32_t json_size = load_u32(header.data() + 24U);
    const std::uint32_t attachment_count = load_u32(header.data() + 28U);
    const std::uint64_t attachment_bytes = load_u64(header.data() + 32U);
    if (header_size != kHeaderSize || generation != 0 || !valid_kind(kind) || flags != 0 ||
        load_u32(header.data() + 40U) != 0 || load_u32(header.data() + 44U) != 0)
    {
        return fail(error, "IPC frame header contains an unsupported or reserved value.");
    }
    if (json_size == 0 || json_size > kMaxJsonBytes || attachment_count > kMaxAttachmentCount ||
        attachment_bytes > kMaxFrameBytes)
    {
        return fail(error, "IPC frame header exceeds an A0 size or count limit.");
    }
    std::size_t complete_size = kHeaderSize;
    if (!add_size(complete_size, json_size, &complete_size) ||
        attachment_bytes > std::numeric_limits<std::size_t>::max() ||
        !add_size(complete_size, static_cast<std::size_t>(attachment_bytes), &complete_size) ||
        complete_size > kMaxFrameBytes)
    {
        return fail(error, "IPC complete-frame size overflows or exceeds the A0 limit.");
    }

    std::vector<unsigned char> payload(complete_size - kHeaderSize);
    std::size_t payload_read = 0;
    if (!read_exact(stream, payload.data(), payload.size(), &payload_read))
    {
        return fail(error, "Unexpected EOF or read error within an IPC frame payload.");
    }

    Frame decoded;
    decoded.kind = static_cast<FrameKind>(kind);
    decoded.request_id = request_id;
    decoded.json.assign(reinterpret_cast<const char*>(payload.data()), json_size);
    std::size_t offset = json_size;
    decoded.attachments.reserve(attachment_count);
    for (std::uint32_t index = 0; index < attachment_count; ++index)
    {
        if (offset > payload.size() || payload.size() - offset < 16U)
        {
            return fail(error, "IPC attachment header exceeds the declared section boundary.");
        }
        const unsigned char* attachment_header = payload.data() + offset;
        const std::uint16_t name_size = load_u16(attachment_header);
        const std::uint16_t media_type_size = load_u16(attachment_header + 2U);
        const std::uint32_t attachment_flags = load_u32(attachment_header + 4U);
        const std::uint64_t data_size = load_u64(attachment_header + 8U);
        if (name_size > kMaxAttachmentTextBytes || media_type_size > kMaxAttachmentTextBytes ||
            attachment_flags != 0 || data_size > kMaxAttachmentBytes)
        {
            return fail(error, "IPC attachment header exceeds an A0 limit or uses reserved flags.");
        }
        std::size_t section_size = 16U;
        if (!add_size(section_size, name_size, &section_size) ||
            !add_size(section_size, media_type_size, &section_size) ||
            data_size > std::numeric_limits<std::size_t>::max() ||
            !add_size(section_size, static_cast<std::size_t>(data_size), &section_size) ||
            section_size > payload.size() - offset)
        {
            return fail(error, "IPC attachment data exceeds the declared section boundary.");
        }
        offset += 16U;
        Attachment attachment;
        attachment.name.assign(reinterpret_cast<const char*>(payload.data() + offset), name_size);
        offset += name_size;
        attachment.media_type.assign(reinterpret_cast<const char*>(payload.data() + offset),
                                     media_type_size);
        offset += media_type_size;
        attachment.data.assign(payload.begin() + static_cast<std::ptrdiff_t>(offset),
                               payload.begin() + static_cast<std::ptrdiff_t>(offset + data_size));
        offset += static_cast<std::size_t>(data_size);
        decoded.attachments.push_back(std::move(attachment));
    }
    if (offset != payload.size() || payload.size() - json_size != attachment_bytes)
    {
        return fail(error, "IPC attachment count or byte total does not match parsed sections.");
    }
    *frame = std::move(decoded);
    return ReadStatus::ok;
}

bool write_frame(std::FILE* stream, const Frame& frame, std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }
    if (frame.json.empty() || frame.json.size() > kMaxJsonBytes ||
        frame.attachments.size() > kMaxAttachmentCount || encoded_size(frame) > kMaxFrameBytes ||
        frame.json.size() > std::numeric_limits<std::uint32_t>::max())
    {
        if (error != nullptr)
        {
            *error = "IPC output frame exceeds an A0 size or count limit.";
        }
        return false;
    }
    std::uint64_t attachment_bytes = 0;
    std::unordered_set<std::string> attachment_names;
    for (const auto& attachment : frame.attachments)
    {
        if (attachment.name.empty() || attachment.media_type.empty() ||
            !valid_utf8(attachment.name) || !valid_utf8(attachment.media_type) ||
            !attachment_names.insert(attachment.name).second ||
            attachment.name.size() > kMaxAttachmentTextBytes ||
            attachment.media_type.size() > kMaxAttachmentTextBytes ||
            attachment.data.size() > kMaxAttachmentBytes)
        {
            if (error != nullptr)
            {
                *error = "IPC output attachment exceeds an A0 size limit.";
            }
            return false;
        }
        attachment_bytes +=
            16U + attachment.name.size() + attachment.media_type.size() + attachment.data.size();
    }

    std::array<unsigned char, kHeaderSize> header{};
    std::copy(kMagic.begin(), kMagic.end(), header.begin());
    store_u16(header.data() + 8U, static_cast<std::uint16_t>(kHeaderSize));
    store_u16(header.data() + 10U, 0);
    store_u16(header.data() + 12U, static_cast<std::uint16_t>(frame.kind));
    store_u16(header.data() + 14U, 0);
    store_u64(header.data() + 16U, frame.request_id);
    store_u32(header.data() + 24U, static_cast<std::uint32_t>(frame.json.size()));
    store_u32(header.data() + 28U, static_cast<std::uint32_t>(frame.attachments.size()));
    store_u64(header.data() + 32U, attachment_bytes);
    if (!write_exact(stream, header.data(), header.size()) ||
        !write_exact(stream, reinterpret_cast<const unsigned char*>(frame.json.data()),
                     frame.json.size()))
    {
        if (error != nullptr)
        {
            *error = "Failed to write an IPC frame header or JSON section.";
        }
        return false;
    }
    for (const auto& attachment : frame.attachments)
    {
        std::array<unsigned char, 16> attachment_header{};
        store_u16(attachment_header.data(), static_cast<std::uint16_t>(attachment.name.size()));
        store_u16(attachment_header.data() + 2U,
                  static_cast<std::uint16_t>(attachment.media_type.size()));
        store_u64(attachment_header.data() + 8U, attachment.data.size());
        if (!write_exact(stream, attachment_header.data(), attachment_header.size()) ||
            !write_exact(stream, reinterpret_cast<const unsigned char*>(attachment.name.data()),
                         attachment.name.size()) ||
            !write_exact(stream,
                         reinterpret_cast<const unsigned char*>(attachment.media_type.data()),
                         attachment.media_type.size()) ||
            !write_exact(stream, attachment.data.data(), attachment.data.size()))
        {
            if (error != nullptr)
            {
                *error = "Failed to write an IPC attachment section.";
            }
            return false;
        }
    }
    if (std::fflush(stream) != 0)
    {
        if (error != nullptr)
        {
            *error = "Failed to flush a complete IPC frame.";
        }
        return false;
    }
    return true;
}

} // namespace geometer::ipc_a0
