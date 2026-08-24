#include "geometer/step_topology_session.h"

#include "geometer/sha256.h"
#include "step_topology_session_internal.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <locale>
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

int reject_group_publication(const geometer::StepTopologyGroupTransactionResult& result,
                             void* context, geometer::Status* status)
{
    auto* observed = static_cast<bool*>(context);
    *observed = !result.session.session_handle.empty() && !result.groups.empty();
    if (status != nullptr)
    {
        status->code = 102;
        status->message = "injected group publication limit";
    }
    return 102;
}

int reject_probe_publication(const geometer::StepTopologyProbeTransactionResult& result,
                             void* context, geometer::Status* status)
{
    auto* observed = static_cast<bool*>(context);
    *observed = !result.session.session_handle.empty() && !result.probes.empty();
    if (status != nullptr)
    {
        status->code = 102;
        status->message = "injected probe publication limit";
    }
    return 102;
}

std::vector<unsigned char> read_fixture()
{
    const std::string path = std::string(GEOMETER_TEST_SOURCE_DIR) +
                             "/tests/fixtures/step/generated_topology/generated_fused_slab.step";
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "failed opening fused slab fixture");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::vector<unsigned char> read_fixture(const std::string& name)
{
    const std::string path =
        std::string(GEOMETER_TEST_SOURCE_DIR) + "/tests/fixtures/step/generated_topology/" + name;
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "failed opening fixture " + name);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void resign_journal(std::vector<unsigned char>* bytes)
{
    require(bytes->size() >= 32U, "journal is too small to resign");
    const std::size_t payload_size = bytes->size() - 32U;
    const auto digest = geometer::sha256(bytes->data(), payload_size);
    std::copy(digest.begin(), digest.end(),
              bytes->begin() + static_cast<std::ptrdiff_t>(payload_size));
}

std::size_t find_last_bytes(const std::vector<unsigned char>& bytes, const std::string& value)
{
    std::size_t found = std::string::npos;
    for (auto iterator = std::search(bytes.begin(), bytes.end(), value.begin(), value.end());
         iterator != bytes.end();
         iterator = std::search(iterator + 1, bytes.end(), value.begin(), value.end()))
    {
        found = static_cast<std::size_t>(iterator - bytes.begin());
    }
    return found;
}

class CommaDecimalPunctuation final : public std::numpunct<char>
{
  protected:
    char do_decimal_point() const override
    {
        return ',';
    }

    char do_thousands_sep() const override
    {
        return '.';
    }

    std::string do_grouping() const override
    {
        return "\1";
    }
};

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

geometer::StepTopologyProbeCommand
attach_probe_command(const std::string& id, const geometer::StepTopologyProbeTarget& target,
                     const std::string& value)
{
    geometer::StepTopologyProbeCommand command;
    command.kind = geometer::StepTopologyProbeCommandKind::attach;
    command.authored_id = id;
    command.target = target;
    command.key = "wn.geometer.research.probe.key.note";
    command.value = value;
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
    require(before.brep_sha256.size() == 64, "inspection must publish a B-rep digest");
    const std::string brep_sha256 = before.brep_sha256;
    const std::size_t initial_string_bytes = before.session.accounted_string_bytes;
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
    require(after.brep_sha256 == brep_sha256,
            "logical grouping must preserve the serialized B-rep digest: " + brep_sha256 +
                " != " + after.brep_sha256);
    require(after.session.accounted_string_bytes > initial_string_bytes,
            "group ids and names must contribute to session-wide string accounting");

    geometer::StepTopologyGroupTransaction stale = create;
    geometer::StepTopologyGroupTransactionResult rejected;
    require(session->apply_logical_groups(stale, &rejected, &status) != 0 &&
                session->info().generation == created.session.generation &&
                rejected.session.generation == 0 && rejected.groups.empty(),
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
                geometry_signature(final_snapshot) == signature &&
                final_snapshot.brep_sha256 == brep_sha256,
            "all grouping commands must preserve geometry");
}

void logical_group_strings_obey_per_string_and_session_wide_limits()
{
    const auto bytes = read_fixture();
    std::unique_ptr<geometer::StepTopologySession> baseline;
    geometer::Status status;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &baseline,
                                                     &status) == 0,
            "baseline open failed: " + status.message);
    geometer::StepTopologySnapshot baseline_snapshot;
    require(baseline->inspect({}, &baseline_snapshot, &status) == 0, "baseline inspect failed");

    geometer::StepTopologyLimits per_string_limits;
    per_string_limits.max_string_bytes = 80;
    std::unique_ptr<geometer::StepTopologySession> per_string_session;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), per_string_limits,
                                                     &per_string_session, &status) == 0,
            "per-string test open failed: " + status.message);
    geometer::StepTopologySnapshot per_string_snapshot;
    require(per_string_session->inspect({}, &per_string_snapshot, &status) == 0,
            "per-string inspect failed");
    geometer::StepTopologyGroupTransaction oversized_id;
    oversized_id.expected_generation = per_string_snapshot.session.generation;
    oversized_id.commands.push_back(
        create_command("wn.geometer.research.group." + std::string(60, 'x'), "bounded",
                       {per_string_snapshot.faces[0].handle}));
    geometer::StepTopologyGroupTransactionResult rejected;
    require(per_string_session->apply_logical_groups(oversized_id, &rejected, &status) != 0 &&
                per_string_session->info().generation == per_string_snapshot.session.generation &&
                rejected.groups.empty(),
            "an oversized authored id must fail without publication or mutation");

    const std::string group_id = "wn.geometer.research.group.session-budget";
    const std::string group_name = "Session budget";
    geometer::StepTopologyLimits total_limits;
    total_limits.max_total_string_bytes =
        baseline_snapshot.session.accounted_string_bytes + group_id.size() + group_name.size() - 1U;
    std::unique_ptr<geometer::StepTopologySession> total_session;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), total_limits,
                                                     &total_session, &status) == 0,
            "total-string test open failed: " + status.message);
    geometer::StepTopologySnapshot total_snapshot;
    require(total_session->inspect({}, &total_snapshot, &status) == 0,
            "total-string inspect failed");
    geometer::StepTopologyGroupTransaction over_total;
    over_total.expected_generation = total_snapshot.session.generation;
    over_total.commands.push_back(
        create_command(group_id, group_name, {total_snapshot.faces[0].handle}));
    require(total_session->apply_logical_groups(over_total, &rejected, &status) != 0 &&
                total_session->info().generation == total_snapshot.session.generation &&
                rejected.groups.empty(),
            "group strings must share the session-wide string budget with snapshot strings");
}

