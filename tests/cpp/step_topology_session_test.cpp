#include "geometer/step_topology_session.h"

#include <rapidjson/document.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
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

std::vector<std::string> all_handles(const geometer::StepTopologySnapshot& snapshot)
{
    std::vector<std::string> handles;
    for (const auto& item : snapshot.definitions)
        handles.push_back(item.handle);
    for (const auto& item : snapshot.root_occurrences)
        handles.push_back(item.handle);
    for (const auto& item : snapshot.occurrences)
        handles.push_back(item.handle);
    for (const auto& item : snapshot.bodies)
        handles.push_back(item.handle);
    for (const auto& item : snapshot.shells)
        handles.push_back(item.handle);
    for (const auto& item : snapshot.faces)
        handles.push_back(item.handle);
    return handles;
}

void require_translation(const geometer::StepTopologyOccurrence& occurrence,
                         const std::array<double, 3>& expected)
{
    constexpr double tolerance = 1.0e-9;
    require(std::abs(occurrence.transform[3] - expected[0]) < tolerance,
            "unexpected occurrence x translation: " + std::to_string(occurrence.transform[3]) +
                " expected " + std::to_string(expected[0]));
    require(std::abs(occurrence.transform[7] - expected[1]) < tolerance,
            "unexpected occurrence y translation");
    require(std::abs(occurrence.transform[11] - expected[2]) < tolerance,
            "unexpected occurrence z translation");
}

void require_rotation(const geometer::StepTopologyOccurrence& occurrence,
                      const std::array<double, 9>& expected)
{
    constexpr double tolerance = 1.0e-9;
    const std::array<double, 9> actual = {
        occurrence.transform[0], occurrence.transform[1], occurrence.transform[2],
        occurrence.transform[4], occurrence.transform[5], occurrence.transform[6],
        occurrence.transform[8], occurrence.transform[9], occurrence.transform[10]};
    for (std::size_t index = 0; index < actual.size(); ++index)
    {
        require(std::abs(actual[index] - expected[index]) < tolerance,
                "unexpected occurrence rotation");
    }
}

std::unique_ptr<geometer::StepTopologySession>
open_repeated(const geometer::StepTopologyLimits& limits = {})
{
    const std::vector<unsigned char> bytes =
        read_bytes("tests/fixtures/step/generated_topology/generated_repeated_occurrences.step");
    std::unique_ptr<geometer::StepTopologySession> session;
    geometer::Status status;
    const int code = geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), limits,
                                                              &session, &status);
    require(code == 0, "failed opening repeated session: " + status.message);
    return session;
}

