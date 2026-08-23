#include "geometer/step_topology_session.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::vector<unsigned char> read_fixture()
{
    const std::string path = std::string(GEOMETER_TEST_SOURCE_DIR) +
                             "/tests/fixtures/step/generated_topology/generated_fused_slab.step";
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "failed opening fused slab fixture");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::vector<double> geometry_signature(const geometer::StepTopologySnapshot& snapshot)
{
    std::vector<double> signature;
    for (const auto& body : snapshot.bodies)
    {
        signature.push_back(body.volume);
        signature.insert(signature.end(), body.bounds.begin(), body.bounds.end());
    }
    for (const auto& face : snapshot.faces)
    {
        signature.push_back(face.area);
        signature.insert(signature.end(), face.bounds.begin(), face.bounds.end());
        signature.insert(signature.end(), face.centroid.begin(), face.centroid.end());
    }
    std::sort(signature.begin(), signature.end());
    return signature;
}

geometer::StepTopologyGroupCommand create_command(const std::string& id, const std::string& name,
                                                  std::vector<std::string> members)
{
    geometer::StepTopologyGroupCommand command;
    command.kind = geometer::StepTopologyGroupCommandKind::create;
    command.authored_id = id;
    command.name = name;
    command.member_handles = std::move(members);
    return command;
}

void logical_group_transactions_are_atomic_and_geometry_neutral()
{
    const auto bytes = read_fixture();
    std::unique_ptr<geometer::StepTopologySession> session;
    geometer::Status status;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &session,
                                                     &status) == 0,
            "open failed: " + status.message);
    geometer::StepTopologySnapshot before;
    require(session->inspect({}, &before, &status) == 0, "inspect failed");
    require(before.faces.size() >= 2 && !before.bodies.empty(),
            "fused slab fixture needs faces and a body");
    const auto signature = geometry_signature(before);
    const std::string first_old_face = before.faces[0].handle;

    geometer::StepTopologyRenderArtifact old_render;
    require(session->render({}, &old_render, &status) == 0, "pre-group render failed");

    geometer::StepTopologyGroupTransaction create;
    create.expected_generation = before.session.generation;
    create.commands.push_back(create_command("wn.geometer.research.group.slab-outline",
                                             "Slab outline",
                                             {before.faces[0].handle, before.faces[1].handle}));
    geometer::StepTopologyGroupTransactionResult created;
    require(session->apply_logical_groups(create, &created, &status) == 0,
            "create failed: " + status.message);
    require(created.session.generation == before.session.generation + 1 &&
                created.groups.size() == 1 && created.groups[0].revision == 1 &&
                created.groups[0].members.size() == 2,
            "create should atomically publish one refreshed group");
    require(created.groups[0].members[0].target_handle != first_old_face,
            "group members must use refreshed generation handles");

    geometer::StepTopologyResolvedTarget resolved;
    require(session->resolve(first_old_face, &resolved, &status) != 0,
            "old member handle must become stale after apply");
    geometer::StepTopologyRenderHit hit;
    require(session->resolve_render_hit(old_render, 0, 0, &hit, &status) != 0,
            "old render artifact must become stale after apply");

    geometer::StepTopologySnapshot after;
    require(session->inspect({}, &after, &status) == 0, "post-group inspect failed");
    require(geometry_signature(after) == signature,
            "logical grouping must not change B-rep counts or geometric properties");

    geometer::StepTopologyGroupTransaction stale = create;
    geometer::StepTopologyGroupTransactionResult rejected;
    require(session->apply_logical_groups(stale, &rejected, &status) != 0 &&
                session->info().generation == created.session.generation,
            "stale generation must fail without mutation");

    geometer::StepTopologyGroupTransaction partial;
    partial.expected_generation = created.session.generation;
    partial.commands.push_back(create_command("wn.geometer.research.group.temporary", "Temporary",
                                              {after.faces[0].handle}));
    partial.commands.push_back(create_command("wn.geometer.research.group.temporary", "Duplicate",
                                              {after.faces[1].handle}));
    require(session->apply_logical_groups(partial, &rejected, &status) != 0 &&
                session->info().generation == created.session.generation,
            "multi-command failure must roll back all commands");

    geometer::StepTopologyGroupTransaction create_temporary;
    create_temporary.expected_generation = created.session.generation;
    create_temporary.commands.push_back(create_command("wn.geometer.research.group.temporary",
                                                       "Temporary", {after.faces[0].handle}));
    geometer::StepTopologyGroupTransactionResult temporary;
    require(session->apply_logical_groups(create_temporary, &temporary, &status) == 0 &&
                temporary.groups.size() == 2,
            "rolled-back authored id must remain available");

    const auto temporary_group =
        std::find_if(temporary.groups.begin(), temporary.groups.end(), [](const auto& group)
                     { return group.authored_id == "wn.geometer.research.group.temporary"; });
    require(temporary_group != temporary.groups.end(), "temporary group should be published");
    geometer::StepTopologyGroupTransaction rename;
    rename.expected_generation = temporary.session.generation;
    geometer::StepTopologyGroupCommand rename_command;
    rename_command.kind = geometer::StepTopologyGroupCommandKind::rename;
    rename_command.authored_id = temporary_group->authored_id;
    rename_command.expected_revision = temporary_group->revision;
    rename_command.name = "Renamed temporary";
    rename.commands.push_back(std::move(rename_command));
    geometer::StepTopologyGroupTransactionResult renamed;
    require(session->apply_logical_groups(rename, &renamed, &status) == 0,
            "rename failed: " + status.message);
    const auto renamed_group =
        std::find_if(renamed.groups.begin(), renamed.groups.end(),
                     [](const auto& group) { return group.name == "Renamed temporary"; });
    require(renamed_group != renamed.groups.end() && renamed_group->revision == 2,
            "rename must advance only the group revision");

    geometer::StepTopologySnapshot current;
    require(session->inspect({}, &current, &status) == 0, "inspect after rename failed");
    geometer::StepTopologyGroupTransaction replace;
    replace.expected_generation = renamed.session.generation;
    geometer::StepTopologyGroupCommand replace_command;
    replace_command.kind = geometer::StepTopologyGroupCommandKind::replace_members;
    replace_command.authored_id = renamed_group->authored_id;
    replace_command.expected_revision = renamed_group->revision;
    replace_command.member_handles = {current.bodies[0].handle};
    replace.commands.push_back(std::move(replace_command));
    geometer::StepTopologyGroupTransactionResult replaced;
    require(session->apply_logical_groups(replace, &replaced, &status) == 0,
            "replace members failed: " + status.message);
    const auto replaced_group =
        std::find_if(replaced.groups.begin(), replaced.groups.end(),
                     [](const auto& group) { return group.name == "Renamed temporary"; });
    require(replaced_group != replaced.groups.end() && replaced_group->revision == 3 &&
                replaced_group->members.size() == 1 &&
                replaced_group->members[0].kind == geometer::StepTopologyTargetKind::body,
            "member replacement should publish the new body target");

    require(session->inspect({}, &current, &status) == 0, "inspect after replace failed");
    geometer::StepTopologyGroupTransaction invalid_target;
    invalid_target.expected_generation = replaced.session.generation;
    invalid_target.commands.push_back(create_command("wn.geometer.research.group.invalid",
                                                     "Invalid", {current.definitions[0].handle}));
    require(session->apply_logical_groups(invalid_target, &rejected, &status) != 0 &&
                session->info().generation == replaced.session.generation,
            "definition targets must fail without mutation");

    geometer::StepTopologyGroupTransaction erase;
    erase.expected_generation = replaced.session.generation;
    geometer::StepTopologyGroupCommand erase_command;
    erase_command.kind = geometer::StepTopologyGroupCommandKind::erase;
    erase_command.authored_id = replaced_group->authored_id;
    erase_command.expected_revision = replaced_group->revision;
    erase.commands.push_back(std::move(erase_command));
    geometer::StepTopologyGroupTransactionResult erased;
    require(session->apply_logical_groups(erase, &erased, &status) == 0 &&
                erased.groups.size() == 1 &&
                erased.groups[0].authored_id == "wn.geometer.research.group.slab-outline",
            "erase should remove only the selected logical group without changing geometry");
    geometer::StepTopologySnapshot final_snapshot;
    require(session->inspect({}, &final_snapshot, &status) == 0 &&
                geometry_signature(final_snapshot) == signature,
            "all grouping commands must preserve geometry");
}

