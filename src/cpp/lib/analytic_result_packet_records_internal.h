#pragma once

#include "geometer/analytic_result_packet_records.h"

namespace geometer::analytic_result_detail
{

[[nodiscard]] AnalyticResultPacketLayoutError
charge_logical_source_reference_expansions(const AnalyticResultPacketRecords& records,
                                           std::uint64_t& total) noexcept;

// Internal fast path for records constructed and governed by the owned
// filtered packet builder. External inputs must use the validating public
// encoder.
[[nodiscard]] AnalyticResultPacketEncodeResult
encode_canonical_records_unchecked(const AnalyticResultPacketRecords& records);

} // namespace geometer::analytic_result_detail
