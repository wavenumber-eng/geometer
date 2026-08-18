#pragma once

#include "geometer/exact_boolean_regions.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace geometer::exact
{

enum class ExactResultNormalizationError : std::uint8_t
{
    none = 0,
    invalid_argument = 1,
    resource_limit_exceeded = 2,
    normalization_error_exceeded = 3,
    normalization_topology_collapse = 4,
};

enum class NormalizedFragmentDirection : std::uint8_t
{
    not_applicable = 0,
    counterclockwise = 1,
    clockwise = 2,
};

struct ExactNormalizedResultVertex
{
    std::int64_t x_nm = 0;
    std::int64_t y_nm = 0;
    std::uint32_t arrangement_vertex = 0;
};

struct ExactNormalizedResultFragment
{
    std::uint32_t start_vertex = 0;
    std::uint32_t end_vertex = 0;
    ExactAtomicCurveKind kind = ExactAtomicCurveKind::line;
    NormalizedFragmentDirection direction = NormalizedFragmentDirection::not_applicable;
    bool major_arc = false;
    std::uint64_t radius_nm = 0;
    std::uint32_t arrangement_half_edge = 0;
};

struct ExactNormalizedResultRing
{
    std::uint32_t fragment_begin = 0;
    std::uint32_t fragment_count = 0;
    std::uint32_t parent_ring = kNoExactResultRing;
    std::uint32_t depth = 0;
    bool counterclockwise = true;
    std::uint32_t exact_ring = 0;
};

struct ExactNormalizedResultRegion
{
    std::uint32_t outer_ring = 0;
    std::uint32_t exact_region = 0;
};

struct ExactNormalizedBooleanResultResult;

class ExactNormalizedBooleanResult
{
  public:
    ~ExactNormalizedBooleanResult();
    ExactNormalizedBooleanResult(ExactNormalizedBooleanResult&& other) noexcept;
    ExactNormalizedBooleanResult& operator=(ExactNormalizedBooleanResult&& other) noexcept;
    ExactNormalizedBooleanResult(const ExactNormalizedBooleanResult&) = delete;
    ExactNormalizedBooleanResult& operator=(const ExactNormalizedBooleanResult&) = delete;

    [[nodiscard]] const std::vector<ExactNormalizedResultVertex>& vertices() const;
    [[nodiscard]] const std::vector<ExactNormalizedResultFragment>& fragments() const;
    [[nodiscard]] const std::vector<ExactNormalizedResultRing>& rings() const;
    [[nodiscard]] const std::vector<ExactNormalizedResultRegion>& regions() const;

  private:
    friend struct ExactNormalizedBooleanResultResult;
    friend ExactNormalizedBooleanResultResult
    normalize_exact_boolean_result(ConstructionArena&, const ExactArrangement&,
                                   const ExactBooleanSelection&, const ExactBooleanRegions&);
    ExactNormalizedBooleanResult(Budget& budget, std::uint64_t charged_bytes,
                                 std::vector<ExactNormalizedResultVertex> vertices,
                                 std::vector<ExactNormalizedResultFragment> fragments,
                                 std::vector<ExactNormalizedResultRing> rings,
                                 std::vector<ExactNormalizedResultRegion> regions) noexcept;
    void release();

    Budget* budget_ = nullptr;
    std::uint64_t charged_bytes_ = 0;
    std::vector<ExactNormalizedResultVertex> vertices_;
    std::vector<ExactNormalizedResultFragment> fragments_;
    std::vector<ExactNormalizedResultRing> rings_;
    std::vector<ExactNormalizedResultRegion> regions_;
};

struct ExactNormalizedBooleanResultResult
{
    ExactResultNormalizationError error = ExactResultNormalizationError::none;
    std::optional<ExactNormalizedBooleanResult> value;
};

[[nodiscard]] ExactNormalizedBooleanResultResult
normalize_exact_boolean_result(ConstructionArena& arena, const ExactArrangement& arrangement,
                               const ExactBooleanSelection& selection,
                               const ExactBooleanRegions& regions);

} // namespace geometer::exact
