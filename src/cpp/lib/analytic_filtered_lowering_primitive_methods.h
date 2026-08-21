// Internal compact-feature and swept-path methods included in FilteredJobLowerer.

bool lower_capsule(const AnalyticRequestOperandRecord& operand)
{
    const auto& capsule = records_.capsules[operand.geometry_index];
    AnalyticIntegerPointNm start;
    AnalyticIntegerPointNm end;
    if (!local_point(capsule.start_x_nm, capsule.start_y_nm, start) ||
        !local_point(capsule.end_x_nm, capsule.end_y_nm, end))
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    if (start.x == end.x && start.y == end.y)
        return fail(AnalyticFilteredLoweringError::invalid_topology);
    const std::int64_t dx = end.x - start.x;
    const std::int64_t dy = end.y - start.y;
    ++telemetry_.square_root_calls;
    const Interval length = square_root(
        add(square(exact(static_cast<double>(dx))), square(exact(static_cast<double>(dy)))));
    if (!valid(length) || length.lower <= 0.0)
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    const Interval half_width = exact(static_cast<double>(capsule.width_nm) * 0.5);
    const Interval factor = divide(half_width, length);
    const Point offset{multiply(exact(static_cast<double>(-dy)), factor),
                       multiply(exact(static_cast<double>(dx)), factor)};
    const Point start_point = point(start);
    const Point end_point = point(end);
    const Point start_left = add(start_point, offset);
    const Point start_right = subtract(start_point, offset);
    const Point end_left = add(end_point, offset);
    const Point end_right = subtract(end_point, offset);
    const LineFamilyKey line_family = canonical_direction(dx, dy);
    const WideInteger width_squared = wide_multiply(static_cast<std::int64_t>(capsule.width_nm),
                                                    static_cast<std::int64_t>(capsule.width_nm));
    const bool forward_agrees = dx != 0 ? dx > 0 : dy > 0;
    const WideInteger base = wide_subtract(wide_multiply(line_family.dx, start.y),
                                           wide_multiply(line_family.dy, start.x));
    const WideInteger base_times_two = wide_add(base, base);
    const WideInteger primitive_squared = wide_add(wide_multiply(line_family.dx, line_family.dx),
                                                   wide_multiply(line_family.dy, line_family.dy));
    std::uint64_t primitive_length = 0;
    const double primitive_approximation =
        std::hypot(static_cast<double>(line_family.dx), static_cast<double>(line_family.dy));
    const bool integral_primitive_length = integer_square_root(
        primitive_squared, primitive_approximation,
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()), primitive_length);
    if (error_ != AnalyticFilteredLoweringError::none)
        return false;
    const auto line_descriptor = [&](bool original_left)
    {
        const bool canonical_left = original_left == forward_agrees;
        const std::int64_t sign = canonical_left ? 1 : -1;
        TokenDescriptor descriptor;
        descriptor.line.family = line_family;
        if (integral_primitive_length)
        {
            const WideInteger shift = wide_multiply(static_cast<std::int64_t>(capsule.width_nm),
                                                    static_cast<std::int64_t>(primitive_length));
            descriptor.line.rational_part_times_two =
                sign > 0 ? wide_add(base_times_two, shift) : wide_subtract(base_times_two, shift);
        }
        else
        {
            descriptor.line.rational_part_times_two = base_times_two;
            descriptor.line.radical_coefficient =
                sign * static_cast<std::int64_t>(capsule.width_nm);
        }
        return descriptor;
    };
    const auto source = [&](AnalyticFilteredSourceRole role)
    {
        return AnalyticFilteredSourceReference{AnalyticFilteredSourceKind::compact_feature_role,
                                               role, operand.operand_id, capsule.feature_id, 0};
    };
    const auto line = [&](Point first, Point second)
    {
        AnalyticAtomicCurveNm curve;
        curve.start = public_point(first);
        curve.end = public_point(second);
        return curve;
    };
    const auto cap = [&](Point first, Point second, AnalyticIntegerPointNm center)
    {
        AnalyticAtomicCurveNm curve;
        curve.kind = AnalyticAtomicCurveKind::circular_arc;
        curve.start = public_point(first);
        curve.end = public_point(second);
        curve.circle.center = public_point(point(center));
        curve.circle.radius = public_interval(half_width);
        curve.counterclockwise = true;
        curve.has_arc_sweep_certificate = true;
        return curve;
    };
    const std::uint32_t curve_begin = static_cast<std::uint32_t>(out_.curves.size());
    if (!emit(line(start_right, end_right), forward_agrees, true, operand.operand_id,
              source(AnalyticFilteredSourceRole::capsule_right_line), line_descriptor(false)) ||
        !emit(cap(end_right, end_left, end), true, true, operand.operand_id,
              source(AnalyticFilteredSourceRole::capsule_end_cap),
              circle_descriptor(end, width_squared)) ||
        !emit(line(end_left, start_left), !forward_agrees, true, operand.operand_id,
              source(AnalyticFilteredSourceRole::capsule_left_line), line_descriptor(true)) ||
        !emit(cap(start_left, start_right, start), true, true, operand.operand_id,
              source(AnalyticFilteredSourceRole::capsule_start_cap),
              circle_descriptor(start, width_squared)))
        return false;
    if (dy == 0)
        horizontal_mirrors_.push_back({curve_begin, curve_begin + 2U, start.y});
    endpoint_tangencies_.push_back({curve_begin, curve_begin + 1U, false, true});
    endpoint_tangencies_.push_back({curve_begin + 2U, curve_begin + 1U, true, false});
    endpoint_tangencies_.push_back({curve_begin + 2U, curve_begin + 3U, false, true});
    endpoint_tangencies_.push_back({curve_begin, curve_begin + 3U, true, false});
    return true;
}

