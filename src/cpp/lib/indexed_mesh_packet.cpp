#include "geometer/indexed_mesh_packet.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>

namespace geometer
{
namespace
{

constexpr std::array<std::uint8_t, 8> kMagic{'G', 'M', 'I', 'M', 'S', 'H', '0', '1'};
constexpr std::size_t kHeaderBytes = 64;
constexpr std::uint16_t kVersion = 1;
constexpr std::uint32_t kHasSourceFaces = 1U;

std::size_t align_eight(std::size_t value)
{
    return (value + 7U) & ~std::size_t{7U};
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

double read_f64(const std::uint8_t* data)
{
    const std::uint64_t bits = read_u64(data);
    double value = 0.0;
    static_assert(sizeof(value) == sizeof(bits), "binary64 size is required");
    std::memcpy(&value, &bits, sizeof(value));
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

void write_f64(std::vector<std::uint8_t>& output, std::size_t offset, double value)
{
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    write_u64(output, offset, bits);
}

bool zero_range(const std::uint8_t* data, std::size_t begin, std::size_t end)
{
    for (std::size_t index = begin; index < end; ++index)
        if (data[index] != 0U)
            return false;
    return true;
}

IndexedMeshPacketDecodeResult decode_failure(IndexedMeshPacketError error)
{
    return {error, std::nullopt};
}

IndexedMeshPacketEncodeResult encode_failure(IndexedMeshPacketError error)
{
    return {error, std::nullopt};
}

IndexedMeshPacketError validate_mesh(const FastHlrIndexedMesh& mesh)
{
    if (mesh.vertices.empty() || mesh.triangles.empty())
        return IndexedMeshPacketError::invalid_mesh;
    if (mesh.vertices.size() > kIndexedMeshPacketMaximumVertices ||
        mesh.triangles.size() > kIndexedMeshPacketMaximumTriangles)
        return IndexedMeshPacketError::limit_exceeded;
    for (const FastHlrVec3& vertex : mesh.vertices)
        if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) || !std::isfinite(vertex.z))
            return IndexedMeshPacketError::invalid_mesh;
    for (const FastHlrIndexedTriangle& triangle : mesh.triangles)
    {
        const auto& indices = triangle.vertices;
        if (indices[0] >= mesh.vertices.size() || indices[1] >= mesh.vertices.size() ||
            indices[2] >= mesh.vertices.size() || indices[0] == indices[1] ||
            indices[1] == indices[2] || indices[2] == indices[0])
            return IndexedMeshPacketError::invalid_mesh;
    }
    return IndexedMeshPacketError::none;
}

} // namespace

IndexedMeshPacketDecodeResult decode_indexed_mesh_packet(const std::uint8_t* data, std::size_t size)
{
    if (data == nullptr || size < kHeaderBytes || size > kIndexedMeshPacketMaximumBytes)
        return decode_failure(size > kIndexedMeshPacketMaximumBytes
                                  ? IndexedMeshPacketError::limit_exceeded
                                  : IndexedMeshPacketError::invalid_packet);
    if (!std::equal(kMagic.begin(), kMagic.end(), data) || read_u16(data + 8) != kVersion ||
        read_u16(data + 10) != kHeaderBytes)
        return decode_failure(IndexedMeshPacketError::invalid_packet);

    const std::uint32_t flags = read_u32(data + 12);
    const std::uint64_t packet_bytes = read_u64(data + 16);
    const std::uint32_t vertex_count = read_u32(data + 24);
    const std::uint32_t triangle_count = read_u32(data + 28);
    const std::uint64_t positions_offset = read_u64(data + 32);
    const std::uint64_t triangles_offset = read_u64(data + 40);
    const std::uint64_t source_faces_offset = read_u64(data + 48);
    if ((flags & ~kHasSourceFaces) != 0U || packet_bytes != size ||
        positions_offset != kHeaderBytes || read_u64(data + 56) != 0U || vertex_count == 0U ||
        triangle_count == 0U)
        return decode_failure(IndexedMeshPacketError::invalid_packet);
    if (vertex_count > kIndexedMeshPacketMaximumVertices ||
        triangle_count > kIndexedMeshPacketMaximumTriangles)
        return decode_failure(IndexedMeshPacketError::limit_exceeded);

    const std::uint64_t positions_end = positions_offset + (std::uint64_t{24} * vertex_count);
    const std::uint64_t expected_triangles_offset = positions_end;
    const std::uint64_t triangles_end = triangles_offset + (std::uint64_t{12} * triangle_count);
    const std::uint64_t aligned_triangles_end =
        align_eight(static_cast<std::size_t>(triangles_end));
    const bool has_source_faces = (flags & kHasSourceFaces) != 0U;
    const std::uint64_t expected_source_offset = has_source_faces ? aligned_triangles_end : 0U;
    const std::uint64_t payload_end =
        has_source_faces ? source_faces_offset + (std::uint64_t{4} * triangle_count)
                         : triangles_end;
    const std::uint64_t expected_size = align_eight(static_cast<std::size_t>(payload_end));
    if (triangles_offset != expected_triangles_offset ||
        source_faces_offset != expected_source_offset || triangles_end > size ||
        payload_end > size || expected_size != size ||
        !zero_range(data, static_cast<std::size_t>(triangles_end),
                    has_source_faces ? static_cast<std::size_t>(source_faces_offset) : size) ||
        !zero_range(data, static_cast<std::size_t>(payload_end), size))
        return decode_failure(IndexedMeshPacketError::invalid_packet);

    FastHlrIndexedMesh mesh;
    mesh.vertices.reserve(vertex_count);
    mesh.triangles.reserve(triangle_count);
    for (std::size_t index = 0; index < vertex_count; ++index)
    {
        const std::size_t offset = static_cast<std::size_t>(positions_offset) + (index * 24U);
        FastHlrVec3 vertex{read_f64(data + offset), read_f64(data + offset + 8U),
                           read_f64(data + offset + 16U)};
        if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) || !std::isfinite(vertex.z))
            return decode_failure(IndexedMeshPacketError::invalid_mesh);
        mesh.vertices.push_back(vertex);
    }
    for (std::size_t index = 0; index < triangle_count; ++index)
    {
        const std::size_t offset = static_cast<std::size_t>(triangles_offset) + (index * 12U);
        FastHlrIndexedTriangle triangle;
        triangle.vertices = {read_u32(data + offset), read_u32(data + offset + 4U),
                             read_u32(data + offset + 8U)};
        if (has_source_faces)
            triangle.source_face =
                read_u32(data + static_cast<std::size_t>(source_faces_offset) + (index * 4U));
        mesh.triangles.push_back(triangle);
    }
    const IndexedMeshPacketError validation = validate_mesh(mesh);
    if (validation != IndexedMeshPacketError::none)
        return decode_failure(validation);
    return {IndexedMeshPacketError::none, std::move(mesh)};
}

