#pragma once

#include "geometer/generated/contracts/contracts.h"
#include "geometer/status.h"

namespace geometer::model_illustration_detail
{
struct AnalyticLoweringStats
{
    std::uint32_t definitions = 0;
    std::uint32_t occurrences = 0;
    std::uint32_t primitives = 0;
    std::uint32_t triangles = 0;
};

int lower_analytic_scene(const contracts::AnalyticIllustrationSourceA0& source,
                         const std::optional<contracts::MeshIllustrationPrepareOptions>& prepare,
                         contracts::MeshCollectionA0* collection, AnalyticLoweringStats* statistics,
                         Status* status = nullptr);
} // namespace geometer::model_illustration_detail