void logical_group_transaction_member_references_are_aggregate_bounded()
{
    const auto bytes = read_fixture();
    geometer::StepTopologyLimits limits;
    limits.max_group_transaction_member_references = 2U;
    std::unique_ptr<geometer::StepTopologySession> session;
    geometer::Status status;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), limits, &session,
                                                     &status) == 0,
            "aggregate-member test open failed: " + status.message);
    geometer::StepTopologySnapshot snapshot;
    require(session->inspect({}, &snapshot, &status) == 0 && snapshot.faces.size() >= 2U,
            "aggregate-member test needs two faces");

    geometer::StepTopologyGroupTransaction exact_transaction;
    exact_transaction.expected_generation = snapshot.session.generation;
    exact_transaction.commands.push_back(create_command("wn.geometer.research.group.aggregate-a",
                                                        "Aggregate A", {snapshot.faces[0].handle}));
    exact_transaction.commands.push_back(create_command("wn.geometer.research.group.aggregate-b",
                                                        "Aggregate B", {snapshot.faces[1].handle}));
    geometer::StepTopologyGroupTransactionResult result;
    require(session->apply_logical_groups(exact_transaction, &result, &status) == 0 &&
                result.groups.size() == 2U,
            "two individually valid commands at the exact aggregate limit must succeed");

    limits.max_group_transaction_member_references = 1U;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), limits, &session,
                                                     &status) == 0,
            "over-aggregate-member test open failed: " + status.message);
    require(session->inspect({}, &snapshot, &status) == 0, "over-aggregate-member inspect failed");
    geometer::StepTopologyGroupTransaction over_transaction;
    over_transaction.expected_generation = snapshot.session.generation;
    over_transaction.commands.push_back(create_command("wn.geometer.research.group.aggregate-a",
                                                       "Aggregate A", {snapshot.faces[0].handle}));
    over_transaction.commands.push_back(create_command("wn.geometer.research.group.aggregate-b",
                                                       "Aggregate B", {snapshot.faces[1].handle}));
    require(session->apply_logical_groups(over_transaction, &result, &status) != 0 &&
                result.groups.empty() && session->info().generation == snapshot.session.generation,
            "aggregate exhaustion across individually valid commands must fail without mutation");
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

void session_store_evicts_a_mutation_that_crosses_its_byte_limit()
{
    const auto bytes = read_fixture();
    geometer::Status status;
    std::unique_ptr<geometer::StepTopologySession> measured;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &measured,
                                                     &status) == 0,
            "measurement open failed: " + status.message);
    geometer::StepTopologySnapshot measured_snapshot;
    require(measured->inspect({}, &measured_snapshot, &status) == 0, "measurement inspect failed");
    const std::size_t before_bytes = measured->info().estimated_resident_bytes;
    geometer::StepTopologyGroupTransaction measured_transaction;
    measured_transaction.expected_generation = measured_snapshot.session.generation;
    measured_transaction.commands.push_back(
        create_command("wn.geometer.research.group.store-eviction", "Store eviction",
                       {measured_snapshot.faces[0].handle, measured_snapshot.faces[1].handle}));
    geometer::StepTopologyGroupTransactionResult measured_result;
    require(measured->apply_logical_groups(measured_transaction, &measured_result, &status) == 0,
            "measurement apply failed: " + status.message);
    const std::size_t after_bytes = measured->info().estimated_resident_bytes;
    require(after_bytes > before_bytes, "logical groups must increase the resident estimate");

    geometer::StepTopologyLimits limits;
    limits.max_store_estimated_bytes = after_bytes - 1U;
    geometer::StepTopologySessionStore store(limits);
    geometer::StepTopologyOpenResult opened;
    require(store.open_step(bytes.data(), bytes.size(), &opened, &status) == 0,
            "bounded store open failed: " + status.message);
    geometer::StepTopologySnapshot snapshot;
    require(store.inspect(opened.session.session_handle, {}, &snapshot, &status) == 0,
            "bounded store inspect failed");
    geometer::StepTopologyGroupTransaction transaction;
    transaction.expected_generation = snapshot.session.generation;
    transaction.commands.push_back(
        create_command("wn.geometer.research.group.store-eviction", "Store eviction",
                       {snapshot.faces[0].handle, snapshot.faces[1].handle}));
    geometer::StepTopologyGroupTransactionResult result;
    require(store.apply_logical_groups(opened.session.session_handle, transaction, &result,
                                       &status) != 0 &&
                store.size() == 0 && store.estimated_resident_bytes() == 0 &&
                result.session.generation == 0 && result.groups.empty(),
            "a post-mutation store overflow must evict the session and clear publication output");
    require(store.inspect(opened.session.session_handle, {}, &snapshot, &status) != 0,
            "a session evicted after mutation must not remain addressable");
}