void inspect_nested_occurrences_and_topology()
{
    std::unique_ptr<geometer::StepTopologySession> session = open_repeated();
    require(session->is_open(), "new topology session should be open");
    require(session->info().generation == 1, "new topology session generation should be one");
    require(session->info().session_handle.size() == 68 &&
                session->info().session_handle.rfind("gts_", 0) == 0,
            "session handle should be an opaque fixed-size token");
    require(session->info().source_sha256.size() == 64, "source SHA-256 should be exact");
    require(!session->info().occt_version.empty(), "OCCT version should be recorded");
    require(session->info().estimated_resident_bytes > session->info().source_bytes,
            "resident estimate should include normalized topology");

    std::unique_ptr<geometer::StepTopologySession> distinct_session = open_repeated();
    require(distinct_session->info().session_handle != session->info().session_handle,
            "independent sessions must have distinct OS-random handles");

    geometer::StepTopologySnapshot snapshot;
    geometer::Status status;
    int code = session->inspect({}, &snapshot, &status);
    require(code == 0, "failed inspecting repeated session: " + status.message);
    require(snapshot.research_format == "geometer.step_topology_inspection.research",
            "unexpected research format");
    require(snapshot.session.session_handle == session->info().session_handle,
            "snapshot should carry the owning session handle");
    require(snapshot.definitions.size() == 3, "nested fixture should have three definitions");
    require(snapshot.root_occurrences.size() == 1,
            "nested fixture should expose one explicit root occurrence");
    require(snapshot.component_label_count == 4,
            "nested fixture should have four unique component labels");
    require(snapshot.occurrences.size() == 6,
            "nested fixture should expand to six occurrence paths");
    require(snapshot.bodies.size() == 1, "box definition should have one body");
    require(snapshot.shells.size() == 1, "box definition should have one shell");
    require(snapshot.faces.size() == 6, "box definition should have six faces");
    require(snapshot.diagnostic_carriers.empty(), "diagnostic carrier labels should be opt-in");
    require(snapshot.reader_posture.metadata && snapshot.reader_posture.product_metadata &&
                snapshot.reader_posture.gdt && snapshot.reader_posture.materials,
            "reader posture should report metadata modes explicitly");
    require(!snapshot.reader_posture.constructive_geometry &&
                !snapshot.reader_posture.nonmanifold &&
                !snapshot.reader_posture.all_top_level_shapes,
            "disabled reader modes should be reported explicitly");
    require(snapshot.metadata.named_labels > 0 && snapshot.metadata.named_data_labels > 0,
            "normal XCAF name and NamedData metadata should be inspected");
    require(snapshot.metadata.mapped_source_entities == 0 &&
                snapshot.metadata.unmapped_source_entities == 0,
            "source entity evidence must be omitted by default");
    require(
        std::all_of(
            snapshot.faces.begin(), snapshot.faces.end(), [](const auto& face)
            { return !face.source_entity.mapped && face.source_entity.mapping_method.empty(); }),
        "default inspection must clear per-target source evidence");
    require(snapshot.metadata.material_definitions > 0 &&
                snapshot.metadata.material_assignments > 0 &&
                snapshot.definitions.back().label.has_material_assignment,
            "generated material definition and shape assignment should survive STEP round-trip");

    geometer::StepTopologyInspectionOptions source_options;
    source_options.include_source_entity_evidence = true;
    geometer::StepTopologySnapshot source_snapshot;
    code = session->inspect(source_options, &source_snapshot, &status);
    require(code == 0, "source-evidence inspection failed: " + status.message);
    require(source_snapshot.metadata.mapped_source_entities +
                    source_snapshot.metadata.unmapped_source_entities ==
                source_snapshot.definitions.size() + source_snapshot.bodies.size() +
                    source_snapshot.shells.size() + source_snapshot.faces.size(),
            "opt-in inspection should report transfer-map evidence for every topology target");
    require(source_snapshot.metadata.mapped_source_entities == 0 &&
                source_snapshot.metadata.unmapped_source_entities ==
                    source_snapshot.definitions.size() + source_snapshot.bodies.size() +
                        source_snapshot.shells.size() + source_snapshot.faces.size(),
            "generated AP242 fixture should preserve the measured unmapped transfer-hook result");

    constexpr std::array<std::array<double, 3>, 6> translations = {
        std::array{0.0, 0.0, 0.0},  std::array{0.0, 0.0, 0.0},  std::array{5.0, 0.0, 0.0},
        std::array{0.0, 10.0, 0.0}, std::array{0.0, 10.0, 0.0}, std::array{0.0, 15.0, 0.0}};
    constexpr std::array<double, 9> identity_rotation = {1.0, 0.0, 0.0, 0.0, 1.0,
                                                         0.0, 0.0, 0.0, 1.0};
    constexpr std::array<double, 9> quarter_turn_rotation = {0.0, -1.0, 0.0, 1.0, 0.0,
                                                             0.0, 0.0,  0.0, 1.0};
    for (std::size_t index = 0; index < snapshot.occurrences.size(); ++index)
    {
        require_translation(snapshot.occurrences[index], translations[index]);
        require_rotation(snapshot.occurrences[index],
                         index < 3 ? identity_rotation : quarter_turn_rotation);
    }
    require(snapshot.occurrences[0].parent_occurrence_handle ==
                snapshot.root_occurrences.front().handle,
            "first-level occurrence should reference its explicit root occurrence");
    require(snapshot.occurrences[1].parent_occurrence_handle == snapshot.occurrences[0].handle,
            "nested occurrence should reference its expanded parent");
    require(snapshot.occurrences[4].parent_occurrence_handle == snapshot.occurrences[3].handle,
            "reused nested occurrence should reference its distinct parent path");

    const std::vector<std::string> handles = all_handles(snapshot);
    const std::set<std::string> unique(handles.begin(), handles.end());
    require(unique.size() == handles.size(), "every session target handle should be unique");
    require(std::all_of(handles.begin(), handles.end(), [](const std::string& handle)
                        { return handle.size() == 68 && handle.rfind("gtt_", 0) == 0; }),
            "target handles should be opaque fixed-size tokens");

    geometer::StepTopologyResolvedTarget resolved;
    for (const std::pair<std::string, geometer::StepTopologyTargetKind>& target :
         {std::pair{snapshot.definitions.front().handle,
                    geometer::StepTopologyTargetKind::definition},
          std::pair{snapshot.occurrences.front().handle,
                    geometer::StepTopologyTargetKind::occurrence},
          std::pair{snapshot.root_occurrences.front().handle,
                    geometer::StepTopologyTargetKind::occurrence},
          std::pair{snapshot.bodies.front().handle, geometer::StepTopologyTargetKind::body},
          std::pair{snapshot.shells.front().handle, geometer::StepTopologyTargetKind::shell},
          std::pair{snapshot.faces.front().handle, geometer::StepTopologyTargetKind::face}})
    {
        code = session->resolve(target.first, &resolved, &status);
        require(code == 0 && resolved.kind == target.second,
                "target handle should resolve to its registered kind");
    }
    code = session->resolve("gtt_forged", &resolved, &status);
    require(code != 0, "forged target handle should fail closed");

    geometer::StepTopologyInspectionOptions diagnostic_options;
    diagnostic_options.include_diagnostic_carriers = true;
    diagnostic_options.include_source_entity_evidence = false;
    geometer::StepTopologySnapshot diagnostic_snapshot;
    code = session->inspect(diagnostic_options, &diagnostic_snapshot, &status);
    require(code == 0, "diagnostic inspection failed: " + status.message);
    require(!diagnostic_snapshot.diagnostic_carriers.empty(),
            "diagnostic inspection should expose XCAF carrier labels");
    require(std::none_of(diagnostic_snapshot.diagnostic_carriers.begin(),
                         diagnostic_snapshot.diagnostic_carriers.end(),
                         [](const geometer::StepTopologyDiagnosticCarrier& carrier)
                         {
                             return carrier.xcaf_label_entry.find("memory.step") !=
                                        std::string::npos ||
                                    carrier.xcaf_label_entry.find("\\") != std::string::npos ||
                                    carrier.xcaf_label_entry.find("/") != std::string::npos;
                         }),
            "diagnostic carriers should not expose source or temporary paths");
    require(diagnostic_snapshot.metadata.mapped_source_entities == 0 &&
                diagnostic_snapshot.metadata.unmapped_source_entities == 0,
            "source entity evidence should be removable from a snapshot");
}

