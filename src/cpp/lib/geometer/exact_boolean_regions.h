#pragma once

#include "geometer/exact_boolean_stages.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace geometer::exact
{

inline constexpr std::uint32_t kNoExactResultRing = std::numeric_limits<std::uint32_t>::max();

struct ExactResultRing
{
    std::uint32_t half_edge_begin = 0;
    std::uint32_t half_edge_count = 0;
    std::uint32_t parent_ring = kNoExactResultRing;
    std::uint32_t depth = 0;
    bool counterclockwise = true;
};

struct ExactResultRegion
{
    std::uint32_t outer_ring = 0;
    std::uint32_t positive_source_begin = 0;
    std::uint32_t positive_source_count = 0;
};

struct ExactResultAssociation
{
    std::uint64_t source_id = 0;
    std::uint32_t result_region = 0;
};

struct ExactBooleanRegionsResult;

class ExactBooleanRegions
{
  public:
    ~ExactBooleanRegions();
    ExactBooleanRegions(ExactBooleanRegions&& other) noexcept;
    ExactBooleanRegions& operator=(ExactBooleanRegions&& other) noexcept;
    ExactBooleanRegions(const ExactBooleanRegions&) = delete;
    ExactBooleanRegions& operator=(const ExactBooleanRegions&) = delete;

    [[nodiscard]] const std::vector<ExactResultRing>& rings() const;
    [[nodiscard]] const std::vector<std::uint32_t>& ring_half_edges() const;
    [[nodiscard]] const std::vector<ExactResultRegion>& regions() const;
    [[nodiscard]] const std::vector<std::uint64_t>& positive_sources() const;
    [[nodiscard]] const std::vector<ExactResultAssociation>& associations() const;

  private:
    friend struct ExactBooleanRegionsResult;
    friend ExactBooleanRegionsResult build_exact_boolean_regions(Budget&, const ExactArrangement&,
                                                                 const ExactBooleanSelection&);
    ExactBooleanRegions(Budget& budget, std::uint64_t charged_bytes,
                        std::vector<ExactResultRing> rings,
                        std::vector<std::uint32_t> ring_half_edges,
                        std::vector<ExactResultRegion> regions,
                        std::vector<std::uint64_t> positive_sources,
                        std::vector<ExactResultAssociation> associations) noexcept;
    void release();

    Budget* budget_ = nullptr;
    std::uint64_t charged_bytes_ = 0;
    std::vector<ExactResultRing> rings_;
    std::vector<std::uint32_t> ring_half_edges_;
    std::vector<ExactResultRegion> regions_;
    std::vector<std::uint64_t> positive_sources_;
    std::vector<ExactResultAssociation> associations_;
};

struct ExactBooleanRegionsResult
{
    Error error = Error::none;
    std::optional<ExactBooleanRegions> value;
};

[[nodiscard]] ExactBooleanRegionsResult
build_exact_boolean_regions(Budget& budget, const ExactArrangement& arrangement,
                            const ExactBooleanSelection& selection);

} // namespace geometer::exact
