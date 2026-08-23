#include "step_topology_session_internal.h"

#include "geometer/sha256.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace geometer::step_topology_internal
{
namespace
{

constexpr double kMillimetersToMeters = 0.001;

struct PrimitiveLayout
{
    std::size_t indices_view = 0;
    std::size_t indices_accessor = 0;
};

struct MeshLayout
{
    std::size_t positions_view = 0;
    std::size_t normals_view = 0;
    std::size_t positions_accessor = 0;
    std::size_t normals_accessor = 0;
    std::array<double, 3> minimum{};
    std::array<double, 3> maximum{};
    std::vector<PrimitiveLayout> primitives;
};

struct BufferView
{
    std::size_t offset = 0;
    std::size_t length = 0;
    int target = 0;
};

struct Accessor
{
    std::size_t buffer_view = 0;
    int component_type = 0;
    std::size_t count = 0;
    const char* type = nullptr;
    bool has_bounds = false;
    std::array<double, 3> minimum{};
    std::array<double, 3> maximum{};
};

void append_u32(std::vector<unsigned char>* bytes, std::uint32_t value)
{
    for (unsigned int shift = 0; shift < 32U; shift += 8U)
    {
        bytes->push_back(static_cast<unsigned char>((value >> shift) & 0xffU));
    }
}

void append_float(std::vector<unsigned char>* bytes, float value)
{
    std::uint32_t bits = 0;
    static_assert(sizeof(value) == sizeof(bits));
    std::memcpy(&bits, &value, sizeof(bits));
    append_u32(bytes, bits);
}

void align_four(std::vector<unsigned char>* bytes, unsigned char padding)
{
    while (bytes->size() % 4U != 0)
    {
        bytes->push_back(padding);
    }
}

bool cancelled(const StepTopologyCancellation* cancellation)
{
    return cancellation != nullptr && cancellation->is_cancelled();
}

int check_progress(const StepTopologyCancellation* cancellation, std::size_t binary_bytes,
                   std::size_t json_bytes, std::size_t byte_limit, Status* status)
{
    if (cancelled(cancellation))
    {
        set_status(status, kCancelled, "STEP topology GLB encoding was cancelled.");
        return kCancelled;
    }
    constexpr std::size_t framing_bytes = 36U;
    if (binary_bytes > byte_limit || json_bytes > byte_limit ||
        binary_bytes > byte_limit - std::min(json_bytes, byte_limit) ||
        binary_bytes + json_bytes > byte_limit - std::min(framing_bytes, byte_limit))
    {
        set_status(status, kResourceLimit, "STEP topology GLB exceeds its byte limit.");
        return kResourceLimit;
    }
    return 0;
}

void hash_u64(Sha256Builder* hash, std::uint64_t value)
{
    std::array<std::uint8_t, 8> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index)
    {
        bytes[index] = static_cast<std::uint8_t>(value >> (index * 8U));
    }
    hash->update(bytes.data(), bytes.size());
}

void hash_string(Sha256Builder* hash, const std::string& value)
{
    hash_u64(hash, static_cast<std::uint64_t>(value.size()));
    hash->update(reinterpret_cast<const std::uint8_t*>(value.data()), value.size());
}

std::string glb_artifact_handle(const SessionData& data, const StepTopologyGlbWorkPacket& packet)
{
    Sha256Builder hash;
    hash.update(data.secret.data(), data.secret.size());
    hash_string(&hash, "geometer.step_topology_glb.seal.a0");
    hash_u64(&hash, data.info.generation);
    hash_string(&hash, packet.content_sha256);
    hash_string(&hash, packet.render.artifact_handle);
    hash_string(&hash, packet.render.content_sha256);
    return "gtg_" + hash.hex_digest();
}

int append_chunk(std::vector<unsigned char>* destination, const unsigned char* source,
                 std::size_t source_size, const StepTopologyCancellation* cancellation,
                 std::size_t byte_limit, Status* status)
{
    constexpr std::size_t chunk_bytes = 64U * 1024U;
    for (std::size_t offset = 0; offset < source_size;)
    {
        if (cancelled(cancellation))
        {
            set_status(status, kCancelled, "STEP topology GLB encoding was cancelled.");
            return kCancelled;
        }
        const std::size_t count = std::min(chunk_bytes, source_size - offset);
        if (destination->size() > byte_limit || count > byte_limit - destination->size())
        {
            set_status(status, kResourceLimit, "STEP topology GLB exceeds its byte limit.");
            return kResourceLimit;
        }
        destination->insert(destination->end(), source + offset, source + offset + count);
        offset += count;
    }
    return 0;
}

int digest_glb(const std::vector<unsigned char>& glb, const StepTopologyCancellation* cancellation,
               std::string* digest, Status* status)
{
    constexpr std::size_t chunk_bytes = 64U * 1024U;
    Sha256Builder hash;
    for (std::size_t offset = 0; offset < glb.size();)
    {
        if (cancelled(cancellation))
        {
            set_status(status, kCancelled, "STEP topology GLB sealing was cancelled.");
            return kCancelled;
        }
        const std::size_t count = std::min(chunk_bytes, glb.size() - offset);
        hash.update(glb.data() + offset, count);
        offset += count;
    }
    *digest = hash.hex_digest();
    return 0;
}

void write_string(rapidjson::Writer<rapidjson::StringBuffer>* writer, const char* key,
                  const std::string& value)
{
    writer->Key(key);
    writer->String(value.data(), static_cast<rapidjson::SizeType>(value.size()));
}

void write_binding_extras(rapidjson::Writer<rapidjson::StringBuffer>* writer,
                          const StepTopologyRenderPrimitive& primitive, std::size_t primitive_index)
{
    writer->Key("extras");
    writer->StartObject();
    writer->Key("wn_geometer");
    writer->StartObject();
    writer->Key("primitive_index");
    writer->Uint64(primitive_index);
    writer->Key("first_triangle");
    writer->Uint64(primitive.first_triangle);
    writer->Key("triangle_count");
    writer->Uint64(primitive.triangle_count);
    write_string(writer, "body_handle", primitive.body_handle);
    write_string(writer, "face_handle", primitive.face_handle);
    writer->EndObject();
    writer->EndObject();
}

int encode_glb(const StepTopologyRenderArtifact& render,
               const StepTopologyCancellation* cancellation, std::size_t byte_limit,
               StepTopologyGlbWorkPacket* packet, Status* status,
               GlbEncodingEntryHook entry_hook = nullptr, void* entry_hook_context = nullptr)
{
    if (byte_limit < 36U)
    {
        set_status(status, kResourceLimit, "STEP topology GLB byte limit is below framing size.");
        return kResourceLimit;
    }
    if (entry_hook != nullptr)
        entry_hook(entry_hook_context);
    std::vector<unsigned char> binary;
    std::vector<BufferView> buffer_views;
    std::vector<Accessor> accessors;
    std::vector<MeshLayout> layouts;
    layouts.reserve(render.meshes.size());
    for (const StepTopologyRenderMesh& mesh : render.meshes)
    {
        if (cancelled(cancellation) || mesh.vertices.empty() || mesh.indices.empty())
        {
            set_status(status, cancelled(cancellation) ? kCancelled : kTransferFailed,
                       cancelled(cancellation) ? "STEP topology GLB encoding was cancelled."
                                               : "STEP topology GLB mesh is empty.");
            return cancelled(cancellation) ? kCancelled : kTransferFailed;
        }
        MeshLayout layout;
        layout.minimum = {std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::infinity()};
        layout.maximum = {-std::numeric_limits<double>::infinity(),
                          -std::numeric_limits<double>::infinity(),
                          -std::numeric_limits<double>::infinity()};
        align_four(&binary, 0);
        layout.positions_view = buffer_views.size();
        const std::size_t positions_offset = binary.size();
        for (const StepTopologyRenderVertex& vertex : mesh.vertices)
        {
            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                const float value =
                    static_cast<float>(vertex.position[axis] * kMillimetersToMeters);
                if (!std::isfinite(value))
                {
                    set_status(status, kTransferFailed,
                               "STEP topology GLB position exceeds FLOAT range.");
                    return kTransferFailed;
                }
                append_float(&binary, value);
                layout.minimum[axis] = std::min(layout.minimum[axis], static_cast<double>(value));
                layout.maximum[axis] = std::max(layout.maximum[axis], static_cast<double>(value));
            }
            const int progress = check_progress(cancellation, binary.size(), 0, byte_limit, status);
            if (progress != 0)
                return progress;
        }
        buffer_views.push_back({positions_offset, binary.size() - positions_offset, 34962});
        layout.positions_accessor = accessors.size();
        accessors.push_back({layout.positions_view, 5126, mesh.vertices.size(), "VEC3", true,
                             layout.minimum, layout.maximum});

        align_four(&binary, 0);
        layout.normals_view = buffer_views.size();
        const std::size_t normals_offset = binary.size();
        for (const StepTopologyRenderVertex& vertex : mesh.vertices)
        {
            for (double source_value : vertex.normal)
            {
                const float value = static_cast<float>(source_value);
                if (!std::isfinite(value))
                {
                    set_status(status, kTransferFailed,
                               "STEP topology GLB normal exceeds FLOAT range.");
                    return kTransferFailed;
                }
                append_float(&binary, value);
            }
            const int progress = check_progress(cancellation, binary.size(), 0, byte_limit, status);
            if (progress != 0)
                return progress;
        }
        buffer_views.push_back({normals_offset, binary.size() - normals_offset, 34962});
        layout.normals_accessor = accessors.size();
        accessors.push_back(
            {layout.normals_view, 5126, mesh.vertices.size(), "VEC3", false, {}, {}});

        layout.primitives.reserve(mesh.primitives.size());
        for (const StepTopologyRenderPrimitive& primitive : mesh.primitives)
        {
            if (cancelled(cancellation) || primitive.first_index > mesh.indices.size() ||
                primitive.index_count > mesh.indices.size() - primitive.first_index)
            {
                set_status(status, cancelled(cancellation) ? kCancelled : kInternalFailure,
                           cancelled(cancellation)
                               ? "STEP topology GLB encoding was cancelled."
                               : "STEP topology GLB primitive indices are invalid.");
                return cancelled(cancellation) ? kCancelled : kInternalFailure;
            }
            align_four(&binary, 0);
            PrimitiveLayout primitive_layout;
            primitive_layout.indices_view = buffer_views.size();
            const std::size_t offset = binary.size();
            for (std::size_t index = primitive.first_index;
                 index < primitive.first_index + primitive.index_count; ++index)
            {
                append_u32(&binary, mesh.indices[index]);
                const int progress =
                    check_progress(cancellation, binary.size(), 0, byte_limit, status);
                if (progress != 0)
                    return progress;
            }
            buffer_views.push_back({offset, binary.size() - offset, 34963});
            primitive_layout.indices_accessor = accessors.size();
            accessors.push_back({primitive_layout.indices_view,
                                 5125,
                                 primitive.index_count,
                                 "SCALAR",
                                 false,
                                 {},
                                 {}});
            layout.primitives.push_back(primitive_layout);
        }
        layouts.push_back(std::move(layout));
        const int progress = check_progress(cancellation, binary.size(), 0, byte_limit, status);
        if (progress != 0)
            return progress;
    }

    rapidjson::StringBuffer json_buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(json_buffer);
    const auto json_progress = [&]()
    {
        return check_progress(cancellation, binary.size(), json_buffer.GetSize(), byte_limit,
                              status);
    };
    writer.StartObject();
    writer.Key("asset");
    writer.StartObject();
    writer.Key("version");
    writer.String("2.0");
    writer.Key("generator");
    writer.String("Wavenumber Geometer experimental topology binding");
    writer.Key("extras");
    writer.StartObject();
    writer.Key("wn_geometer");
    writer.StartObject();
    writer.Key("schema");
    writer.String("wn.geometer.topology_glb_binding.a0");
    write_string(&writer, "source_sha256", render.session.source_sha256);
    write_string(&writer, "occt_version", render.session.occt_version);
    write_string(&writer, "session_handle", render.session.session_handle);
    writer.Key("generation");
    writer.Uint64(render.session.generation);
    write_string(&writer, "artifact_handle", render.artifact_handle);
    write_string(&writer, "content_sha256", render.content_sha256);
    writer.Key("binding_layout");
    writer.String("node-primitive-a0");
    writer.Key("geometry_length_unit");
    writer.String("meter");
    writer.Key("source_length_unit");
    writer.String("millimeter");
    writer.Key("material_policy");
    writer.String("single-neutral-research-material");
    writer.EndObject();
    writer.EndObject();
    writer.EndObject();
    if (const int progress = json_progress(); progress != 0)
        return progress;

    writer.Key("buffers");
    writer.StartArray();
    writer.StartObject();
    writer.Key("byteLength");
    writer.Uint64(binary.size());
    writer.EndObject();
    writer.EndArray();

    writer.Key("bufferViews");
    writer.StartArray();
    for (const BufferView& view : buffer_views)
    {
        writer.StartObject();
        writer.Key("buffer");
        writer.Int(0);
        writer.Key("byteOffset");
        writer.Uint64(view.offset);
        writer.Key("byteLength");
        writer.Uint64(view.length);
        writer.Key("target");
        writer.Int(view.target);
        writer.EndObject();
        if (const int progress = json_progress(); progress != 0)
            return progress;
    }
    writer.EndArray();

    writer.Key("accessors");
    writer.StartArray();
    for (const Accessor& accessor : accessors)
    {
        writer.StartObject();
        writer.Key("bufferView");
        writer.Uint64(accessor.buffer_view);
        writer.Key("componentType");
        writer.Int(accessor.component_type);
        writer.Key("count");
        writer.Uint64(accessor.count);
        writer.Key("type");
        writer.String(accessor.type);
        if (accessor.has_bounds)
        {
            writer.Key("min");
            writer.StartArray();
            for (double value : accessor.minimum)
                writer.Double(value);
            writer.EndArray();
            writer.Key("max");
            writer.StartArray();
            for (double value : accessor.maximum)
                writer.Double(value);
            writer.EndArray();
        }
        writer.EndObject();
        if (const int progress = json_progress(); progress != 0)
            return progress;
    }
    writer.EndArray();

    writer.Key("materials");
    writer.StartArray();
    writer.StartObject();
    writer.Key("name");
    writer.String("Geometer topology selection neutral");
    writer.Key("pbrMetallicRoughness");
    writer.StartObject();
    writer.Key("baseColorFactor");
    writer.StartArray();
    writer.Double(0.72);
    writer.Double(0.76);
    writer.Double(0.82);
    writer.Double(1.0);
    writer.EndArray();
    writer.Key("metallicFactor");
    writer.Double(0.0);
    writer.Key("roughnessFactor");
    writer.Double(0.72);
    writer.EndObject();
    writer.Key("doubleSided");
    writer.Bool(true);
    writer.EndObject();
    writer.EndArray();
    if (const int progress = json_progress(); progress != 0)
        return progress;

    writer.Key("meshes");
    writer.StartArray();
    for (std::size_t mesh_index = 0; mesh_index < render.meshes.size(); ++mesh_index)
    {
        const StepTopologyRenderMesh& mesh = render.meshes[mesh_index];
        const MeshLayout& layout = layouts[mesh_index];
        writer.StartObject();
        writer.Key("extras");
        writer.StartObject();
        writer.Key("wn_geometer");
        writer.StartObject();
        writer.Key("mesh_index");
        writer.Uint64(mesh_index);
        write_string(&writer, "definition_handle", mesh.definition_handle);
        writer.EndObject();
        writer.EndObject();
        writer.Key("primitives");
        writer.StartArray();
        for (std::size_t primitive_index = 0; primitive_index < mesh.primitives.size();
             ++primitive_index)
        {
            writer.StartObject();
            writer.Key("attributes");
            writer.StartObject();
            writer.Key("POSITION");
            writer.Uint64(layout.positions_accessor);
            writer.Key("NORMAL");
            writer.Uint64(layout.normals_accessor);
            writer.EndObject();
            writer.Key("indices");
            writer.Uint64(layout.primitives[primitive_index].indices_accessor);
            writer.Key("mode");
            writer.Int(4);
            writer.Key("material");
            writer.Int(0);
            write_binding_extras(&writer, mesh.primitives[primitive_index], primitive_index);
            writer.EndObject();
            if (const int progress = json_progress(); progress != 0)
                return progress;
        }
        writer.EndArray();
        writer.EndObject();
        if (const int progress = json_progress(); progress != 0)
            return progress;
    }
    writer.EndArray();

    writer.Key("nodes");
    writer.StartArray();
    for (std::size_t instance_index = 0; instance_index < render.instances.size(); ++instance_index)
    {
        const StepTopologyRenderInstance& instance = render.instances[instance_index];
        writer.StartObject();
        writer.Key("mesh");
        writer.Uint64(instance.mesh_index);
        writer.Key("matrix");
        writer.StartArray();
        const auto& value = instance.transform;
        for (std::size_t column = 0; column < 3; ++column)
        {
            writer.Double(value[column]);
            writer.Double(value[4U + column]);
            writer.Double(value[8U + column]);
            writer.Double(0.0);
        }
        writer.Double(value[3] * kMillimetersToMeters);
        writer.Double(value[7] * kMillimetersToMeters);
        writer.Double(value[11] * kMillimetersToMeters);
        writer.Double(1.0);
        writer.EndArray();
        writer.Key("extras");
        writer.StartObject();
        writer.Key("wn_geometer");
        writer.StartObject();
        writer.Key("instance_index");
        writer.Uint64(instance_index);
        writer.Key("mesh_index");
        writer.Uint64(instance.mesh_index);
        write_string(&writer, "occurrence_handle", instance.occurrence_handle);
        write_string(&writer, "definition_handle", instance.definition_handle);
        writer.Key("front_face_reversed");
        writer.Bool(instance.front_face_reversed);
        writer.EndObject();
        writer.EndObject();
        if (const int progress = json_progress(); progress != 0)
            return progress;
        writer.EndObject();
    }
    writer.EndArray();

    writer.Key("scenes");
    writer.StartArray();
    writer.StartObject();
    writer.Key("nodes");
    writer.StartArray();
    for (std::size_t index = 0; index < render.instances.size(); ++index)
        writer.Uint64(index);
    writer.EndArray();
    writer.EndObject();
    writer.EndArray();
    writer.Key("scene");
    writer.Int(0);
    writer.EndObject();
    if (const int progress = json_progress(); progress != 0)
        return progress;

    const std::size_t json_size = json_buffer.GetSize();
    const std::size_t padded_json_size = (json_size + 3U) & ~std::size_t{3U};
    const std::size_t binary_size = binary.size();
    align_four(&binary, 0);
    const std::size_t total_size = 12U + 8U + padded_json_size + 8U + binary.size();
    if (total_size > byte_limit || total_size > std::numeric_limits<std::uint32_t>::max())
    {
        set_status(status, kResourceLimit, "STEP topology GLB exceeds its byte limit.");
        return kResourceLimit;
    }
    if (cancelled(cancellation))
    {
        set_status(status, kCancelled, "STEP topology GLB encoding was cancelled.");
        return kCancelled;
    }
    packet->glb.reserve(total_size);
    append_u32(&packet->glb, 0x46546c67U);
    append_u32(&packet->glb, 2U);
    append_u32(&packet->glb, static_cast<std::uint32_t>(total_size));
    append_u32(&packet->glb, static_cast<std::uint32_t>(padded_json_size));
    append_u32(&packet->glb, 0x4e4f534aU);
    int append_code =
        append_chunk(&packet->glb, reinterpret_cast<const unsigned char*>(json_buffer.GetString()),
                     json_size, cancellation, byte_limit, status);
    if (append_code != 0)
        return append_code;
    while (packet->glb.size() % 4U != 0)
        packet->glb.push_back(' ');
    append_u32(&packet->glb, static_cast<std::uint32_t>(binary.size()));
    append_u32(&packet->glb, 0x004e4942U);
    append_code =
        append_chunk(&packet->glb, binary.data(), binary.size(), cancellation, byte_limit, status);
    if (append_code != 0)
        return append_code;
    packet->json_bytes = json_size;
    packet->binary_bytes = binary_size;
    return 0;
}

} // namespace

