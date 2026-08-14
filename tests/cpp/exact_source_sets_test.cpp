#include "geometer/exact_source_sets.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

using namespace geometer::exact;

void require(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

std::string signature(const ExactSourceSetTable& table)
{
    std::ostringstream out;
    out << "s";
    for (const ExactSourceReference& source : table.source_references())
        out << static_cast<unsigned>(source.kind) << ',' << static_cast<unsigned>(source.role)
            << ',' << source.operand_id << ',' << source.primary_id << ',' << source.secondary_id
            << ';';
    out << "t";
    for (const ExactSourceSetRecord& set : table.source_sets())
        out << set.source_reference_index_begin << ',' << set.source_reference_index_count << ';';
    out << "i";
    for (std::uint32_t index : table.source_reference_indices())
        out << index << ',';
    out << "h";
    for (std::uint32_t handle : table.input_handles())
        out << handle << ',';
    return out.str();
}

void require_role_mapping()
{
    const std::uint64_t offset_key = std::uint64_t{2} << 32U;
    const std::uint64_t join_key = (std::uint64_t{2} << 32U) | 3U;
    const std::vector<ExactSourceReference> valid{
        {ExactSourceKind::authored_segment_curve, ExactSourceRole::authored_line, 1, 10, 20},
        {ExactSourceKind::authored_segment_curve, ExactSourceRole::authored_circular_arc, 1, 11,
         21},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::primitive_outer_circle, 2, 30, 0},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::primitive_inner_circle, 2, 30, 0},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::capsule_left_line, 2, 30, 0},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::capsule_end_cap, 2, 30, 0},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::capsule_right_line, 2, 30, 0},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::capsule_start_cap, 2, 30, 0},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::swept_left_offset_line, 2, 30,
         offset_key},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::swept_left_offset_arc, 2, 30,
         offset_key},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::swept_right_offset_line, 2, 30,
         offset_key},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::swept_right_offset_arc, 2, 30,
         offset_key},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::swept_round_join, 2, 30, join_key},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::swept_start_cap, 2, 30,
         std::uint64_t{1} << 32U},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::swept_end_cap, 2, 30, offset_key},
        {ExactSourceKind::subtractive_operand_effect, ExactSourceRole::none, 3, 40, 0},
    };
    Budget valid_budget({1'000'000, 1'000'000});
    ExactSourceSetTableResult accepted = build_exact_source_sets(valid_budget, {valid});
    require(accepted.error == Error::none && accepted.value,
            "complete governed source-role mapping was rejected");

    const std::vector<ExactSourceReference> invalid{
        {ExactSourceKind::authored_segment_curve, ExactSourceRole::authored_line, 1, 10, 0},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::primitive_outer_circle, 2, 30, 1},
        {ExactSourceKind::compact_feature_role, ExactSourceRole::swept_round_join, 2, 30,
         offset_key},
        {ExactSourceKind::subtractive_operand_effect, ExactSourceRole::none, 3, 40, 1},
    };
    for (const ExactSourceReference& source : invalid)
    {
        Budget invalid_budget({1'000'000, 1'000'000});
        ExactSourceSetTableResult rejected = build_exact_source_sets(invalid_budget, {{source}});
        require(rejected.error == Error::invalid_argument && !rejected.value,
                "invalid source-role/id mapping was accepted");
    }
}

} // namespace

int main()
{
    const ExactSourceReference a{ExactSourceKind::authored_segment_curve,
                                 ExactSourceRole::authored_line, 10, 101, 1001};
    const ExactSourceReference b{ExactSourceKind::authored_segment_curve,
                                 ExactSourceRole::authored_circular_arc, 20, 202, 2002};
    const ExactSourceReference c{ExactSourceKind::compact_feature_role,
                                 ExactSourceRole::primitive_outer_circle, 30, 303, 0};
    const ExactSourceReference d{ExactSourceKind::subtractive_operand_effect, ExactSourceRole::none,
                                 40, 404, 0};
    const std::vector<std::vector<ExactSourceReference>> inputs{
        {b, a, a}, {}, {c, a}, {a, b}, {d},
    };

    Budget budget({1'000'000, 1'000'000});
    ExactSourceSetTableResult result = build_exact_source_sets(budget, inputs);
    require(result.error == Error::none && result.value, "source-set interning failed");
    require(result.value->input_handles() == std::vector<std::uint32_t>({1, 0, 2, 1, 3}),
            "source-set handles are not canonical and interned");
    require(result.value->source_reference_indices() == std::vector<std::uint32_t>({0, 1, 0, 2, 3}),
            "source-reference indirection is not canonical");
    const BudgetUsage success = budget.usage();
    require(success.work_units == 5'760 && success.owned_bytes == 6'272,
            "source-set success budget changed");

    Budget short_work({success.work_units - 1, 1'000'000});
    ExactSourceSetTableResult work_failure = build_exact_source_sets(short_work, inputs);
    require(work_failure.error == Error::resource_limit_exceeded && !work_failure.value &&
                short_work.usage().owned_bytes == 0,
            "one-unit-short source-set work boundary retained storage");
    Budget short_storage({1'000'000, success.owned_bytes - 1});
    ExactSourceSetTableResult storage_failure = build_exact_source_sets(short_storage, inputs);
    require(storage_failure.error == Error::resource_limit_exceeded && !storage_failure.value &&
                short_storage.usage().owned_bytes == 0,
            "one-byte-short source-set storage boundary retained storage");

    require_role_mapping();
    std::cout << "EXACT_SOURCE_SETS_VECTOR=" << signature(*result.value) << '\n';
    std::cout << "EXACT_SOURCE_SETS_WORK=" << success.work_units << '\n';
    std::cout << "EXACT_SOURCE_SETS_STORAGE=" << success.owned_bytes << '\n';
    return 0;
}
