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
    std::array<std::uint8_t, 32> secret{};
    std::uint64_t handle_counter = 0;
    std::size_t total_string_bytes = 0;
    bool open = false;
};

void set_status(Status* status, int code, const std::string& message);
int import_step_session(SessionData* data, const StepTopologyCancellation* cancellation,
                        Status* status);
int rebuild_snapshot(SessionData* data, const StepTopologyCancellation* cancellation,
                     Status* status);
int build_render_artifact(SessionData* data, const StepTopologyTessellationOptions& options,
                          const StepTopologyCancellation* cancellation,
                          StepTopologyRenderArtifact* artifact, Status* status);
bool verify_render_artifact_seal(const SessionData* data,
                                 const StepTopologyRenderArtifact& artifact, Status* status);
int build_glb_work_packet(SessionData* data, const StepTopologyGlbOptions& options,
                          const StepTopologyCancellation* cancellation,
                          StepTopologyGlbWorkPacket* packet, Status* status);
bool verify_glb_work_packet_seal(const SessionData* data, const StepTopologyGlbWorkPacket& packet,
                                 Status* status);
using GlbEncodingEntryHook = void (*)(void* context);
int encode_glb_from_render_for_test(const StepTopologyRenderArtifact& render,
                                    const StepTopologyCancellation* cancellation,
                                    std::size_t byte_limit, StepTopologyGlbWorkPacket* packet,
                                    Status* status, GlbEncodingEntryHook entry_hook,
                                    void* entry_hook_context);
std::string issue_handle(SessionData* data, StepTopologyTargetKind kind, const TopoDS_Shape& shape);
bool account_string(SessionData* data, const std::string& value, Status* status);
int apply_logical_group_transaction(SessionData* data,
                                    const StepTopologyGroupTransaction& transaction,
                                    StepTopologyGroupTransactionResult* result, Status* status);

} // namespace geometer::step_topology_internal
