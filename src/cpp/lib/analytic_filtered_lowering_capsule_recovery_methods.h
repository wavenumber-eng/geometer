    bool prepare_capsule_recovery()
    {
        capsule_recovery_.assign(operand_end_ - operand_begin_, {});
        if (input_capsules_ == 0)
            return true;
        std::map<CapsuleBucketKey, std::vector<CapsuleRepresentative>> buckets;
        for (std::uint32_t stage_offset = 0; stage_offset < job_->stage_count; ++stage_offset)
        {
            const auto& stage = records_.stages[job_->stage_begin + stage_offset];
            if (stage.operation != 1)
                continue;
            for (std::uint32_t offset = 0; offset < stage.operand_count; ++offset)
            {
                const std::uint32_t operand_index = stage.operand_begin + offset;
                const auto& operand = records_.operands[operand_index];
                if (operand.geometry_kind != 4)
                    continue;
                if (!charge_work())
                    return false;
                const auto& capsule = records_.capsules[operand.geometry_index];
                const CanonicalCapsule canonical = canonical_capsule(capsule);
                const std::array<std::int64_t, 4> base{
                    recovery_bucket(canonical.endpoints[0]),
                    recovery_bucket(canonical.endpoints[1]),
                    recovery_bucket(canonical.endpoints[2]),
                    recovery_bucket(canonical.endpoints[3]),
                };
                const CapsuleRepresentative* selected = nullptr;
                std::uint64_t selected_squared = 0;
                for (std::int64_t a = -1; a <= 1; ++a)
                    for (std::int64_t b = -1; b <= 1; ++b)
                        for (std::int64_t c = -1; c <= 1; ++c)
                            for (std::int64_t d = -1; d <= 1; ++d)
                            {
                                if (!charge_work(ordered_index_work(buckets.size())))
                                    return false;
                                const CapsuleBucketKey key{capsule.width_nm, base[0] + a,
                                                           base[1] + b, base[2] + c, base[3] + d};
                                const auto found = buckets.find(key);
                                if (found == buckets.end())
                                    continue;
                                for (const CapsuleRepresentative& candidate : found->second)
                                {
                                    if (!charge_predicate())
                                        return false;
                                    std::uint64_t squared = 0;
                                    if (endpoint_adjustment(canonical, candidate.canonical,
                                                            squared) &&
                                        (selected == nullptr ||
                                         candidate.record_index < selected->record_index))
                                    {
                                        selected = &candidate;
                                        selected_squared = squared;
                                    }
                                }
                            }
                if (selected == nullptr)
                {
                    if (!charge_work(ordered_index_work(buckets.size()) + 1))
                        return false;
                    const CapsuleBucketKey key{capsule.width_nm, base[0], base[1], base[2],
                                               base[3]};
                    buckets[key].push_back({operand_index, operand.operand_id,
                                            operand.geometry_index, capsule.feature_id,
                                            canonical});
                    continue;
                }
                const std::uint64_t adjustment = ceil_square_root(selected_squared);
                capsule_recovery_[operand_index - operand_begin_] = {
                    selected->geometry_index, canonical.reversed != selected->canonical.reversed};
                if (adjustment != 0)
                {
                    out_.capsule_coalescences.push_back({stage.stage_id, operand.operand_id,
                                                         selected->operand_id, selected->feature_id,
                                                         adjustment});
                    ++telemetry_.capsule_coalescences;
                    telemetry_.maximum_capsule_adjustment_nm =
                        std::max(telemetry_.maximum_capsule_adjustment_nm, adjustment);
                }
            }
        }
        return true;
    }

    AnalyticRequestCapsuleRecord
    effective_capsule(const AnalyticRequestOperandRecord& operand) const noexcept
    {
        AnalyticRequestCapsuleRecord value = records_.capsules[operand.geometry_index];
        const std::uint32_t operand_index =
            static_cast<std::uint32_t>(&operand - records_.operands.data());
        if (operand_index < operand_begin_ || operand_index >= operand_end_)
            return value;
        const CapsuleRecoveryBinding& binding = capsule_recovery_[operand_index - operand_begin_];
        if (binding.geometry_index == std::numeric_limits<std::uint32_t>::max())
            return value;
        const std::uint64_t authored_feature_id = value.feature_id;
        value = records_.capsules[binding.geometry_index];
        value.feature_id = authored_feature_id;
        if (binding.reverse_representative)
        {
            std::swap(value.start_x_nm, value.end_x_nm);
            std::swap(value.start_y_nm, value.end_y_nm);
        }
        return value;
    }
