#pragma once

#include "geometer/projection.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace geometer::fast_hlr_internal
{

constexpr std::uint32_t kNoTopologyVertex = std::numeric_limits<std::uint32_t>::max();

struct FragmentProvenance
{
    std::uint8_t category_mask = 0;
    std::uint32_t first_source_face = 0;
    std::uint32_t second_source_face = 0;
    std::uint32_t unique_edge = 0;

    bool operator<(const FragmentProvenance& other) const;
};

struct ProjectedFragment
{
    ProjectedSegment segment;
    std::uint32_t start_vertex = kNoTopologyVertex;
    std::uint32_t end_vertex = kNoTopologyVertex;
    FragmentProvenance provenance;
};

struct ReconstructionStatistics
{
    std::size_t joins = 0;
    std::size_t rejected = 0;
};

std::vector<ProjectedSegment>
reconstruct_collinear_fragments(const std::vector<ProjectedFragment>& fragments,
                                ReconstructionStatistics* statistics = nullptr);

} // namespace geometer::fast_hlr_internal
