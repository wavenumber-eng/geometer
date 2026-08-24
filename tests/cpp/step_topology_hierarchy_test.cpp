#include "geometer/step_topology_hierarchy.h"

#include <algorithm>
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

#ifndef GEOMETER_TEST_SOURCE_DIR
#define GEOMETER_TEST_SOURCE_DIR "."
#endif

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::vector<unsigned char> read_fixture(const std::string& name)
{
    const std::filesystem::path path = std::filesystem::path(GEOMETER_TEST_SOURCE_DIR) /
                                       "tests/fixtures/step/generated_topology" / name;
    std::ifstream stream(path, std::ios::binary);
    require(stream.good(), "failed opening hierarchy fixture: " + path.string());
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

geometer::StepTopologyHierarchyCommand create_product(const std::string& id,
                                                      const std::string& name,
                                                      geometer::StepTopologyTargetKind kind,
                                                      const std::string& handle)
{
    geometer::StepTopologyHierarchyCommand command;
    command.kind = geometer::StepTopologyHierarchyCommandKind::create_product;
    command.authored_id = id;
    command.name = name;
    command.source_kind = kind;
    command.source_handle = handle;
    return command;
}

geometer::StepTopologyHierarchyCommand create_assembly(const std::string& id,
                                                       const std::string& name)
{
    geometer::StepTopologyHierarchyCommand command;
    command.kind = geometer::StepTopologyHierarchyCommandKind::create_assembly;
    command.authored_id = id;
    command.name = name;
    return command;
}

geometer::StepTopologyHierarchyCommand
create_occurrence(const std::string& id, const std::string& child, const std::string& parent)
{
    geometer::StepTopologyHierarchyCommand command;
    command.kind = geometer::StepTopologyHierarchyCommandKind::create_occurrence;
    command.authored_id = id;
    command.child_authored_id = child;
    command.parent_assembly_authored_id = parent;
    return command;
}

std::pair<std::unique_ptr<geometer::StepTopologySession>, geometer::StepTopologySnapshot>
open_fixture(const std::string& name)
{
    const std::vector<unsigned char> bytes = read_fixture(name);
    std::unique_ptr<geometer::StepTopologySession> session;
    geometer::Status status;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &session,
                                                     &status) == 0,
            "hierarchy fixture open failed: " + status.message);
    geometer::StepTopologySnapshot snapshot;
    require(session->inspect({}, &snapshot, &status) == 0,
            "hierarchy fixture inspect failed: " + status.message);
    return {std::move(session), std::move(snapshot)};
}

