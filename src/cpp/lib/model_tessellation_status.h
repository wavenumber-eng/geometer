#pragma once

#include <IMeshData_Status.hxx>

namespace geometer::model_tessellation_detail
{
constexpr bool meshing_succeeded(bool done, int flags)
{
    // Refinement and reuse are successful outcomes. All problem, cancellation,
    // stale and unknown bits fail closed instead of returning partial geometry.
    constexpr int successful_flags = IMeshData_ReMesh | IMeshData_Reused;
    return done && (flags & ~successful_flags) == 0;
}
} // namespace geometer::model_tessellation_detail
