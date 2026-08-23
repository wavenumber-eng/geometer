#include "geometer/sha256.h"
#include "geometer/step_topology_session.h"
#include "step_topology_session_internal.h"

#include <rapidjson/document.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

std::vector<unsigned char> read_bytes(const std::string& relative_path)
{
    const std::string path = std::string(GEOMETER_TEST_SOURCE_DIR) + "/" + relative_path;
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "failed opening fixture: " + path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::unique_ptr<geometer::StepTopologySession>
open_fixture(const std::string& relative_path, const geometer::StepTopologyLimits& limits = {})
{
    const std::vector<unsigned char> bytes = read_bytes(relative_path);
    std::unique_ptr<geometer::StepTopologySession> session;
    geometer::Status status;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), limits, &session,
                                                     &status) == 0,
            "failed opening render fixture: " + status.message);
    return session;
}

std::array<double, 3> subtract(const std::array<double, 3>& first,
                               const std::array<double, 3>& second)
{
    return {first[0] - second[0], first[1] - second[1], first[2] - second[2]};
}

std::array<double, 3> cross(const std::array<double, 3>& first, const std::array<double, 3>& second)
{
    return {first[1] * second[2] - first[2] * second[1],
            first[2] * second[0] - first[0] * second[2],
            first[0] * second[1] - first[1] * second[0]};
}

double dot(const std::array<double, 3>& first, const std::array<double, 3>& second)
{
    return first[0] * second[0] + first[1] * second[1] + first[2] * second[2];
}