void handles_fail_closed_across_generation_session_and_close()
{
    std::unique_ptr<geometer::StepTopologySession> first = open_repeated();
    std::unique_ptr<geometer::StepTopologySession> second = open_repeated();
    geometer::StepTopologySnapshot initial;
    geometer::Status status;
    require(first->inspect({}, &initial, &status) == 0, "initial inspect failed");
    const std::string old_face = initial.faces.front().handle;

    geometer::StepTopologyResolvedTarget resolved;
    require(second->resolve(old_face, &resolved, &status) != 0,
            "cross-session target should fail closed");

    geometer::StepTopologySnapshot refreshed;
    require(first->refresh(&refreshed, &status) == 0, "refresh failed: " + status.message);
    require(refreshed.session.generation == 2, "refresh should increment generation");
    require(refreshed.faces.front().handle != old_face,
            "refresh should replace generation-scoped handles");
    require(first->resolve(old_face, &resolved, &status) != 0,
            "stale-generation target should fail closed");
    require(first->resolve(refreshed.faces.front().handle, &resolved, &status) == 0,
            "refreshed target should resolve");

    require(first->close(&status) == 0, "close failed: " + status.message);
    require(!first->is_open(), "closed session should report closed");
    require(first->inspect({}, &initial, &status) != 0, "closed session should not inspect");
    require(first->resolve(refreshed.faces.front().handle, &resolved, &status) != 0,
            "closed session should invalidate every target");
}

void cancelled_open_and_refresh_are_atomic()
{
    const std::vector<unsigned char> bytes =
        read_bytes("tests/fixtures/step/generated_topology/generated_repeated_occurrences.step");
    geometer::StepTopologyCancellation cancelled_open;
    cancelled_open.request_cancel();
    std::unique_ptr<geometer::StepTopologySession> rejected = open_repeated();
    geometer::Status status;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {},
                                                     &cancelled_open, &rejected, &status) != 0,
            "pre-cancelled open should fail");
    require(rejected == nullptr, "failed open must clear the session output");

    std::unique_ptr<geometer::StepTopologySession> session = open_repeated();
    geometer::StepTopologySnapshot before;
    require(session->inspect({}, &before, &status) == 0, "pre-refresh inspect failed");
    const std::string old_face = before.faces.front().handle;

    geometer::StepTopologyCancellation cancelled_refresh;
    cancelled_refresh.request_cancel();
    geometer::StepTopologySnapshot failed = before;
    require(session->refresh(&cancelled_refresh, &failed, &status) != 0,
            "pre-cancelled refresh should fail");
    require(failed.faces.empty() && failed.session.session_handle.empty(),
            "failed refresh must clear its output snapshot");
    require(session->info().generation == before.session.generation,
            "failed refresh must preserve the previous generation");
    geometer::StepTopologyResolvedTarget resolved;
    require(session->resolve(old_face, &resolved, &status) == 0,
            "failed refresh must preserve prior generation handles");
}