void flat_bodies_gain_atomic_reversible_hierarchy()
{
    auto [session, snapshot] = open_fixture("generated_flat_multi_solid.step");
    require(snapshot.definitions.size() == 1 && snapshot.bodies.size() == 2,
            "flat hierarchy fixture shape changed");
    geometer::Status status;
    geometer::StepTopologyHierarchyState empty;
    require(geometer::initialize_step_topology_hierarchy(snapshot, &empty, &status) == 0 &&
                empty.nodes.empty() && empty.occurrences.empty() &&
                empty.source_brep_sha256 == snapshot.brep_sha256,
            "hierarchy initialization failed");

    constexpr const char* product_a = "wn.geometer.research.product.flat-a";
    constexpr const char* product_b = "wn.geometer.research.product.flat-b";
    constexpr const char* root = "wn.geometer.research.assembly.root";
    constexpr const char* sub = "wn.geometer.research.assembly.sub";
    constexpr const char* occurrence_a = "wn.geometer.research.occurrence.flat-a";
    constexpr const char* occurrence_b = "wn.geometer.research.occurrence.flat-b";
    constexpr const char* occurrence_sub = "wn.geometer.research.occurrence.sub";

    geometer::StepTopologyHierarchyTransaction create;
    create.commands = {
        create_product(product_a, "Flat body A", geometer::StepTopologyTargetKind::body,
                       snapshot.bodies[0].handle),
        create_product(product_b, "Flat body B", geometer::StepTopologyTargetKind::body,
                       snapshot.bodies[1].handle),
        create_assembly(root, "Root assembly"),
        create_assembly(sub, "Subassembly"),
        create_occurrence(occurrence_a, product_a, root),
        create_occurrence(occurrence_b, product_b, sub),
        create_occurrence(occurrence_sub, sub, root),
    };
    create.commands[5].transform[3] = 25.0;
    geometer::StepTopologyHierarchyState created;
    require(geometer::apply_step_topology_hierarchy_transaction(snapshot, {}, empty, create,
                                                                &created, &status) == 0 &&
                created.hierarchy_revision == 1 && created.nodes.size() == 4 &&
                created.occurrences.size() == 3,
            "flat hierarchy create failed: " + status.message);

    geometer::StepTopologyHierarchyTransaction edit;
    edit.expected_hierarchy_revision = created.hierarchy_revision;
    geometer::StepTopologyHierarchyCommand rename;
    rename.kind = geometer::StepTopologyHierarchyCommandKind::rename_node;
    rename.authored_id = product_a;
    rename.expected_revision = 1;
    rename.name = "Renamed flat body A";
    geometer::StepTopologyHierarchyCommand reparent;
    reparent.kind = geometer::StepTopologyHierarchyCommandKind::reparent_occurrence;
    reparent.authored_id = occurrence_b;
    reparent.expected_revision = 1;
    reparent.parent_assembly_authored_id = root;
    reparent.transform[3] = 50.0;
    edit.commands = {rename, reparent};
    geometer::StepTopologyHierarchyState edited;
    require(geometer::apply_step_topology_hierarchy_transaction(snapshot, {}, created, edit,
                                                                &edited, &status) == 0 &&
                edited.hierarchy_revision == 2,
            "hierarchy rename/reparent failed: " + status.message);
    const auto renamed =
        std::find_if(edited.nodes.begin(), edited.nodes.end(),
                     [=](const auto& node) { return node.authored_id == product_a; });
    const auto moved =
        std::find_if(edited.occurrences.begin(), edited.occurrences.end(),
                     [=](const auto& value) { return value.authored_id == occurrence_b; });
    require(renamed != edited.nodes.end() && renamed->revision == 2 &&
                renamed->name == "Renamed flat body A" && moved != edited.occurrences.end() &&
                moved->revision == 2 && moved->parent_assembly_authored_id == root &&
                moved->transform[3] == 50.0,
            "hierarchy edit did not publish canonical revisions and placement");

    geometer::StepTopologyHierarchyTransaction cycle;
    cycle.expected_hierarchy_revision = edited.hierarchy_revision;
    cycle.commands.push_back(create_occurrence("wn.geometer.research.occurrence.cycle", root, sub));
    geometer::StepTopologyHierarchyState rejected;
    require(geometer::apply_step_topology_hierarchy_transaction(snapshot, {}, edited, cycle,
                                                                &rejected, &status) != 0 &&
                rejected.nodes.empty() && edited.nodes.size() == 4,
            "assembly cycles must fail without changing the prior state");

    geometer::StepTopologyHierarchyTransaction invalid_transform;
    invalid_transform.expected_hierarchy_revision = edited.hierarchy_revision;
    geometer::StepTopologyHierarchyCommand distort;
    distort.kind = geometer::StepTopologyHierarchyCommandKind::reparent_occurrence;
    distort.authored_id = occurrence_a;
    distort.expected_revision = 1;
    distort.parent_assembly_authored_id = root;
    distort.transform[0] = 2.0;
    invalid_transform.commands.push_back(distort);
    require(geometer::apply_step_topology_hierarchy_transaction(
                snapshot, {}, edited, invalid_transform, &rejected, &status) != 0 &&
                rejected.occurrences.empty(),
            "non-rigid hierarchy placement must fail without partial output");

    geometer::StepTopologyHierarchyTransaction stale;
    stale.expected_hierarchy_revision = created.hierarchy_revision;
    stale.commands.push_back(rename);
    require(geometer::apply_step_topology_hierarchy_transaction(snapshot, {}, edited, stale,
                                                                &rejected, &status) != 0 &&
                rejected.nodes.empty(),
            "stale hierarchy transaction revision must fail closed");
    geometer::StepTopologyHierarchyState in_place = edited;
    require(geometer::apply_step_topology_hierarchy_transaction(snapshot, {}, in_place, stale,
                                                                &in_place, &status) != 0 &&
                in_place.hierarchy_revision == edited.hierarchy_revision &&
                in_place.nodes.size() == edited.nodes.size() &&
                in_place.occurrences.size() == edited.occurrences.size(),
            "failed in-place hierarchy transaction must preserve the current state");

    geometer::StepTopologyHierarchyTransaction conflicting_owner;
    conflicting_owner.expected_hierarchy_revision = edited.hierarchy_revision;
    conflicting_owner.commands.push_back(create_product(
        "wn.geometer.research.product.whole-definition", "Whole definition",
        geometer::StepTopologyTargetKind::definition, snapshot.definitions[0].handle));
    require(geometer::apply_step_topology_hierarchy_transaction(
                snapshot, {}, edited, conflicting_owner, &rejected, &status) != 0 &&
                rejected.nodes.empty(),
            "a whole definition and its independently assigned bodies must not overlap ownership");

    geometer::StepTopologyHierarchyTransaction revert;
    revert.expected_hierarchy_revision = edited.hierarchy_revision;
    for (const auto& occurrence : edited.occurrences)
    {
        geometer::StepTopologyHierarchyCommand erase;
        erase.kind = geometer::StepTopologyHierarchyCommandKind::erase_occurrence;
        erase.authored_id = occurrence.authored_id;
        erase.expected_revision = occurrence.revision;
        revert.commands.push_back(erase);
    }
    for (const auto& node : edited.nodes)
    {
        geometer::StepTopologyHierarchyCommand erase;
        erase.kind = geometer::StepTopologyHierarchyCommandKind::erase_node;
        erase.authored_id = node.authored_id;
        erase.expected_revision = node.revision;
        revert.commands.push_back(erase);
    }
    geometer::StepTopologyHierarchyState reverted;
    require(geometer::apply_step_topology_hierarchy_transaction(snapshot, {}, edited, revert,
                                                                &reverted, &status) == 0 &&
                reverted.hierarchy_revision == 3 && reverted.nodes.empty() &&
                reverted.occurrences.empty(),
            "hierarchy revert failed: " + status.message);

    geometer::StepTopologyHierarchyTransaction recreate;
    recreate.expected_hierarchy_revision = reverted.hierarchy_revision;
    recreate.commands.push_back(create_product(
        "wn.geometer.research.product.complete-definition", "Complete definition",
        geometer::StepTopologyTargetKind::definition, snapshot.definitions[0].handle));
    geometer::StepTopologyHierarchyState recreated;
    require(geometer::apply_step_topology_hierarchy_transaction(snapshot, {}, reverted, recreate,
                                                                &recreated, &status) == 0 &&
                recreated.nodes.size() == 1 &&
                recreated.nodes[0].source_kind == geometer::StepTopologyTargetKind::definition,
            "revert must release ownership for a complete-definition product");
    geometer::StepTopologyHierarchyTransaction rename_recreated;
    rename_recreated.expected_hierarchy_revision = recreated.hierarchy_revision;
    geometer::StepTopologyHierarchyCommand rename_complete;
    rename_complete.kind = geometer::StepTopologyHierarchyCommandKind::rename_node;
    rename_complete.authored_id = recreated.nodes[0].authored_id;
    rename_complete.expected_revision = recreated.nodes[0].revision;
    rename_complete.name = "Renamed complete definition";
    rename_recreated.commands.push_back(rename_complete);
    require(geometer::apply_step_topology_hierarchy_transaction(
                snapshot, {}, recreated, rename_recreated, &recreated, &status) == 0 &&
                recreated.hierarchy_revision == 5 && recreated.nodes[0].revision == 2 &&
                recreated.nodes[0].name == "Renamed complete definition",
            "successful in-place hierarchy transaction must publish the complete result");

    geometer::StepTopologySnapshot after;
    require(session->inspect({}, &after, &status) == 0 &&
                after.brep_sha256 == snapshot.brep_sha256 &&
                after.session.generation == snapshot.session.generation,
            "hierarchy-only value edits must not mutate the source B-rep or session");
}

