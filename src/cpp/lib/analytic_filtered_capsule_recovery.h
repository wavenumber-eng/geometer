#pragma once

// Internal same-width capsule recovery records and bounded-distance helpers.
struct CanonicalCapsule
{
    std::array<std::int64_t, 4> endpoints{};
    bool reversed = false;
};

struct CapsuleRepresentative
{
    std::uint32_t record_index = 0;
    std::uint64_t operand_id = 0;
    std::uint32_t geometry_index = 0;
    std::uint64_t feature_id = 0;
    CanonicalCapsule canonical;
};

struct CapsuleRecoveryBinding
{
    std::uint32_t geometry_index = std::numeric_limits<std::uint32_t>::max();
    bool reverse_representative = false;
};

using CapsuleBucketKey =
    std::tuple<std::uint64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t>;

CanonicalCapsule canonical_capsule(const AnalyticRequestCapsuleRecord& value) noexcept
{
    CanonicalCapsule result{{value.start_x_nm, value.start_y_nm, value.end_x_nm, value.end_y_nm},
                            false};
    if (std::tie(value.end_x_nm, value.end_y_nm) < std::tie(value.start_x_nm, value.start_y_nm))
    {
        result.endpoints = {value.end_x_nm, value.end_y_nm, value.start_x_nm, value.start_y_nm};
        result.reversed = true;
    }
    return result;
}

std::int64_t recovery_bucket(std::int64_t value) noexcept
{
    const std::int64_t divisor = static_cast<std::int64_t>(kAnalyticCapsuleCoalescenceEnvelopeNm);
    std::int64_t quotient = value / divisor;
    if (value < 0 && value % divisor != 0)
        --quotient;
    return quotient;
}

std::uint64_t absolute_difference(std::int64_t left, std::int64_t right) noexcept
{
    return left >= right ? static_cast<std::uint64_t>(left) - static_cast<std::uint64_t>(right)
                         : static_cast<std::uint64_t>(right) - static_cast<std::uint64_t>(left);
}

bool endpoint_adjustment(const CanonicalCapsule& left, const CanonicalCapsule& right,
                         std::uint64_t& maximum_squared) noexcept
{
    maximum_squared = 0;
    for (std::size_t endpoint = 0; endpoint < 2; ++endpoint)
    {
        const std::uint64_t dx =
            absolute_difference(left.endpoints[endpoint * 2], right.endpoints[endpoint * 2]);
        const std::uint64_t dy = absolute_difference(left.endpoints[endpoint * 2 + 1],
                                                     right.endpoints[endpoint * 2 + 1]);
        if (dx > kAnalyticCapsuleCoalescenceEnvelopeNm ||
            dy > kAnalyticCapsuleCoalescenceEnvelopeNm)
            return false;
        const std::uint64_t squared = dx * dx + dy * dy;
        if (squared > kAnalyticCapsuleCoalescenceEnvelopeNm * kAnalyticCapsuleCoalescenceEnvelopeNm)
            return false;
        maximum_squared = std::max(maximum_squared, squared);
    }
    return true;
}

std::uint64_t ceil_square_root(std::uint64_t squared) noexcept
{
    std::uint64_t low = 0;
    std::uint64_t high = kAnalyticCapsuleCoalescenceEnvelopeNm;
    while (low < high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        if (middle * middle >= squared)
            high = middle;
        else
            low = middle + 1;
    }
    return low;
}

std::uint64_t ordered_index_work(std::size_t count) noexcept
{
    std::uint64_t units = 1;
    while (count > 1)
    {
        count = (count + 1) / 2;
        ++units;
    }
    return units;
}
