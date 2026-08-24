#include "step_topology_session_internal.h"

#include "geometer/sha256.h"

#include <Standard_Failure.hxx>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace geometer::step_topology_internal
{
namespace
{

bool checked_add(std::size_t* total, std::size_t value)
{
    if (*total > std::numeric_limits<std::size_t>::max() - value)
        return false;
    *total += value;
    return true;
}

bool cancellation_requested(const StepTopologyCancellation* cancellation, Status* status)
{
    if (cancellation == nullptr || !cancellation->is_cancelled())
        return false;
    set_status(status, kCancelled, "Metadata-probe replay transaction was cancelled.");
    return true;
}

bool valid_namespaced_value(const std::string& value, std::string_view prefix)
{
    if (value.size() <= prefix.size() || value.size() > 128U || value.rfind(prefix, 0) != 0)
        return false;
    return std::all_of(value.begin() + static_cast<std::ptrdiff_t>(prefix.size()), value.end(),
                       [](unsigned char character)
                       {
                           return (character >= 'a' && character <= 'z') ||
                                  (character >= 'A' && character <= 'Z') ||
                                  (character >= '0' && character <= '9') || character == '.' ||
                                  character == '_' || character == '-';
                       });
}

std::string number(double value)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return stream.str();
}

template <std::size_t Size>
void append_values(std::ostringstream* stream, const std::array<double, Size>& values)
{
    for (double value : values)
        *stream << '|' << number(value);
}

std::string digest(const std::string& evidence)
{
    return sha256_hex(reinterpret_cast<const std::uint8_t*>(evidence.data()), evidence.size());
}

struct ProbeLocator
{
    StepTopologyProbeTargetKind kind = StepTopologyProbeTargetKind::document;
    std::size_t index = 0;
    std::string evidence_sha256;
};

std::string definition_evidence(const StepTopologyDefinition& definition, std::size_t index)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << "definition|" << index << '|' << definition.is_assembly << '|'
           << definition.body_handles.size();
    return digest(stream.str());
}

std::string root_occurrence_evidence(const StepTopologyRootOccurrence& occurrence,
                                     std::size_t definition_index, std::size_t index)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << "root_occurrence|" << index << '|' << definition_index;
    append_values(&stream, occurrence.transform);
    return digest(stream.str());
}

std::string occurrence_evidence(const StepTopologyOccurrence& occurrence,
                                std::size_t definition_index, std::size_t index)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << "occurrence|" << index << '|' << definition_index << '|' << occurrence.depth;
    append_values(&stream, occurrence.transform);
    return digest(stream.str());
}

std::string body_evidence(const StepTopologyBody& body, std::size_t definition_index,
                          std::size_t index)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << "body|" << index << '|' << definition_index << '|' << body.topology_kind << '|'
           << body.shell_handles.size() << '|' << body.face_handles.size() << '|'
           << number(body.volume);
    append_values(&stream, body.bounds);
    return digest(stream.str());
}

std::string face_evidence(const StepTopologyFace& face, std::size_t definition_index,
                          std::size_t index)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << "face|" << index << '|' << definition_index << '|' << face.body_handles.size() << '|'
           << face.shell_handles.size() << '|' << number(face.area);
    append_values(&stream, face.bounds);
    append_values(&stream, face.centroid);
    return digest(stream.str());
}