void fused_face_subset_cannot_become_a_product()
{
    auto [session, snapshot] = open_fixture("generated_fused_slab.step");
    require(snapshot.bodies.size() == 1 && snapshot.faces.size() > 1,
            "fused hierarchy fixture shape changed");
    geometer::Status status;
    geometer::StepTopologyHierarchyState empty;
    require(geometer::initialize_step_topology_hierarchy(snapshot, &empty, &status) == 0,
            "fused hierarchy initialization failed");
    geometer::StepTopologyHierarchyTransaction face_product;
    face_product.commands.push_back(
        create_product("wn.geometer.research.product.face-subset", "Invalid face product",
                       geometer::StepTopologyTargetKind::face, snapshot.faces[0].handle));
    geometer::StepTopologyHierarchyState rejected;
    require(geometer::apply_step_topology_hierarchy_transaction(snapshot, {}, empty, face_product,
                                                                &rejected, &status) != 0 &&
                rejected.nodes.empty(),
            "a fused-body face subset must remain a logical group, not a product");

    geometer::StepTopologyHierarchyTransaction whole_body;
    whole_body.commands.push_back(
        create_product("wn.geometer.research.product.fused-body", "Complete fused body",
                       geometer::StepTopologyTargetKind::body, snapshot.bodies[0].handle));
    geometer::StepTopologyHierarchyState accepted;
    require(geometer::apply_step_topology_hierarchy_transaction(snapshot, {}, empty, whole_body,
                                                                &accepted, &status) == 0 &&
                accepted.nodes.size() == 1,
            "the complete fused body should remain a valid product source");
}