IndexedMeshPacketEncodeResult encode_indexed_mesh_packet(const FastHlrIndexedMesh& mesh)
{
    const IndexedMeshPacketError validation = validate_mesh(mesh);
    if (validation != IndexedMeshPacketError::none)
        return encode_failure(validation);

    const bool has_source_faces = std::any_of(
        mesh.triangles.begin(), mesh.triangles.end(), [](const FastHlrIndexedTriangle& triangle)
        { return triangle.source_face != kFastHlrUnspecifiedSourceFace; });
    const std::size_t positions_offset = kHeaderBytes;
    const std::size_t triangles_offset = positions_offset + (mesh.vertices.size() * 24U);
    const std::size_t triangles_end = triangles_offset + (mesh.triangles.size() * 12U);
    const std::size_t source_faces_offset = has_source_faces ? align_eight(triangles_end) : 0U;
    const std::size_t payload_end =
        has_source_faces ? source_faces_offset + (mesh.triangles.size() * 4U) : triangles_end;
    const std::size_t packet_size = align_eight(payload_end);
    if (packet_size > kIndexedMeshPacketMaximumBytes)
        return encode_failure(IndexedMeshPacketError::limit_exceeded);

    std::vector<std::uint8_t> output(packet_size, 0U);
    std::copy(kMagic.begin(), kMagic.end(), output.begin());
    write_u16(output, 8, kVersion);
    write_u16(output, 10, static_cast<std::uint16_t>(kHeaderBytes));
    write_u32(output, 12, has_source_faces ? kHasSourceFaces : 0U);
    write_u64(output, 16, packet_size);
    write_u32(output, 24, static_cast<std::uint32_t>(mesh.vertices.size()));
    write_u32(output, 28, static_cast<std::uint32_t>(mesh.triangles.size()));
    write_u64(output, 32, positions_offset);
    write_u64(output, 40, triangles_offset);
    write_u64(output, 48, source_faces_offset);
    for (std::size_t index = 0; index < mesh.vertices.size(); ++index)
    {
        const std::size_t offset = positions_offset + (index * 24U);
        write_f64(output, offset, mesh.vertices[index].x);
        write_f64(output, offset + 8U, mesh.vertices[index].y);
        write_f64(output, offset + 16U, mesh.vertices[index].z);
    }
    for (std::size_t index = 0; index < mesh.triangles.size(); ++index)
    {
        const std::size_t offset = triangles_offset + (index * 12U);
        write_u32(output, offset, mesh.triangles[index].vertices[0]);
        write_u32(output, offset + 4U, mesh.triangles[index].vertices[1]);
        write_u32(output, offset + 8U, mesh.triangles[index].vertices[2]);
        if (has_source_faces)
            write_u32(output, source_faces_offset + (index * 4U),
                      mesh.triangles[index].source_face);
    }
    return {IndexedMeshPacketError::none, std::move(output)};
}

} // namespace geometer
