#pragma once

#include <IMeshData_Status.hxx>

namespace geometer::model_tessellation_detail
{
constexpr int successful_flags = IMeshData_ReMesh | IMeshData_Reused;
constexpr int face_problem_flags = IMeshData_OpenWire | IMeshData_SelfIntersectingWire |
                                   IMeshData_Failure | IMeshData_UnorientedWire |
                                   IMeshData_TooFewPoints | IMeshData_Outdated;

constexpr bool meshing_succeeded(bool done, int flags, bool allow_partial = false)
{
    // Partial output tolerates local face problems, never incomplete work,
    // cancellation or unknown statuses. Outdated faces are explicitly omitted
    // after a clean remesh retry, just like other locally failed faces.
    const int accepted = successful_flags | (allow_partial ? face_problem_flags : 0);
    return done && (flags & ~accepted) == 0;
}
} // namespace geometer::model_tessellation_detail
