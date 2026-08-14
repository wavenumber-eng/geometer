#pragma once

#include "geometer/analytic_result_packet_records.h"

namespace geometer
{

// Replays each job's normalized line/arc boundary through the independent exact
// arrangement and checks winding, containment, and result-component claims.
// Call this on solver output before promotion; the structural packet codec stays
// allocation-bounded and does not invoke the geometry engine.
[[nodiscard]] AnalyticResultPacketLayoutError
validate_analytic_result_packet_topology(const AnalyticResultPacketRecords& records);

} // namespace geometer
