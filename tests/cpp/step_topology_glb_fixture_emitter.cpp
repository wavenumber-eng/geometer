#include "geometer/step_topology_session.h"

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

namespace fs = std::filesystem;

std::vector<unsigned char> read_bytes(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("failed opening STEP input");
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_bytes(const fs::path& path, const std::vector<unsigned char>& bytes)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        throw std::runtime_error("failed opening GLB output");
    }
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    if (!output)
    {
        throw std::runtime_error("failed writing GLB output");
    }
}

void write_expected(const fs::path& path, const geometer::StepTopologyGlbWorkPacket& packet)
{
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    writer.SetIndent(' ', 2);
    writer.StartObject();
    writer.Key("schema");
    writer.String("wn.geometer.topology_glb_raycast_expected.a0");
    writer.Key("source_sha256");
    writer.String(packet.render.session.source_sha256.c_str());
    writer.Key("session_handle");
    writer.String(packet.render.session.session_handle.c_str());
    writer.Key("generation");
    writer.Uint64(packet.render.session.generation);
    writer.Key("artifact_handle");
    writer.String(packet.artifact_handle.c_str());
    writer.Key("content_sha256");
    writer.String(packet.content_sha256.c_str());
    writer.Key("render_artifact_handle");
    writer.String(packet.render.artifact_handle.c_str());
    writer.Key("render_content_sha256");
    writer.String(packet.render.content_sha256.c_str());
    writer.Key("mesh_count");
    writer.Uint64(packet.render.meshes.size());
    writer.Key("instances");
    writer.StartArray();
    for (std::size_t instance_index = 0; instance_index < packet.render.instances.size();
         ++instance_index)
    {
        const auto& instance = packet.render.instances[instance_index];
        const auto& mesh = packet.render.meshes[instance.mesh_index];
        writer.StartObject();
        writer.Key("instance_index");
        writer.Uint64(instance_index);
        writer.Key("occurrence_handle");
        writer.String(instance.occurrence_handle.c_str());
        writer.Key("mesh_index");
        writer.Uint64(instance.mesh_index);
        writer.Key("definition_handle");
        writer.String(instance.definition_handle.c_str());
        writer.Key("front_face_reversed");
        writer.Bool(instance.front_face_reversed);
        writer.Key("transform");
        writer.StartArray();
        for (double value : instance.transform)
        {
            writer.Double(value);
        }
        writer.EndArray();
        writer.Key("primitives");
        writer.StartArray();
        for (std::size_t primitive_index = 0; primitive_index < mesh.primitives.size();
             ++primitive_index)
        {
            const auto& primitive = mesh.primitives[primitive_index];
            writer.StartObject();
            writer.Key("primitive_index");
            writer.Uint64(primitive_index);
            writer.Key("body_handle");
            writer.String(primitive.body_handle.c_str());
            writer.Key("face_handle");
            writer.String(primitive.face_handle.c_str());
            writer.Key("first_triangle");
            writer.Uint64(primitive.first_triangle);
            writer.Key("triangle_count");
            writer.Uint64(primitive.triangle_count);
            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output || !(output << std::string(buffer.GetString(), buffer.GetSize()) << '\n'))
    {
        throw std::runtime_error("failed writing raycast expectation");
    }
}

void validate_native_return_path(geometer::StepTopologySession* session,
                                 const geometer::StepTopologyGlbWorkPacket& packet)
{
    geometer::Status status;
    for (std::size_t instance_index = 0; instance_index < packet.render.instances.size();
         ++instance_index)
    {
        const auto& instance = packet.render.instances[instance_index];
        const auto& mesh = packet.render.meshes[instance.mesh_index];
        for (std::size_t primitive_index = 0; primitive_index < mesh.primitives.size();
             ++primitive_index)
        {
            const auto& primitive = mesh.primitives[primitive_index];
            geometer::StepTopologyGlbHitDescriptor descriptor;
            descriptor.artifact_handle = packet.artifact_handle;
            descriptor.content_sha256 = packet.content_sha256;
            descriptor.instance_index = instance_index;
            descriptor.primitive_index = primitive_index;
            descriptor.primitive_triangle_index = 0;
            descriptor.occurrence_handle = instance.occurrence_handle;
            descriptor.body_handle = primitive.body_handle;
            descriptor.face_handle = primitive.face_handle;
            geometer::StepTopologyRenderHit hit;
            if (session->resolve_glb_hit(packet, descriptor, &hit, &status) != 0)
            {
                throw std::runtime_error("native GLB return-path validation failed: " +
                                         status.message);
            }
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    try
    {
        if (argc != 4 && argc != 5)
        {
            std::cerr << "usage: step_topology_glb_fixture_emitter STEP GLB EXPECTED_JSON "
                         "[--reflect-x]\n";
            return 2;
        }
        if (argc == 5 && std::string(argv[4]) != "--reflect-x")
        {
            throw std::runtime_error("unknown GLB fixture-emitter option");
        }
        const std::vector<unsigned char> source = read_bytes(argv[1]);
        std::unique_ptr<geometer::StepTopologySession> session;
        geometer::Status status;
        if (geometer::StepTopologySession::open_step(source.data(), source.size(), {}, &session,
                                                     &status) != 0)
        {
            throw std::runtime_error("failed opening topology session: " + status.message);
        }
        geometer::StepTopologyGlbWorkPacket packet;
        geometer::StepTopologyGlbOptions options;
        if (argc == 5)
        {
            options.tessellation.source_to_render = {-1.0, 0.0, 0.0, 0.0,  0.0, 0.0,
                                                     1.0,  0.0, 0.0, -1.0, 0.0, 0.0};
        }
        if (session->render_glb_work_packet(options, &packet, &status) != 0)
        {
            throw std::runtime_error("failed rendering topology GLB: " + status.message);
        }
        validate_native_return_path(session.get(), packet);
        write_bytes(argv[2], packet.glb);
        write_expected(argv[3], packet);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
