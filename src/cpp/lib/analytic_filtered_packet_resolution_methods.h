bool publish_resolution_diagnostics()
{
    if (!budget_.charge(geometry_.capsule_coalescences.size()))
        return fail_resource();
    records_out_.diagnostics.reserve(geometry_.capsule_coalescences.size());
    const std::uint64_t job_id = records_.jobs[job_index_].job_id;
    for (const AnalyticCapsuleCoalescence& recovery : geometry_.capsule_coalescences)
    {
        const OperandInfo* operand = find_operand(operands_, recovery.operand_id);
        const OperandInfo* representative =
            find_operand(operands_, recovery.representative_operand_id);
        if (recovery.stage_id == 0 || recovery.representative_feature_id == 0 ||
            recovery.maximum_adjustment_nm == 0 ||
            recovery.maximum_adjustment_nm > kAnalyticCapsuleCoalescenceEnvelopeNm ||
            operand == nullptr || operand->stage_id != recovery.stage_id ||
            operand->operation != 1 || operand->geometry_kind != 4 || representative == nullptr ||
            representative->operation != 1 || representative->geometry_kind != 4 ||
            representative->geometry_index >= records_.capsules.size() ||
            records_.capsules[representative->geometry_index].feature_id !=
                recovery.representative_feature_id)
            return false;
        bool representative_precedes_in_job = false;
        for (std::uint32_t stage_offset = 0; stage_offset < records_.jobs[job_index_].stage_count;
             ++stage_offset)
        {
            const auto& stage =
                records_.stages[records_.jobs[job_index_].stage_begin + stage_offset];
            for (std::uint32_t offset = 0; offset < stage.operand_count; ++offset)
            {
                const auto& candidate = records_.operands[stage.operand_begin + offset];
                if (candidate.operand_id == recovery.representative_operand_id)
                    representative_precedes_in_job = true;
                if (candidate.operand_id == recovery.operand_id)
                    break;
            }
            if (stage.stage_id == recovery.stage_id)
                break;
        }
        if (!representative_precedes_in_job)
            return false;
        records_out_.diagnostics.push_back({65'548, 2, 15, job_id, recovery.stage_id,
                                            recovery.operand_id, recovery.representative_feature_id,
                                            8});
        ++result_.telemetry.capsule_coalescences;
        result_.telemetry.maximum_capsule_adjustment_nm = std::max(
            result_.telemetry.maximum_capsule_adjustment_nm, recovery.maximum_adjustment_nm);
    }
    std::sort(records_out_.diagnostics.begin(), records_out_.diagnostics.end(),
              [](const AnalyticDiagnosticRecord& left, const AnalyticDiagnosticRecord& right)
              {
                  return std::tuple{left.job_id,
                                    left.severity,
                                    left.code,
                                    left.presence_flags,
                                    left.stage_id,
                                    left.operand_id,
                                    left.geometry_source_id,
                                    left.path_token} < std::tuple{right.job_id,
                                                                  right.severity,
                                                                  right.code,
                                                                  right.presence_flags,
                                                                  right.stage_id,
                                                                  right.operand_id,
                                                                  right.geometry_source_id,
                                                                  right.path_token};
              });
    return std::adjacent_find(
               records_out_.diagnostics.begin(), records_out_.diagnostics.end(),
               [](const AnalyticDiagnosticRecord& left, const AnalyticDiagnosticRecord& right)
               {
                   return std::tie(left.job_id, left.severity, left.code, left.presence_flags,
                                   left.stage_id, left.operand_id, left.geometry_source_id,
                                   left.path_token) ==
                          std::tie(right.job_id, right.severity, right.code, right.presence_flags,
                                   right.stage_id, right.operand_id, right.geometry_source_id,
                                   right.path_token);
               }) == records_out_.diagnostics.end();
}