void publication_gate_rejects_before_mutation_commit()
{
    const auto bytes = read_fixture();
    geometer::StepTopologySessionStore store;
    geometer::StepTopologyOpenResult opened;
    geometer::Status status;
    require(store.open_step(bytes.data(), bytes.size(), &opened, &status) == 0,
            "publication-gate source open failed: " + status.message);
    geometer::StepTopologySnapshot snapshot;
    require(store.inspect(opened.session.session_handle, {}, &snapshot, &status) == 0,
            "publication-gate inspect failed: " + status.message);
    const std::size_t before_group_store_bytes = store.estimated_resident_bytes();
    const std::size_t before_group_session_bytes = snapshot.session.estimated_resident_bytes;

    geometer::StepTopologyGroupTransaction create;
    create.expected_generation = snapshot.session.generation;
    create.commands.push_back(create_command("wn.geometer.research.group.publication-gate",
                                             "Publication gate", {snapshot.faces[0].handle}));
    bool group_candidate_observed = false;
    geometer::StepTopologyGroupTransactionResult rejected_group;
    require(store.apply_logical_groups(opened.session.session_handle, create,
                                       reject_group_publication, &group_candidate_observed,
                                       &rejected_group, &status) != 0 &&
                group_candidate_observed && rejected_group.session.session_handle.empty(),
            "the group publication gate must inspect and reject the candidate result");
    geometer::StepTopologySessionInfo after_group_rejection;
    require(store.info(opened.session.session_handle, &after_group_rejection, &status) == 0 &&
                after_group_rejection.generation == snapshot.session.generation &&
                after_group_rejection.edit_journal_revision == 0 &&
                after_group_rejection.estimated_resident_bytes == before_group_session_bytes &&
                store.estimated_resident_bytes() == before_group_store_bytes,
            "a rejected group publication at the empty-journal capacity boundary must preserve "
            "session and store accounting");

    geometer::StepTopologyGroupTransactionResult created;
    require(store.apply_logical_groups(opened.session.session_handle, create, &created, &status) ==
                0,
            "group creation after publication rollback failed: " + status.message);
    geometer::StepTopologyProbeTransaction attach;
    attach.expected_generation = created.session.generation;
    geometer::StepTopologyProbeCommand command;
    command.kind = geometer::StepTopologyProbeCommandKind::attach;
    command.authored_id = "wn.geometer.research.probe.publication-gate";
    command.target.kind = geometer::StepTopologyProbeTargetKind::logical_group;
    command.target.group_authored_id = created.groups[0].authored_id;
    command.key = "wn.geometer.research.probe.key.publication-gate";
    command.value = "candidate";
    attach.commands.push_back(std::move(command));
    const std::size_t before_probe_store_bytes = store.estimated_resident_bytes();
    const std::size_t before_probe_session_bytes = created.session.estimated_resident_bytes;
    bool probe_candidate_observed = false;
    geometer::StepTopologyProbeTransactionResult rejected_probe;
    require(store.apply_metadata_probes(opened.session.session_handle, attach,
                                        reject_probe_publication, &probe_candidate_observed,
                                        &rejected_probe, &status) != 0 &&
                probe_candidate_observed && rejected_probe.session.session_handle.empty(),
            "the probe publication gate must inspect and reject the candidate result");
    geometer::StepTopologySessionInfo after_probe_rejection;
    require(store.info(opened.session.session_handle, &after_probe_rejection, &status) == 0 &&
                after_probe_rejection.generation == created.session.generation &&
                after_probe_rejection.edit_journal_revision == 1 &&
                after_probe_rejection.estimated_resident_bytes == before_probe_session_bytes &&
                store.estimated_resident_bytes() == before_probe_store_bytes,
            "a rejected probe publication at the one-entry journal capacity boundary must "
            "preserve session and store accounting");
    geometer::StepTopologyProbeTransactionResult attached;
    require(store.apply_metadata_probes(opened.session.session_handle, attach, &attached,
                                        &status) == 0 &&
                attached.probes.size() == 1,
            "probe attach after publication rollback failed: " + status.message);
}

