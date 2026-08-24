#pragma once

#include "geometer/step_topology_session.h"

#include <STEPCAFControl_Reader.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Shape.hxx>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace geometer::step_topology_internal
{

constexpr int kInvalidArgument = 101;
constexpr int kResourceLimit = 102;
constexpr int kReadFailed = 103;
constexpr int kTransferFailed = 104;
constexpr int kClosed = 105;
constexpr int kUnknownSession = 106;
constexpr int kUnknownTarget = 107;
constexpr int kInternalFailure = 108;
constexpr int kCancelled = 109;
constexpr int kConflict = 110;

struct HandleRecord
{
    StepTopologyTargetKind kind = StepTopologyTargetKind::definition;
    std::uint64_t generation = 0;
    TopoDS_Shape shape;
};

struct LogicalGroupMemberRecord
{
    StepTopologyTargetKind kind = StepTopologyTargetKind::face;
    TopoDS_Shape shape;
};

struct LogicalGroupRecord
{
    std::string authored_id;
    std::uint64_t revision = 0;
    std::string name;
    std::vector<LogicalGroupMemberRecord> members;
};

struct MetadataProbeRecord
{
    std::string authored_id;
    std::uint64_t revision = 0;
    StepTopologyProbeTargetKind target_kind = StepTopologyProbeTargetKind::document;
    std::size_t target_index = 0;
    std::string target_evidence_sha256;
    std::string group_authored_id;
    std::string key;
    std::string value;
};

struct EditJournalTargetRecord
{
    StepTopologyTargetKind kind = StepTopologyTargetKind::face;
    std::size_t target_index = 0;
    std::size_t definition_index = 0;
    std::string evidence_sha256;
};

struct EditJournalCommandRecord
{
    StepTopologyGroupCommandKind kind = StepTopologyGroupCommandKind::create;
    std::string authored_id;
    std::uint64_t expected_revision = 0;
    std::string name;
    std::vector<EditJournalTargetRecord> members;
};

struct EditJournalProbeCommandRecord
{
    StepTopologyProbeCommandKind kind = StepTopologyProbeCommandKind::attach;
    std::string authored_id;
    std::uint64_t expected_revision = 0;
    StepTopologyProbeTargetKind target_kind = StepTopologyProbeTargetKind::document;
    std::size_t target_index = 0;
    std::string target_evidence_sha256;
    std::string group_authored_id;
    std::string key;
    std::string value;
};

struct EditJournalTransactionRecord
{
    enum class Kind : std::uint8_t
    {
        logical_groups,
        metadata_probes,
    };

    std::uint64_t sequence = 0;
    Kind kind = Kind::logical_groups;
    std::vector<EditJournalCommandRecord> commands;
    std::vector<EditJournalProbeCommandRecord> probe_commands;
};

struct SessionData
{
    StepTopologyLimits limits;
    StepReaderPosture reader_posture;
    StepTopologySessionInfo info;
    std::vector<unsigned char> source;
    Handle(TDocStd_Document) document;
    STEPCAFControl_Reader reader;
    StepTopologySnapshot snapshot;
    std::unordered_map<std::string, HandleRecord> handles;
    std::vector<LogicalGroupRecord> logical_groups;
    std::vector<MetadataProbeRecord> metadata_probes;
    std::vector<EditJournalTransactionRecord> edit_journal;
    std::array<std::uint8_t, 32> secret{};
    std::uint64_t handle_counter = 0;
    std::size_t snapshot_string_bytes = 0;
    std::size_t metadata_probe_string_bytes = 0;
    std::size_t journal_string_bytes = 0;
    std::size_t total_string_bytes = 0;
    std::size_t edit_journal_encoded_bytes = 0;
    bool open = false;
};

void set_status(Status* status, int code, const std::string& message);
int import_step_session(SessionData* data, const StepTopologyCancellation* cancellation,
                        Status* status);
int rebuild_snapshot(SessionData* data, const StepTopologyCancellation* cancellation,
                     Status* status);
std::size_t estimated_resident_bytes(const SessionData& data);
int build_render_artifact(SessionData* data, const StepTopologyTessellationOptions& options,
                          const StepTopologyCancellation* cancellation, std::size_t byte_limit,
                          StepTopologyRenderArtifact* artifact, Status* status);
bool verify_render_artifact_seal(const SessionData* data,
                                 const StepTopologyRenderArtifact& artifact, Status* status);
int build_glb_work_packet(SessionData* data, const StepTopologyGlbOptions& options,
                          const StepTopologyCancellation* cancellation,
                          StepTopologyGlbWorkPacket* packet, Status* status);
bool verify_glb_work_packet_seal(const SessionData* data, const StepTopologyGlbWorkPacket& packet,
                                 Status* status);
using GlbEncodingEntryHook = void (*)(void* context);
struct GlbEncodingBudgetStats
{
    std::size_t pre_destination_resident_bytes = 0;
    std::size_t peak_source_resident_bytes = 0;
    std::size_t resident_bytes_before_rejected_allocation = 0;
    std::size_t rejected_allocation_bytes = 0;
};
int encode_glb_from_render_for_test(const StepTopologyRenderArtifact& render,
                                    const StepTopologyCancellation* cancellation,
                                    std::size_t wire_byte_limit, std::size_t transient_byte_limit,
                                    StepTopologyGlbWorkPacket* packet, Status* status,
                                    GlbEncodingEntryHook entry_hook, void* entry_hook_context,
                                    GlbEncodingBudgetStats* budget_stats = nullptr);
std::string issue_handle(SessionData* data, StepTopologyTargetKind kind, const TopoDS_Shape& shape);
bool account_string(SessionData* data, const std::string& value, Status* status);
int account_logical_group_strings(SessionData* data, const StepTopologyCancellation* cancellation,
                                  Status* status);
int account_metadata_probe_strings(SessionData* data, const StepTopologyCancellation* cancellation,
                                   Status* status);
int account_edit_journal_strings(SessionData* data, const StepTopologyCancellation* cancellation,
                                 Status* status);
int stage_edit_journal_transaction(const SessionData* data,
                                   const StepTopologyGroupTransaction& transaction,
                                   const StepTopologyCancellation* cancellation,
                                   EditJournalTransactionRecord* entry,
                                   std::size_t* entry_string_bytes, Status* status);
int stage_probe_journal_transaction(const SessionData* data,
                                    const StepTopologyProbeTransaction& transaction,
                                    const StepTopologyCancellation* cancellation,
                                    EditJournalTransactionRecord* entry,
                                    std::size_t* entry_string_bytes, Status* status);
int initialize_edit_journal_accounting(SessionData* data, Status* status);
int validate_edit_journal_append(const SessionData* data, const EditJournalTransactionRecord& entry,
                                 std::size_t* projected_bytes, Status* status);
std::string edit_journal_target_inventory_sha256(const StepTopologySnapshot& snapshot);
int restore_probe_transaction(const SessionData* data,
                              const EditJournalTransactionRecord& journal_entry,
                              const StepTopologyCancellation* cancellation,
                              StepTopologyProbeTransaction* transaction, Status* status);
int encode_edit_journal(const SessionData* data, StepTopologyEditJournalCheckpoint* checkpoint,
                        Status* status);
int decode_edit_journal(const unsigned char* bytes, std::size_t size,
                        const StepTopologyLimits& limits, std::string* source_sha256,
                        std::string* source_brep_sha256, std::string* target_inventory_sha256,
                        std::string* occt_version, const StepTopologyCancellation* cancellation,
                        std::vector<EditJournalTransactionRecord>* transactions, Status* status);
int replay_edit_journal(SessionData* data,
                        const std::vector<EditJournalTransactionRecord>& transactions,
                        const StepTopologyCancellation* cancellation,
                        StepTopologyEditJournalRestoreResult* result, Status* status);
using EditJournalReplayApplyEntryHook = void (*)(void* context);
void set_edit_journal_replay_apply_entry_hook_for_test(EditJournalReplayApplyEntryHook hook,
                                                       void* context);
int apply_metadata_probe_transaction(SessionData* data,
                                     const StepTopologyProbeTransaction& transaction,
                                     const StepTopologyCancellation* cancellation,
                                     StepTopologyProbePublicationGate publication_gate,
                                     void* publication_context,
                                     StepTopologyProbeTransactionResult* result, Status* status);
int publish_logical_groups(const SessionData* data, const std::vector<LogicalGroupRecord>& groups,
                           const StepTopologyCancellation* cancellation,
                           std::vector<StepTopologyLogicalGroup>* published, Status* status);
int publish_metadata_probes(const SessionData* data,
                            const std::vector<MetadataProbeRecord>& records,
                            const StepTopologyCancellation* cancellation,
                            std::vector<StepTopologyMetadataProbe>* probes, Status* status);
int apply_logical_group_transaction(SessionData* data,
                                    const StepTopologyGroupTransaction& transaction,
                                    const StepTopologyCancellation* cancellation,
                                    StepTopologyGroupPublicationGate publication_gate,
                                    void* publication_context,
                                    StepTopologyGroupTransactionResult* result, Status* status);

} // namespace geometer::step_topology_internal
