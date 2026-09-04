use crate::generated::contracts::{
    AnalyticPlanarBooleanBatchResultA0, AnalyticPlanarBooleanJobResult, ArcDirection,
    DiagnosticSeverity, DirectedFragment, FailedJobResult, IntersectionDimension, JobDiagnostic,
    JobDiagnosticCode, JobDiagnosticPath, JobId, OperandId, OperandOutcomeEvent,
    OperandOutcomeKind, PlanarRelationshipResult, PointNm, QueryId, RelationshipRegionPair,
    RelationshipStatus, ResultCircularArcFragment, ResultFragmentId, ResultLineFragment,
    ResultRegion, ResultRegionId, ResultRing, ResultRingId, ResultVertex, ResultVertexId,
    SourceKind, SourceReference, SourceRole, SourceSet, StageId, SuccessfulJobResult,
};

use super::super::error::{AnalyticPacketError, invalid_id, invalid_packet, invalid_reference};
use super::records::{DiagnosticRecord, ResultRecords, SourceRecord};
use super::standalone::Selection;
use super::validate::range;

const NONE: u32 = u32::MAX;

pub(crate) fn project(
    records: &ResultRecords,
    selections: &[Selection],
    digests: &[String],
) -> Result<AnalyticPlanarBooleanBatchResultA0, AnalyticPacketError> {
    let job_results = records
        .jobs
        .iter()
        .enumerate()
        .map(|(index, _)| {
            project_job(
                records,
                index,
                selections
                    .get(index)
                    .ok_or_else(|| invalid_reference("job selection is missing"))?,
                digests
                    .get(index)
                    .ok_or_else(|| invalid_reference("job digest is missing"))?,
            )
        })
        .collect::<Result<_, _>>()?;
    let relationship_results = records
        .relationships
        .iter()
        .map(|value| project_relationship(records, value))
        .collect::<Result<_, _>>()?;
    Ok(AnalyticPlanarBooleanBatchResultA0 {
        job_results,
        relationship_results,
    })
}

fn project_job(
    records: &ResultRecords,
    index: usize,
    selected: &Selection,
    digest: &str,
) -> Result<AnalyticPlanarBooleanJobResult, AnalyticPacketError> {
    let job = records
        .jobs
        .get(index)
        .ok_or_else(|| invalid_reference("job record is missing"))?;
    let job_id = JobId::new(job.id).ok_or_else(|| invalid_id("job id is zero"))?;
    let diagnostics = range(
        &records.diagnostics,
        job.diagnostic_begin,
        job.diagnostic_count,
    )?
    .iter()
    .map(project_diagnostic)
    .collect::<Result<Vec<_>, _>>()?;
    if job.status == 1 {
        return Ok(AnalyticPlanarBooleanJobResult::Failure(FailedJobResult {
            job_id,
            status: "failure".to_owned(),
            diagnostics,
            digest_sha256: digest.to_owned(),
        }));
    }
    Ok(AnalyticPlanarBooleanJobResult::Success(
        SuccessfulJobResult {
            job_id,
            status: "success".to_owned(),
            diagnostics,
            vertices: project_vertices(records, selected)?,
            directed_fragments: project_fragments(records, selected)?,
            rings: project_rings(records, selected)?,
            result_regions: project_regions(records, selected)?,
            operand_outcomes: project_events(records, selected)?,
            digest_sha256: digest.to_owned(),
        },
    ))
}

fn project_vertices(
    records: &ResultRecords,
    selected: &Selection,
) -> Result<Vec<ResultVertex>, AnalyticPacketError> {
    selected
        .vertices
        .iter()
        .map(|index| {
            let value = &records.vertices[*index];
            Ok(ResultVertex {
                vertex_id: ResultVertexId::new(value.id)
                    .ok_or_else(|| invalid_id("result vertex id is zero"))?,
                point: PointNm {
                    x: value.x,
                    y: value.y,
                },
                intersection_sources: project_source_set(records, value.source_set)?,
            })
        })
        .collect()
}