void hierarchy_work_and_command_limits_are_preflighted()
{
    auto [session, snapshot] = open_fixture("generated_flat_multi_solid.step");
    geometer::Status status;
    geometer::StepTopologyHierarchyState empty;
    require(geometer::initialize_step_topology_hierarchy(snapshot, &empty, &status) == 0,
            "bounded hierarchy initialization failed");
    geometer::StepTopologyHierarchyTransaction create;
    create.commands = {
        create_product("wn.geometer.research.product.limit-a", "A",
                       geometer::StepTopologyTargetKind::body, snapshot.bodies[0].handle),
        create_product("wn.geometer.research.product.limit-b", "B",
                       geometer::StepTopologyTargetKind::body, snapshot.bodies[1].handle),
        create_assembly("wn.geometer.research.assembly.limit", "Limit"),
    };
    geometer::StepTopologyLimits limits;
    limits.max_hierarchy_transaction_commands = create.commands.size();
    limits.max_hierarchy_transaction_work_items = 66;
    geometer::StepTopologyHierarchyState exact;
    require(geometer::apply_step_topology_hierarchy_transaction(snapshot, limits, empty, create,
                                                                &exact, &status) == 0,
            "exact hierarchy work limit should be accepted: " + status.message);
    limits.max_hierarchy_transaction_work_items = 65;
    geometer::StepTopologyHierarchyState rejected;
    require(geometer::apply_step_topology_hierarchy_transaction(snapshot, limits, empty, create,
                                                                &rejected, &status) != 0 &&
                rejected.nodes.empty(),
            "one-over hierarchy work must fail before publication");
    limits.max_hierarchy_transaction_work_items = 66;
    limits.max_hierarchy_transaction_commands = create.commands.size() - 1;
    require(geometer::apply_step_topology_hierarchy_transaction(snapshot, limits, empty, create,
                                                                &rejected, &status) != 0 &&
                rejected.nodes.empty(),
            "one-over hierarchy command count must fail before publication");

    geometer::StepTopologyHierarchyTransaction string_bounded;
    auto bounded_command =
        create_assembly("wn.geometer.research.assembly.string-limit", "String limit");
    bounded_command.child_authored_id = "ignored-but-still-bounded";
    string_bounded.commands.push_back(bounded_command);
    const std::size_t exact_string_bytes =
        bounded_command.authored_id.size() + bounded_command.name.size() +
        bounded_command.source_handle.size() + bounded_command.child_authored_id.size() +
        bounded_command.parent_assembly_authored_id.size();
    const std::size_t exact_max_string_bytes =
        std::max({bounded_command.authored_id.size(), bounded_command.name.size(),
                  bounded_command.source_handle.size(), bounded_command.child_authored_id.size(),
                  bounded_command.parent_assembly_authored_id.size()});
    limits = {};
    limits.max_string_bytes = exact_max_string_bytes;
    limits.max_total_string_bytes = exact_string_bytes;
    geometer::StepTopologyHierarchyState string_exact;
    require(geometer::apply_step_topology_hierarchy_transaction(
                snapshot, limits, empty, string_bounded, &string_exact, &status) == 0,
            "exact hierarchy command string limits should be accepted: " + status.message);
    limits.max_total_string_bytes = exact_string_bytes - 1;
    require(geometer::apply_step_topology_hierarchy_transaction(
                snapshot, limits, empty, string_bounded, &rejected, &status) != 0 &&
                rejected.nodes.empty(),
            "one-over hierarchy aggregate command strings must fail before publication");
    limits.max_total_string_bytes = exact_string_bytes;
    limits.max_string_bytes = exact_max_string_bytes - 1;
    require(geometer::apply_step_topology_hierarchy_transaction(
                snapshot, limits, empty, string_bounded, &rejected, &status) != 0 &&
                rejected.nodes.empty(),
            "one-over hierarchy command string must fail before publication");

    geometer::StepTopologyHierarchyTransaction place;
    place.expected_hierarchy_revision = exact.hierarchy_revision;
    place.commands.push_back(create_occurrence("wn.geometer.research.occurrence.limit",
                                               exact.nodes[1].authored_id,
                                               exact.nodes[2].authored_id));
    geometer::StepTopologyHierarchyState with_occurrence;
    require(geometer::apply_step_topology_hierarchy_transaction(snapshot, {}, exact, place,
                                                                &with_occurrence, &status) == 0,
            "failed preparing bounded erase-node state: " + status.message);
    geometer::StepTopologyHierarchyTransaction erase_unreferenced;
    erase_unreferenced.expected_hierarchy_revision = with_occurrence.hierarchy_revision;
    geometer::StepTopologyHierarchyCommand erase;
    erase.kind = geometer::StepTopologyHierarchyCommandKind::erase_node;
    erase.authored_id = with_occurrence.nodes[0].authored_id;
    erase.expected_revision = with_occurrence.nodes[0].revision;
    erase_unreferenced.commands.push_back(erase);
    limits = {};
    limits.max_hierarchy_transaction_work_items = 76;
    require(geometer::apply_step_topology_hierarchy_transaction(
                snapshot, limits, with_occurrence, erase_unreferenced, &exact, &status) == 0,
            "exact worst-path hierarchy work limit should be accepted: " + status.message);
    limits.max_hierarchy_transaction_work_items = 75;
    require(geometer::apply_step_topology_hierarchy_transaction(
                snapshot, limits, with_occurrence, erase_unreferenced, &rejected, &status) != 0 &&
                rejected.nodes.empty(),
            "one-over worst-path hierarchy work must fail before publication");
}

} // namespace

int main()
{
    try
    {
        flat_bodies_gain_atomic_reversible_hierarchy();
        fused_face_subset_cannot_become_a_product();
        hierarchy_work_and_command_limits_are_preflighted();
        std::cout << "STEP topology synthetic hierarchy tests passed\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