int build_probe_locators(const SessionData* data, const StepTopologyCancellation* cancellation,
                         std::unordered_map<std::string, ProbeLocator>* locators, Status* status)
{
    std::unordered_map<std::string, std::size_t> definitions;
    definitions.reserve(data->snapshot.definitions.size());
    locators->reserve(data->snapshot.definitions.size() + data->snapshot.root_occurrences.size() +
                      data->snapshot.occurrences.size() + data->snapshot.bodies.size() +
                      data->snapshot.faces.size());
    for (std::size_t index = 0; index < data->snapshot.definitions.size(); ++index)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        const StepTopologyDefinition& definition = data->snapshot.definitions[index];
        definitions.emplace(definition.handle, index);
        locators->emplace(definition.handle,
                          ProbeLocator{StepTopologyProbeTargetKind::definition, index,
                                       definition_evidence(definition, index)});
    }
    for (std::size_t index = 0; index < data->snapshot.root_occurrences.size(); ++index)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        const StepTopologyRootOccurrence& occurrence = data->snapshot.root_occurrences[index];
        const auto definition = definitions.find(occurrence.definition_handle);
        if (definition == definitions.end())
        {
            set_status(status, kInternalFailure,
                       "Root occurrence definition is missing while staging a metadata probe.");
            return kInternalFailure;
        }
        locators->emplace(
            occurrence.handle,
            ProbeLocator{StepTopologyProbeTargetKind::root_occurrence, index,
                         root_occurrence_evidence(occurrence, definition->second, index)});
    }
    for (std::size_t index = 0; index < data->snapshot.occurrences.size(); ++index)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        const StepTopologyOccurrence& occurrence = data->snapshot.occurrences[index];
        const auto definition = definitions.find(occurrence.definition_handle);
        if (definition == definitions.end())
        {
            set_status(status, kInternalFailure,
                       "Occurrence definition is missing while staging a metadata probe.");
            return kInternalFailure;
        }
        locators->emplace(occurrence.handle,
                          ProbeLocator{StepTopologyProbeTargetKind::occurrence, index,
                                       occurrence_evidence(occurrence, definition->second, index)});
    }
    for (std::size_t index = 0; index < data->snapshot.bodies.size(); ++index)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        const StepTopologyBody& body = data->snapshot.bodies[index];
        const auto definition = definitions.find(body.definition_handle);
        if (definition == definitions.end())
        {
            set_status(status, kInternalFailure,
                       "Body definition is missing while staging a metadata probe.");
            return kInternalFailure;
        }
        locators->emplace(body.handle,
                          ProbeLocator{StepTopologyProbeTargetKind::body, index,
                                       body_evidence(body, definition->second, index)});
    }
    for (std::size_t index = 0; index < data->snapshot.faces.size(); ++index)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        const StepTopologyFace& face = data->snapshot.faces[index];
        const auto definition = definitions.find(face.definition_handle);
        if (definition == definitions.end())
        {
            set_status(status, kInternalFailure,
                       "Face definition is missing while staging a metadata probe.");
            return kInternalFailure;
        }
        locators->emplace(face.handle,
                          ProbeLocator{StepTopologyProbeTargetKind::face, index,
                                       face_evidence(face, definition->second, index)});
    }
    return 0;
}

std::string locator_key(StepTopologyProbeTargetKind kind, std::size_t index,
                        const std::string& evidence_sha256)
{
    return std::to_string(static_cast<int>(kind)) + ":" + std::to_string(index) + ":" +
           evidence_sha256;
}

int publish_probes_impl(const SessionData* data, const std::vector<MetadataProbeRecord>& records,
                        const StepTopologyCancellation* cancellation,
                        std::vector<StepTopologyMetadataProbe>* probes, Status* status)
{
    std::unordered_map<std::string, ProbeLocator> locators;
    const int locator_code = build_probe_locators(data, cancellation, &locators, status);
    if (locator_code != 0)
        return locator_code;
    std::unordered_map<std::string, std::string> handle_by_locator;
    handle_by_locator.reserve(locators.size());
    for (const auto& [handle, locator] : locators)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        const std::string key = locator_key(locator.kind, locator.index, locator.evidence_sha256);
        if (!handle_by_locator.emplace(key, handle).second)
        {
            set_status(status, kConflict, "Metadata-probe target resolves to multiple handles.");
            return kConflict;
        }
    }
    probes->reserve(records.size());
    for (const MetadataProbeRecord& record : records)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        StepTopologyMetadataProbe published;
        published.authored_id = record.authored_id;
        published.revision = record.revision;
        published.target.kind = record.target_kind;
        published.target.group_authored_id = record.group_authored_id;
        published.key = record.key;
        published.value = record.value;
        if (record.target_kind != StepTopologyProbeTargetKind::document &&
            record.target_kind != StepTopologyProbeTargetKind::logical_group)
        {
            const std::string key =
                locator_key(record.target_kind, record.target_index, record.target_evidence_sha256);
            const auto found = handle_by_locator.find(key);
            if (found == handle_by_locator.end())
            {
                set_status(status, kConflict,
                           "Metadata-probe target did not survive the generation refresh.");
                return kConflict;
            }
            published.target.target_handle = found->second;
        }
        probes->push_back(std::move(published));
    }
    return 0;
}