fn project_fragments(
    records: &ResultRecords,
    selected: &Selection,
) -> Result<Vec<DirectedFragment>, AnalyticPacketError> {
    selected
        .fragments
        .iter()
        .map(|index| {
            let value = &records.fragments[*index];
            let fragment_id = ResultFragmentId::new(value.id)
                .ok_or_else(|| invalid_id("result fragment id is zero"))?;
            let start_vertex_id = vertex_id(records, value.start)?;
            let end_vertex_id = vertex_id(records, value.end)?;
            let positive = project_source_set(records, value.positive_set)?;
            let subtraction = project_source_set(records, value.subtraction_set)?;
            if value.kind == 1 {
                Ok(DirectedFragment::Line(ResultLineFragment {
                    fragment_id,
                    kind: "line".to_owned(),
                    start_vertex_id,
                    end_vertex_id,
                    coincident_positive_sources: positive,
                    surviving_subtraction_sources: subtraction,
                }))
            } else {
                Ok(DirectedFragment::CircularArc(ResultCircularArcFragment {
                    fragment_id,
                    kind: "circular_arc".to_owned(),
                    start_vertex_id,
                    end_vertex_id,
                    radius_nm: value.radius,
                    direction: if value.direction == 1 {
                        ArcDirection::Ccw
                    } else {
                        ArcDirection::Cw
                    },
                    major_arc: value.major == 1,
                    coincident_positive_sources: positive,
                    surviving_subtraction_sources: subtraction,
                }))
            }
        })
        .collect()
}

fn project_rings(
    records: &ResultRecords,
    selected: &Selection,
) -> Result<Vec<ResultRing>, AnalyticPacketError> {
    selected
        .rings
        .iter()
        .map(|index| {
            let value = &records.rings[*index];
            let fragment_ids = range(
                &records.fragment_references,
                value.reference_begin,
                value.reference_count,
            )?
            .iter()
            .map(|reference| {
                let fragment = records
                    .fragments
                    .get(*reference as usize)
                    .ok_or_else(|| invalid_reference("ring fragment is missing"))?;
                ResultFragmentId::new(fragment.id)
                    .ok_or_else(|| invalid_id("ring fragment id is zero"))
            })
            .collect::<Result<_, _>>()?;
            let parent_ring_id = if value.parent == NONE {
                None
            } else {
                let parent = records
                    .rings
                    .get(value.parent as usize)
                    .ok_or_else(|| invalid_reference("parent ring is missing"))?;
                Some(
                    ResultRingId::new(parent.id)
                        .ok_or_else(|| invalid_id("parent ring id is zero"))?,
                )
            };
            Ok(ResultRing {
                ring_id: ResultRingId::new(value.id)
                    .ok_or_else(|| invalid_id("result ring id is zero"))?,
                fragment_ids,
                parent_ring_id,
                depth: value.depth,
                hole: value.flags & 1 != 0,
            })
        })
        .collect()
}

fn project_regions(
    records: &ResultRecords,
    selected: &Selection,
) -> Result<Vec<ResultRegion>, AnalyticPacketError> {
    selected
        .regions
        .iter()
        .map(|index| {
            let value = &records.regions[*index];
            let outer = records
                .rings
                .get(value.outer as usize)
                .ok_or_else(|| invalid_reference("region outer ring is missing"))?;
            Ok(ResultRegion {
                result_region_id: ResultRegionId::new(value.id)
                    .ok_or_else(|| invalid_id("result region id is zero"))?,
                outer_ring_id: ResultRingId::new(outer.id)
                    .ok_or_else(|| invalid_id("outer ring id is zero"))?,
                positive_contributors: project_source_set(records, value.positive_set)?,
            })
        })
        .collect()
}

fn project_events(
    records: &ResultRecords,
    selected: &Selection,
) -> Result<Vec<OperandOutcomeEvent>, AnalyticPacketError> {
    selected
        .events
        .iter()
        .map(|index| {
            let value = &records.events[*index];
            let mut result_ring_ids = Vec::new();
            let mut result_region_ids = Vec::new();
            for reference in range(
                &records.ring_region_references,
                value.reference_begin,
                value.reference_count,
            )? {
                let kind = reference >> 32;
                let target = (*reference & u64::from(u32::MAX)) as usize;
                if kind == 1 {
                    let ring = records
                        .rings
                        .get(target)
                        .ok_or_else(|| invalid_reference("event ring is missing"))?;
                    result_ring_ids.push(
                        ResultRingId::new(ring.id)
                            .ok_or_else(|| invalid_id("event ring id is zero"))?,
                    );
                } else {
                    let region = records
                        .regions
                        .get(target)
                        .ok_or_else(|| invalid_reference("event region is missing"))?;
                    result_region_ids.push(
                        ResultRegionId::new(region.id)
                            .ok_or_else(|| invalid_id("event region id is zero"))?,
                    );
                }
            }
            Ok(OperandOutcomeEvent {
                operand_id: OperandId::new(value.operand_id)
                    .ok_or_else(|| invalid_id("event operand id is zero"))?,
                kind: outcome_kind(value.kind)?,
                result_ring_ids,
                result_region_ids,
                sources: project_source_set(records, value.source_set)?,
            })
        })
        .collect()
}