void edit_journal_checkpoint_replays_only_against_the_exact_source()
{
    const auto bytes = read_fixture();
    geometer::Status status;
    std::unique_ptr<geometer::StepTopologySession> session;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &session,
                                                     &status) == 0,
            "journal source open failed: " + status.message);
    geometer::StepTopologySnapshot snapshot;
    require(session->inspect({}, &snapshot, &status) == 0, "journal source inspect failed");
    const std::string original_face_handle = snapshot.faces[0].handle;

    geometer::StepTopologyGroupTransaction create;
    create.expected_generation = snapshot.session.generation;
    create.commands.push_back(create_command("wn.geometer.research.group.restart", "Restart",
                                             {snapshot.faces[0].handle, snapshot.faces[1].handle}));
    geometer::StepTopologyGroupTransactionResult created;
    require(session->apply_logical_groups(create, &created, &status) == 0,
            "journal create failed: " + status.message);
    require(created.session.edit_journal_revision == 1,
            "first committed transaction must advance the journal revision");

    geometer::StepTopologyGroupTransaction rename;
    rename.expected_generation = created.session.generation;
    geometer::StepTopologyGroupCommand rename_command;
    rename_command.kind = geometer::StepTopologyGroupCommandKind::rename;
    rename_command.authored_id = created.groups[0].authored_id;
    rename_command.expected_revision = created.groups[0].revision;
    rename_command.name = "Restarted group";
    rename.commands.push_back(std::move(rename_command));
    geometer::StepTopologyGroupTransactionResult renamed;
    require(session->apply_logical_groups(rename, &renamed, &status) == 0,
            "journal rename failed: " + status.message);

    geometer::StepTopologyEditJournalCheckpoint checkpoint;
    require(session->checkpoint_edit_journal(&checkpoint, &status) == 0,
            "journal checkpoint failed: " + status.message);
    require(checkpoint.research_format == "geometer.step_topology_edit_journal.a0" &&
                checkpoint.source_sha256 == session->info().source_sha256 &&
                checkpoint.target_inventory_sha256.size() == 64 &&
                checkpoint.transaction_count == 2 && !checkpoint.bytes.empty() &&
                checkpoint.content_sha256 ==
                    geometer::sha256_hex(checkpoint.bytes.data(), checkpoint.bytes.size()),
            "journal checkpoint must be typed, source-bound, counted, and content-addressed");

    std::unique_ptr<geometer::StepTopologySession> restored;
    geometer::StepTopologyEditJournalRestoreResult restored_groups;
    require(geometer::StepTopologySession::open_step_with_edit_journal(
                bytes.data(), bytes.size(), checkpoint.bytes.data(), checkpoint.bytes.size(), {},
                &restored, &restored_groups, &status) == 0,
            "journal replay failed: " + status.message);
    require(restored->info().session_handle != session->info().session_handle &&
                restored->info().generation == 3 && restored->info().edit_journal_revision == 2 &&
                restored_groups.groups.size() == 1 &&
                restored_groups.groups[0].name == "Restarted group" &&
                restored_groups.groups[0].revision == 2 &&
                restored_groups.groups[0].members.size() == 2 &&
                restored_groups.groups[0].members[0].target_handle != original_face_handle,
            "replay must rebuild group state with fresh runtime handles");
    geometer::StepTopologySnapshot restored_snapshot;
    require(restored->inspect({}, &restored_snapshot, &status) == 0 &&
                restored_snapshot.brep_sha256 == checkpoint.source_brep_sha256,
            "replay must preserve and verify the source B-rep evidence");
    geometer::StepTopologyEditJournalCheckpoint replayed_checkpoint;
    require(restored->checkpoint_edit_journal(&replayed_checkpoint, &status) == 0 &&
                replayed_checkpoint.bytes == checkpoint.bytes,
            "replayed transactions must reproduce the canonical checkpoint bytes");

    std::vector<unsigned char> tampered = checkpoint.bytes;
    tampered[tampered.size() / 2U] ^= 0x01U;
    restored.reset();
    restored_groups = {};
    require(geometer::StepTopologySession::open_step_with_edit_journal(
                bytes.data(), bytes.size(), tampered.data(), tampered.size(), {}, &restored,
                &restored_groups, &status) != 0 &&
                restored == nullptr && restored_groups.groups.empty(),
            "a tampered checkpoint must fail closed before publishing a restored session");

    std::vector<unsigned char> wrong_inventory = checkpoint.bytes;
    constexpr std::size_t inventory_offset = 12U + 4U + 64U + 4U + 64U + 4U;
    require(inventory_offset < wrong_inventory.size() - 32U,
            "journal inventory digest offset is invalid");
    wrong_inventory[inventory_offset] = wrong_inventory[inventory_offset] == '0' ? '1' : '0';
    resign_journal(&wrong_inventory);
    require(geometer::StepTopologySession::open_step_with_edit_journal(
                bytes.data(), bytes.size(), wrong_inventory.data(), wrong_inventory.size(), {},
                &restored, &restored_groups, &status) != 0 &&
                restored == nullptr && restored_groups.groups.empty(),
            "a validly checksummed journal with a different target inventory must fail closed");

    const auto other_source = read_fixture("generated_flat_multi_solid.step");
    require(geometer::StepTopologySession::open_step_with_edit_journal(
                other_source.data(), other_source.size(), checkpoint.bytes.data(),
                checkpoint.bytes.size(), {}, &restored, &restored_groups, &status) != 0 &&
                restored == nullptr && restored_groups.groups.empty(),
            "a checkpoint must not replay against a different STEP source");

    geometer::StepTopologyLimits journal_limits;
    journal_limits.max_edit_journal_transactions = 1;
    std::unique_ptr<geometer::StepTopologySession> bounded;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), journal_limits,
                                                     &bounded, &status) == 0,
            "bounded journal open failed: " + status.message);
    require(bounded->inspect({}, &snapshot, &status) == 0, "bounded journal inspect failed");
    create.expected_generation = snapshot.session.generation;
    create.commands[0].member_handles = {snapshot.faces[0].handle};
    require(bounded->apply_logical_groups(create, &created, &status) == 0,
            "bounded journal first apply failed: " + status.message);
    rename.expected_generation = created.session.generation;
    rename.commands[0].expected_revision = created.groups[0].revision;
    geometer::StepTopologyGroupTransactionResult bounded_rejected;
    require(bounded->apply_logical_groups(rename, &bounded_rejected, &status) != 0 &&
                bounded->info().generation == created.session.generation &&
                bounded->info().edit_journal_revision == 1 && bounded_rejected.groups.empty(),
            "journal exhaustion must reject a transaction without mutation or publication");
}