void clear_result(StepTopologyProbeTransactionResult* result) noexcept
{
    result->session.session_handle.clear();
    result->session.generation = 0;
    result->session.source_sha256.clear();
    result->session.occt_version.clear();
    result->session.source_bytes = 0;
    result->session.edit_journal_revision = 0;
    result->session.accounted_string_bytes = 0;
    result->session.estimated_resident_bytes = 0;
    result->groups.clear();
    result->probes.clear();
}

void publish_result(StepTopologyProbeTransactionResult* destination,
                    StepTopologyProbeTransactionResult* source) noexcept
{
    destination->session.session_handle.swap(source->session.session_handle);
    destination->session.generation = source->session.generation;
    destination->session.source_sha256.swap(source->session.source_sha256);
    destination->session.occt_version.swap(source->session.occt_version);
    destination->session.source_bytes = source->session.source_bytes;
    destination->session.edit_journal_revision = source->session.edit_journal_revision;
    destination->session.accounted_string_bytes = source->session.accounted_string_bytes;
    destination->session.estimated_resident_bytes = source->session.estimated_resident_bytes;
    destination->groups.swap(source->groups);
    destination->probes.swap(source->probes);
}

std::size_t group_string_bytes(const SessionData* data)
{
    std::size_t total = 0;
    for (const LogicalGroupRecord& group : data->logical_groups)
    {
        if (!checked_add(&total, group.authored_id.size()) ||
            !checked_add(&total, group.name.size()))
            return std::numeric_limits<std::size_t>::max();
    }
    return total;
}

} // namespace

int publish_metadata_probes(const SessionData* data,
                            const std::vector<MetadataProbeRecord>& records,
                            const StepTopologyCancellation* cancellation,
                            std::vector<StepTopologyMetadataProbe>* probes, Status* status)
{
    if (probes == nullptr)
    {
        set_status(status, kInvalidArgument, "Metadata-probe publication output is null.");
        return kInvalidArgument;
    }
    probes->clear();
    return publish_probes_impl(data, records, cancellation, probes, status);
}

int account_metadata_probe_strings(SessionData* data, const StepTopologyCancellation* cancellation,
                                   Status* status)
{
    const std::size_t before = data->total_string_bytes;
    for (const MetadataProbeRecord& probe : data->metadata_probes)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        if (!account_string(data, probe.authored_id, status) ||
            !account_string(data, probe.target_evidence_sha256, status) ||
            !account_string(data, probe.group_authored_id, status) ||
            !account_string(data, probe.key, status) || !account_string(data, probe.value, status))
            return kResourceLimit;
    }
    data->metadata_probe_string_bytes = data->total_string_bytes - before;
    return 0;
}

