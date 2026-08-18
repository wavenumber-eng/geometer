#pragma once

#include "geometer/analytic_result_packet_records.h"

#include <optional>
#include <string>

namespace geometer
{

struct AnalyticStandaloneJob
{
    AnalyticResultPacketRecords records;
    std::vector<std::uint8_t> bytes;
    std::string digest_sha256;
};

struct AnalyticStandaloneJobResult
{
    AnalyticResultPacketLayoutError error = AnalyticResultPacketLayoutError::none;
    std::optional<AnalyticStandaloneJob> value;
};

[[nodiscard]] AnalyticStandaloneJobResult
build_analytic_standalone_job(const AnalyticResultPacketRecords& records, std::uint64_t job_id);

} // namespace geometer