void flat_multi_solid_has_two_bodies_without_occurrences()
{
    const std::vector<unsigned char> bytes =
        read_bytes("tests/fixtures/step/generated_topology/generated_flat_multi_solid.step");
    std::unique_ptr<geometer::StepTopologySession> session;
    geometer::Status status;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &session,
                                                     &status) == 0,
            "failed opening multi-solid fixture: " + status.message);
    geometer::StepTopologySnapshot snapshot;
    require(session->inspect({}, &snapshot, &status) == 0, "multi-solid inspect failed");
    require(snapshot.definitions.size() == 1 && snapshot.occurrences.empty(),
            "flat fixture should have one definition and no occurrences");
    require(snapshot.root_occurrences.size() == 1,
            "flat fixture should have one explicit root occurrence");
    require(snapshot.bodies.size() == 2, "flat fixture should expose two independent bodies");
    require(snapshot.shells.size() == 2 && snapshot.faces.size() == 12,
            "flat fixture topology counts are wrong");
    require(snapshot.definitions.front().body_handles.size() == 2,
            "definition should own both body handles");
    require(std::all_of(snapshot.bodies.begin(), snapshot.bodies.end(),
                        [](const geometer::StepTopologyBody& body)
                        { return body.topology_kind == "solid" && body.volume > 0.0; }),
            "flat fixture bodies should be volume-bearing solids");

    const auto contains = [](const std::vector<std::string>& handles, const std::string& handle)
    { return std::find(handles.begin(), handles.end(), handle) != handles.end(); };
    for (const auto& body : snapshot.bodies)
    {
        require(!body.shell_handles.empty() && !body.face_handles.empty(),
                "each body should expose shell and face membership");
        for (const std::string& shell_handle : body.shell_handles)
        {
            const auto shell = std::find_if(snapshot.shells.begin(), snapshot.shells.end(),
                                            [&shell_handle](const auto& item)
                                            { return item.handle == shell_handle; });
            require(shell != snapshot.shells.end() && contains(shell->body_handles, body.handle),
                    "body-to-shell membership must be reciprocal");
        }
        for (const std::string& face_handle : body.face_handles)
        {
            const auto face = std::find_if(snapshot.faces.begin(), snapshot.faces.end(),
                                           [&face_handle](const auto& item)
                                           { return item.handle == face_handle; });
            require(face != snapshot.faces.end() && contains(face->body_handles, body.handle),
                    "body-to-face membership must be reciprocal");
        }
    }
    for (const auto& shell : snapshot.shells)
    {
        for (const std::string& face_handle : shell.face_handles)
        {
            const auto face = std::find_if(snapshot.faces.begin(), snapshot.faces.end(),
                                           [&face_handle](const auto& item)
                                           { return item.handle == face_handle; });
            require(face != snapshot.faces.end() && contains(face->shell_handles, shell.handle),
                    "shell-to-face membership must be reciprocal");
        }
    }
}

void root_placement_is_separate_from_definition_geometry()
{
    const std::vector<unsigned char> bytes =
        read_bytes("tests/fixtures/step/generated_topology/generated_fused_slab.step");
    std::unique_ptr<geometer::StepTopologySession> session;
    geometer::Status status;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &session,
                                                     &status) == 0,
            "failed opening root-placement fixture: " + status.message);
    geometer::StepTopologySnapshot snapshot;
    require(session->inspect({}, &snapshot, &status) == 0,
            "root-placement inspection failed: " + status.message);
    require(snapshot.root_occurrences.size() == 1 && snapshot.bodies.size() == 1,
            "root-placement fixture should expose one root and one local body");
    constexpr std::array<double, 9> half_turn_y = {-1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, -1.0};
    geometer::StepTopologyOccurrence root_as_occurrence;
    root_as_occurrence.transform = snapshot.root_occurrences.front().transform;
    require_translation(root_as_occurrence, {20.0, 0.0, 0.0});
    require_rotation(root_as_occurrence, half_turn_y);
    const auto& bounds = snapshot.bodies.front().bounds;
    require(std::abs(bounds[0]) < 1.0e-9 && std::abs(bounds[1]) < 1.0e-9 &&
                std::abs(bounds[2]) < 1.0e-9,
            "definition-local body bounds should remain at the modelling origin");

    geometer::StepTopologyInspectionOptions source_options;
    source_options.include_source_entity_evidence = true;
    geometer::StepTopologySnapshot source_snapshot;
    require(session->inspect(source_options, &source_snapshot, &status) == 0,
            "root-placement source inspection failed: " + status.message);
    require(source_snapshot.bodies.front().source_entity.mapped &&
                std::any_of(source_snapshot.faces.begin(), source_snapshot.faces.end(),
                            [](const auto& face) { return face.source_entity.mapped; }),
            "cumulative source indexing should map located-root body and face targets");
}