int stage_probe_journal_transaction(const SessionData* data,
                                    const StepTopologyProbeTransaction& transaction,
                                    const StepTopologyCancellation* cancellation,
                                    EditJournalTransactionRecord* entry,
                                    std::size_t* entry_string_bytes, Status* status)
{
    if (entry == nullptr || entry_string_bytes == nullptr)
    {
        set_status(status, kInvalidArgument, "Metadata-probe journal staging output is null.");
        return kInvalidArgument;
    }
    *entry = {};
    *entry_string_bytes = 0;
    if (data->edit_journal.size() >= data->limits.max_edit_journal_transactions ||
        data->edit_journal.size() == std::numeric_limits<std::uint64_t>::max())
    {
        set_status(status, kResourceLimit, "Edit-journal transaction limit is exhausted.");
        return kResourceLimit;
    }
    std::unordered_map<std::string, ProbeLocator> locators;
    const int locator_code = build_probe_locators(data, cancellation, &locators, status);
    if (locator_code != 0)
        return locator_code;
    entry->sequence = static_cast<std::uint64_t>(data->edit_journal.size()) + 1U;
    entry->kind = EditJournalTransactionRecord::Kind::metadata_probes;
    entry->probe_commands.reserve(transaction.commands.size());
    for (const StepTopologyProbeCommand& command : transaction.commands)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        EditJournalProbeCommandRecord recorded;
        recorded.kind = command.kind;
        recorded.authored_id = command.authored_id;
        recorded.expected_revision = command.expected_revision;
        recorded.target_kind = command.target.kind;
        recorded.group_authored_id = command.target.group_authored_id;
        recorded.key = command.key;
        recorded.value = command.value;
        if (command.target.kind != StepTopologyProbeTargetKind::document &&
            command.target.kind != StepTopologyProbeTargetKind::logical_group)
        {
            const auto found = locators.find(command.target.target_handle);
            if (found == locators.end() || found->second.kind != command.target.kind)
            {
                set_status(status, kUnknownTarget,
                           "Metadata-probe target handle is stale, unknown, or wrong-kind.");
                return kUnknownTarget;
            }
            recorded.target_index = found->second.index;
            recorded.target_evidence_sha256 = found->second.evidence_sha256;
        }
        for (const std::string* value :
             {&recorded.authored_id, &recorded.target_evidence_sha256, &recorded.group_authored_id,
              &recorded.key, &recorded.value})
        {
            if (!checked_add(entry_string_bytes, value->size()))
            {
                set_status(status, kResourceLimit,
                           "Metadata-probe journal string accounting overflowed.");
                return kResourceLimit;
            }
        }
        entry->probe_commands.push_back(std::move(recorded));
    }
    std::size_t projected_bytes = 0;
    return validate_edit_journal_append(data, *entry, &projected_bytes, status);
}

int restore_probe_transaction(const SessionData* data,
                              const EditJournalTransactionRecord& journal_entry,
                              const StepTopologyCancellation* cancellation,
                              StepTopologyProbeTransaction* transaction, Status* status)
{
    if (transaction == nullptr ||
        journal_entry.kind != EditJournalTransactionRecord::Kind::metadata_probes)
    {
        set_status(status, kInvalidArgument, "Edit-journal probe transaction is invalid.");
        return kInvalidArgument;
    }
    *transaction = {};
    transaction->expected_generation = data->info.generation;
    transaction->commands.reserve(journal_entry.probe_commands.size());
    std::unordered_map<std::string, ProbeLocator> locators;
    const int locator_code = build_probe_locators(data, cancellation, &locators, status);
    if (locator_code != 0)
        return locator_code;
    std::unordered_map<std::string, std::string> handles;
    handles.reserve(locators.size());
    for (const auto& [handle, locator] : locators)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        if (!handles
                 .emplace(locator_key(locator.kind, locator.index, locator.evidence_sha256), handle)
                 .second)
        {
            set_status(status, kConflict,
                       "Edit-journal probe target resolves to multiple handles.");
            return kConflict;
        }
    }
    for (const EditJournalProbeCommandRecord& recorded : journal_entry.probe_commands)
    {
        if (cancellation_requested(cancellation, status))
            return kCancelled;
        StepTopologyProbeCommand command;
        command.kind = recorded.kind;
        command.authored_id = recorded.authored_id;
        command.expected_revision = recorded.expected_revision;
        command.target.kind = recorded.target_kind;
        command.target.group_authored_id = recorded.group_authored_id;
        command.key = recorded.key;
        command.value = recorded.value;
        if (recorded.target_kind != StepTopologyProbeTargetKind::document &&
            recorded.target_kind != StepTopologyProbeTargetKind::logical_group)
        {
            const auto found = handles.find(locator_key(recorded.target_kind, recorded.target_index,
                                                        recorded.target_evidence_sha256));
            if (found == handles.end())
            {
                set_status(status, kConflict,
                           "Edit-journal probe target evidence does not match the source.");
                return kConflict;
            }
            command.target.target_handle = found->second;
        }
        transaction->commands.push_back(std::move(command));
    }
    return 0;
}