void session_store_routes_group_transactions_and_updates_accounting()
{
    const auto bytes = read_fixture();
    geometer::StepTopologySessionStore store;
    geometer::StepTopologyOpenResult opened;
    geometer::Status status;
    require(store.open_step(bytes.data(), bytes.size(), &opened, &status) == 0,
            "store open failed: " + status.message);
    const std::size_t before_bytes = store.estimated_resident_bytes();
    geometer::StepTopologySnapshot snapshot;
    require(store.inspect(opened.session.session_handle, {}, &snapshot, &status) == 0,
            "store inspect failed");
    geometer::StepTopologyGroupTransaction transaction;
    transaction.expected_generation = snapshot.session.generation;
    transaction.commands.push_back(
        create_command("wn.geometer.research.group.store", "Store", {snapshot.faces[0].handle}));
    geometer::StepTopologyGroupTransactionResult result;
    require(store.apply_logical_groups(opened.session.session_handle, transaction, &result,
                                       &status) == 0,
            "store apply failed: " + status.message);
    require(result.groups.size() == 1 && store.estimated_resident_bytes() >= before_bytes,
            "store must publish the group and update resident accounting");
    geometer::StepTopologyResolvedTarget resolved;
    require(store.resolve(opened.session.session_handle, result.groups[0].members[0].target_handle,
                          &resolved, &status) == 0,
            "store should resolve the refreshed group member");
}

} // namespace

int main()
{
    try
    {
        logical_group_transactions_are_atomic_and_geometry_neutral();
        session_store_routes_group_transactions_and_updates_accounting();
        std::cout << "STEP topology logical group tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
