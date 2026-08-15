#pragma once

#include "geometer/analytic_request_packet.h"
#include "geometer/exact_arrangement.h"
#include "geometer/exact_boolean_provenance.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace geometer
{

// Job-local lowering failures map one-to-one to governed operation
// diagnostics in docs/contracts/analytic-planar-boolean-a0-catalog.toml
// [operation_diagnostic]. They fail the isolated job, never the batch.
enum class AnalyticOperandLoweringError : std::uint8_t
{
    none = 0,
    invalid_topology = 1,
    invalid_arc = 2,
    unsupported_geometry = 3,
    resource_limit_exceeded = 4,
};

struct AnalyticLoweredGeometry
{
    std::vector<exact::ExactAtomicCurve> curves;
    std::vector<exact::ExactCoverageOccurrence> coverages;
    std::vector<exact::ExactOccurrenceSource> occurrence_sources;
};

struct AnalyticLoweredGeometryResult
{
    AnalyticOperandLoweringError error = AnalyticOperandLoweringError::none;
    std::optional<AnalyticLoweredGeometry> value;
};

// Lowers every operand owned by one job of an already packet-validated
// record set into exact atomic boundary curves with coverage occurrences
// and canonical source references. Coverage ids are operand ids; occurrence
// ids are allocated densely from one in canonical owner-traversal order.
// Authored ring winding is accepted in either direction and resolved with
// exact predicates; the emitted material-side flags always describe the
// operand interior. Capsules lower directly to two exact offset lines and
// two semicircular caps; swept paths resolve overlap among their per-vertex
// disks and per-segment offset strips through a job-local exact
// pre-arrangement whose union boundary is re-emitted, so both keep the
// exact rational width/2 inflation. Any violated job-local geometric rule
// (degenerate or incoherent segments, cusps, self-intersecting centerlines,
// coordinate span, occurrence budget) fails this job only.
[[nodiscard]] AnalyticLoweredGeometryResult
lower_analytic_job_geometry(exact::ConstructionArena& arena,
                            const AnalyticRequestPacketRecords& records, std::uint32_t job_index);

} // namespace geometer