fn project_diagnostic(value: &DiagnosticRecord) -> Result<JobDiagnostic, AnalyticPacketError> {
    Ok(JobDiagnostic {
        code: diagnostic_code(value.code)?,
        severity: if value.severity == 1 {
            DiagnosticSeverity::Error
        } else {
            DiagnosticSeverity::Warning
        },
        job_id: JobId::new(value.job_id).ok_or_else(|| invalid_id("diagnostic job id is zero"))?,
        stage_id: optional_id(value.stage_id, StageId::new)?,
        operand_id: optional_id(value.operand_id, OperandId::new)?,
        geometry_id: (value.geometry_id != 0).then_some(value.geometry_id),
        path_identity: path_identity(value.path_token)?,
    })
}

fn project_source_set(
    records: &ResultRecords,
    handle: u32,
) -> Result<SourceSet, AnalyticPacketError> {
    if handle == 0 {
        return Ok(SourceSet {
            sources: Vec::new(),
        });
    }
    let set = records
        .source_sets
        .get(handle as usize - 1)
        .ok_or_else(|| invalid_reference("source-set handle is invalid"))?;
    let sources = range(&records.source_indices, set.begin, set.count)?
        .iter()
        .map(|index| {
            let value = records
                .sources
                .get(*index as usize)
                .ok_or_else(|| invalid_reference("source reference is missing"))?;
            project_source(value)
        })
        .collect::<Result<_, _>>()?;
    Ok(SourceSet { sources })
}

fn project_source(value: &SourceRecord) -> Result<SourceReference, AnalyticPacketError> {
    let kind = match value.kind {
        1 => SourceKind::AuthoredSegmentCurve,
        2 => SourceKind::CompactFeatureRole,
        3 => SourceKind::SubtractiveOperandEffect,
        _ => return Err(invalid_packet("unknown source kind")),
    };
    Ok(SourceReference {
        kind,
        role: source_role(value.role)?,
        operand_id: OperandId::new(value.operand_id)
            .ok_or_else(|| invalid_id("source operand id is zero"))?,
        primary_id: value.primary_id,
        secondary_id: value.secondary_id,
    })
}

fn project_relationship(
    records: &ResultRecords,
    value: &super::records::RelationshipRecord,
) -> Result<PlanarRelationshipResult, AnalyticPacketError> {
    let pairs = range(&records.pairs, value.pair_begin, value.pair_count)?
        .iter()
        .map(|pair| {
            Ok(RelationshipRegionPair {
                left_result_region_id: ResultRegionId::new(pair.left)
                    .ok_or_else(|| invalid_id("left relationship region id is zero"))?,
                right_result_region_id: ResultRegionId::new(pair.right)
                    .ok_or_else(|| invalid_id("right relationship region id is zero"))?,
                dimension: dimension(pair.dimension)?,
                equality: pair.equality == 1,
                left_contains_right: pair.left_contains == 1,
                right_contains_left: pair.right_contains == 1,
            })
        })
        .collect::<Result<_, _>>()?;
    Ok(PlanarRelationshipResult {
        query_id: QueryId::new(value.query_id)
            .ok_or_else(|| invalid_id("relationship query id is zero"))?,
        status: if value.status == 0 {
            RelationshipStatus::Success
        } else {
            RelationshipStatus::SkippedDependencyFailed
        },
        aggregate_dimension: dimension(value.dimension)?,
        pairs,
    })
}

fn vertex_id(records: &ResultRecords, index: u32) -> Result<ResultVertexId, AnalyticPacketError> {
    let value = records
        .vertices
        .get(index as usize)
        .ok_or_else(|| invalid_reference("fragment vertex is missing"))?;
    ResultVertexId::new(value.id).ok_or_else(|| invalid_id("fragment vertex id is zero"))
}

fn optional_id<T>(
    value: u64,
    make: impl Fn(u64) -> Option<T>,
) -> Result<Option<T>, AnalyticPacketError> {
    if value == 0 {
        Ok(None)
    } else {
        make(value)
            .map(Some)
            .ok_or_else(|| invalid_id("optional trusted id is invalid"))
    }
}

