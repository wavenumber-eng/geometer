#include "geometer/indexed_mesh_packet.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

namespace
{

int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

geometer::FastHlrIndexedMesh sample_mesh(bool source_faces)
{
    geometer::FastHlrIndexedMesh mesh;
    mesh.vertices = {{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {2.0, 1.0, 0.0}, {0.0, 1.0, 0.0}};
    mesh.triangles = {{{0, 1, 2}, source_faces ? 7U : geometer::kFastHlrUnspecifiedSourceFace},
                      {{0, 2, 3}, source_faces ? 7U : geometer::kFastHlrUnspecifiedSourceFace}};
    return mesh;
}

void round_trip(bool source_faces)
{
    const auto mesh = sample_mesh(source_faces);
    const auto encoded = geometer::encode_indexed_mesh_packet(mesh);
    expect(encoded.error == geometer::IndexedMeshPacketError::none && encoded.value.has_value(),
           "valid mesh encodes");
    if (!encoded.value)
        return;
    const auto decoded =
        geometer::decode_indexed_mesh_packet(encoded.value->data(), encoded.value->size());
    expect(decoded.error == geometer::IndexedMeshPacketError::none && decoded.value.has_value(),
           "canonical packet decodes");
    if (!decoded.value)
        return;
    expect(decoded.value->vertices.size() == 4U && decoded.value->triangles.size() == 2U,
           "counts survive round trip");
    expect(decoded.value->triangles[0].source_face == mesh.triangles[0].source_face,
           "source-face provenance survives round trip");
    const auto canonical = geometer::encode_indexed_mesh_packet(*decoded.value);
    expect(canonical.value == encoded.value, "decode/encode is byte-canonical");
}

} // namespace

int main()
{
    round_trip(false);
    round_trip(true);

    auto invalid = sample_mesh(false);
    invalid.vertices[0].x = std::numeric_limits<double>::quiet_NaN();
    expect(geometer::encode_indexed_mesh_packet(invalid).error ==
               geometer::IndexedMeshPacketError::invalid_mesh,
           "non-finite vertex is rejected");
    invalid = sample_mesh(false);
    invalid.triangles[0].vertices[2] = 99U;
    expect(geometer::encode_indexed_mesh_packet(invalid).error ==
               geometer::IndexedMeshPacketError::invalid_mesh,
           "out-of-range index is rejected");
    invalid = sample_mesh(false);
    invalid.triangles[0].vertices[2] = invalid.triangles[0].vertices[1];
    expect(geometer::encode_indexed_mesh_packet(invalid).error ==
               geometer::IndexedMeshPacketError::invalid_mesh,
           "repeated triangle index is rejected");

    const auto encoded = geometer::encode_indexed_mesh_packet(sample_mesh(true));
    if (encoded.value)
    {
        std::vector<std::uint8_t> mutated = *encoded.value;
        mutated[56] = 1U;
        expect(geometer::decode_indexed_mesh_packet(mutated.data(), mutated.size()).error ==
                   geometer::IndexedMeshPacketError::invalid_packet,
               "nonzero reserved header is rejected");
        mutated = *encoded.value;
        mutated[12] = 0x80U;
        expect(geometer::decode_indexed_mesh_packet(mutated.data(), mutated.size()).error ==
                   geometer::IndexedMeshPacketError::invalid_packet,
               "unknown flags are rejected");
    }

    auto single_triangle = sample_mesh(false);
    single_triangle.vertices.resize(3U);
    single_triangle.triangles.resize(1U);
    const auto padded = geometer::encode_indexed_mesh_packet(single_triangle);
    if (padded.value)
    {
        std::vector<std::uint8_t> mutated = *padded.value;
        mutated.back() = 1U;
        expect(geometer::decode_indexed_mesh_packet(mutated.data(), mutated.size()).error ==
                   geometer::IndexedMeshPacketError::invalid_packet,
               "nonzero alignment padding is rejected");
    }

    if (failures != 0)
        std::cerr << failures << " indexed-mesh packet assertion(s) failed.\n";
    return failures == 0 ? 0 : 1;
}
