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
[[nodiscard]] Ordering
compare_cycle_germs_at_minimum(ConstructionArena& arena, std::uint32_t outgoing_half_edge,
                               std::uint32_t reverse_incoming_half_edge,
                               const std::vector<ExactArrangementVertex>& vertices,
                               const std::vector<ExactArrangementEdge>& edges,
                               const std::vector<ExactArrangementHalfEdge>& half_edges);

[[nodiscard]] Error build_face_topology(ConstructionArena& arena,
                                        const std::vector<ExactArrangementVertex>& vertices,
                                        const std::vector<ExactArrangementEdge>& edges,
                                        std::vector<ExactArrangementHalfEdge>& half_edges,
                                        std::vector<ExactArrangementCycle>& cycles,
                                        std::vector<std::uint32_t>& cycle_half_edges,
                                        std::vector<ExactArrangementFace>& faces,
                                        std::vector<std::uint32_t>& face_boundary_cycles);

[[nodiscard]] Error
classify_face_coverages(const std::vector<ExactCoverageOccurrence>& coverage_occurrences,
                        const std::vector<ExactArrangementEdge>& edges,
                        const std::vector<ExactCurveMembership>& memberships,
                        const std::vector<ExactArrangementHalfEdge>& half_edges,
                        std::vector<ExactArrangementFace>& faces,
                        std::vector<std::uint64_t>& face_coverages);

} // namespace geometer::exact::arrangement_detail
