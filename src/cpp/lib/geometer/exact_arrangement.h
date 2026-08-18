#pragma once

#include "geometer/exact_curve_domain.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace geometer::exact
{

enum class ExactAtomicCurveKind : std::uint8_t
{
    line = 1,
    circular_arc = 2,
};

struct ExactCurveMembership
{
    std::uint64_t occurrence_id = 0;
    // True when the source occurrence follows the canonical carrier direction:
    // positive first nonzero tangent for a line, counterclockwise for a circle.
    bool agrees_with_carrier = true;
};

struct ExactCoverageOccurrence
{
    std::uint64_t occurrence_id = 0;
    std::uint64_t coverage_id = 0;
    bool material_on_left_of_occurrence = true;
};

struct ExactAtomicCurve
{
    ExactAtomicCurveKind kind = ExactAtomicCurveKind::line;
    ExactPoint start;
    ExactPoint end;
    ExactCircle circle;
    bool counterclockwise = true;
    bool major_arc = false;
    std::vector<ExactCurveMembership> memberships;
};

struct ExactArrangementVertex
{
    ExactPoint point;
    std::uint32_t outgoing_begin = 0;
    std::uint32_t outgoing_count = 0;
};

struct ExactArrangementEdge
{
    std::uint32_t start_vertex = 0;
    std::uint32_t end_vertex = 0;
    ExactAtomicCurveKind kind = ExactAtomicCurveKind::line;
    ExactCircle circle;
    bool counterclockwise = true;
    bool major_arc = false;
    std::uint32_t membership_begin = 0;
    std::uint32_t membership_count = 0;
};

struct ExactArrangementHalfEdge
{
    std::uint32_t origin_vertex = 0;
    std::uint32_t twin = 0;
    std::uint32_t next = 0;
    std::uint32_t previous = 0;
    std::uint32_t edge = 0;
    bool forward = true;
    std::uint32_t cycle = 0;
    std::uint32_t face = 0;
};

struct ExactArrangementCycle
{
    std::uint32_t half_edge_begin = 0;
    std::uint32_t half_edge_count = 0;
    std::uint32_t component = 0;
    std::uint32_t face = 0;
    bool counterclockwise = true;
};

struct ExactArrangementFace
{
    std::uint32_t boundary_cycle_begin = 0;
    std::uint32_t boundary_cycle_count = 0;
    std::uint32_t coverage_begin = 0;
    std::uint32_t coverage_count = 0;
    bool unbounded = false;
};

struct ExactArrangementResult;

class ExactArrangement
{
  public:
    ~ExactArrangement();
    ExactArrangement(ExactArrangement&& other) noexcept;
    ExactArrangement& operator=(ExactArrangement&& other) noexcept;
    ExactArrangement(const ExactArrangement&) = delete;
    ExactArrangement& operator=(const ExactArrangement&) = delete;

    [[nodiscard]] const std::vector<ExactArrangementVertex>& vertices() const;
    [[nodiscard]] const std::vector<ExactArrangementEdge>& edges() const;
    [[nodiscard]] const std::vector<ExactArrangementHalfEdge>& half_edges() const;
    [[nodiscard]] const std::vector<std::uint32_t>& outgoing_half_edges() const;
    [[nodiscard]] const std::vector<ExactCurveMembership>& memberships() const;
    [[nodiscard]] const std::vector<ExactArrangementCycle>& cycles() const;
    [[nodiscard]] const std::vector<std::uint32_t>& cycle_half_edges() const;
    [[nodiscard]] const std::vector<ExactArrangementFace>& faces() const;
    [[nodiscard]] const std::vector<std::uint32_t>& face_boundary_cycles() const;
    [[nodiscard]] const std::vector<std::uint64_t>& face_coverages() const;

  private:
    friend struct ExactArrangementResult;
    friend ExactArrangementResult build_exact_arrangement(ConstructionArena&,
                                                          const std::vector<ExactAtomicCurve>&);
    friend ExactArrangementResult
    build_exact_arrangement(ConstructionArena&, const std::vector<ExactAtomicCurve>&,
                            const std::vector<ExactCoverageOccurrence>&);
    ExactArrangement(
        Budget& budget, std::uint64_t charged_bytes, std::vector<ExactArrangementVertex> vertices,
        std::vector<ExactArrangementEdge> edges, std::vector<ExactArrangementHalfEdge> half_edges,
        std::vector<std::uint32_t> outgoing_half_edges,
        std::vector<ExactCurveMembership> memberships, std::vector<ExactArrangementCycle> cycles,
        std::vector<std::uint32_t> cycle_half_edges, std::vector<ExactArrangementFace> faces,
        std::vector<std::uint32_t> face_boundary_cycles,
        std::vector<std::uint64_t> face_coverages) noexcept;

    void release();

    Budget* budget_ = nullptr;
    std::uint64_t charged_bytes_ = 0;
    std::vector<ExactArrangementVertex> vertices_;
    std::vector<ExactArrangementEdge> edges_;
    std::vector<ExactArrangementHalfEdge> half_edges_;
    std::vector<std::uint32_t> outgoing_half_edges_;
    std::vector<ExactCurveMembership> memberships_;
    std::vector<ExactArrangementCycle> cycles_;
    std::vector<std::uint32_t> cycle_half_edges_;
    std::vector<ExactArrangementFace> faces_;
    std::vector<std::uint32_t> face_boundary_cycles_;
    std::vector<std::uint64_t> face_coverages_;
};

struct ExactArrangementResult
{
    Error error = Error::none;
    std::optional<ExactArrangement> value;
};

[[nodiscard]] ExactArrangementResult
build_exact_arrangement(ConstructionArena& arena, const std::vector<ExactAtomicCurve>& curves);

[[nodiscard]] ExactArrangementResult
build_exact_arrangement(ConstructionArena& arena, const std::vector<ExactAtomicCurve>& curves,
                        const std::vector<ExactCoverageOccurrence>& coverages);

} // namespace geometer::exact