void store_restore_preconditions_fail_before_session_eviction()
{
    const auto bytes = read_fixture();
    geometer::StepTopologyLimits limits;
    limits.max_sessions = 1;
    geometer::StepTopologySessionStore store(limits);
    geometer::Status status;
    geometer::StepTopologyOpenResult opened;
    require(store.open_step(bytes.data(), bytes.size(), &opened, &status) == 0,
            "restore-precondition source open failed: " + status.message);

    geometer::StepTopologyEditJournalCheckpoint checkpoint;
    require(store.checkpoint_edit_journal(opened.session.session_handle, &checkpoint, &status) == 0,
            "restore-precondition checkpoint failed: " + status.message);
    geometer::StepTopologyEditJournalReplayPreconditions preconditions;
    preconditions.source_sha256 = checkpoint.source_sha256;
    preconditions.source_brep_sha256 = checkpoint.source_brep_sha256;
    preconditions.target_inventory_sha256 = checkpoint.target_inventory_sha256;
    preconditions.occt_version = checkpoint.occt_version;
    preconditions.transaction_count = checkpoint.transaction_count + 1U;

    geometer::StepTopologyOpenResult rejected;
    geometer::StepTopologyEditJournalRestoreResult rejected_state;
    require(store.open_step_with_edit_journal(bytes.data(), bytes.size(), checkpoint.bytes.data(),
                                              checkpoint.bytes.size(), preconditions, &rejected,
                                              &rejected_state, &status) != 0 &&
                store.size() == 1 && rejected.session.session_handle.empty() &&
                rejected.evicted_session_handles.empty() &&
                rejected_state.session.session_handle.empty(),
            "a replay-precondition mismatch must fail before publishing or evicting a session");
    geometer::StepTopologySessionInfo still_live;
    require(store.info(opened.session.session_handle, &still_live, &status) == 0 &&
                still_live.session_handle == opened.session.session_handle,
            "a rejected restore must preserve the existing store session");

    preconditions.transaction_count = checkpoint.transaction_count;
    geometer::StepTopologyOpenResult restored;
    geometer::StepTopologyEditJournalRestoreResult restored_state;
    require(store.open_step_with_edit_journal(bytes.data(), bytes.size(), checkpoint.bytes.data(),
                                              checkpoint.bytes.size(), preconditions, &restored,
                                              &restored_state, &status) == 0,
            "valid store restore failed: " + status.message);
    require(store.size() == 1 && restored.session.session_handle != opened.session.session_handle &&
                restored.evicted_session_handles.size() == 1 &&
                restored.evicted_session_handles.front() == opened.session.session_handle &&
                restored_state.session.session_handle == restored.session.session_handle,
            "a valid restore must publish a fresh session and report deterministic eviction");
    require(store.info(opened.session.session_handle, &still_live, &status) != 0,
            "the session evicted by a successful restore must become unaddressable");
}

void edit_journal_byte_limits_are_preflighted_before_mutation()
{
    const auto bytes = read_fixture();
    geometer::Status status;
    std::unique_ptr<geometer::StepTopologySession> measured;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &measured,
                                                     &status) == 0,
            "journal byte measurement open failed: " + status.message);
    geometer::StepTopologyEditJournalCheckpoint empty;
    require(measured->checkpoint_edit_journal(&empty, &status) == 0 && !empty.bytes.empty(),
            "empty journal measurement failed: " + status.message);

    geometer::StepTopologyLimits too_small;
    too_small.max_edit_journal_bytes = empty.bytes.size() - 1U;
    std::unique_ptr<geometer::StepTopologySession> bounded;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), too_small,
                                                     &bounded, &status) != 0 &&
                bounded == nullptr,
            "a session must not open when its empty checkpoint cannot fit");

    geometer::StepTopologyLimits exact_empty;
    exact_empty.max_edit_journal_bytes = empty.bytes.size();
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), exact_empty,
                                                     &bounded, &status) == 0,
            "the exact empty-journal byte limit must be admitted: " + status.message);
    geometer::StepTopologyEditJournalCheckpoint exact_empty_checkpoint;
    require(bounded->checkpoint_edit_journal(&exact_empty_checkpoint, &status) == 0 &&
                exact_empty_checkpoint.bytes.size() == exact_empty.max_edit_journal_bytes,
            "the exact empty-journal byte limit must remain checkpointable");

    geometer::StepTopologySnapshot snapshot;
    require(measured->inspect({}, &snapshot, &status) == 0, "journal measurement inspect failed");
    geometer::StepTopologyGroupTransaction transaction;
    transaction.expected_generation = snapshot.session.generation;
    transaction.commands.push_back(create_command("wn.geometer.research.group.byte-limit",
                                                  "Byte limit", {snapshot.faces[0].handle}));
    geometer::StepTopologyGroupTransactionResult applied;
    require(measured->apply_logical_groups(transaction, &applied, &status) == 0,
            "journal measurement apply failed: " + status.message);
    geometer::StepTopologyEditJournalCheckpoint one_transaction;
    require(measured->checkpoint_edit_journal(&one_transaction, &status) == 0,
            "one-transaction journal measurement failed: " + status.message);

    geometer::StepTopologyLimits one_byte_short;
    one_byte_short.max_edit_journal_bytes = one_transaction.bytes.size() - 1U;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), one_byte_short,
                                                     &bounded, &status) == 0,
            "one-byte-short transaction session should still fit its empty journal");
    require(bounded->inspect({}, &snapshot, &status) == 0, "bounded journal inspect failed");
    transaction.expected_generation = snapshot.session.generation;
    transaction.commands[0].member_handles = {snapshot.faces[0].handle};
    geometer::StepTopologyGroupTransactionResult rejected;
    require(bounded->apply_logical_groups(transaction, &rejected, &status) != 0 &&
                bounded->info().generation == snapshot.session.generation &&
                bounded->info().edit_journal_revision == 0 && rejected.groups.empty(),
            "a journal that is one byte too large must be rejected before mutation");

    geometer::StepTopologyLimits exact_one;
    exact_one.max_edit_journal_bytes = one_transaction.bytes.size();
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), exact_one,
                                                     &bounded, &status) == 0,
            "exact one-transaction journal open failed: " + status.message);
    require(bounded->inspect({}, &snapshot, &status) == 0, "exact journal inspect failed");
    transaction.expected_generation = snapshot.session.generation;
    transaction.commands[0].member_handles = {snapshot.faces[0].handle};
    require(bounded->apply_logical_groups(transaction, &applied, &status) == 0,
            "the exact one-transaction byte limit must be admitted: " + status.message);
    geometer::StepTopologyEditJournalCheckpoint exact_one_checkpoint;
    require(bounded->checkpoint_edit_journal(&exact_one_checkpoint, &status) == 0 &&
                exact_one_checkpoint.bytes.size() == exact_one.max_edit_journal_bytes,
            "an admitted exact-limit mutation must always remain checkpointable");
}

