#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace geometer
{

inline constexpr std::size_t kAnalyticResultTableCount = 14;

enum class AnalyticResultPacketLayoutError : std::uint8_t
{
    none = 0,
    invalid_packet = 1,
    limit_exceeded = 2,
};

struct AnalyticResultTableView
{
    std::uint16_t kind = 0;
    std::uint32_t record_bytes = 0;
    std::uint64_t offset = 0;
    std::uint64_t byte_length = 0;
    std::uint64_t record_count = 0;
};

struct AnalyticResultPacketLayout
{
    std::uint32_t job_result_count = 0;
    std::uint32_t relationship_result_count = 0;
    std::uint64_t canonical_payload_bytes = 0;
    std::array<AnalyticResultTableView, kAnalyticResultTableCount> tables{};
};

struct AnalyticResultPacketLayoutResult
{
    AnalyticResultPacketLayoutError error = AnalyticResultPacketLayoutError::none;
    std::optional<AnalyticResultPacketLayout> value;
};

struct AnalyticResultPacketEncodeResult
{
    AnalyticResultPacketLayoutError error = AnalyticResultPacketLayoutError::none;
    std::optional<std::vector<std::uint8_t>> value;
};

using AnalyticResultTableBytes = std::array<std::vector<std::uint8_t>, kAnalyticResultTableCount>;

[[nodiscard]] AnalyticResultPacketLayoutResult
decode_analytic_result_packet_layout(const std::uint8_t* data, std::size_t size);

[[nodiscard]] AnalyticResultPacketEncodeResult
encode_analytic_result_packet_layout(const AnalyticResultTableBytes& tables);

} // namespace geometer
