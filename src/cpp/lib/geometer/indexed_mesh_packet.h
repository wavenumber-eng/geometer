#pragma once

#include "fast_hlr.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace geometer
{

inline constexpr std::size_t kIndexedMeshPacketMaximumBytes = 268'435'456;
inline constexpr std::size_t kIndexedMeshPacketMaximumVertices = 2'000'000;
inline constexpr std::size_t kIndexedMeshPacketMaximumTriangles = 2'000'000;

enum class IndexedMeshPacketError : std::uint8_t
{
    none = 0,
    invalid_packet = 1,
    invalid_mesh = 2,
    limit_exceeded = 3,
};

struct IndexedMeshPacketDecodeResult
{
    IndexedMeshPacketError error = IndexedMeshPacketError::none;
    std::optional<FastHlrIndexedMesh> value;
};

struct IndexedMeshPacketEncodeResult
{
    IndexedMeshPacketError error = IndexedMeshPacketError::none;
    std::optional<std::vector<std::uint8_t>> value;
};

/** Decode and structurally validate a canonical GMIMSH01 indexed-mesh packet. */
[[nodiscard]] IndexedMeshPacketDecodeResult decode_indexed_mesh_packet(const std::uint8_t* data,
                                                                       std::size_t size);

/** Encode a mesh to the unique canonical GMIMSH01 byte representation. */
[[nodiscard]] IndexedMeshPacketEncodeResult
encode_indexed_mesh_packet(const FastHlrIndexedMesh& mesh);

} // namespace geometer
