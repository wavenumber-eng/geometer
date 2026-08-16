#pragma once

#include "geometer/analytic_filtered_packet.h"
#include "geometer/analytic_result_packet_records.h"

#include <cstdint>
#include <vector>

namespace geometer::analytic_packet_detail
{

[[nodiscard]] bool checked_add(std::uint64_t left, std::uint64_t right,
                               std::uint64_t& output) noexcept;
[[nodiscard]] bool checked_multiply(std::uint64_t left, std::uint64_t right,
                                    std::uint64_t& output) noexcept;
[[nodiscard]] std::uint64_t sort_units(std::uint64_t count) noexcept;

struct WorkBudget
{
    std::uint64_t limit = 0;
    std::uint64_t used = 0;
    AnalyticFilteredPacketTelemetry* telemetry = nullptr;

    [[nodiscard]] bool charge(std::uint64_t units) noexcept;
    [[nodiscard]] bool charge_sort(std::uint64_t count) noexcept;
};

struct SequenceRange
{
    std::uint32_t begin = 0;
    std::uint32_t count = 0;
};

struct CanonicalSequences
{
    std::vector<std::uint32_t> handles;
    std::vector<AnalyticPacketSourceSetRecord> records;
    std::vector<std::uint32_t> indices;
    std::uint64_t logical_bytes = 0;
};

// Interns fixed-width label sequences through a fixed-capacity exact
// transition table. Lexicographic ranks come from one sorted-child DFS, so
// long common prefixes are traversed once rather than inside sort comparators.
[[nodiscard]] bool canonicalize_sequences(const std::vector<std::uint32_t>& labels,
                                          const std::vector<SequenceRange>& ranges, bool serialize,
                                          std::uint64_t live_base_bytes, std::uint64_t memory_limit,
                                          WorkBudget& budget, CanonicalSequences& output);

} // namespace geometer::analytic_packet_detail
