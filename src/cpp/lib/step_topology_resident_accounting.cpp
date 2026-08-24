#include "step_topology_session_internal.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace geometer::step_topology_internal
{
namespace
{

class ResidentByteEstimate
{
  public:
    void add(std::size_t bytes)
    {
        if (value_ > std::numeric_limits<std::size_t>::max() - bytes)
        {
            value_ = std::numeric_limits<std::size_t>::max();
            return;
        }
        value_ += bytes;
    }

    void add_product(std::size_t count, std::size_t element_size)
    {
        if (count != 0 && element_size > std::numeric_limits<std::size_t>::max() / count)
        {
            value_ = std::numeric_limits<std::size_t>::max();
            return;
        }
        add(count * element_size);
    }

    std::size_t value() const
    {
        return value_;
    }

  private:
    std::size_t value_ = 0;
};

void add_string_storage(ResidentByteEstimate* estimate, const std::string& value)
{
    // Counting capacity even for small-string storage deliberately overestimates retained heap.
    estimate->add(value.capacity());
    estimate->add(1U);
}

void add_string_vector_storage(ResidentByteEstimate* estimate,
                               const std::vector<std::string>& values)
{
    estimate->add_product(values.capacity(), sizeof(std::string));
    for (const std::string& value : values)
        add_string_storage(estimate, value);
}

} // namespace

std::size_t estimated_resident_bytes(const SessionData& data)
{
    const StepTopologySnapshot& snapshot = data.snapshot;
    ResidentByteEstimate estimate;
    estimate.add(sizeof(SessionData));
    estimate.add_product(data.source.capacity(), sizeof(unsigned char));
    add_string_storage(&estimate, data.info.session_handle);
    add_string_storage(&estimate, data.info.source_sha256);
    add_string_storage(&estimate, data.info.occt_version);
    add_string_storage(&estimate, snapshot.research_format);
    add_string_storage(&estimate, snapshot.brep_sha256);
    add_string_storage(&estimate, snapshot.session.session_handle);
    add_string_storage(&estimate, snapshot.session.source_sha256);
    add_string_storage(&estimate, snapshot.session.occt_version);
    estimate.add_product(data.logical_groups.capacity(), sizeof(LogicalGroupRecord));
    for (const LogicalGroupRecord& group : data.logical_groups)
    {
        add_string_storage(&estimate, group.authored_id);
        add_string_storage(&estimate, group.name);
        estimate.add_product(group.members.capacity(), sizeof(LogicalGroupMemberRecord));
    }
    estimate.add_product(data.metadata_probes.capacity(), sizeof(MetadataProbeRecord));
    for (const MetadataProbeRecord& probe : data.metadata_probes)
    {
        add_string_storage(&estimate, probe.authored_id);
        add_string_storage(&estimate, probe.target_evidence_sha256);
        add_string_storage(&estimate, probe.group_authored_id);
        add_string_storage(&estimate, probe.key);
        add_string_storage(&estimate, probe.value);
    }
    estimate.add_product(data.edit_journal.capacity(), sizeof(EditJournalTransactionRecord));
    for (const EditJournalTransactionRecord& transaction : data.edit_journal)
    {
        estimate.add_product(transaction.commands.capacity(), sizeof(EditJournalCommandRecord));
        for (const EditJournalCommandRecord& command : transaction.commands)
        {
            add_string_storage(&estimate, command.authored_id);
            add_string_storage(&estimate, command.name);
            estimate.add_product(command.members.capacity(), sizeof(EditJournalTargetRecord));
            for (const EditJournalTargetRecord& member : command.members)
                add_string_storage(&estimate, member.evidence_sha256);
        }
        estimate.add_product(transaction.probe_commands.capacity(),
                             sizeof(EditJournalProbeCommandRecord));
        for (const EditJournalProbeCommandRecord& command : transaction.probe_commands)
        {
            add_string_storage(&estimate, command.authored_id);
            add_string_storage(&estimate, command.target_evidence_sha256);
            add_string_storage(&estimate, command.group_authored_id);
            add_string_storage(&estimate, command.key);
            add_string_storage(&estimate, command.value);
        }
    }
    estimate.add_product(snapshot.definitions.capacity(), sizeof(StepTopologyDefinition));
    for (const StepTopologyDefinition& definition : snapshot.definitions)
    {
        add_string_storage(&estimate, definition.handle);
        add_string_storage(&estimate, definition.label.name);
        add_string_storage(&estimate, definition.source_entity.entity_type);
        add_string_storage(&estimate, definition.source_entity.mapping_method);
        add_string_vector_storage(&estimate, definition.body_handles);
    }
    estimate.add_product(snapshot.occurrences.capacity(), sizeof(StepTopologyOccurrence));
    for (const StepTopologyOccurrence& occurrence : snapshot.occurrences)
    {
        add_string_storage(&estimate, occurrence.handle);
        add_string_storage(&estimate, occurrence.definition_handle);
        add_string_storage(&estimate, occurrence.parent_occurrence_handle);
        add_string_storage(&estimate, occurrence.label.name);
    }
    estimate.add_product(snapshot.root_occurrences.capacity(), sizeof(StepTopologyRootOccurrence));
    for (const StepTopologyRootOccurrence& root : snapshot.root_occurrences)
    {
        add_string_storage(&estimate, root.handle);
        add_string_storage(&estimate, root.definition_handle);
        add_string_storage(&estimate, root.label.name);
    }
    estimate.add_product(snapshot.bodies.capacity(), sizeof(StepTopologyBody));
    for (const StepTopologyBody& body : snapshot.bodies)
    {
        add_string_storage(&estimate, body.handle);
        add_string_storage(&estimate, body.definition_handle);
        add_string_storage(&estimate, body.topology_kind);
        add_string_storage(&estimate, body.label.name);
        add_string_storage(&estimate, body.source_entity.entity_type);
        add_string_storage(&estimate, body.source_entity.mapping_method);
        add_string_vector_storage(&estimate, body.shell_handles);
        add_string_vector_storage(&estimate, body.face_handles);
    }
    estimate.add_product(snapshot.shells.capacity(), sizeof(StepTopologyShell));
    for (const StepTopologyShell& shell : snapshot.shells)
    {
        add_string_storage(&estimate, shell.handle);
        add_string_storage(&estimate, shell.definition_handle);
        add_string_storage(&estimate, shell.label.name);
        add_string_storage(&estimate, shell.source_entity.entity_type);
        add_string_storage(&estimate, shell.source_entity.mapping_method);
        add_string_vector_storage(&estimate, shell.body_handles);
        add_string_vector_storage(&estimate, shell.face_handles);
    }
    estimate.add_product(snapshot.faces.capacity(), sizeof(StepTopologyFace));
    for (const StepTopologyFace& face : snapshot.faces)
    {
        add_string_storage(&estimate, face.handle);
        add_string_storage(&estimate, face.definition_handle);
        add_string_storage(&estimate, face.label.name);
        add_string_storage(&estimate, face.source_entity.entity_type);
        add_string_storage(&estimate, face.source_entity.mapping_method);
        add_string_vector_storage(&estimate, face.body_handles);
        add_string_vector_storage(&estimate, face.shell_handles);
    }
    estimate.add_product(snapshot.diagnostic_carriers.capacity(),
                         sizeof(StepTopologyDiagnosticCarrier));
    for (const StepTopologyDiagnosticCarrier& diagnostic : snapshot.diagnostic_carriers)
    {
        add_string_storage(&estimate, diagnostic.target_handle);
        add_string_storage(&estimate, diagnostic.xcaf_label_entry);
    }
    estimate.add_product(data.handles.bucket_count(), sizeof(void*));
    estimate.add_product(data.handles.size(),
                         sizeof(std::pair<const std::string, HandleRecord>) + 2U * sizeof(void*));
    for (const auto& item : data.handles)
        add_string_storage(&estimate, item.first);
    // OCCT/XCAF allocation is intentionally not guessed here; hard containment belongs to the
    // worker process boundary. This estimate conservatively accounts all Geometer-owned storage.
    return estimate.value();
}

} // namespace geometer::step_topology_internal