int encode_glb_from_render_for_test(const StepTopologyRenderArtifact& render,
                                    const StepTopologyCancellation* cancellation,
                                    std::size_t byte_limit, StepTopologyGlbWorkPacket* packet,
                                    Status* status, GlbEncodingEntryHook entry_hook,
                                    void* entry_hook_context)
{
    if (packet == nullptr)
    {
        set_status(status, kInvalidArgument, "STEP topology test GLB output pointer is null.");
        return kInvalidArgument;
    }
    *packet = {};
    const int code = encode_glb(render, cancellation, byte_limit, packet, status, entry_hook,
                                entry_hook_context);
    if (code != 0)
        *packet = {};
    return code;
}

int build_glb_work_packet(SessionData* data, const StepTopologyGlbOptions& options,
                          const StepTopologyCancellation* cancellation,
                          StepTopologyGlbWorkPacket* packet, Status* status)
{
    try
    {
        const int render_code = build_render_artifact(data, options.tessellation, cancellation,
                                                      &packet->render, status);
        if (render_code != 0)
        {
            *packet = {};
            return render_code;
        }
        const int encode_code = encode_glb(packet->render, cancellation,
                                           data->limits.max_render_glb_bytes, packet, status);
        if (encode_code != 0)
        {
            *packet = {};
            return encode_code;
        }
        const int digest_code =
            digest_glb(packet->glb, cancellation, &packet->content_sha256, status);
        if (digest_code != 0)
        {
            *packet = {};
            return digest_code;
        }
        packet->artifact_handle = glb_artifact_handle(*data, *packet);
        set_status(status, 0, "");
        return 0;
    }
    catch (const std::exception& error)
    {
        *packet = {};
        set_status(status, kInternalFailure, error.what());
        return kInternalFailure;
    }
}

bool verify_glb_work_packet_seal(const SessionData* data, const StepTopologyGlbWorkPacket& packet,
                                 Status* status)
{
    if (packet.glb.size() > data->limits.max_render_glb_bytes)
    {
        set_status(status, kResourceLimit, "STEP topology GLB exceeds its verification limit.");
        return false;
    }
    std::string content_digest;
    if (digest_glb(packet.glb, nullptr, &content_digest, status) != 0)
        return false;
    if (packet.content_sha256 != content_digest ||
        packet.artifact_handle != glb_artifact_handle(*data, packet) ||
        !verify_render_artifact_seal(data, packet.render, status))
    {
        set_status(status, kUnknownTarget,
                   "STEP topology GLB content, render binding, or session seal is invalid.");
        return false;
    }
    return true;
}

} // namespace geometer::step_topology_internal