void transfer_mapping_is_measured_across_the_fixture_corpus()
{
    constexpr std::array<const char*, 6> fixture_paths = {
        "tests/fixtures/step/embedded_models/miniature_test_point.stp",
        "tests/fixtures/step/embedded_models/SOT-23.STEP",
        "tests/fixtures/step/embedded_models/RESC1608X06L.step",
        "tests/fixtures/step/embedded_models/SOIC-20-300.STEP",
        "tests/fixtures/step/embedded_models/ABM3B.STEP",
        "tests/fixtures/step/generated_topology/generated_repeated_occurrences.step"};
    std::size_t mapped = 0;
    std::size_t unmapped = 0;
    geometer::StepTopologyInspectionOptions options;
    options.include_source_entity_evidence = true;
    for (const char* fixture_path : fixture_paths)
    {
        const std::vector<unsigned char> bytes = read_bytes(fixture_path);
        std::unique_ptr<geometer::StepTopologySession> session;
        geometer::Status status;
        require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &session,
                                                         &status) == 0,
                std::string("failed opening source-mapping fixture: ") + fixture_path + ": " +
                    status.message);
        geometer::StepTopologySnapshot snapshot;
        require(session->inspect(options, &snapshot, &status) == 0,
                std::string("failed source-mapping inspection: ") + fixture_path);
        std::size_t mapped_records = 0;
        const auto inspect_evidence = [&mapped_records](const auto& record)
        {
            if (!record.source_entity.mapped)
            {
                return;
            }
            ++mapped_records;
            require(record.source_entity.model_number > 0 &&
                        !record.source_entity.entity_type.empty() &&
                        !record.source_entity.mapping_method.empty(),
                    "positive source evidence must include entity number, type, and method");
        };
        std::for_each(snapshot.definitions.begin(), snapshot.definitions.end(), inspect_evidence);
        std::for_each(snapshot.bodies.begin(), snapshot.bodies.end(), inspect_evidence);
        std::for_each(snapshot.shells.begin(), snapshot.shells.end(), inspect_evidence);
        std::for_each(snapshot.faces.begin(), snapshot.faces.end(), inspect_evidence);
        require(mapped_records == snapshot.metadata.mapped_source_entities,
                "positive source-evidence records should match their summary count");
        mapped += snapshot.metadata.mapped_source_entities;
        unmapped += snapshot.metadata.unmapped_source_entities;
    }
    require(unmapped > 0, "fixture corpus should exercise negative source mapping evidence");
    require(mapped == 109, "fixture corpus source mapping measurement changed (mapped count " +
                               std::to_string(mapped) + ")");
}

void resource_limits_reject_before_exposing_a_session()
{
    const std::vector<unsigned char> bytes =
        read_bytes("tests/fixtures/step/generated_topology/generated_repeated_occurrences.step");
    std::unique_ptr<geometer::StepTopologySession> session;
    geometer::Status status;
    geometer::StepTopologyLimits limits;
    limits.max_source_bytes = bytes.size() - 1U;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), limits, &session,
                                                     &status) != 0 &&
                session == nullptr,
            "source byte limit should reject before session creation");

    limits = {};
    limits.max_expanded_occurrences = 5;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), limits, &session,
                                                     &status) != 0 &&
                session == nullptr,
            "occurrence cardinality limit should reject opening");

    limits = {};
    limits.max_faces = 5;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), limits, &session,
                                                     &status) != 0 &&
                session == nullptr,
            "face cardinality limit should reject opening");

    const auto require_rejected =
        [&bytes, &session, &status](const geometer::StepTopologyLimits& value, const char* message)
    {
        session.reset();
        require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), value,
                                                         &session, &status) != 0 &&
                    session == nullptr,
                message);
    };
    limits = {};
    limits.max_definitions = 2;
    require_rejected(limits, "definition cardinality limit should reject opening");
    limits = {};
    limits.max_component_labels = 3;
    require_rejected(limits, "component-label cardinality limit should reject opening");
    limits = {};
    limits.max_handles = 1;
    require_rejected(limits, "handle cardinality limit should reject opening");
    limits = {};
    limits.max_string_bytes = 1;
    require_rejected(limits, "per-string byte limit should reject opening");
    limits = {};
    limits.max_transfer_index_shapes = 1;
    require_rejected(limits, "transfer shape-index limit should fail instead of truncating");
    limits = {};
    limits.max_transfer_work_items = 1;
    require_rejected(limits, "transfer indexing execution-work limit should reject opening");
    limits = {};
    limits.max_group_transaction_member_references = 0;
    require_rejected(limits, "zero aggregate group-member limit should reject opening");
    limits = {};
    limits.max_hierarchy_transaction_commands = 0;
    require_rejected(limits, "zero hierarchy command limit should reject opening");
    limits = {};
    limits.max_hierarchy_transaction_work_items = 0;
    require_rejected(limits, "zero hierarchy work limit should reject opening");

    std::unique_ptr<geometer::StepTopologySession> measured = open_repeated();
    limits = {};
    limits.max_session_estimated_bytes = measured->info().estimated_resident_bytes - 1U;
    require_rejected(limits, "resident-byte estimate limit should reject opening");

    const std::vector<unsigned char> multi_bytes =
        read_bytes("tests/fixtures/step/generated_topology/generated_flat_multi_solid.step");
    const auto require_multi_rejected =
        [&multi_bytes, &session, &status](const geometer::StepTopologyLimits& value,
                                          const char* message)
    {
        session.reset();
        require(geometer::StepTopologySession::open_step(multi_bytes.data(), multi_bytes.size(),
                                                         value, &session, &status) != 0 &&
                    session == nullptr,
                message);
    };
    limits = {};
    limits.max_bodies = 1;
    require_multi_rejected(limits, "body cardinality limit should reject opening");
    limits = {};
    limits.max_shells = 1;
    require_multi_rejected(limits, "shell cardinality limit should reject opening");
}

