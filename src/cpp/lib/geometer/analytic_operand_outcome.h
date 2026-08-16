#pragma once

#include <cstdint>

namespace geometer
{

// Shared governed A0 outcome values. The algebraic exact pipeline remains a
// test oracle and aliases this production-neutral enum; it is not the primary
// execution authority.
enum class AnalyticOperandOutcomeKind : std::uint16_t
{
    contributes_final_material = 1,
    redundant_or_absorbed_coverage = 2,
    partially_removed_later = 3,
    completely_removed_later = 4,
    subtraction_effect_survives = 5,
    subtraction_effect_overwritten_later = 6,
    no_effect = 7,
};

} // namespace geometer