std::uint32_t read_u32(const std::vector<unsigned char>& bytes, std::size_t offset)
{
    require(offset <= bytes.size() && bytes.size() - offset >= 4U, "GLB u32 is out of bounds");
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

void repeated_occurrences_share_geometry_and_every_triangle_resolves()
{
    std::unique_ptr<geometer::StepTopologySession> session =
        open_fixture("tests/fixtures/step/generated_topology/generated_repeated_occurrences.step");
    geometer::StepTopologyRenderArtifact artifact;
    geometer::Status status;
    require(session->render({}, &artifact, &status) == 0,
            "failed rendering repeated fixture: " + status.message);
    require(artifact.research_format == "geometer.step_topology_render.research" &&
                artifact.session.generation == session->info().generation &&
                artifact.normalized_length_unit == "millimeter",
            "render artifact provenance is incomplete");
    require(artifact.meshes.size() == 1 && artifact.instances.size() == 4,
            "four leaf occurrences should share one definition mesh");
    require(std::all_of(artifact.instances.begin(), artifact.instances.end(),
                        [](const auto& instance) { return instance.mesh_index == 0; }),
            "repeated occurrences should reference the same mesh");
    const geometer::StepTopologyRenderMesh& mesh = artifact.meshes.front();
    require(mesh.primitives.size() == 6 && mesh.indices.size() / 3U == 12,
            "box mesh should expose six face primitives and twelve triangles");
    require(artifact.bindings.size() == 24 && artifact.geometry_triangle_count == 12 &&
                artifact.instanced_triangle_count == 48,
            "render binding cardinality is incorrect");

    std::set<std::string> occurrence_handles;
    for (std::size_t instance_index = 0; instance_index < artifact.instances.size();
         ++instance_index)
    {
        occurrence_handles.insert(artifact.instances[instance_index].occurrence_handle);
        for (std::size_t triangle_index = 0; triangle_index < mesh.indices.size() / 3U;
             ++triangle_index)
        {
            geometer::StepTopologyRenderHit hit;
            require(session->resolve_render_hit(artifact, instance_index, triangle_index, &hit,
                                                &status) == 0,
                    "render triangle failed reverse resolution: " + status.message);
            require(hit.instance_index == instance_index &&
                        hit.occurrence_handle ==
                            artifact.instances[instance_index].occurrence_handle,
                    "render hit lost occurrence context");
            geometer::StepTopologyResolvedTarget target;
            require(session->resolve(hit.occurrence_handle, &target, &status) == 0 &&
                        target.kind == geometer::StepTopologyTargetKind::occurrence,
                    "render occurrence target did not resolve");
            require(session->resolve(hit.body_handle, &target, &status) == 0 &&
                        target.kind == geometer::StepTopologyTargetKind::body,
                    "render body target did not resolve");
            require(session->resolve(hit.face_handle, &target, &status) == 0 &&
                        target.kind == geometer::StepTopologyTargetKind::face,
                    "render face target did not resolve");
        }
    }
    require(occurrence_handles.size() == artifact.instances.size(),
            "shared geometry must retain distinct occurrence targets");
    geometer::StepTopologyRenderHit out_of_bounds;
    require(session->resolve_render_hit(artifact, 0, mesh.indices.size() / 3U, &out_of_bounds,
                                        &status) != 0,
            "out-of-range render triangle should fail closed");
}

void winding_normals_coordinates_and_root_placement_are_explicit()
{
    std::unique_ptr<geometer::StepTopologySession> repeated =
        open_fixture("tests/fixtures/step/generated_topology/generated_repeated_occurrences.step");
    geometer::StepTopologyRenderArtifact artifact;
    geometer::Status status;
    require(repeated->render({}, &artifact, &status) == 0, "identity render failed");
    const auto& mesh = artifact.meshes.front();
    for (std::size_t triangle = 0; triangle < mesh.indices.size() / 3U; ++triangle)
    {
        const auto& first = mesh.vertices[mesh.indices[triangle * 3U]];
        const auto& second = mesh.vertices[mesh.indices[triangle * 3U + 1U]];
        const auto& third = mesh.vertices[mesh.indices[triangle * 3U + 2U]];
        const std::array<double, 3> geometric = cross(subtract(second.position, first.position),
                                                      subtract(third.position, first.position));
        const std::array<double, 3> average = {first.normal[0] + second.normal[0] + third.normal[0],
                                               first.normal[1] + second.normal[1] + third.normal[1],
                                               first.normal[2] + second.normal[2] +
                                                   third.normal[2]};
        require(dot(geometric, average) > 0.0,
                "face orientation, triangle winding, and normals disagree");
    }

    geometer::StepTopologyTessellationOptions mirrored;
    mirrored.source_to_render = {-1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
    geometer::StepTopologyRenderArtifact reflected;
    require(repeated->render(mirrored, &reflected, &status) == 0,
            "signed-rigid reflected render failed: " + status.message);
    require(std::all_of(reflected.instances.begin(), reflected.instances.end(),
                        [](const auto& instance) { return instance.front_face_reversed; }),
            "a reflected render frame must mark every instance's front face as reversed");
    require(std::abs(reflected.instances.front().transform[3] +
                     artifact.instances.front().transform[3]) < 1.0e-9,
            "source-to-render reflection should left-multiply occurrence translation");

    std::unique_ptr<geometer::StepTopologySession> placed =
        open_fixture("tests/fixtures/step/generated_topology/generated_fused_slab.step");
    geometer::StepTopologyRenderArtifact placed_artifact;
    require(placed->render({}, &placed_artifact, &status) == 0,
            "root-placement render failed: " + status.message);
    require(placed_artifact.instances.size() == 1 &&
                std::abs(placed_artifact.instances.front().transform[3] - 20.0) < 1.0e-9 &&
                std::abs(placed_artifact.instances.front().transform[0] + 1.0) < 1.0e-9 &&
                std::abs(placed_artifact.instances.front().transform[10] + 1.0) < 1.0e-9,
            "render instance should preserve the source root rotation and translation");
    const double local_min_x = std::min_element(placed_artifact.meshes.front().vertices.begin(),
                                                placed_artifact.meshes.front().vertices.end(),
                                                [](const auto& first, const auto& second)
                                                { return first.position[0] < second.position[0]; })
                                   ->position[0];
    require(std::abs(local_min_x) < 1.0e-9,
            "definition mesh must remain in its local modelling frame");
}

void render_limits_cancellation_and_generation_fail_closed()
{
    const std::string fixture =
        "tests/fixtures/step/generated_topology/generated_repeated_occurrences.step";
    geometer::StepTopologyLimits limits;
    limits.max_render_bindings = 1;
    std::unique_ptr<geometer::StepTopologySession> limited = open_fixture(fixture, limits);
    geometer::StepTopologyRenderArtifact rejected;
    geometer::Status status;
    require(limited->render({}, &rejected, &status) != 0 && rejected.meshes.empty() &&
                rejected.instances.empty() && rejected.bindings.empty(),
            "render binding limit must reject atomically");

    limits = {};
    limits.max_render_instanced_triangles = 47;
    limited = open_fixture(fixture, limits);
    require(limited->render({}, &rejected, &status) != 0 && rejected.bindings.empty(),
            "effective instanced-triangle limit must reject atomically");

    limits = {};
    limits.max_render_estimated_bytes = 1;
    limited = open_fixture(fixture, limits);
    require(limited->render({}, &rejected, &status) != 0 && rejected.meshes.empty(),
            "render artifact byte limit must reject atomically");

    std::unique_ptr<geometer::StepTopologySession> session = open_fixture(fixture);
    geometer::StepTopologyCancellation cancellation;
    cancellation.request_cancel();
    require(session->render({}, &cancellation, &rejected, &status) != 0 && rejected.meshes.empty(),
            "cancelled render must reject atomically");

    geometer::StepTopologyTessellationOptions scaling;
    scaling.source_to_render[0] = 2.0;
    require(session->render(scaling, &rejected, &status) != 0 && rejected.meshes.empty(),
            "scaled coordinate transforms are not valid render-frame transforms");

    geometer::StepTopologyRenderArtifact artifact;
    require(session->render({}, &artifact, &status) == 0, "render before refresh failed");
    require(artifact.artifact_handle.rfind("gtr_", 0) == 0 &&
                artifact.artifact_handle.size() == 68 && artifact.content_sha256.size() == 64,
            "render artifact should carry a session-secret seal and content digest");
    geometer::StepTopologyRenderHit hit;

    geometer::StepTopologyRenderArtifact forged = artifact;
    forged.bindings.front().face_handle = "gtt_forged";
    require(session->resolve_render_hit(forged, 0, 0, &hit, &status) != 0 &&
                hit.face_handle.empty(),
            "forged render target must fail closed");

    geometer::StepTopologyRenderArtifact mismatched = artifact;
    mismatched.bindings.front().primitive_index = 1;
    require(session->resolve_render_hit(mismatched, 0, 0, &hit, &status) != 0,
            "mismatched render primitive must fail closed");

    geometer::StepTopologyRenderArtifact overlapping = artifact;
    overlapping.bindings[1].first_triangle = overlapping.bindings[0].first_triangle;
    require(session->resolve_render_hit(overlapping, 0, 0, &hit, &status) != 0,
            "overlapping render spans must fail closed");

    geometer::StepTopologyRenderArtifact unsorted = artifact;
    std::swap(unsorted.bindings[0], unsorted.bindings[1]);
    require(session->resolve_render_hit(unsorted, 0, 0, &hit, &status) != 0,
            "unsorted render bindings must fail closed");

    std::unique_ptr<geometer::StepTopologySession> other = open_fixture(fixture);
    geometer::StepTopologyRenderArtifact restamped = artifact;
    restamped.session = other->info();
    require(other->resolve_render_hit(restamped, 0, 0, &hit, &status) != 0,
            "a render artifact re-stamped with another public session record must fail closed");

    geometer::StepTopologySnapshot refreshed;
    require(session->refresh(&refreshed, &status) == 0, "refresh after render failed");
    require(session->resolve_render_hit(artifact, 0, 0, &hit, &status) != 0,
            "stale render artifact should fail closed after generation refresh");

    require(other->resolve_render_hit(artifact, 0, 0, &hit, &status) != 0,
            "cross-session render artifact should fail closed");
}

void glb_packet_is_self_describing_exact_and_bounded()
{
    const std::string fixture =
        "tests/fixtures/step/generated_topology/generated_repeated_occurrences.step";
    std::unique_ptr<geometer::StepTopologySession> session = open_fixture(fixture);
    geometer::StepTopologyGlbWorkPacket packet;
    geometer::Status status;
    require(session->render_glb_work_packet({}, &packet, &status) == 0,
            "topology GLB packet failed: " + status.message);
    require(packet.research_format == "geometer.step_topology_glb_work_packet.research" &&
                packet.artifact_handle.rfind("gtg_", 0) == 0 &&
                packet.content_sha256 ==
                    geometer::sha256_hex(packet.glb.data(), packet.glb.size()) &&
                packet.render.instances.size() == 4 && !packet.glb.empty(),
            "topology GLB packet result is incomplete");
    const geometer::StepTopologyGlbWorkPacket sealed_packet = packet;
    require(read_u32(packet.glb, 0) == 0x46546c67U && read_u32(packet.glb, 4) == 2U &&
                read_u32(packet.glb, 8) == packet.glb.size() &&
                read_u32(packet.glb, 16) == 0x4e4f534aU,
            "topology GLB header is invalid");
    const std::size_t json_size = read_u32(packet.glb, 12);
    require(json_size >= packet.json_bytes && 20U + json_size + 8U <= packet.glb.size(),
            "topology GLB JSON chunk is invalid");
    rapidjson::Document document;
    document.Parse(reinterpret_cast<const char*>(packet.glb.data() + 20U), json_size);
    require(!document.HasParseError() && document.IsObject(), "topology GLB JSON is invalid");
    const auto& metadata = document["asset"]["extras"]["wn_geometer"];
    require(std::string(metadata["schema"].GetString()) == "wn.geometer.topology_glb_binding.a0" &&
                std::string(metadata["artifact_handle"].GetString()) ==
                    packet.render.artifact_handle &&
                std::string(metadata["content_sha256"].GetString()) == packet.render.content_sha256,
            "topology GLB asset metadata does not bind the native artifact");
    require(document["meshes"].Size() == 1 && document["nodes"].Size() == 4 &&
                document["meshes"][0]["primitives"].Size() == 6,
            "topology GLB should share one six-face mesh across four nodes");
    for (rapidjson::SizeType index = 0; index < document["nodes"].Size(); ++index)
    {
        const auto& node = document["nodes"][index]["extras"]["wn_geometer"];
        require(node["instance_index"].GetUint64() == index &&
                    std::string(node["occurrence_handle"].GetString()) ==
                        packet.render.instances[index].occurrence_handle,
                "topology GLB node lost occurrence binding");
    }
    for (rapidjson::SizeType index = 0; index < document["meshes"][0]["primitives"].Size(); ++index)
    {
        const auto& binding = document["meshes"][0]["primitives"][index]["extras"]["wn_geometer"];
        require(binding["primitive_index"].GetUint64() == index &&
                    std::string(binding["face_handle"].GetString()) ==
                        packet.render.meshes[0].primitives[index].face_handle,
                "topology GLB primitive lost face binding");
    }
    require(std::abs(packet.render.source_to_render[0] - 1.0) < 1.0e-12 &&
                std::abs(packet.render.source_to_render[6] - 1.0) < 1.0e-12 &&
                std::abs(packet.render.source_to_render[9] + 1.0) < 1.0e-12,
            "topology GLB must explicitly convert OCCT Z-up to glTF Y-up");

    geometer::StepTopologyGlbHitDescriptor descriptor;
    descriptor.artifact_handle = packet.artifact_handle;
    descriptor.content_sha256 = packet.content_sha256;
    descriptor.instance_index = 0;
    descriptor.primitive_index = 0;
    descriptor.primitive_triangle_index = 0;
    descriptor.occurrence_handle = packet.render.instances[0].occurrence_handle;
    descriptor.body_handle = packet.render.meshes[0].primitives[0].body_handle;
    descriptor.face_handle = packet.render.meshes[0].primitives[0].face_handle;
    geometer::StepTopologyRenderHit hit;
    require(session->resolve_glb_hit(packet, descriptor, &hit, &status) == 0 &&
                hit.instance_index == 0 && hit.primitive_index == 0 && hit.triangle_index == 0,
            "sealed topology GLB hit did not resolve through the native return path");

    geometer::StepTopologyGlbWorkPacket modified_geometry = packet;
    const std::size_t binary_offset = 28U + read_u32(modified_geometry.glb, 12U);
    require(binary_offset < modified_geometry.glb.size(), "GLB BIN offset is invalid");
    modified_geometry.glb[binary_offset] ^= 1U;
    require(session->resolve_glb_hit(modified_geometry, descriptor, &hit, &status) != 0,
            "modified GLB geometry must invalidate the native artifact seal");
    geometer::StepTopologyGlbHitDescriptor forged_target = descriptor;
    forged_target.face_handle = packet.render.meshes[0].primitives[1].face_handle;
    require(session->resolve_glb_hit(packet, forged_target, &hit, &status) != 0,
            "GLB hit metadata must match the authoritative native target");
    geometer::StepTopologyGlbHitDescriptor wrong_primitive = descriptor;
    wrong_primitive.primitive_index = 1;
    require(session->resolve_glb_hit(packet, wrong_primitive, &hit, &status) != 0,
            "GLB primitive and claimed face must agree");

    geometer::StepTopologyLimits limits;
    limits.max_render_glb_bytes = 1;
    std::unique_ptr<geometer::StepTopologySession> limited = open_fixture(fixture, limits);
    require(limited->render_glb_work_packet({}, &packet, &status) != 0 && packet.glb.empty() &&
                packet.render.meshes.empty(),
            "topology GLB byte limit must reject atomically");

    geometer::StepTopologyCancellation cancellation;
    cancellation.request_cancel();
    require(session->render_glb_work_packet({}, &cancellation, &packet, &status) != 0 &&
                packet.glb.empty(),
            "cancelled topology GLB packet must reject atomically");

    std::unique_ptr<geometer::StepTopologySession> other = open_fixture(fixture);
    descriptor.artifact_handle = sealed_packet.artifact_handle;
    descriptor.content_sha256 = sealed_packet.content_sha256;
    require(other->resolve_glb_hit(sealed_packet, descriptor, &hit, &status) != 0,
            "cross-session GLB hit must fail closed");
    geometer::StepTopologyGlbWorkPacket live_packet;
    require(session->render_glb_work_packet({}, &live_packet, &status) == 0,
            "fresh GLB packet failed before stale-generation check");
    descriptor.artifact_handle = live_packet.artifact_handle;
    descriptor.content_sha256 = live_packet.content_sha256;
    descriptor.occurrence_handle = live_packet.render.instances[0].occurrence_handle;
    descriptor.body_handle = live_packet.render.meshes[0].primitives[0].body_handle;
    descriptor.face_handle = live_packet.render.meshes[0].primitives[0].face_handle;
    geometer::StepTopologySnapshot refreshed;
    require(session->refresh(&refreshed, &status) == 0,
            "refresh failed before stale GLB generation check");
    require(session->resolve_glb_hit(live_packet, descriptor, &hit, &status) != 0,
            "stale-generation GLB hit must fail closed");
}

void incremental_sha256_matches_the_standard_vector()
{
    geometer::Sha256Builder hash;
    const std::array<std::uint8_t, 1> first = {'a'};
    const std::array<std::uint8_t, 2> remainder = {'b', 'c'};
    hash.update(first.data(), first.size());
    hash.update(remainder.data(), remainder.size());
    require(hash.hex_digest() == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            "incremental SHA-256 changed the standard abc vector");

    const std::string multi_block = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    geometer::Sha256Builder split;
    split.update(reinterpret_cast<const std::uint8_t*>(multi_block.data()), 13);
    split.update(reinterpret_cast<const std::uint8_t*>(multi_block.data() + 13),
                 multi_block.size() - 13U);
    require(split.hex_digest() ==
                "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
            "incremental SHA-256 changed the standard multi-block vector");

    const std::array<std::uint8_t, 1000> thousand_a = []
    {
        std::array<std::uint8_t, 1000> value{};
        value.fill('a');
        return value;
    }();
    geometer::Sha256Builder million;
    for (std::size_t index = 0; index < 1000; ++index)
    {
        million.update(thousand_a.data(), thousand_a.size());
    }
    require(million.hex_digest() ==
                "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0",
            "incremental SHA-256 changed the standard million-a vector");
}

void in_flight_glb_cancellation_is_observed()
{
    std::unique_ptr<geometer::StepTopologySession> session =
        open_fixture("tests/fixtures/step/embedded_models/ABM3B.STEP");
    geometer::StepTopologyRenderArtifact render;
    geometer::Status status;
    require(session->render({}, &render, &status) == 0,
            "prebuilt render artifact failed before encoder cancellation test");
    geometer::StepTopologyCancellation cancellation;
    geometer::StepTopologyGlbWorkPacket packet;
    std::atomic<bool> entered{false};
    struct EntryContext
    {
        std::atomic<bool>* entered = nullptr;
        const geometer::StepTopologyCancellation* cancellation = nullptr;
    } context{&entered, &cancellation};
    const auto entry_hook = [](void* opaque)
    {
        auto* value = static_cast<EntryContext*>(opaque);
        value->entered->store(true, std::memory_order_release);
        while (!value->cancellation->is_cancelled())
            std::this_thread::yield();
    };
    int code = 0;
    std::thread worker(
        [&]()
        {
            code = geometer::step_topology_internal::encode_glb_from_render_for_test(
                render, &cancellation, 512U * 1024U * 1024U, &packet, &status, entry_hook,
                &context);
        });
    while (!entered.load(std::memory_order_acquire))
        std::this_thread::yield();
    cancellation.request_cancel();
    worker.join();
    require(code != 0 && packet.glb.empty(),
            "in-flight GLB cancellation must interrupt and reject atomically");
}

} // namespace

int main()
{
    try
    {
        repeated_occurrences_share_geometry_and_every_triangle_resolves();
        winding_normals_coordinates_and_root_placement_are_explicit();
        render_limits_cancellation_and_generation_fail_closed();
        glb_packet_is_self_describing_exact_and_bounded();
        in_flight_glb_cancellation_is_observed();
        incremental_sha256_matches_the_standard_vector();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
