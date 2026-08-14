#pragma once

#include "geometer/exact_arrangement.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace geometer::exact::arrangement_detail
{

struct Ordering
{
    Error error = Error::none;
    std::optional<std::int8_t> value;
};

struct EdgeDraft
{
    std::uint32_t start_vertex = 0;
    std::uint32_t end_vertex = 0;
    ExactAtomicCurveKind kind = ExactAtomicCurveKind::line;
    ExactCircle circle;
    bool counterclockwise = true;
    bool major_arc = false;
    std::vector<ExactCurveMembership> memberships;
};

[[nodiscard]] Ordering compare_points(ConstructionArena& arena, const ExactPoint& left,
                                      const ExactPoint& right);
[[nodiscard]] Error validate_curve(ConstructionArena& arena, const ExactAtomicCurve& curve);
[[nodiscard]] Error validate_atomic_pairs(ConstructionArena& arena,
                                          const std::vector<ExactAtomicCurve>& curves);
[[nodiscard]] Ordering compare_edges(ConstructionArena& arena, const EdgeDraft& left,
                                     const EdgeDraft& right);
[[nodiscard]] Ordering compare_outgoing(ConstructionArena& arena, std::uint32_t left_half_edge,
                                        std::uint32_t right_half_edge,
                                        const std::vector<ExactArrangementVertex>& vertices,
                                        const std::vector<ExactArrangementEdge>& edges,
                                        const std::vector<ExactArrangementHalfEdge>& half_edges);

} // namespace geometer::exact::arrangement_detail
