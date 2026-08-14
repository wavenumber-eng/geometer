#include "geometer/exact_arrangement.h"

#include <utility>

namespace geometer::exact
{

ExactArrangement::ExactArrangement(
    Budget& budget, std::uint64_t charged_bytes, std::vector<ExactArrangementVertex> vertices,
    std::vector<ExactArrangementEdge> edges, std::vector<ExactArrangementHalfEdge> half_edges,
    std::vector<std::uint32_t> outgoing_half_edges, std::vector<ExactCurveMembership> memberships,
    std::vector<ExactArrangementCycle> cycles, std::vector<std::uint32_t> cycle_half_edges,
    std::vector<ExactArrangementFace> faces, std::vector<std::uint32_t> face_boundary_cycles,
    std::vector<std::uint64_t> face_coverages) noexcept
    : budget_(&budget), charged_bytes_(charged_bytes), vertices_(std::move(vertices)),
      edges_(std::move(edges)), half_edges_(std::move(half_edges)),
      outgoing_half_edges_(std::move(outgoing_half_edges)), memberships_(std::move(memberships)),
      cycles_(std::move(cycles)), cycle_half_edges_(std::move(cycle_half_edges)),
      faces_(std::move(faces)), face_boundary_cycles_(std::move(face_boundary_cycles)),
      face_coverages_(std::move(face_coverages))
{
}

ExactArrangement::~ExactArrangement()
{
    release();
}

ExactArrangement::ExactArrangement(ExactArrangement&& other) noexcept
    : budget_(std::exchange(other.budget_, nullptr)),
      charged_bytes_(std::exchange(other.charged_bytes_, 0)), vertices_(std::move(other.vertices_)),
      edges_(std::move(other.edges_)), half_edges_(std::move(other.half_edges_)),
      outgoing_half_edges_(std::move(other.outgoing_half_edges_)),
      memberships_(std::move(other.memberships_)), cycles_(std::move(other.cycles_)),
      cycle_half_edges_(std::move(other.cycle_half_edges_)), faces_(std::move(other.faces_)),
      face_boundary_cycles_(std::move(other.face_boundary_cycles_)),
      face_coverages_(std::move(other.face_coverages_))
{
}

ExactArrangement& ExactArrangement::operator=(ExactArrangement&& other) noexcept
{
    if (this != &other)
    {
        release();
        budget_ = std::exchange(other.budget_, nullptr);
        charged_bytes_ = std::exchange(other.charged_bytes_, 0);
        vertices_ = std::move(other.vertices_);
        edges_ = std::move(other.edges_);
        half_edges_ = std::move(other.half_edges_);
        outgoing_half_edges_ = std::move(other.outgoing_half_edges_);
        memberships_ = std::move(other.memberships_);
        cycles_ = std::move(other.cycles_);
        cycle_half_edges_ = std::move(other.cycle_half_edges_);
        faces_ = std::move(other.faces_);
        face_boundary_cycles_ = std::move(other.face_boundary_cycles_);
        face_coverages_ = std::move(other.face_coverages_);
    }
    return *this;
}

void ExactArrangement::release()
{
    if (budget_ != nullptr)
        budget_->release_storage(charged_bytes_);
    budget_ = nullptr;
    charged_bytes_ = 0;
}

const std::vector<ExactArrangementVertex>& ExactArrangement::vertices() const
{
    return vertices_;
}
const std::vector<ExactArrangementEdge>& ExactArrangement::edges() const
{
    return edges_;
}
const std::vector<ExactArrangementHalfEdge>& ExactArrangement::half_edges() const
{
    return half_edges_;
}
const std::vector<std::uint32_t>& ExactArrangement::outgoing_half_edges() const
{
    return outgoing_half_edges_;
}
const std::vector<ExactCurveMembership>& ExactArrangement::memberships() const
{
    return memberships_;
}
const std::vector<ExactArrangementCycle>& ExactArrangement::cycles() const
{
    return cycles_;
}
const std::vector<std::uint32_t>& ExactArrangement::cycle_half_edges() const
{
    return cycle_half_edges_;
}
const std::vector<ExactArrangementFace>& ExactArrangement::faces() const
{
    return faces_;
}
const std::vector<std::uint32_t>& ExactArrangement::face_boundary_cycles() const
{
    return face_boundary_cycles_;
}
const std::vector<std::uint64_t>& ExactArrangement::face_coverages() const
{
    return face_coverages_;
}

} // namespace geometer::exact