void store_evicts_expires_and_invalidates_on_process_replacement()
{
    const std::vector<unsigned char> bytes =
        read_bytes("tests/fixtures/step/generated_topology/generated_repeated_occurrences.step");
    geometer::StepTopologyLimits limits;
    limits.max_sessions = 2;
    limits.inactivity_timeout = std::chrono::hours(1);
    geometer::StepTopologySessionStore store(limits);
    geometer::Status status;
    geometer::StepTopologyOpenResult first;
    geometer::StepTopologyOpenResult second;
    geometer::StepTopologyOpenResult third;
    require(store.open_step(bytes.data(), bytes.size(), &first, &status) == 0,
            "store first open failed: " + status.message);
    require(store.open_step(bytes.data(), bytes.size(), &second, &status) == 0,
            "store second open failed: " + status.message);
    geometer::StepTopologySnapshot snapshot;
    require(store.inspect(first.session.session_handle, {}, &snapshot, &status) == 0,
            "touching first store session failed: " + status.message);
    require(store.open_step(bytes.data(), bytes.size(), &third, &status) == 0,
            "store third open failed: " + status.message);
    require(store.size() == 2, "store should enforce maximum session count");
    require(third.evicted_session_handles.size() == 1 &&
                third.evicted_session_handles.front() == second.session.session_handle,
            "store access should change the deterministic LRU victim");
    require(store.inspect(second.session.session_handle, {}, &snapshot, &status) != 0,
            "least-recently-used session should fail lookup after eviction");
    require(store.inspect(first.session.session_handle, {}, &snapshot, &status) == 0,
            "recently touched session should remain live");
    const std::size_t before_render_bytes = store.estimated_resident_bytes();
    geometer::StepTopologyGlbRenderOutput first_render;
    require(store.render_glb_work_packet(first.session.session_handle, {}, &first_render,
                                         &status) == 0 &&
                !first_render.glb.empty(),
            "stored GLB render failed: " + status.message);
    const std::size_t after_render_bytes = store.estimated_resident_bytes();
    require(after_render_bytes > before_render_bytes,
            "store should account the retained authoritative render artifact");
    geometer::StepTopologyGlbHitDescriptor superseded;
    superseded.artifact_handle = first_render.artifact_handle;
    superseded.content_sha256 = first_render.content_sha256;
    geometer::StepTopologyGlbOptions second_options;
    second_options.tessellation.source_to_render = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
                                                    0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
    geometer::StepTopologyGlbRenderOutput second_render;
    require(store.render_glb_work_packet(first.session.session_handle, second_options,
                                         &second_render, &status) == 0,
            "replacement GLB render failed: " + status.message);
    require(second_render.artifact_handle != first_render.artifact_handle &&
                store.estimated_resident_bytes() == after_render_bytes,
            "a new render should replace, rather than accumulate, retained artifact state");
    geometer::StepTopologyRenderHit stale_hit;
    require(store.resolve_glb_hit(first.session.session_handle, superseded, &stale_hit, &status) !=
                0,
            "a superseded GLB artifact must fail closed");
    geometer::StepTopologyGlbOptions over_wire_options;
    over_wire_options.glb_byte_limit = 35U;
    const std::size_t retained_bytes = store.estimated_resident_bytes();
    geometer::StepTopologyGlbRenderOutput over_wire_render;
    require(store.render_glb_work_packet(first.session.session_handle, over_wire_options,
                                         &over_wire_render, &status) != 0 &&
                over_wire_render.glb.empty() && store.estimated_resident_bytes() == retained_bytes,
            "an over-wire candidate must fail progressively and preserve retained state");

    const std::vector<std::string> replaced = store.clear_for_process_replacement();
    require(replaced.size() == 2 && store.size() == 0 && store.estimated_resident_bytes() == 0,
            "process replacement should invalidate and release every session");
    require(store.inspect(first.session.session_handle, {}, &snapshot, &status) != 0,
            "pre-replacement session should remain invalid");

    std::unique_ptr<geometer::StepTopologySession> measured = open_repeated();
    limits = {};
    limits.max_sessions = 3;
    limits.max_store_estimated_bytes = measured->info().estimated_resident_bytes * 2U;
    geometer::StepTopologySessionStore byte_store(limits);
    geometer::StepTopologyOpenResult byte_first;
    geometer::StepTopologyOpenResult byte_second;
    geometer::StepTopologyOpenResult byte_third;
    require(byte_store.open_step(bytes.data(), bytes.size(), &byte_first, &status) == 0 &&
                byte_store.open_step(bytes.data(), bytes.size(), &byte_second, &status) == 0 &&
                byte_store.open_step(bytes.data(), bytes.size(), &byte_third, &status) == 0,
            "aggregate byte-limited store opens failed: " + status.message);
    require(byte_store.size() == 2 && byte_third.evicted_session_handles.size() == 1 &&
                byte_third.evicted_session_handles.front() == byte_first.session.session_handle,
            "aggregate store byte limit should evict the least-recently-used session");

    limits = {};
    limits.max_sessions = 1;
    limits.max_store_estimated_bytes = measured->info().estimated_resident_bytes + 1U;
    geometer::StepTopologySessionStore render_limited_store(limits);
    geometer::StepTopologyOpenResult render_limited;
    require(render_limited_store.open_step(bytes.data(), bytes.size(), &render_limited, &status) ==
                0,
            "render-limited store open failed: " + status.message);
    const std::size_t render_limited_bytes = render_limited_store.estimated_resident_bytes();
    geometer::StepTopologyGlbRenderOutput rejected_render;
    require(render_limited_store.render_glb_work_packet(render_limited.session.session_handle, {},
                                                        &rejected_render, &status) != 0 &&
                rejected_render.glb.empty() &&
                render_limited_store.estimated_resident_bytes() == render_limited_bytes,
            "render admission must reject atomically without retaining unaccounted state");

    limits.max_sessions = 1;
    limits.inactivity_timeout = std::chrono::milliseconds(1);
    geometer::StepTopologySessionStore expiring_store(limits);
    geometer::StepTopologyOpenResult expiring;
    require(expiring_store.open_step(bytes.data(), bytes.size(), &expiring, &status) == 0,
            "expiring store open failed: " + status.message);
    const std::vector<std::string> expired =
        expiring_store.evict_expired(std::chrono::steady_clock::now() + std::chrono::seconds(1));
    require(expired.size() == 1 && expired.front() == expiring.session.session_handle &&
                expiring_store.size() == 0,
            "inactive session should expire deterministically");
}