int apply_metadata_probe_transaction(SessionData* data,
                                     const StepTopologyProbeTransaction& transaction,
                                     const StepTopologyCancellation* cancellation,
                                     StepTopologyProbePublicationGate publication_gate,
                                     void* publication_context,
                                     StepTopologyProbeTransactionResult* result, Status* status)
{
    if (result == nullptr)
    {
        set_status(status, kInvalidArgument, "Metadata-probe transaction output is null.");
        return kInvalidArgument;
    }
    clear_result(result);
    if (cancellation_requested(cancellation, status))
        return kCancelled;
    if (transaction.expected_generation == 0 ||
        transaction.expected_generation != data->info.generation)
    {
        set_status(status, kConflict, "Metadata-probe transaction generation is stale.");
        return kConflict;
    }
    if (transaction.commands.empty() ||
        transaction.commands.size() > data->limits.max_metadata_probes)
    {
        set_status(status, kResourceLimit,
                   "Metadata-probe transaction is empty or exceeds the command limit.");
        return kResourceLimit;
    }

    StepTopologySnapshot previous_snapshot;
    std::unordered_map<std::string, HandleRecord> previous_handles;
    StepTopologySessionInfo previous_info;
    std::vector<MetadataProbeRecord> previous_probes;
    std::vector<EditJournalTransactionRecord> previous_journal;
    std::size_t previous_snapshot_string_bytes = 0;
    std::size_t previous_probe_string_bytes = 0;
    std::size_t previous_journal_string_bytes = 0;
    std::size_t previous_total_string_bytes = 0;
    std::size_t previous_journal_encoded_bytes = 0;
    std::uint64_t previous_counter = 0;
    bool mutation_started = false;
    const auto rollback = [&]() noexcept
    {
        if (!mutation_started)
            return;
        data->snapshot = std::move(previous_snapshot);
        data->handles = std::move(previous_handles);
        data->info = std::move(previous_info);
        data->metadata_probes = std::move(previous_probes);
        data->edit_journal = std::move(previous_journal);
        data->snapshot_string_bytes = previous_snapshot_string_bytes;
        data->metadata_probe_string_bytes = previous_probe_string_bytes;
        data->journal_string_bytes = previous_journal_string_bytes;
        data->total_string_bytes = previous_total_string_bytes;
        data->edit_journal_encoded_bytes = previous_journal_encoded_bytes;
        data->handle_counter = previous_counter;
        mutation_started = false;
    };

    try
    {
        EditJournalTransactionRecord journal_entry;
        std::size_t journal_entry_strings = 0;
        const int journal_code = stage_probe_journal_transaction(
            data, transaction, cancellation, &journal_entry, &journal_entry_strings, status);
        if (journal_code != 0)
            return journal_code;
        std::size_t projected_journal_bytes = 0;
        const int journal_size_code =
            validate_edit_journal_append(data, journal_entry, &projected_journal_bytes, status);
        if (journal_size_code != 0)
            return journal_size_code;

        using ProbeMap = std::unordered_map<std::string, MetadataProbeRecord>;
        ProbeMap candidate_by_id;
        candidate_by_id.reserve(data->metadata_probes.size() + transaction.commands.size());
        std::unordered_set<std::string> group_ids;
        group_ids.reserve(data->logical_groups.size());
        for (const LogicalGroupRecord& group : data->logical_groups)
        {
            if (cancellation_requested(cancellation, status))
                return kCancelled;
            group_ids.emplace(group.authored_id);
        }
        std::size_t probe_strings = 0;
        for (const MetadataProbeRecord& probe : data->metadata_probes)
        {
            if (cancellation_requested(cancellation, status))
                return kCancelled;
            if (!checked_add(&probe_strings, probe.authored_id.size()) ||
                !checked_add(&probe_strings, probe.target_evidence_sha256.size()) ||
                !checked_add(&probe_strings, probe.group_authored_id.size()) ||
                !checked_add(&probe_strings, probe.key.size()) ||
                !checked_add(&probe_strings, probe.value.size()) ||
                !candidate_by_id.emplace(probe.authored_id, probe).second)
            {
                set_status(status, kInternalFailure,
                           "Stored metadata-probe state is duplicate or unaccountable.");
                return kInternalFailure;
            }
        }
        for (std::size_t index = 0; index < transaction.commands.size(); ++index)
        {
            if (cancellation_requested(cancellation, status))
                return kCancelled;
            const StepTopologyProbeCommand& command = transaction.commands[index];
            const EditJournalProbeCommandRecord& staged = journal_entry.probe_commands[index];
            if (!valid_namespaced_value(command.authored_id, "wn.geometer.research.probe.") ||
                (!command.key.empty() &&
                 !valid_namespaced_value(command.key, "wn.geometer.research.probe.key.")) ||
                command.authored_id.size() > data->limits.max_string_bytes ||
                command.key.size() > data->limits.max_string_bytes ||
                command.value.size() > data->limits.max_string_bytes ||
                command.target.group_authored_id.size() > data->limits.max_string_bytes)
            {
                set_status(status, kInvalidArgument,
                           "Metadata-probe id, key, value, or group target is invalid.");
                return kInvalidArgument;
            }
            auto existing = candidate_by_id.find(command.authored_id);
            if (command.kind == StepTopologyProbeCommandKind::attach ||
                command.kind == StepTopologyProbeCommandKind::replace)
            {
                if (command.key.empty() || command.value.empty() ||
                    (command.target.kind == StepTopologyProbeTargetKind::document &&
                     (!command.target.target_handle.empty() ||
                      !command.target.group_authored_id.empty())) ||
                    (command.target.kind == StepTopologyProbeTargetKind::logical_group &&
                     (command.target.group_authored_id.empty() ||
                      !command.target.target_handle.empty())) ||
                    (command.target.kind != StepTopologyProbeTargetKind::document &&
                     command.target.kind != StepTopologyProbeTargetKind::logical_group &&
                     (command.target.target_handle.empty() ||
                      !command.target.group_authored_id.empty())))
                {
                    set_status(status, kInvalidArgument,
                               "Metadata-probe target or payload shape is invalid.");
                    return kInvalidArgument;
                }
                if (command.target.kind == StepTopologyProbeTargetKind::logical_group &&
                    group_ids.count(command.target.group_authored_id) == 0)
                {
                    set_status(status, kUnknownTarget,
                               "Metadata-probe logical-group target is unknown.");
                    return kUnknownTarget;
                }
                MetadataProbeRecord replacement;
                replacement.authored_id = staged.authored_id;
                replacement.revision = 1;
                replacement.target_kind = staged.target_kind;
                replacement.target_index = staged.target_index;
                replacement.target_evidence_sha256 = staged.target_evidence_sha256;
                replacement.group_authored_id = staged.group_authored_id;
                replacement.key = staged.key;
                replacement.value = staged.value;
                if (command.kind == StepTopologyProbeCommandKind::attach)
                {
                    if (existing != candidate_by_id.end() || command.expected_revision != 0 ||
                        candidate_by_id.size() >= data->limits.max_metadata_probes)
                    {
                        set_status(status, kConflict, "Metadata-probe attach precondition failed.");
                        return kConflict;
                    }
                    candidate_by_id.emplace(replacement.authored_id, std::move(replacement));
                }
                else
                {
                    if (existing == candidate_by_id.end() ||
                        command.expected_revision != existing->second.revision ||
                        existing->second.revision == std::numeric_limits<std::uint64_t>::max())
                    {
                        set_status(status, kConflict,
                                   "Metadata-probe replace precondition failed.");
                        return kConflict;
                    }
                    replacement.revision = existing->second.revision + 1U;
                    existing->second = std::move(replacement);
                }
            }
            else if (command.kind == StepTopologyProbeCommandKind::erase)
            {
                if (command.target.kind != StepTopologyProbeTargetKind::document ||
                    !command.key.empty() || !command.value.empty() ||
                    !command.target.target_handle.empty() ||
                    !command.target.group_authored_id.empty())
                {
                    set_status(
                        status, kInvalidArgument,
                        "Metadata-probe erase command has a noncanonical target or payload.");
                    return kInvalidArgument;
                }
                if (existing == candidate_by_id.end() ||
                    command.expected_revision != existing->second.revision)
                {
                    set_status(status, kConflict, "Metadata-probe erase precondition failed.");
                    return kConflict;
                }
                candidate_by_id.erase(existing);
            }
            else
            {
                set_status(status, kInvalidArgument, "Unknown metadata-probe command kind.");
                return kInvalidArgument;
            }
        }

        std::vector<MetadataProbeRecord> candidate;
        candidate.reserve(candidate_by_id.size());
        probe_strings = 0;
        for (auto& [authored_id, probe] : candidate_by_id)
        {
            if (cancellation_requested(cancellation, status))
                return kCancelled;
            (void)authored_id;
            for (const std::string* value : {&probe.authored_id, &probe.target_evidence_sha256,
                                             &probe.group_authored_id, &probe.key, &probe.value})
            {
                if (!checked_add(&probe_strings, value->size()))
                {
                    set_status(status, kResourceLimit,
                               "Metadata-probe string accounting overflowed.");
                    return kResourceLimit;
                }
            }
            candidate.push_back(std::move(probe));
        }
        std::sort(candidate.begin(), candidate.end(), [](const auto& left, const auto& right)
                  { return left.authored_id < right.authored_id; });

        std::size_t total_strings = data->snapshot_string_bytes;
        const std::size_t groups_strings = group_string_bytes(data);
        if (groups_strings == std::numeric_limits<std::size_t>::max() ||
            !checked_add(&total_strings, groups_strings) ||
            !checked_add(&total_strings, probe_strings) ||
            !checked_add(&total_strings, data->journal_string_bytes) ||
            !checked_add(&total_strings, journal_entry_strings) ||
            total_strings > data->limits.max_total_string_bytes)
        {
            set_status(status, kResourceLimit,
                       "Metadata-probe state exceeds the session string limit.");
            return kResourceLimit;
        }
        if (data->info.generation == std::numeric_limits<std::uint64_t>::max())
        {
            set_status(status, kResourceLimit, "Metadata-probe generation is exhausted.");
            return kResourceLimit;
        }

        std::vector<EditJournalTransactionRecord> candidate_journal = data->edit_journal;
        candidate_journal.push_back(std::move(journal_entry));

        previous_info = data->info;
        previous_snapshot = std::move(data->snapshot);
        previous_handles = std::move(data->handles);
        previous_probes = std::move(data->metadata_probes);
        previous_journal = std::move(data->edit_journal);
        previous_snapshot_string_bytes = data->snapshot_string_bytes;
        previous_probe_string_bytes = data->metadata_probe_string_bytes;
        previous_journal_string_bytes = data->journal_string_bytes;
        previous_total_string_bytes = data->total_string_bytes;
        previous_journal_encoded_bytes = data->edit_journal_encoded_bytes;
        previous_counter = data->handle_counter;
        mutation_started = true;

        data->metadata_probes = std::move(candidate);
        data->edit_journal = std::move(candidate_journal);
        data->edit_journal_encoded_bytes = projected_journal_bytes;
        ++data->info.generation;
        const int refresh_code = rebuild_snapshot(data, cancellation, status);
        if (refresh_code != 0)
        {
            rollback();
            return refresh_code;
        }
        StepTopologyProbeTransactionResult published;
        published.session = data->info;
        const int group_code = publish_logical_groups(data, data->logical_groups, cancellation,
                                                      &published.groups, status);
        if (group_code != 0)
        {
            rollback();
            return group_code;
        }
        const int probe_code = publish_metadata_probes(data, data->metadata_probes, cancellation,
                                                       &published.probes, status);
        if (probe_code != 0)
        {
            rollback();
            return probe_code;
        }
        if (publication_gate != nullptr)
        {
            const int gate_code = publication_gate(published, publication_context, status);
            if (gate_code != 0)
            {
                rollback();
                return gate_code;
            }
        }
        publish_result(result, &published);
        mutation_started = false;
        set_status(status, 0, "");
        return 0;
    }
    catch (const Standard_Failure& failure)
    {
        rollback();
        clear_result(result);
        set_status(status, kInternalFailure, failure.GetMessageString());
        return kInternalFailure;
    }
    catch (const std::exception& error)
    {
        rollback();
        clear_result(result);
        set_status(status, kInternalFailure, error.what());
        return kInternalFailure;
    }
}

} // namespace geometer::step_topology_internal