void edit_journal_replay_is_bounded_cancellable_and_locale_independent()
{
    const auto bytes = read_fixture();
    geometer::Status status;
    std::unique_ptr<geometer::StepTopologySession> session;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &session,
                                                     &status) == 0,
            "replay bound source open failed: " + status.message);
    geometer::StepTopologySnapshot snapshot;
    require(session->inspect({}, &snapshot, &status) == 0, "replay bound inspect failed");
    geometer::StepTopologyGroupTransaction create;
    create.expected_generation = snapshot.session.generation;
    create.commands.push_back(create_command("wn.geometer.research.group.replay-bound", "Replay 0",
                                             {snapshot.faces[0].handle}));
    geometer::StepTopologyGroupTransactionResult result;
    require(session->apply_logical_groups(create, &result, &status) == 0,
            "replay bound create failed: " + status.message);
    constexpr std::size_t replay_transaction_count = 129U;
    for (std::size_t index = 1; index < replay_transaction_count; ++index)
    {
        geometer::StepTopologyGroupTransaction rename;
        rename.expected_generation = result.session.generation;
        geometer::StepTopologyGroupCommand command;
        command.kind = geometer::StepTopologyGroupCommandKind::rename;
        command.authored_id = result.groups[0].authored_id;
        command.expected_revision = result.groups[0].revision;
        command.name = "Replay " + std::to_string(index);
        rename.commands.push_back(std::move(command));
        require(session->apply_logical_groups(rename, &result, &status) == 0,
                "large replay history construction failed: " + status.message);
    }
    geometer::StepTopologyEditJournalCheckpoint checkpoint;
    require(session->checkpoint_edit_journal(&checkpoint, &status) == 0 &&
                checkpoint.transaction_count == replay_transaction_count,
            "large replay checkpoint failed: " + status.message);

    geometer::StepTopologyLimits bounded_limits;
    bounded_limits.max_edit_journal_replay_work_items = 1U;
    std::unique_ptr<geometer::StepTopologySession> restored;
    geometer::StepTopologyEditJournalRestoreResult restored_state;
    require(geometer::StepTopologySession::open_step_with_edit_journal(
                bytes.data(), bytes.size(), checkpoint.bytes.data(), checkpoint.bytes.size(),
                bounded_limits, &restored, &restored_state, &status) != 0 &&
                restored == nullptr,
            "an oversized replay history must fail its explicit work budget");

    geometer::StepTopologyCancellation cancellation;
    const auto cancel_at_apply_entry = [](void* context)
    { static_cast<geometer::StepTopologyCancellation*>(context)->request_cancel(); };
    geometer::step_topology_internal::set_edit_journal_replay_apply_entry_hook_for_test(
        cancel_at_apply_entry, &cancellation);
    const int cancelled_code = geometer::StepTopologySession::open_step_with_edit_journal(
        bytes.data(), bytes.size(), checkpoint.bytes.data(), checkpoint.bytes.size(), {},
        &cancellation, &restored, &restored_state, &status);
    require(cancelled_code != 0 && restored == nullptr && status.code == 109 &&
                status.message.find("replay transaction") != std::string::npos,
            "replay apply-entry cancellation must fail without publishing a session: " +
                status.message);

    std::unique_ptr<geometer::StepTopologySession> classic_session;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {},
                                                     &classic_session, &status) == 0,
            "classic-locale source open failed");
    require(classic_session->inspect({}, &snapshot, &status) == 0,
            "classic-locale source inspect failed");
    create.expected_generation = snapshot.session.generation;
    create.commands[0].authored_id = "wn.geometer.research.group.locale";
    create.commands[0].name = "Locale";
    create.commands[0].member_handles = {snapshot.faces[0].handle};
    require(classic_session->apply_logical_groups(create, &result, &status) == 0,
            "classic-locale apply failed");
    geometer::StepTopologyEditJournalCheckpoint classic_checkpoint;
    require(classic_session->checkpoint_edit_journal(&classic_checkpoint, &status) == 0,
            "classic-locale checkpoint failed");

    std::unique_ptr<geometer::StepTopologySession> comma_session;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &comma_session,
                                                     &status) == 0,
            "alternate-locale source open failed");
    require(comma_session->inspect({}, &snapshot, &status) == 0,
            "alternate-locale source inspect failed");
    const std::locale previous = std::locale();
    std::locale::global(std::locale(previous, new CommaDecimalPunctuation));
    create.expected_generation = snapshot.session.generation;
    create.commands[0].member_handles = {snapshot.faces[0].handle};
    const int locale_apply_code = comma_session->apply_logical_groups(create, &result, &status);
    geometer::StepTopologyEditJournalCheckpoint comma_checkpoint;
    const int locale_checkpoint_code =
        comma_session->checkpoint_edit_journal(&comma_checkpoint, &status);
    std::locale::global(previous);
    require(locale_apply_code == 0 && locale_checkpoint_code == 0 &&
                comma_checkpoint.bytes == classic_checkpoint.bytes,
            "checkpoint evidence must not depend on the process numeric locale");

    require(comma_session->inspect({}, &snapshot, &status) == 0,
            "post-locale namespace inspect failed");
    geometer::StepTopologyGroupTransaction non_ascii;
    non_ascii.expected_generation = snapshot.session.generation;
    non_ascii.commands.push_back(
        create_command("wn.geometer.research.group." + std::string(1, static_cast<char>(0xe9)),
                       "Non ASCII", {snapshot.faces[0].handle}));
    geometer::StepTopologyGroupTransactionResult rejected;
    require(comma_session->apply_logical_groups(non_ascii, &rejected, &status) != 0 &&
                comma_session->info().generation == snapshot.session.generation &&
                rejected.groups.empty(),
            "namespace suffixes must remain explicitly ASCII under every process locale");
}