void stored_render_hit_resolution_is_constant_work()
{
    const std::vector<unsigned char> bytes =
        read_bytes("tests/fixtures/step/embedded_models/SOIC-20-300.STEP");
    geometer::StepTopologySessionStore store;
    geometer::StepTopologyOpenResult opened;
    geometer::Status status;
    require(store.open_step(bytes.data(), bytes.size(), &opened, &status) == 0,
            "high-cardinality hit store open failed: " + status.message);
    geometer::StepTopologyGlbRenderOutput rendered;
    require(store.render_glb_work_packet(opened.session.session_handle, {}, &rendered, &status) ==
                    0 &&
                rendered.instance_count > 10U && rendered.primitive_count > 10U,
            "high-cardinality hit render failed: " + status.message);
    require(rendered.glb.size() >= 20U, "rendered GLB is too short");
    const auto read_u32 = [&rendered](std::size_t offset)
    {
        return static_cast<std::uint32_t>(rendered.glb[offset]) |
               (static_cast<std::uint32_t>(rendered.glb[offset + 1U]) << 8U) |
               (static_cast<std::uint32_t>(rendered.glb[offset + 2U]) << 16U) |
               (static_cast<std::uint32_t>(rendered.glb[offset + 3U]) << 24U);
    };
    const std::size_t json_size = read_u32(12U);
    require(json_size <= rendered.glb.size() - 20U, "rendered GLB JSON chunk is invalid");
    rapidjson::Document document;
    document.Parse(reinterpret_cast<const char*>(rendered.glb.data() + 20U), json_size);
    require(!document.HasParseError(), "rendered GLB JSON should parse");
    const auto& node = document["nodes"][0U]["extras"]["wn_geometer"];
    const std::size_t mesh_index = node["mesh_index"].GetUint64();
    const auto& primitive = document["meshes"][static_cast<rapidjson::SizeType>(mesh_index)]
                                    ["primitives"][0U]["extras"]["wn_geometer"];
    geometer::StepTopologyGlbHitDescriptor descriptor;
    descriptor.artifact_handle = rendered.artifact_handle;
    descriptor.content_sha256 = rendered.content_sha256;
    descriptor.instance_index = node["instance_index"].GetUint64();
    descriptor.primitive_index = primitive["primitive_index"].GetUint64();
    descriptor.primitive_triangle_index = 0U;
    descriptor.occurrence_handle = node["occurrence_handle"].GetString();
    descriptor.body_handle = primitive["body_handle"].GetString();
    descriptor.face_handle = primitive["face_handle"].GetString();
    geometer::StepTopologyRenderHit hit;
    require(store.resolve_glb_hit(opened.session.session_handle, descriptor, &hit, &status) == 0,
            "high-cardinality stored hit failed: " + status.message);
    require(hit.lookup_work_items == 1U && hit.occurrence_handle == descriptor.occurrence_handle &&
                hit.body_handle == descriptor.body_handle &&
                hit.face_handle == descriptor.face_handle,
            "stored hit resolution must use one direct authoritative lookup regardless of artifact "
            "size");
}