fn diagnostic_code(value: u32) -> Result<JobDiagnosticCode, AnalyticPacketError> {
    Ok(match value {
        0x10003 => JobDiagnosticCode::InvalidTopology,
        0x10004 => JobDiagnosticCode::InvalidArc,
        0x10005 => JobDiagnosticCode::UnsupportedGeometry,
        0x10007 => JobDiagnosticCode::NormalizationErrorExceeded,
        0x10008 => JobDiagnosticCode::NormalizationTopologyCollapse,
        0x10009 => JobDiagnosticCode::NonanalyticResult,
        0x1000a => JobDiagnosticCode::SolverFailed,
        0x1000b => JobDiagnosticCode::ResourceLimitExceeded,
        0x1000c => JobDiagnosticCode::ResolutionCoalesced,
        _ => return Err(invalid_packet("unknown diagnostic code")),
    })
}

fn outcome_kind(value: u16) -> Result<OperandOutcomeKind, AnalyticPacketError> {
    Ok(match value {
        1 => OperandOutcomeKind::ContributesFinalMaterial,
        2 => OperandOutcomeKind::RedundantOrAbsorbedCoverage,
        3 => OperandOutcomeKind::PartiallyRemovedLater,
        4 => OperandOutcomeKind::CompletelyRemovedLater,
        5 => OperandOutcomeKind::SubtractionEffectSurvives,
        6 => OperandOutcomeKind::SubtractionEffectOverwrittenLater,
        7 => OperandOutcomeKind::NoEffect,
        _ => return Err(invalid_packet("unknown operand outcome kind")),
    })
}

fn dimension(value: u8) -> Result<IntersectionDimension, AnalyticPacketError> {
    Ok(match value {
        0 => IntersectionDimension::Disjoint,
        1 => IntersectionDimension::Point,
        2 => IntersectionDimension::Curve,
        3 => IntersectionDimension::Area,
        _ => return Err(invalid_packet("unknown intersection dimension")),
    })
}

fn source_role(value: u16) -> Result<SourceRole, AnalyticPacketError> {
    let roles = [
        SourceRole::None,
        SourceRole::AuthoredLine,
        SourceRole::AuthoredCircularArc,
        SourceRole::PrimitiveOuterCircle,
        SourceRole::PrimitiveInnerCircle,
        SourceRole::CapsuleLeftLine,
        SourceRole::CapsuleEndCap,
        SourceRole::CapsuleRightLine,
        SourceRole::CapsuleStartCap,
        SourceRole::SweptLeftOffsetLine,
        SourceRole::SweptLeftOffsetArc,
        SourceRole::SweptRightOffsetLine,
        SourceRole::SweptRightOffsetArc,
        SourceRole::SweptRoundJoin,
        SourceRole::SweptStartCap,
        SourceRole::SweptEndCap,
    ];
    let index = match value {
        0..=2 => usize::from(value),
        16..=17 => usize::from(value - 13),
        32..=35 => usize::from(value - 27),
        48..=54 => usize::from(value - 39),
        _ => return Err(invalid_packet("unknown source role")),
    };
    Ok(roles[index].clone())
}

fn path_identity(value: u32) -> Result<Option<JobDiagnosticPath>, AnalyticPacketError> {
    let paths = [
        JobDiagnosticPath::RequestJobs,
        JobDiagnosticPath::JobId,
        JobDiagnosticPath::JobStages,
        JobDiagnosticPath::StageId,
        JobDiagnosticPath::StageOperation,
        JobDiagnosticPath::StageOperands,
        JobDiagnosticPath::OperandId,
        JobDiagnosticPath::OperandGeometry,
        JobDiagnosticPath::RegionOuter,
        JobDiagnosticPath::RegionHoles,
        JobDiagnosticPath::RingVertices,
        JobDiagnosticPath::RingSegments,
        JobDiagnosticPath::PathVertices,
        JobDiagnosticPath::PathSegments,
        JobDiagnosticPath::SegmentCurve,
        JobDiagnosticPath::DiskRadius,
        JobDiagnosticPath::AnnulusInnerRadius,
        JobDiagnosticPath::AnnulusOuterRadius,
        JobDiagnosticPath::CapsuleStart,
        JobDiagnosticPath::CapsuleEnd,
        JobDiagnosticPath::CapsuleWidth,
        JobDiagnosticPath::SweptPathCenterline,
        JobDiagnosticPath::SweptPathWidth,
        JobDiagnosticPath::RelationshipQueries,
        JobDiagnosticPath::RelationshipLeftJobId,
        JobDiagnosticPath::RelationshipRightJobId,
    ];
    if value == 0 {
        Ok(None)
    } else {
        paths
            .into_iter()
            .nth(value as usize - 1)
            .map(Some)
            .ok_or_else(|| invalid_packet("unknown diagnostic path token"))
    }
}