void metadata_probes_cover_all_research_targets_and_replay()
{
    const auto bytes = read_fixture("generated_repeated_occurrences.step");
    geometer::Status status;
    std::unique_ptr<geometer::StepTopologySession> session;
    require(geometer::StepTopologySession::open_step(bytes.data(), bytes.size(), {}, &session,
                                                     &status) == 0,
            "probe source open failed: " + status.message);
    geometer::StepTopologySnapshot snapshot;
    require(session->inspect({}, &snapshot, &status) == 0 && !snapshot.definitions.empty() &&
                !snapshot.root_occurrences.empty() && !snapshot.occurrences.empty() &&
                !snapshot.bodies.empty() && !snapshot.faces.empty(),
            "probe fixture must expose every supported native target class");
    const std::string brep_sha256 = snapshot.brep_sha256;

    geometer::StepTopologyGroupTransaction create_group;
    create_group.expected_generation = snapshot.session.generation;
    create_group.commands.push_back(create_command("wn.geometer.research.group.probe-target",
                                                   "Probe target", {snapshot.faces[0].handle}));
    geometer::StepTopologyGroupTransactionResult group_result;
    require(session->apply_logical_groups(create_group, &group_result, &status) == 0,
            "probe target group create failed: " + status.message);
    require(session->inspect({}, &snapshot, &status) == 0, "probe target refresh inspect failed");

    geometer::StepTopologyProbeTarget document_target;
    document_target.kind = geometer::StepTopologyProbeTargetKind::document;
    geometer::StepTopologyProbeTarget definition_target;
    definition_target.kind = geometer::StepTopologyProbeTargetKind::definition;
    definition_target.target_handle = snapshot.definitions[0].handle;
    geometer::StepTopologyProbeTarget root_target;
    root_target.kind = geometer::StepTopologyProbeTargetKind::root_occurrence;
    root_target.target_handle = snapshot.root_occurrences[0].handle;
    geometer::StepTopologyProbeTarget occurrence_target;
    occurrence_target.kind = geometer::StepTopologyProbeTargetKind::occurrence;
    occurrence_target.target_handle = snapshot.occurrences[0].handle;
    geometer::StepTopologyProbeTarget body_target;
    body_target.kind = geometer::StepTopologyProbeTargetKind::body;
    body_target.target_handle = snapshot.bodies[0].handle;
    geometer::StepTopologyProbeTarget face_target;
    face_target.kind = geometer::StepTopologyProbeTargetKind::face;
    face_target.target_handle = snapshot.faces[0].handle;
    const std::string old_face_handle = face_target.target_handle;
    geometer::StepTopologyProbeTarget group_target;
    group_target.kind = geometer::StepTopologyProbeTargetKind::logical_group;
    group_target.group_authored_id = "wn.geometer.research.group.probe-target";

    geometer::StepTopologyProbeTransaction attach;
    attach.expected_generation = snapshot.session.generation;
    attach.commands = {
        attach_probe_command("wn.geometer.research.probe.document", document_target, "document"),
        attach_probe_command("wn.geometer.research.probe.definition", definition_target,
                             "definition"),
        attach_probe_command("wn.geometer.research.probe.root", root_target, "root"),
        attach_probe_command("wn.geometer.research.probe.occurrence", occurrence_target,
                             "occurrence"),
        attach_probe_command("wn.geometer.research.probe.body", body_target, "body"),
        attach_probe_command("wn.geometer.research.probe.face", face_target, "face"),
        attach_probe_command("wn.geometer.research.probe.group", group_target, "group"),
    };
    geometer::StepTopologyProbeTransactionResult attached;
    require(session->apply_metadata_probes(attach, &attached, &status) == 0,
            "probe attach failed: " + status.message);
    require(attached.probes.size() == 7 && attached.groups.size() == 1 &&
                attached.session.edit_journal_revision == 2,
            "probe attachment must atomically publish all targets and refreshed groups");
    const auto face_probe =
        std::find_if(attached.probes.begin(), attached.probes.end(), [](const auto& probe)
                     { return probe.authored_id == "wn.geometer.research.probe.face"; });
    require(face_probe != attached.probes.end() &&
                face_probe->target.target_handle != old_face_handle,
            "probe publication must replace stale runtime handles");
    require(session->inspect({}, &snapshot, &status) == 0 && snapshot.brep_sha256 == brep_sha256,
            "metadata probes must not change the B-rep evidence digest");

    geometer::StepTopologyGroupTransaction erase_group;
    erase_group.expected_generation = attached.session.generation;
    geometer::StepTopologyGroupCommand erase_group_command;
    erase_group_command.kind = geometer::StepTopologyGroupCommandKind::erase;
    erase_group_command.authored_id = attached.groups[0].authored_id;
    erase_group_command.expected_revision = attached.groups[0].revision;
    erase_group.commands.push_back(std::move(erase_group_command));
    geometer::StepTopologyGroupTransactionResult group_rejected;
    require(session->apply_logical_groups(erase_group, &group_rejected, &status) != 0 &&
                session->info().generation == attached.session.generation &&
                group_rejected.groups.empty(),
            "a logical group with an attached probe must not be erased");

    geometer::StepTopologyProbeTransaction invalid;
    invalid.expected_generation = attached.session.generation;
    geometer::StepTopologyProbeTarget forged_target;
    forged_target.kind = geometer::StepTopologyProbeTargetKind::face;
    forged_target.target_handle = "forged";
    invalid.commands.push_back(
        attach_probe_command("wn.geometer.research.probe.invalid", forged_target, "invalid"));
    geometer::StepTopologyProbeTransactionResult probe_rejected;
    require(session->apply_metadata_probes(invalid, &probe_rejected, &status) != 0 &&
                session->info().generation == attached.session.generation &&
                probe_rejected.probes.empty(),
            "a forged probe target must fail without mutation or publication");

    geometer::StepTopologyProbeTransaction replace;
    replace.expected_generation = attached.session.generation;
    geometer::StepTopologyProbeCommand replace_command;
    replace_command.kind = geometer::StepTopologyProbeCommandKind::replace;
    replace_command.authored_id = face_probe->authored_id;
    replace_command.expected_revision = face_probe->revision;
    replace_command.target = face_probe->target;
    replace_command.key = face_probe->key;
    replace_command.value = "updated face";
    replace.commands.push_back(std::move(replace_command));
    geometer::StepTopologyProbeTransactionResult replaced;
    require(session->apply_metadata_probes(replace, &replaced, &status) == 0,
            "probe replace failed: " + status.message);
    const auto updated_face_probe =
        std::find_if(replaced.probes.begin(), replaced.probes.end(), [](const auto& probe)
                     { return probe.authored_id == "wn.geometer.research.probe.face"; });
    require(updated_face_probe != replaced.probes.end() && updated_face_probe->revision == 2 &&
                updated_face_probe->value == "updated face",
            "probe replacement must advance its revision and publish the new value");

    geometer::StepTopologyProbeTransaction erase_probe;
    erase_probe.expected_generation = replaced.session.generation;
    geometer::StepTopologyProbeCommand erase_probe_command;
    erase_probe_command.kind = geometer::StepTopologyProbeCommandKind::erase;
    erase_probe_command.authored_id = "wn.geometer.research.probe.group";
    erase_probe_command.expected_revision = 1;
    erase_probe.commands.push_back(std::move(erase_probe_command));
    geometer::StepTopologyProbeTransaction malformed_erase = erase_probe;
    malformed_erase.commands[0].target.kind = geometer::StepTopologyProbeTargetKind::logical_group;
    require(session->apply_metadata_probes(malformed_erase, &probe_rejected, &status) != 0 &&
                session->info().generation == replaced.session.generation &&
                probe_rejected.probes.empty(),
            "probe erase must reject a noncanonical target discriminant without mutation");
    geometer::StepTopologyProbeTransactionResult probe_erased;
    require(session->apply_metadata_probes(erase_probe, &probe_erased, &status) == 0 &&
                probe_erased.probes.size() == 6,
            "probe erase must remove only the authored probe");

    erase_group.expected_generation = probe_erased.session.generation;
    erase_group.commands[0].expected_revision = probe_erased.groups[0].revision;
    geometer::StepTopologyGroupTransactionResult group_erased;
    require(session->apply_logical_groups(erase_group, &group_erased, &status) == 0 &&
                group_erased.groups.empty(),
            "logical group erase must succeed after its attached probe is removed");

    geometer::StepTopologyEditJournalCheckpoint checkpoint;
    require(session->checkpoint_edit_journal(&checkpoint, &status) == 0 &&
                checkpoint.transaction_count == 5,
            "group and probe create/replace/erase transactions must share one ordered checkpoint");
    std::unique_ptr<geometer::StepTopologySession> restored;
    geometer::StepTopologyEditJournalRestoreResult restored_state;
    require(geometer::StepTopologySession::open_step_with_edit_journal(
                bytes.data(), bytes.size(), checkpoint.bytes.data(), checkpoint.bytes.size(), {},
                &restored, &restored_state, &status) == 0,
            "probe journal replay failed: " + status.message);
    require(restored_state.groups.empty() && restored_state.probes.size() == 6 &&
                restored_state.session.edit_journal_revision == 5,
            "probe replay must restore the final authored group and probe state");
    geometer::StepTopologyEditJournalCheckpoint replayed;
    require(restored->checkpoint_edit_journal(&replayed, &status) == 0 &&
                replayed.bytes == checkpoint.bytes,
            "probe replay must reproduce canonical journal bytes");

    std::vector<unsigned char> malformed = checkpoint.bytes;
    const std::string erased_id = "wn.geometer.research.probe.group";
    const std::size_t erased_id_offset = find_last_bytes(malformed, erased_id);
    require(erased_id_offset != std::string::npos &&
                erased_id_offset + erased_id.size() + 8U < malformed.size() - 32U,
            "failed locating the canonical erased probe command");
    malformed[erased_id_offset + erased_id.size() + 8U] =
        static_cast<unsigned char>(geometer::StepTopologyProbeTargetKind::logical_group);
    resign_journal(&malformed);
    restored.reset();
    restored_state = {};
    require(geometer::StepTopologySession::open_step_with_edit_journal(
                bytes.data(), bytes.size(), malformed.data(), malformed.size(), {}, &restored,
                &restored_state, &status) != 0 &&
                restored == nullptr && restored_state.probes.empty(),
            "a validly checksummed journal with a noncanonical erase must fail closed");
}

} // namespace

int main()
{
    try
    {
        logical_group_transactions_are_atomic_and_geometry_neutral();
        logical_group_strings_obey_per_string_and_session_wide_limits();
        logical_group_transaction_member_references_are_aggregate_bounded();
        session_store_routes_group_transactions_and_updates_accounting();
        session_store_evicts_a_mutation_that_crosses_its_byte_limit();
        publication_gate_rejects_before_mutation_commit();
        edit_journal_checkpoint_replays_only_against_the_exact_source();
        store_restore_preconditions_fail_before_session_eviction();
        edit_journal_byte_limits_are_preflighted_before_mutation();
        edit_journal_replay_is_bounded_cancellable_and_locale_independent();
        metadata_probes_cover_all_research_targets_and_replay();
        std::cout << "STEP topology logical group tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
