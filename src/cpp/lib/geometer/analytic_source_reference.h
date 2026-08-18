#pragma once

#include <cstdint>

namespace geometer
{

// Production-neutral A0 source identity. The filtered solver is the primary
// authority; the algebraic implementation aliases these types as a test
// oracle so result packets do not depend on the exact namespace.
enum class AnalyticSourceKind : std::uint16_t
{
    authored_segment_curve = 1,
    compact_feature_role = 2,
    subtractive_operand_effect = 3,
};

enum class AnalyticSourceRole : std::uint16_t
{
    none = 0,
    authored_line = 1,
    authored_circular_arc = 2,
    primitive_outer_circle = 16,
    primitive_inner_circle = 17,
    capsule_left_line = 32,
    capsule_end_cap = 33,
    capsule_right_line = 34,
    capsule_start_cap = 35,
    swept_left_offset_line = 48,
    swept_left_offset_arc = 49,
    swept_right_offset_line = 50,
    swept_right_offset_arc = 51,
    swept_round_join = 52,
    swept_start_cap = 53,
    swept_end_cap = 54,
};

struct AnalyticSourceReference
{
    AnalyticSourceKind kind = AnalyticSourceKind::authored_segment_curve;
    AnalyticSourceRole role = AnalyticSourceRole::none;
    std::uint64_t operand_id = 0;
    std::uint64_t primary_id = 0;
    std::uint64_t secondary_id = 0;
};

} // namespace geometer