void immutable_page_cursor_streams_records_and_memberships()
{
    static_assert(
        std::is_same_v<typename decltype(geometer::StepTopologySnapshotPage::bodies)::value_type,
                       geometer::StepTopologyBodyPageSummary>);
    static_assert(
        !std::is_same_v<geometer::StepTopologyBodyPageSummary, geometer::StepTopologyBody>);
    static_assert(
        std::is_same_v<typename decltype(geometer::StepTopologySnapshotPage::shells)::value_type,
                       geometer::StepTopologyShellPageSummary>);
    static_assert(
        std::is_same_v<typename decltype(geometer::StepTopologySnapshotPage::faces)::value_type,
                       geometer::StepTopologyFacePageSummary>);
    const std::vector<unsigned char> bytes =
        read_bytes("tests/fixtures/step/generated_topology/generated_flat_multi_solid.step");
    std::unique_ptr<geometer::StepTopologySession> session;
    geometer::Status status;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &session,
                                                     &status) == 0,
            "paged session open failed: " + status.message);
    geometer::StepTopologyPagePosition position;
    std::unordered_map<std::string, std::size_t> declared_faces;
    std::unordered_map<std::string, std::size_t> observed_faces;
    std::size_t observed_records = 0U;
    std::size_t expected_records = 0U;
    bool saw_target_summary = false;
    bool saw_isolated_membership = false;
    for (;;)
    {
        geometer::StepTopologySnapshotPage page;
        require(session->inspect_page({}, position, 1U, &page, &status) == 0,
                "limit-one page failed: " + status.message);
        const std::size_t page_records =
            page.definitions.size() + page.root_occurrences.size() + page.occurrences.size() +
            page.bodies.size() + page.shells.size() + page.faces.size() + page.memberships.size();
        require(page_records == 1U,
                "each nonterminal limit-one page should make one-record progress");
        if (expected_records == 0U)
            expected_records = page.definition_count + page.root_occurrence_count +
                               page.component_occurrence_count + page.body_count +
                               page.shell_count + page.face_count + page.membership_count;
        for (const auto& definition : page.definitions)
            declared_faces[definition.handle] = definition.face_count;
        for (const auto& face : page.faces)
            ++observed_faces[face.definition_handle];
        for (const auto& body : page.bodies)
        {
            saw_target_summary = true;
            require(body.shell_count + body.face_count > 0U && page.memberships.empty(),
                    "a target page must expose only membership counts, not hidden edge arrays");
        }
        if (!page.memberships.empty())
        {
            saw_isolated_membership = true;
            require(page.definitions.empty() && page.root_occurrences.empty() &&
                        page.occurrences.empty() && page.bodies.empty() && page.shells.empty() &&
                        page.faces.empty(),
                    "a limit-one membership page must materialize exactly one edge");
        }
        ++observed_records;
        if (!page.has_next)
            break;
        position = page.next;
    }
    require(observed_records == expected_records,
            "paged topology stream should exactly satisfy its declared counts");
    require(declared_faces == observed_faces,
            "definition face counts should count unique face records, not body memberships");
    require(saw_target_summary && saw_isolated_membership,
            "paged inspection should separate bounded target summaries from membership edges");
}

} // namespace

int main()
{
    try
    {
        inspect_nested_occurrences_and_topology();
        handles_fail_closed_across_generation_session_and_close();
        cancelled_open_and_refresh_are_atomic();
        flat_multi_solid_has_two_bodies_without_occurrences();
        transfer_mapping_is_measured_across_the_fixture_corpus();
        root_placement_is_separate_from_definition_geometry();
        resource_limits_reject_before_exposing_a_session();
        immutable_page_cursor_streams_records_and_memberships();
        stored_render_hit_resolution_is_constant_work();
        store_evicts_expires_and_invalidates_on_process_replacement();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