bool lower_swept_path(const AnalyticRequestOperandRecord& operand)
{
    const std::uint64_t prior_projected_curves = projected_curves_;
    AnalyticSolverLimits swept_limits = limits_;
    if (telemetry_.work_units > swept_limits.predicate_calls)
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    swept_limits.predicate_calls -= telemetry_.work_units;
    if (prior_projected_curves > std::numeric_limits<std::uint64_t>::max() / kLogicalBytesPerCurve)
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    const std::uint64_t retained =
        std::max(kRetainedGeometryFixedBytes, prior_projected_curves * kLogicalBytesPerCurve);
    if (retained > swept_limits.working_memory_bytes)
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    swept_limits.working_memory_bytes -= retained;
    SweptPathLoweringResult swept = lower_filtered_swept_path(records_, operand, out_.origin_x_nm,
                                                              out_.origin_y_nm, swept_limits);
    telemetry_.work_units += swept.telemetry.work_units;
    telemetry_.fixed_width_predicates += swept.telemetry.fixed_width_predicates;
    telemetry_.square_root_calls += swept.telemetry.square_root_calls;
    telemetry_.algebraic_fallback_calls += swept.telemetry.algebraic_fallback_calls;
    if (swept.telemetry.peak_working_memory_bytes >
        std::numeric_limits<std::uint64_t>::max() - retained)
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    telemetry_.peak_working_memory_bytes = std::max(
        telemetry_.peak_working_memory_bytes, retained + swept.telemetry.peak_working_memory_bytes);
    if (swept.telemetry.required_working_memory_bytes != 0)
    {
        if (swept.telemetry.required_working_memory_bytes >
            std::numeric_limits<std::uint64_t>::max() - retained)
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        telemetry_.required_working_memory_bytes =
            std::max(telemetry_.required_working_memory_bytes,
                     retained + swept.telemetry.required_working_memory_bytes);
    }
    if (swept.error != AnalyticFilteredLoweringError::none)
        return fail(swept.error);
    if (swept.curves.size() > limits_.boundary_occurrences - projected_curves_)
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    if (!add_projected(swept.curves.size()))
        return false;
    const std::uint64_t projected_bytes = projected_curves_ * kLogicalBytesPerCurve;
    if (swept.telemetry.retained_geometry_bytes >
        std::numeric_limits<std::uint64_t>::max() - projected_bytes)
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    const std::uint64_t parent_child_overlap_bytes =
        projected_bytes + swept.telemetry.retained_geometry_bytes;
    if (parent_child_overlap_bytes > limits_.working_memory_bytes)
    {
        telemetry_.required_working_memory_bytes =
            std::max(telemetry_.required_working_memory_bytes, parent_child_overlap_bytes);
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    }
    telemetry_.peak_working_memory_bytes =
        std::max(telemetry_.peak_working_memory_bytes, parent_child_overlap_bytes);
    if (swept.curves.size() > std::numeric_limits<std::uint64_t>::max() / 6U)
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    std::uint64_t reemit_work = swept.curves.size() * 6U;
    if (swept.endpoint_tangencies.size() >
        (std::numeric_limits<std::uint64_t>::max() - reemit_work) / 2U)
        return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
    reemit_work += swept.endpoint_tangencies.size() * 2U;
    if (!charge_work(reemit_work))
        return false;
    const std::uint32_t curve_begin = static_cast<std::uint32_t>(out_.curves.size());
    out_.curves.reserve(projected_curves_);
    out_.bounds.reserve(projected_curves_);
    out_.occurrences.reserve(projected_curves_);
    descriptors_.reserve(projected_curves_);
    endpoint_tangencies_.reserve(projected_curves_);
    for (EmittedCurve& value : swept.curves)
        if (!emit(std::move(value.curve), value.agrees_with_carrier, value.material_on_left,
                  operand.operand_id, value.source, std::move(value.descriptor)))
            return false;
    for (const EmittedEndpointTangency& tangent : swept.endpoint_tangencies)
    {
        if (tangent.first_curve >= swept.curves.size() ||
            tangent.second_curve >= swept.curves.size())
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        const std::uint32_t first = curve_begin + tangent.first_curve;
        const std::uint32_t second = curve_begin + tangent.second_curve;
        if (first >= out_.curves.size() || second >= out_.curves.size() ||
            (out_.curves[first].kind == AnalyticAtomicCurveKind::line &&
             out_.curves[second].kind == AnalyticAtomicCurveKind::line))
            return fail(AnalyticFilteredLoweringError::resource_limit_exceeded);
        endpoint_tangencies_.push_back({first, second, tangent.first_start, tangent.second_start,
                                        tangent.construction_identity});
    }
    return true;
}
