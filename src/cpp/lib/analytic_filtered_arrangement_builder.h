#pragma once

// Internal arrangement builder and its bounded publication phases.
class ArrangementBuilder
{
  public:
    ArrangementBuilder(const AnalyticFilteredGeometry& geometry,
                       const AnalyticFilteredOverlayResult& overlay, AnalyticSolverLimits limits,
                       std::uint64_t admission_work_units,
                       analytic_execution_detail::TopologyPolicy policy)
        : geometry_(geometry), overlay_(overlay), limits_(limits), policy_(policy)
    {
        result_.telemetry.admission_work_units = admission_work_units;
        result_.telemetry.input_spans = overlay.spans.size();
        result_.telemetry.input_memberships = overlay.memberships.size();
        result_.telemetry.overlay_predicate_calls = overlay.telemetry.predicate_calls;
        result_.telemetry.overlay_peak_working_memory_bytes =
            overlay.telemetry.peak_working_memory_bytes;
        result_.telemetry.predicate_calls =
            admission_work_units + overlay.telemetry.predicate_calls;
        result_.telemetry.peak_working_memory_bytes = overlay.telemetry.peak_working_memory_bytes;
        result_.telemetry.required_working_memory_bytes =
            overlay.telemetry.required_working_memory_bytes;
        result_.telemetry.algebraic_fallback_calls = overlay.telemetry.algebraic_fallback_calls;
    }

    AnalyticFilteredArrangementResult build()
    {
        try
        {
            if (!preflight() || !validate_inputs() || !build_endpoint_clusters() ||
                !build_edges() || !build_half_edges() || !build_cycles())
            {
                clear_output();
                return result_;
            }
            return result_;
        }
        catch (const std::bad_alloc&)
        {
            result_.telemetry.required_working_memory_bytes = limits_.working_memory_bytes + 1;
            result_.error = AnalyticFilteredArrangementError::resource_limit_exceeded;
            clear_output();
        }
        return result_;
    }

  private:
    bool fail(AnalyticFilteredArrangementError error)
    {
        result_.error = error;
        return false;
    }

    bool fail_unresolved()
    {
        result_.telemetry.unresolved_predicate_failure = true;
        return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
    }

    void clear_output()
    {
        result_.vertices.clear();
        result_.edges.clear();
        result_.half_edges.clear();
        result_.outgoing_half_edges.clear();
        result_.collapsed_spans.clear();
        result_.memberships.clear();
        result_.cycles.clear();
        result_.cycle_half_edges.clear();
    }

    bool charge(std::uint64_t units)
    {
        if (result_.telemetry.predicate_calls > limits_.predicate_calls ||
            units > limits_.predicate_calls - result_.telemetry.predicate_calls)
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
        result_.telemetry.predicate_calls += units;
        return true;
    }

    bool charge_sort(std::uint64_t count)
    {
        const std::uint64_t units = sort_units(count);
        if (!charge(units))
            return false;
        result_.telemetry.sort_work_units += units;
        return true;
    }

    bool valid_preflight_shape() const noexcept
    {
        return analytic_solver_limits_within_hard_ceilings(limits_) &&
               overlay_.error == AnalyticFilteredOverlayError::none &&
               geometry_.curves.size() == geometry_.bounds.size() &&
               geometry_.curves.size() == geometry_.occurrences.size();
    }

    bool base_limits_exceeded() const noexcept
    {
        return geometry_.curves.size() > limits_.boundary_occurrences ||
               overlay_.spans.size() > limits_.arrangement_half_edges / 2 ||
               overlay_.memberships.size() > limits_.source_reference_memberships ||
               overlay_.memberships.size() > limits_.provenance_references ||
               overlay_.telemetry.predicate_calls > limits_.predicate_calls ||
               overlay_.telemetry.algebraic_fallback_calls > limits_.algebraic_fallback_calls;
    }

    bool derived_limits_exceeded(std::uint64_t collapsed_domains, std::uint64_t maximum_collapsed,
                                 std::uint64_t memberships) const noexcept
    {
        return collapsed_domains > geometry_.curves.size() ||
               maximum_collapsed > limits_.arrangement_half_edges / 2 ||
               memberships > limits_.source_reference_memberships ||
               memberships > limits_.provenance_references;
    }

    bool preflight()
    {
        if (!valid_preflight_shape())
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        if (base_limits_exceeded())
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);

        bool valid = true;
        const std::uint64_t spans = static_cast<std::uint64_t>(overlay_.spans.size());
        const std::uint64_t collapsed_domains = overlay_.telemetry.collapsed_domains;
        const std::uint64_t memberships = checked_add(
            static_cast<std::uint64_t>(overlay_.memberships.size()), collapsed_domains, valid);
        const std::uint64_t endpoints =
            checked_add(checked_multiply(spans, 2, valid), collapsed_domains, valid);
        const std::uint64_t half_edges = checked_multiply(spans, 2, valid);
        const std::uint64_t maximum_collapsed = checked_add(spans, collapsed_domains, valid);
        if (!valid || derived_limits_exceeded(collapsed_domains, maximum_collapsed, memberships))
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
        const std::uint64_t bytes = arrangement_memory_requirement(
            geometry_.curves.size(), spans, collapsed_domains, overlay_.memberships.size(),
            memberships, endpoints, half_edges, overlay_.telemetry.peak_working_memory_bytes,
            valid);
        if (!valid || bytes > limits_.working_memory_bytes)
        {
            if (valid)
                result_.telemetry.required_working_memory_bytes = bytes;
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
        }
        result_.telemetry.peak_working_memory_bytes = bytes;
        return true;
    }

    bool validate_geometry()
    {
        for (std::size_t index = 0; index < geometry_.curves.size(); ++index)
        {
            const AnalyticAtomicCurveNm& curve = geometry_.curves[index];
            const AnalyticFilteredOccurrence& occurrence = geometry_.occurrences[index];
            if (curve.curve_index != index + 1 ||
                geometry_.bounds[index].curve_index != index + 1 ||
                occurrence.occurrence_id != index + 1 || curve.construction_carrier_id == 0 ||
                !analytic_filtered_curve_is_valid(curve) ||
                (curve.kind == AnalyticAtomicCurveKind::circular_arc &&
                 occurrence.agrees_with_carrier != curve.counterclockwise))
                return fail(AnalyticFilteredArrangementError::invalid_argument);
        }
        return true;
    }

    bool validate_span_shape(const AnalyticAtomicSpanNm& span, std::size_t span_offset,
                             std::uint64_t membership_cursor)
    {
        if (span.span_index != span_offset + 1 || span.carrier_curve_index == 0 ||
            span.carrier_curve_index > geometry_.curves.size() || span.membership_count == 0 ||
            span.membership_begin != membership_cursor ||
            membership_cursor > overlay_.memberships.size() ||
            span.membership_count > overlay_.memberships.size() - membership_cursor ||
            !valid_point(span.start) || !valid_point(span.end))
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        return true;
    }

    bool validate_span_carrier(const AnalyticAtomicSpanNm& span,
                               const AnalyticAtomicCurveNm& carrier)
    {
        const AnalyticFilteredPointCurveStatus start_status =
            classify_analytic_filtered_point_on_curve(carrier, span.start);
        const AnalyticFilteredPointCurveStatus end_status =
            classify_analytic_filtered_point_on_curve(carrier, span.end);
        if (start_status == AnalyticFilteredPointCurveStatus::uncertain ||
            end_status == AnalyticFilteredPointCurveStatus::uncertain)
            return fail_unresolved();
        if (span.kind != carrier.kind || span.major_arc ||
            start_status != AnalyticFilteredPointCurveStatus::certified_on_domain ||
            end_status != AnalyticFilteredPointCurveStatus::certified_on_domain)
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        return true;
    }

    bool validate_span_memberships(const AnalyticAtomicSpanNm& span,
                                   const AnalyticAtomicCurveNm& carrier)
    {
        std::uint32_t previous_curve = 0;
        for (std::uint32_t local = 0; local < span.membership_count; ++local)
        {
            const AnalyticSpanMembership& membership =
                overlay_.memberships[span.membership_begin + local];
            if (membership.curve_index == 0 || membership.curve_index > geometry_.curves.size() ||
                membership.curve_index <= previous_curve)
                return fail(AnalyticFilteredArrangementError::invalid_argument);
            previous_curve = membership.curve_index;
            const AnalyticAtomicCurveNm& curve = geometry_.curves[membership.curve_index - 1];
            const AnalyticFilteredOccurrence& occurrence =
                geometry_.occurrences[membership.curve_index - 1];
            if (curve.construction_carrier_id != carrier.construction_carrier_id ||
                membership.agrees_with_span != occurrence.agrees_with_carrier ||
                membership.material_on_span_left != (occurrence.agrees_with_carrier
                                                         ? occurrence.material_on_left
                                                         : !occurrence.material_on_left))
                return fail(AnalyticFilteredArrangementError::invalid_argument);
        }
        return true;
    }

    bool validate_inputs()
    {
        if (!charge(geometry_.curves.size() * 2 + overlay_.spans.size() * 2 +
                    overlay_.memberships.size()) ||
            !validate_geometry())
            return false;
        curve_referenced_.assign(geometry_.curves.size(), 0);
        collapsed_curve_indices_.reserve(
            static_cast<std::size_t>(overlay_.telemetry.collapsed_domains));
        std::uint64_t membership_cursor = 0;
        for (std::size_t span_offset = 0; span_offset < overlay_.spans.size(); ++span_offset)
        {
            const AnalyticAtomicSpanNm& span = overlay_.spans[span_offset];
            if (!validate_span_shape(span, span_offset, membership_cursor))
                return false;
            const AnalyticAtomicCurveNm& carrier = geometry_.curves[span.carrier_curve_index - 1];
            if (!validate_span_carrier(span, carrier) || !validate_span_memberships(span, carrier))
                return false;
            for (std::uint32_t local = 0; local < span.membership_count; ++local)
                curve_referenced_[overlay_.memberships[span.membership_begin + local].curve_index -
                                  1] = 1;
            membership_cursor += span.membership_count;
        }
        if (membership_cursor != overlay_.memberships.size())
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        for (std::uint32_t curve_offset = 0; curve_offset < geometry_.curves.size(); ++curve_offset)
            if (curve_referenced_[curve_offset] == 0)
            {
                const AnalyticAtomicCurveNm& curve = geometry_.curves[curve_offset];
                const AnalyticFilteredPointNm representative = point_hull(curve.start, curve.end);
                const bool collapsed =
                    analytic_execution_detail::allows_resolution_topology(policy_)
                        ? complete_points_within_resolution(curve.start, curve.end)
                        : same_singleton_point(curve.start, curve.end);
                if (!collapsed || !valid_point(representative))
                    return fail(AnalyticFilteredArrangementError::invalid_argument);
                collapsed_curve_indices_.push_back(curve_offset);
            }
        if (collapsed_curve_indices_.size() > overlay_.telemetry.collapsed_domains)
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        return true;
    }

    bool build_endpoint_clusters()
    {
        const std::size_t span_endpoint_count = overlay_.spans.size() * 2;
        const std::size_t endpoint_count = span_endpoint_count + collapsed_curve_indices_.size();
        result_.telemetry.endpoint_records = endpoint_count;
        if (!charge(endpoint_count) || !charge_sort(endpoint_count))
            return false;
        endpoints_.reserve(endpoint_count);
        endpoint_vertices_.resize(endpoint_count);
        for (std::uint32_t span = 0; span < overlay_.spans.size(); ++span)
        {
            const std::uint32_t carrier = overlay_.spans[span].carrier_curve_index;
            const AnalyticAtomicCurveNm& curve = geometry_.curves[carrier - 1];
            const std::uint32_t authoritative =
                curve.has_endpoint_authoritative_arc_certificate ? carrier : 0;
            endpoints_.push_back(
                {overlay_.spans[span].start, span * 2, span, authoritative, true,
                 proven_singleton_integer_endpoint(overlay_.spans[span].start, curve) ||
                     proven_singleton_integer_intersection(overlay_.spans[span].start, curve)});
            endpoints_.push_back(
                {overlay_.spans[span].end, span * 2 + 1, span, authoritative, false,
                 proven_singleton_integer_endpoint(overlay_.spans[span].end, curve) ||
                     proven_singleton_integer_intersection(overlay_.spans[span].end, curve)});
        }
        for (std::uint32_t local = 0; local < collapsed_curve_indices_.size(); ++local)
        {
            const std::uint32_t curve_offset = collapsed_curve_indices_[local];
            const AnalyticAtomicCurveNm& curve = geometry_.curves[curve_offset];
            endpoints_.push_back(
                {point_hull(curve.start, curve.end),
                 static_cast<std::uint32_t>(span_endpoint_count + local),
                 static_cast<std::uint32_t>(overlay_.spans.size() + local),
                 curve.has_endpoint_authoritative_arc_certificate ? curve.curve_index : 0, false,
                 false});
        }
        std::sort(endpoints_.begin(), endpoints_.end(),
                  [](const EndpointRecord& left, const EndpointRecord& right)
                  {
                      return std::make_tuple(left.point.x.lower, left.point.x.upper,
                                             left.point.y.lower, left.point.y.upper,
                                             left.span_offset, left.start) <
                             std::make_tuple(right.point.x.lower, right.point.x.upper,
                                             right.point.y.lower, right.point.y.upper,
                                             right.span_offset, right.start);
                  });

        detail::AnalyticIntervalIndex index(endpoint_count);
        std::unique_ptr<ExpiryEntry[]> expiry =
            endpoint_count == 0 ? nullptr : std::make_unique<ExpiryEntry[]>(endpoint_count);
        std::size_t expiry_size = 0;
        clusters_.reserve(endpoint_count);
        for (const EndpointRecord& endpoint : endpoints_)
        {
            const double minimum_x = expanded_lower(endpoint.point.x.lower, policy_);
            while (expiry_size != 0 && expiry[0].maximum_x < minimum_x)
            {
                std::pop_heap(expiry.get(), expiry.get() + expiry_size, ExpiryLater{});
                const ExpiryEntry expired = expiry[--expiry_size];
                index.erase(expired.minimum_y, expired.cluster + 1);
            }

            std::uint32_t selected = kNoIndex;
            const std::uint64_t visits_before = result_.telemetry.predicate_calls;
            const bool completed = index.query(
                expanded_lower(endpoint.point.y.lower, policy_),
                expanded_upper(endpoint.point.y.upper, policy_), result_.telemetry.predicate_calls,
                limits_.predicate_calls,
                [&](std::size_t payload, std::uint32_t)
                {
                    const std::uint32_t cluster = static_cast<std::uint32_t>(payload);
                    if (endpoint_can_join_cluster(clusters_[cluster], endpoint, policy_))
                        selected = std::min(selected, cluster);
                    return true;
                });
            result_.telemetry.endpoint_index_node_visits +=
                result_.telemetry.predicate_calls - visits_before;
            if (!completed)
                return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);

            if (selected == kNoIndex)
            {
                if (clusters_.size() == std::numeric_limits<std::uint32_t>::max())
                    return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
                selected = static_cast<std::uint32_t>(clusters_.size());
                const std::uint64_t update_units = tree_update_units(endpoint_count);
                if (!charge(update_units))
                    return false;
                result_.telemetry.endpoint_index_update_work_units += update_units;
                clusters_.push_back({endpoint.point, endpoint.point,
                                     endpoint.endpoint_authoritative_curve,
                                     endpoint.singleton_integer_construction_endpoint});
                if (!index.insert(endpoint.point.y.lower, endpoint.point.y.upper, selected,
                                  selected + 1))
                    return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
                expiry[expiry_size++] = {expanded_upper(endpoint.point.x.upper, policy_),
                                         endpoint.point.y.lower, selected};
                std::push_heap(expiry.get(), expiry.get() + expiry_size, ExpiryLater{});
            }
            else
            {
                if (!clusters_[selected].singleton_integer_construction_endpoint &&
                    endpoint.singleton_integer_construction_endpoint)
                    clusters_[selected].singleton_integer_point = endpoint.point;
                clusters_[selected].hull = point_hull(clusters_[selected].hull, endpoint.point);
                clusters_[selected].singleton_integer_construction_endpoint =
                    clusters_[selected].singleton_integer_construction_endpoint ||
                    endpoint.singleton_integer_construction_endpoint;
                ++result_.telemetry.merged_endpoint_records;
            }
            endpoint_vertices_[endpoint.endpoint_slot] = selected;
        }
        std::vector<EndpointRecord>().swap(endpoints_);
        return true;
    }

    void collect_edge_drafts(std::vector<EdgeDraft>& drafts, std::vector<CollapsedDraft>& collapsed,
                             std::vector<bool>& used)
    {
        for (std::uint32_t span_offset = 0; span_offset < overlay_.spans.size(); ++span_offset)
        {
            const AnalyticAtomicSpanNm& span = overlay_.spans[span_offset];
            const std::uint32_t start = endpoint_vertices_[span_offset * 2];
            const std::uint32_t end = endpoint_vertices_[span_offset * 2 + 1];
            if (start == end)
            {
                used[start] = true;
                collapsed.push_back(
                    {{0, span.carrier_curve_index, 0, span.membership_count}, start, span_offset});
                ++result_.telemetry.collapsed_spans;
                continue;
            }
            used[start] = true;
            used[end] = true;
            const AnalyticAtomicCurveNm& carrier = geometry_.curves[span.carrier_curve_index - 1];
            AnalyticArrangementEdgeNm edge;
            edge.start_vertex = start;
            edge.end_vertex = end;
            edge.carrier_curve_index = span.carrier_curve_index;
            edge.kind = span.kind;
            edge.carrier_start = span.start;
            edge.carrier_end = span.end;
            edge.circle = carrier.circle;
            edge.counterclockwise = true;
            edge.major_arc = span.major_arc;
            edge.membership_count = span.membership_count;
            edge.x_monotone_branch = span.x_monotone_branch;
            edge.endpoint_authoritative_arc = carrier.has_endpoint_authoritative_arc_certificate;
            edge.has_construction_line_direction = carrier.has_construction_line_direction;
            edge.construction_line_dx = carrier.construction_line_dx;
            edge.construction_line_dy = carrier.construction_line_dy;
            edge.construction_carrier_id = carrier.construction_carrier_id;
            const auto same_coordinates =
                [](const AnalyticFilteredPointNm& left, const AnalyticFilteredPointNm& right)
            {
                return left.x.lower == right.x.lower && left.x.upper == right.x.upper &&
                       left.y.lower == right.y.lower && left.y.upper == right.y.upper;
            };
            if (same_coordinates(span.start, carrier.start))
                edge.construction_start_tangent_id = carrier.construction_start_tangent_id;
            else if (same_coordinates(span.start, carrier.end))
                edge.construction_start_tangent_id = carrier.construction_end_tangent_id;
            if (same_coordinates(span.end, carrier.start))
                edge.construction_end_tangent_id = carrier.construction_start_tangent_id;
            else if (same_coordinates(span.end, carrier.end))
                edge.construction_end_tangent_id = carrier.construction_end_tangent_id;
            drafts.push_back({edge, span_offset});
        }
        const std::uint32_t collapsed_endpoint_begin =
            static_cast<std::uint32_t>(overlay_.spans.size() * 2);
        for (std::uint32_t local = 0; local < collapsed_curve_indices_.size(); ++local)
        {
            const std::uint32_t curve_offset = collapsed_curve_indices_[local];
            const std::uint32_t cluster = endpoint_vertices_[collapsed_endpoint_begin + local];
            used[cluster] = true;
            collapsed.push_back({{0, curve_offset + 1, 0, 1}, cluster, kNoIndex});
            ++result_.telemetry.collapsed_spans;
        }
    }

    bool publish_vertices(std::vector<EdgeDraft>& drafts, std::vector<CollapsedDraft>& collapsed,
                          const std::vector<bool>& used)
    {
        std::vector<std::uint32_t> compact(clusters_.size(), kNoIndex);
        result_.vertices.reserve(clusters_.size());
        for (std::uint32_t cluster = 0; cluster < clusters_.size(); ++cluster)
            if (used[cluster])
            {
                if (result_.vertices.size() == limits_.arrangement_vertices)
                    return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
                compact[cluster] = static_cast<std::uint32_t>(result_.vertices.size());
                result_.vertices.push_back({clusters_[cluster].hull, 0, 0});
            }
        for (EdgeDraft& draft : drafts)
        {
            draft.edge.start_vertex = compact[draft.edge.start_vertex];
            draft.edge.end_vertex = compact[draft.edge.end_vertex];
        }
        for (CollapsedDraft& draft : collapsed)
            draft.span.vertex = compact[draft.cluster];
        return true;
    }

    static void sort_drafts(std::vector<EdgeDraft>& drafts, std::vector<CollapsedDraft>& collapsed)
    {
        std::sort(drafts.begin(), drafts.end(),
                  [](const EdgeDraft& left, const EdgeDraft& right)
                  {
                      return std::make_tuple(std::min(left.edge.start_vertex, left.edge.end_vertex),
                                             std::max(left.edge.start_vertex, left.edge.end_vertex),
                                             left.edge.start_vertex, left.edge.end_vertex,
                                             left.edge.kind, left.edge.carrier_curve_index,
                                             left.span_offset) <
                             std::make_tuple(
                                 std::min(right.edge.start_vertex, right.edge.end_vertex),
                                 std::max(right.edge.start_vertex, right.edge.end_vertex),
                                 right.edge.start_vertex, right.edge.end_vertex, right.edge.kind,
                                 right.edge.carrier_curve_index, right.span_offset);
                  });
        std::sort(collapsed.begin(), collapsed.end(),
                  [](const CollapsedDraft& left, const CollapsedDraft& right)
                  {
                      return std::tie(left.span.vertex, left.span.carrier_curve_index,
                                      left.span_offset) < std::tie(right.span.vertex,
                                                                   right.span.carrier_curve_index,
                                                                   right.span_offset);
                  });
    }

    void append_span_memberships(const AnalyticAtomicSpanNm& span)
    {
        result_.memberships.insert(
            result_.memberships.end(), overlay_.memberships.begin() + span.membership_begin,
            overlay_.memberships.begin() + span.membership_begin + span.membership_count);
    }

    void append_collapsed_memberships(const CollapsedDraft& draft)
    {
        if (draft.span_offset != kNoIndex)
        {
            append_span_memberships(overlay_.spans[draft.span_offset]);
            return;
        }
        const std::uint32_t curve_offset = draft.span.carrier_curve_index - 1;
        const AnalyticFilteredOccurrence& occurrence = geometry_.occurrences[curve_offset];
        result_.memberships.push_back({curve_offset + 1, occurrence.agrees_with_carrier,
                                       occurrence.agrees_with_carrier
                                           ? occurrence.material_on_left
                                           : !occurrence.material_on_left});
    }

    bool collect_endpoint_tangent_witness(const AnalyticAtomicSpanNm& span,
                                          const AnalyticFilteredPointNm& endpoint,
                                          std::uint64_t& witness)
    {
        witness = 0;
        const std::uint64_t span_carrier =
            geometry_.curves[span.carrier_curve_index - 1].construction_carrier_id;
        for (std::uint32_t local = 0; local < span.membership_count; ++local)
        {
            const AnalyticSpanMembership& membership =
                overlay_.memberships[span.membership_begin + local];
            const AnalyticAtomicCurveNm& curve = geometry_.curves[membership.curve_index - 1];
            if (span_carrier == 0 || curve.construction_carrier_id != span_carrier)
                continue;
            std::uint64_t candidate = 0;
            bool endpoint_start = false;
            if (analytic_arrangement_detail::same_endpoint_enclosure(endpoint, curve.start))
            {
                candidate = curve.construction_start_tangent_id;
                endpoint_start = true;
            }
            else if (analytic_arrangement_detail::same_endpoint_enclosure(endpoint, curve.end))
                candidate = curve.construction_end_tangent_id;
            if (candidate == 0)
                continue;
            if (!analytic_endpoint_tangent_matches(candidate, curve.kind,
                                                   curve.construction_carrier_id, endpoint_start))
                return fail(AnalyticFilteredArrangementError::invalid_argument);
            if (witness != 0 && witness != candidate)
                return fail_unresolved();
            witness = candidate;
        }
        return true;
    }

    bool publish_edges(std::vector<EdgeDraft>& drafts, std::vector<CollapsedDraft>& collapsed)
    {
        result_.edges.reserve(drafts.size());
        edge_tangent_witnesses_.reserve(drafts.size());
        result_.collapsed_spans.reserve(collapsed.size());
        result_.memberships.reserve(overlay_.memberships.size() + collapsed_curve_indices_.size());
        if (!charge(overlay_.memberships.size() + collapsed_curve_indices_.size()) ||
            !charge(overlay_.memberships.size() * 2))
            return false;
        for (EdgeDraft& draft : drafts)
        {
            const AnalyticAtomicSpanNm& span = overlay_.spans[draft.span_offset];
            draft.edge.membership_begin = static_cast<std::uint32_t>(result_.memberships.size());
            EdgeTangentWitness witness;
            if (!collect_endpoint_tangent_witness(span, draft.edge.carrier_start, witness.start) ||
                !collect_endpoint_tangent_witness(span, draft.edge.carrier_end, witness.end))
                return false;
            append_span_memberships(span);
            result_.edges.push_back(draft.edge);
            edge_tangent_witnesses_.push_back(witness);
        }
        for (CollapsedDraft& draft : collapsed)
        {
            draft.span.membership_begin = static_cast<std::uint32_t>(result_.memberships.size());
            append_collapsed_memberships(draft);
            result_.collapsed_spans.push_back(draft.span);
        }
        return true;
    }

    bool build_edges()
    {
        const std::uint64_t basic_work = static_cast<std::uint64_t>(overlay_.spans.size()) * 2 +
                                         collapsed_curve_indices_.size() + clusters_.size() +
                                         overlay_.memberships.size() +
                                         collapsed_curve_indices_.size();
        if (!charge(basic_work) || !charge_sort(overlay_.spans.size()) ||
            !charge_sort(overlay_.spans.size() + collapsed_curve_indices_.size()))
            return false;
        std::vector<EdgeDraft> drafts;
        std::vector<CollapsedDraft> collapsed;
        drafts.reserve(overlay_.spans.size());
        collapsed.reserve(overlay_.spans.size() + collapsed_curve_indices_.size());
        std::vector<bool> used(clusters_.size());
        collect_edge_drafts(drafts, collapsed, used);
        if (!publish_vertices(drafts, collapsed, used))
            return false;
        sort_drafts(drafts, collapsed);
        if (!publish_edges(drafts, collapsed))
            return false;
        result_.telemetry.emitted_vertices = result_.vertices.size();
        result_.telemetry.emitted_edges = result_.edges.size();
        std::vector<std::uint32_t>().swap(endpoint_vertices_);
        std::vector<VertexCluster>().swap(clusters_);
        std::vector<std::uint8_t>().swap(curve_referenced_);
        std::vector<std::uint32_t>().swap(collapsed_curve_indices_);
        return true;
    }

    void initialize_half_edges()
    {
        result_.half_edges.resize(result_.edges.size() * 2);
        result_.outgoing_half_edges.resize(result_.half_edges.size());
        for (std::uint32_t edge = 0; edge < result_.edges.size(); ++edge)
        {
            const std::uint32_t forward = edge * 2;
            const std::uint32_t reverse = forward + 1;
            result_.half_edges[forward] = {result_.edges[edge].start_vertex,
                                           reverse,
                                           kNoIndex,
                                           kNoIndex,
                                           edge,
                                           true,
                                           kNoIndex};
            result_.half_edges[reverse] = {
                result_.edges[edge].end_vertex, forward, kNoIndex, kNoIndex, edge, false, kNoIndex};
            result_.outgoing_half_edges[forward] = forward;
            result_.outgoing_half_edges[reverse] = reverse;
        }
    }

    TangentEndpointIdentity edge_tangent_identity(std::uint32_t half_edge) const
    {
        TangentEndpointIdentity value = outgoing_tangent_identity(half_edge, result_);
        if (value.tangent_id == 0)
        {
            const AnalyticArrangementHalfEdge& edge = result_.half_edges[half_edge];
            const EdgeTangentWitness& witness = edge_tangent_witnesses_[edge.edge];
            value.tangent_id = edge.forward ? witness.start : witness.end;
        }
        return value;
    }

    TangentEndpointIdentity resolved_tangent_identity(std::uint32_t half_edge) const
    {
        TangentEndpointIdentity value = edge_tangent_identity(half_edge);
        if (value.tangent_id == 0 && half_edge < outgoing_tangent_ids_.size())
            value.tangent_id = outgoing_tangent_ids_[half_edge];
        return value;
    }

    std::optional<std::int8_t> compare_frozen_tangents(std::uint32_t left,
                                                       std::uint32_t right) const
    {
        if (left >= outgoing_tangent_angles_.size() || right >= outgoing_tangent_angles_.size())
            return std::nullopt;
        const Tangent left_tangent = outgoing_tangent(left, result_);
        const Tangent right_tangent = outgoing_tangent(right, result_);
        const Interval tangent_dot = dot(left_tangent.direction, right_tangent.direction);
        const auto left_key = outgoing_key(left, result_);
        const auto right_key = outgoing_key(right, result_);
        const auto canonical = compare_canonical_tangent_class(
            resolved_tangent_identity(left), resolved_tangent_identity(right), tangent_dot.lower,
            outgoing_tangent_angles_[left], outgoing_tangent_angles_[right], std::get<1>(left_key),
            std::get<1>(right_key), std::get<2>(left_key), std::get<2>(right_key),
            std::get<3>(left_key), std::get<3>(right_key), std::get<4>(left_key),
            std::get<4>(right_key));
        if (canonical)
            return canonical;
        return compare_tangents(left_tangent, right_tangent);
    }

    bool prepare_canonical_tangent_angles()
    {
        const std::uint64_t count = result_.half_edges.size();
        if (count > std::numeric_limits<std::uint64_t>::max() / 8 || !charge(count * 8) ||
            !charge_sort(count) || !charge_sort(count))
            return false;
        outgoing_tangent_angles_.resize(count);
        outgoing_tangent_ids_.resize(count);
        outgoing_tangent_projected_.resize(count);
        tangent_class_half_edges_.resize(count);
        for (std::uint32_t half_edge = 0; half_edge < count; ++half_edge)
        {
            outgoing_tangent_angles_[half_edge] = std::get<0>(outgoing_key(half_edge, result_));
            outgoing_tangent_ids_[half_edge] = edge_tangent_identity(half_edge).tangent_id;
            tangent_class_half_edges_[half_edge] = half_edge;
        }
        const auto carrier_key = [&](std::uint32_t half_edge)
        {
            const TangentEndpointIdentity value = edge_tangent_identity(half_edge);
            const std::uint32_t vertex = result_.half_edges[half_edge].origin_vertex;
            return std::tuple(vertex, value.kind, value.carrier_id, value.point.x.lower,
                              value.point.x.upper, value.point.y.lower, value.point.y.upper,
                              value.point.construction_x_column_id, half_edge);
        };
        std::sort(tangent_class_half_edges_.begin(), tangent_class_half_edges_.end(),
                  [&](std::uint32_t left, std::uint32_t right)
                  { return carrier_key(left) < carrier_key(right); });
        std::size_t begin = 0;
        while (begin < tangent_class_half_edges_.size())
        {
            const TangentEndpointIdentity key =
                edge_tangent_identity(tangent_class_half_edges_[begin]);
            const std::uint32_t vertex =
                result_.half_edges[tangent_class_half_edges_[begin]].origin_vertex;
            std::size_t end = begin + 1;
            while (end < tangent_class_half_edges_.size())
            {
                const std::uint32_t candidate_half = tangent_class_half_edges_[end];
                const TangentEndpointIdentity candidate = edge_tangent_identity(candidate_half);
                if (result_.half_edges[candidate_half].origin_vertex != vertex ||
                    candidate.kind != key.kind || candidate.carrier_id != key.carrier_id ||
                    !analytic_arrangement_detail::same_endpoint_enclosure(candidate.point,
                                                                          key.point))
                    break;
                ++end;
            }
            std::uint32_t representatives[2] = {kNoIndex, kNoIndex};
            std::uint64_t tokens[2] = {0, 0};
            for (std::size_t at = begin; at < end; ++at)
            {
                const std::uint32_t half_edge = tangent_class_half_edges_[at];
                const Tangent tangent = outgoing_tangent(half_edge, result_);
                std::uint32_t ray = kNoIndex;
                for (std::uint32_t candidate_ray = 0; candidate_ray < 2; ++candidate_ray)
                {
                    if (representatives[candidate_ray] == kNoIndex)
                        continue;
                    const Interval product =
                        dot(tangent.direction,
                            outgoing_tangent(representatives[candidate_ray], result_).direction);
                    if (product.lower > 0.0)
                    {
                        if (ray != kNoIndex)
                            return fail_unresolved();
                        ray = candidate_ray;
                    }
                    else if (product.upper >= 0.0)
                        return fail_unresolved();
                }
                if (ray == kNoIndex)
                {
                    ray = representatives[0] == kNoIndex
                              ? 0
                              : (representatives[1] == kNoIndex ? 1 : kNoIndex);
                    if (ray == kNoIndex)
                        return fail_unresolved();
                    representatives[ray] = half_edge;
                }
                const TangentEndpointIdentity candidate = edge_tangent_identity(half_edge);
                if (candidate.tangent_id == 0)
                    continue;
                if (!tangent_token_names_endpoint(candidate))
                    return fail(AnalyticFilteredArrangementError::invalid_argument);
                if (tokens[ray] != 0 && tokens[ray] != candidate.tangent_id)
                    return fail_unresolved();
                tokens[ray] = candidate.tangent_id;
            }
            const std::uint64_t group_token =
                tokens[0] == 0 ? tokens[1]
                               : (tokens[1] == 0 || tokens[1] == tokens[0] ? tokens[0] : 0);
            for (std::size_t at = begin; at < end; ++at)
            {
                const std::uint32_t half_edge = tangent_class_half_edges_[at];
                const Tangent tangent = outgoing_tangent(half_edge, result_);
                std::uint32_t ray = kNoIndex;
                for (std::uint32_t candidate_ray = 0; candidate_ray < 2; ++candidate_ray)
                {
                    if (representatives[candidate_ray] == kNoIndex)
                        continue;
                    const Interval product =
                        dot(tangent.direction,
                            outgoing_tangent(representatives[candidate_ray], result_).direction);
                    if (product.lower > 0.0)
                    {
                        if (ray != kNoIndex)
                            return fail_unresolved();
                        ray = candidate_ray;
                    }
                    else if (product.upper >= 0.0)
                        return fail_unresolved();
                }
                if (ray == kNoIndex)
                    return fail_unresolved();
                const std::uint64_t token = group_token != 0 ? group_token : tokens[ray];
                if (token != 0 && outgoing_tangent_ids_[half_edge] == 0)
                {
                    outgoing_tangent_ids_[half_edge] = token;
                    outgoing_tangent_projected_[half_edge] = 1;
                }
            }
            begin = end;
        }

        const auto token_key = [&](std::uint32_t half_edge)
        {
            const TangentEndpointIdentity value = resolved_tangent_identity(half_edge);
            const std::uint32_t vertex = result_.half_edges[half_edge].origin_vertex;
            return std::tuple(vertex, value.tangent_id, value.point.x.lower, value.point.x.upper,
                              value.point.y.lower, value.point.y.upper,
                              value.point.construction_x_column_id, half_edge);
        };
        std::sort(tangent_class_half_edges_.begin(), tangent_class_half_edges_.end(),
                  [&](std::uint32_t left, std::uint32_t right)
                  { return token_key(left) < token_key(right); });
        begin = 0;
        while (begin < tangent_class_half_edges_.size())
        {
            const std::uint32_t first_half = tangent_class_half_edges_[begin];
            const TangentEndpointIdentity key = resolved_tangent_identity(first_half);
            const std::uint32_t vertex = result_.half_edges[first_half].origin_vertex;
            std::size_t end = begin + 1;
            while (end < tangent_class_half_edges_.size())
            {
                const std::uint32_t candidate_half = tangent_class_half_edges_[end];
                const TangentEndpointIdentity candidate = resolved_tangent_identity(candidate_half);
                if (result_.half_edges[candidate_half].origin_vertex != vertex ||
                    candidate.tangent_id != key.tangent_id ||
                    !analytic_arrangement_detail::same_endpoint_enclosure(candidate.point,
                                                                          key.point))
                    break;
                ++end;
            }
            if (key.tangent_id != 0)
            {
                std::uint32_t authorities[2] = {kNoIndex, kNoIndex};
                for (std::size_t at = begin; at < end; ++at)
                {
                    const std::uint32_t half_edge = tangent_class_half_edges_[at];
                    const TangentEndpointIdentity candidate = edge_tangent_identity(half_edge);
                    if (candidate.kind != AnalyticAtomicCurveKind::line ||
                        candidate.tangent_id != key.tangent_id)
                        continue;
                    const Tangent tangent = outgoing_tangent(half_edge, result_);
                    std::uint32_t& authority =
                        authorities[0] == kNoIndex ||
                                dot(outgoing_tangent(authorities[0], result_).direction,
                                    tangent.direction)
                                        .lower > 0.0
                            ? authorities[0]
                            : authorities[1];
                    if (authority != kNoIndex)
                    {
                        const TangentEndpointIdentity existing = edge_tangent_identity(authority);
                        if (candidate.carrier_id != existing.carrier_id ||
                            dot(outgoing_tangent(authority, result_).direction, tangent.direction)
                                    .lower <= 0.0 ||
                            outgoing_tangent_angles_[authority] !=
                                outgoing_tangent_angles_[half_edge])
                            return fail_unresolved();
                    }
                    if (authority == kNoIndex || half_edge < authority)
                        authority = half_edge;
                }
                for (std::size_t at = begin; at < end; ++at)
                {
                    const std::uint32_t half_edge = tangent_class_half_edges_[at];
                    const Tangent tangent = outgoing_tangent(half_edge, result_);
                    std::uint32_t authority = kNoIndex;
                    for (const std::uint32_t candidate : authorities)
                        if (candidate != kNoIndex &&
                            dot(tangent.direction, outgoing_tangent(candidate, result_).direction)
                                    .lower > 0.0)
                            authority = candidate;
                    if (authority == kNoIndex)
                    {
                        if (outgoing_tangent_projected_[half_edge] != 0)
                            outgoing_tangent_ids_[half_edge] = 0;
                        continue;
                    }
                    const TangentEndpointIdentity value = resolved_tangent_identity(half_edge);
                    const TangentEndpointIdentity authority_value =
                        resolved_tangent_identity(authority);
                    const double tangent_dot =
                        dot(tangent.direction, outgoing_tangent(authority, result_).direction)
                            .lower;
                    if (!shares_exact_tangent_contact(value, authority_value, tangent_dot))
                        return fail_unresolved();
                    const auto forward =
                        compare_collinear_tangents(tangent, outgoing_tangent(authority, result_));
                    const auto reverse =
                        compare_collinear_tangents(outgoing_tangent(authority, result_), tangent);
                    if (!forward || !reverse || *forward != -*reverse)
                        return fail(AnalyticFilteredArrangementError::invalid_argument);
                    outgoing_tangent_angles_[half_edge] = outgoing_tangent_angles_[authority];
                }
            }
            begin = end;
        }
        return true;
    }

    bool certify_and_link_vertex(std::size_t begin, std::size_t end)
    {
        const std::uint32_t vertex =
            result_.half_edges[result_.outgoing_half_edges[begin]].origin_vertex;
        result_.vertices[vertex].outgoing_begin = static_cast<std::uint32_t>(begin);
        result_.vertices[vertex].outgoing_count = static_cast<std::uint32_t>(end - begin);
        bool endpoint_authoritative_degree_two = false;
        if (end - begin == 2)
            endpoint_authoritative_degree_two =
                result_.edges[result_.half_edges[result_.outgoing_half_edges[begin]].edge]
                    .endpoint_authoritative_arc ||
                result_.edges[result_.half_edges[result_.outgoing_half_edges[begin + 1]].edge]
                    .endpoint_authoritative_arc;
        for (std::size_t index = begin + 1; index < end; ++index)
        {
            if (endpoint_authoritative_degree_two)
                break;
            if (!charge(1))
                return false;
            ++result_.telemetry.angular_predicates;
            const std::uint32_t left_half = result_.outgoing_half_edges[index - 1];
            const std::uint32_t right_half = result_.outgoing_half_edges[index];
            std::optional<std::int8_t> order = compare_frozen_tangents(left_half, right_half);
            if (!order)
            {
                if (!charge(static_cast<std::uint64_t>(end - begin)))
                    return false;
            }
            if (!order)
                return fail_unresolved();
            if (*order >= 0)
                return fail(AnalyticFilteredArrangementError::invalid_argument);
        }
        std::uint32_t previous = result_.outgoing_half_edges[end - 1];
        for (std::size_t index = begin; index < end; ++index)
        {
            const std::uint32_t outgoing = result_.outgoing_half_edges[index];
            result_.half_edges[result_.half_edges[outgoing].twin].next = previous;
            previous = outgoing;
        }
        return true;
    }

    bool build_rotation_system()
    {
        std::size_t begin = 0;
        while (begin < result_.outgoing_half_edges.size())
        {
            const std::uint32_t vertex =
                result_.half_edges[result_.outgoing_half_edges[begin]].origin_vertex;
            std::size_t end = begin + 1;
            while (end < result_.outgoing_half_edges.size() &&
                   result_.half_edges[result_.outgoing_half_edges[end]].origin_vertex == vertex)
                ++end;
            if (!certify_and_link_vertex(begin, end))
                return false;
            begin = end;
        }
        return true;
    }

    bool fill_previous_links()
    {
        for (std::uint32_t half_edge = 0; half_edge < result_.half_edges.size(); ++half_edge)
        {
            const std::uint32_t next = result_.half_edges[half_edge].next;
            if (next == kNoIndex || next >= result_.half_edges.size() ||
                result_.half_edges[next].previous != kNoIndex)
                return fail(AnalyticFilteredArrangementError::invalid_argument);
            result_.half_edges[next].previous = half_edge;
        }
        return true;
    }

    bool build_half_edges()
    {
        if (result_.edges.size() > limits_.arrangement_half_edges / 2)
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);
        const std::uint64_t half_edge_count = result_.edges.size() * 2;
        if (!charge(result_.edges.size() + half_edge_count * 3) || !charge_sort(half_edge_count))
            return false;
        initialize_half_edges();
        std::sort(result_.outgoing_half_edges.begin(), result_.outgoing_half_edges.end(),
                  [&](std::uint32_t left, std::uint32_t right)
                  {
                      const std::uint32_t left_origin = result_.half_edges[left].origin_vertex;
                      const std::uint32_t right_origin = result_.half_edges[right].origin_vertex;
                      return left_origin != right_origin ? left_origin < right_origin
                                                         : left < right;
                  });
        if (!prepare_canonical_tangent_angles())
            return false;
        std::sort(result_.outgoing_half_edges.begin(), result_.outgoing_half_edges.end(),
                  [&](std::uint32_t left, std::uint32_t right)
                  {
                      const std::uint32_t left_origin = result_.half_edges[left].origin_vertex;
                      const std::uint32_t right_origin = result_.half_edges[right].origin_vertex;
                      if (left_origin != right_origin)
                          return left_origin < right_origin;
                      const auto exact = compare_frozen_tangents(left, right);
                      if (exact)
                          return *exact < 0;
                      auto left_key = outgoing_key(left, result_);
                      auto right_key = outgoing_key(right, result_);
                      std::get<0>(left_key) = outgoing_tangent_angles_[left];
                      std::get<0>(right_key) = outgoing_tangent_angles_[right];
                      return left_key < right_key;
                  });

        if (!build_rotation_system() || !fill_previous_links())
            return false;
        result_.telemetry.emitted_half_edges = result_.half_edges.size();
        return true;
    }

    void build_components(DisjointSet& components, std::vector<std::uint32_t>& component_by_root)
    {
        for (const AnalyticArrangementEdgeNm& edge : result_.edges)
            components.unite(edge.start_vertex, edge.end_vertex);
        std::uint32_t component_count = 0;
        for (std::uint32_t vertex = 0; vertex < result_.vertices.size(); ++vertex)
        {
            const std::uint32_t root = components.find(vertex);
            if (component_by_root[root] == kNoIndex)
                component_by_root[root] = component_count++;
        }
    }

    bool trace_cycle(std::uint32_t start, std::vector<bool>& visited, std::size_t cycle_begin)
    {
        std::uint32_t current = start;
        while (!visited[current])
        {
            visited[current] = true;
            result_.cycle_half_edges.push_back(current);
            current = result_.half_edges[current].next;
            if (current >= result_.half_edges.size() ||
                result_.cycle_half_edges.size() - cycle_begin > result_.half_edges.size())
                return fail(AnalyticFilteredArrangementError::invalid_argument);
        }
        if (current != start || result_.cycle_half_edges.size() == cycle_begin)
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        return true;
    }

    bool publish_cycle(std::size_t cycle_begin, DisjointSet& components,
                       const std::vector<std::uint32_t>& component_by_root)
    {
        auto first = result_.cycle_half_edges.begin() + cycle_begin;
        auto last = result_.cycle_half_edges.end();
        if (!charge(static_cast<std::uint64_t>(last - first)))
            return false;
        bool endpoint_authoritative_cycle = false;
        for (auto at = first; at != last; ++at)
        {
            const std::uint32_t half_edge = *at;
            endpoint_authoritative_cycle =
                endpoint_authoritative_cycle ||
                result_.edges[result_.half_edges[half_edge].edge].endpoint_authoritative_arc;
        }
        auto canonical = std::min_element(
            first, last,
            [&](std::uint32_t left, std::uint32_t right)
            {
                const std::uint32_t left_vertex = result_.half_edges[left].origin_vertex;
                const std::uint32_t right_vertex = result_.half_edges[right].origin_vertex;
                if (endpoint_authoritative_cycle)
                {
                    const auto& left_point = result_.vertices[left_vertex].point;
                    const auto& right_point = result_.vertices[right_vertex].point;
                    const auto left_key =
                        std::tie(left_point.x.lower, left_point.x.upper, left_point.y.lower,
                                 left_point.y.upper, left_vertex, left);
                    const auto right_key =
                        std::tie(right_point.x.lower, right_point.x.upper, right_point.y.lower,
                                 right_point.y.upper, right_vertex, right);
                    return left_key < right_key;
                }
                return left_vertex != right_vertex ? left_vertex < right_vertex : left < right;
            });
        std::rotate(first, canonical, last);
        const auto orientation_at = [&](std::uint32_t outgoing)
        {
            const std::uint32_t reverse_incoming =
                result_.half_edges[result_.half_edges[outgoing].previous].twin;
            std::optional<std::int8_t> orientation = compare_cycle_germs(
                outgoing_tangent(outgoing, result_), outgoing_tangent(reverse_incoming, result_));
            if (orientation)
                return orientation;
            auto outgoing_half = cycle_germ_half(outgoing_tangent(outgoing, result_));
            auto incoming_half = cycle_germ_half(outgoing_tangent(reverse_incoming, result_));
            if (!outgoing_half)
                outgoing_half = endpoint_authoritative_cycle_half(outgoing, result_);
            if (!incoming_half)
                incoming_half = endpoint_authoritative_cycle_half(reverse_incoming, result_);
            if (outgoing_half && incoming_half && *outgoing_half != *incoming_half)
                orientation = *outgoing_half < *incoming_half ? -1 : 1;
            else if (outgoing_half && incoming_half)
                orientation = compare_frozen_tangents(outgoing, reverse_incoming);
            return orientation;
        };
        const std::uint32_t outgoing = *first;
        if (!charge(1))
            return false;
        ++result_.telemetry.angular_predicates;
        std::optional<std::int8_t> orientation = orientation_at(outgoing);
        if (!orientation)
        {
            if (!charge(static_cast<std::uint64_t>(last - first)))
                return false;
            for (auto at = first + 1; at != last; ++at)
            {
                ++result_.telemetry.angular_predicates;
                const std::optional<std::int8_t> candidate = orientation_at(*at);
                if (!merge_certified_cycle_orientation(candidate, orientation))
                    return fail(AnalyticFilteredArrangementError::invalid_argument);
            }
        }
        if (!orientation)
            return fail_unresolved();
        if (*orientation == 0)
            return fail(AnalyticFilteredArrangementError::invalid_argument);
        if (result_.cycles.size() == std::numeric_limits<std::uint32_t>::max())
            return fail(AnalyticFilteredArrangementError::resource_limit_exceeded);

        const std::uint32_t cycle = static_cast<std::uint32_t>(result_.cycles.size());
        const std::uint32_t count =
            static_cast<std::uint32_t>(result_.cycle_half_edges.size() - cycle_begin);
        const std::uint32_t root = components.find(result_.half_edges[outgoing].origin_vertex);
        result_.cycles.push_back({static_cast<std::uint32_t>(cycle_begin), count,
                                  component_by_root[root], *orientation < 0});
        for (std::size_t index = cycle_begin; index < result_.cycle_half_edges.size(); ++index)
            result_.half_edges[result_.cycle_half_edges[index]].cycle = cycle;
        return true;
    }

    bool build_cycles()
    {
        const std::uint64_t basic_work =
            result_.vertices.size() + result_.edges.size() + result_.half_edges.size() * 5;
        if (!charge(basic_work))
            return false;
        DisjointSet components(result_.vertices.size());
        std::vector<std::uint32_t> component_by_root(result_.vertices.size(), kNoIndex);
        build_components(components, component_by_root);

        std::vector<bool> visited(result_.half_edges.size());
        result_.cycles.reserve(result_.half_edges.size());
        result_.cycle_half_edges.reserve(result_.half_edges.size());
        for (std::uint32_t start = 0; start < result_.half_edges.size(); ++start)
        {
            if (visited[start])
                continue;
            const std::size_t cycle_begin = result_.cycle_half_edges.size();
            if (!trace_cycle(start, visited, cycle_begin) ||
                !publish_cycle(cycle_begin, components, component_by_root))
                return false;
        }
        result_.telemetry.emitted_cycles = result_.cycles.size();
        return true;
    }

    const AnalyticFilteredGeometry& geometry_;
    const AnalyticFilteredOverlayResult& overlay_;
    AnalyticSolverLimits limits_;
    analytic_execution_detail::TopologyPolicy policy_;
    AnalyticFilteredArrangementResult result_;
    std::vector<EndpointRecord> endpoints_;
    std::vector<std::uint32_t> endpoint_vertices_;
    std::vector<VertexCluster> clusters_;
    std::vector<std::uint8_t> curve_referenced_;
    std::vector<std::uint32_t> collapsed_curve_indices_;
    std::vector<EdgeTangentWitness> edge_tangent_witnesses_;
    std::vector<double> outgoing_tangent_angles_;
    std::vector<std::uint64_t> outgoing_tangent_ids_;
    std::vector<std::uint8_t> outgoing_tangent_projected_;
    std::vector<std::uint32_t> tangent_class_half_edges_;
};

static_assert(sizeof(EndpointRecord) <= kEndpointLogicalBytes);
static_assert(sizeof(VertexCluster) <= kClusterLogicalBytes);
static_assert(sizeof(ExpiryEntry) <= kExpiryLogicalBytes);
static_assert(sizeof(EdgeDraft) <= kEdgeDraftLogicalBytes);
static_assert(sizeof(CollapsedDraft) <= kCollapsedSpanLogicalBytes);
static_assert(sizeof(AnalyticArrangementVertexNm) <= kVertexLogicalBytes);
static_assert(sizeof(AnalyticArrangementEdgeNm) <= kEdgeLogicalBytes);
static_assert(sizeof(AnalyticArrangementHalfEdge) <= kHalfEdgeLogicalBytes);
static_assert(sizeof(AnalyticArrangementCollapsedSpan) <= kCollapsedSpanLogicalBytes);
static_assert(sizeof(AnalyticArrangementCycle) <= kCycleLogicalBytes);
