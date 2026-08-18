use std::path::{Path, PathBuf};

use geometer_client::contracts::{
    AnalyticPlanarBooleanBatchRequestA0, AnalyticPlanarBooleanJobResult,
};
use geometer_client::{
    AnalyticPacketErrorKind, decode_analytic_planar_boolean_batch_result_a0_packet,
    encode_analytic_planar_boolean_batch_request_a0_packet,
};

#[test]
fn empty_request_matches_the_governed_exact_bytes() {
    let request = AnalyticPlanarBooleanBatchRequestA0 {
        jobs: Vec::new(),
        relationship_queries: Vec::new(),
    };
    let encoded = encode_analytic_planar_boolean_batch_request_a0_packet(&request).unwrap();
    assert_eq!(encoded, vector("request-empty.hex"));
    assert_eq!(encoded.len(), 480);
}

#[test]
fn full_logical_request_matches_the_governed_exemplar_bytes() {
    let encoded =
        encode_analytic_planar_boolean_batch_request_a0_packet(&exemplar_request()).unwrap();
    assert_eq!(encoded, vector("request-exemplar.hex"));
    assert_eq!(encoded.len(), 1608);
}

#[test]
fn governed_result_vectors_decode_with_normative_job_digests() {
    let canonical = decode_analytic_planar_boolean_batch_result_a0_packet(&vector(
        "result-canonical-mixed.hex",
    ))
    .unwrap();
    assert_eq!(canonical.job_results.len(), 2);
    let success = canonical
        .job_results
        .iter()
        .find_map(|value| match value {
            AnalyticPlanarBooleanJobResult::Success(value) => Some(value),
            AnalyticPlanarBooleanJobResult::Failure(_) => None,
        })
        .unwrap();
    let failure = canonical
        .job_results
        .iter()
        .find_map(|value| match value {
            AnalyticPlanarBooleanJobResult::Failure(value) => Some(value),
            AnalyticPlanarBooleanJobResult::Success(_) => None,
        })
        .unwrap();
    assert_eq!(
        success.digest_sha256,
        "efea77545331219e62e187228f7fcd73be961549b48ee439464aa161d61c90a3"
    );
    assert_eq!(
        failure.digest_sha256,
        "87f193938f0c1d60d8d196eb256adf0fa423f56175be63a61f0ef6cc8bebd46e"
    );
    for name in [
        "result-success-standalone.hex",
        "result-mixed-success-standalone.hex",
        "result-failure-standalone.hex",
    ] {
        decode_analytic_planar_boolean_batch_result_a0_packet(&vector(name)).unwrap();
    }
}

#[test]
fn governed_result_mutation_families_are_rejected() {
    let canonical = vector("result-canonical-mixed.hex");
    let mut bad_magic = canonical.clone();
    bad_magic[0] = b'X';
    require_rejected(&bad_magic, AnalyticPacketErrorKind::InvalidPacket);
    require_rejected(
        &canonical[..canonical.len() - 1],
        AnalyticPacketErrorKind::InvalidPacket,
    );

    let mut reserved = canonical.clone();
    reserved[12] = 1;
    require_rejected(&reserved, AnalyticPacketErrorKind::InvalidPacket);

    let mut duplicate_job = canonical.clone();
    let job_offset = table_offset(&duplicate_job, 0);
    duplicate_job[job_offset + 48..job_offset + 56]
        .copy_from_slice(&canonical[job_offset..job_offset + 8]);
    require_rejected(&duplicate_job, AnalyticPacketErrorKind::InvalidId);

    let mut invalid_source = canonical;
    let source_offset = table_offset(&invalid_source, 9);
    let source_count = table_count(&invalid_source, 9);
    let authored = (0..source_count)
        .find(|index| u16_at(&invalid_source, source_offset + index * 32) == 1)
        .unwrap();
    invalid_source[source_offset + authored * 32 + 24..source_offset + authored * 32 + 32].fill(0);
    require_rejected(&invalid_source, AnalyticPacketErrorKind::InvalidPacket);
}

#[test]
fn governed_result_graph_and_semantic_mutations_are_rejected() {
    let canonical = vector("result-canonical-mixed.hex");

    let mut duplicate_fragment = canonical.clone();
    let references = table_offset(&duplicate_fragment, 5);
    put_u32(&mut duplicate_fragment, references + 4, 0);
    require_any_rejection(&duplicate_fragment);

    let mut disconnected_ring = canonical.clone();
    put_u32(&mut disconnected_ring, references + 4, 3);
    put_u32(&mut disconnected_ring, references + 8, 2);
    require_any_rejection(&disconnected_ring);

    let mut zero_region_source = canonical.clone();
    let regions = table_offset(&zero_region_source, 6);
    put_u32(&mut zero_region_source, regions + 12, 0);
    require_any_rejection(&zero_region_source);

    let mut invalid_primitive_source = canonical.clone();
    let sources = table_offset(&invalid_primitive_source, 9);
    invalid_primitive_source[sources..sources + 2].copy_from_slice(&2_u16.to_le_bytes());
    invalid_primitive_source[sources + 2..sources + 4].copy_from_slice(&48_u16.to_le_bytes());
    require_any_rejection(&invalid_primitive_source);

    let mut duplicate_event_reference = canonical.clone();
    let result_references = table_offset(&duplicate_event_reference, 7);
    let reference = duplicate_event_reference[result_references..result_references + 8].to_vec();
    insert_table_record(&mut duplicate_event_reference, 7, &reference);
    let events = table_offset(&duplicate_event_reference, 10);
    put_u32(&mut duplicate_event_reference, events + 16, 2);
    require_any_rejection(&duplicate_event_reference);

    let mut bad_aggregate = canonical.clone();
    let relationships = table_offset(&bad_aggregate, 11);
    bad_aggregate[relationships + 9] = 2;
    require_any_rejection(&bad_aggregate);

    let mut out_of_range_pair = canonical.clone();
    let pairs = table_offset(&out_of_range_pair, 12);
    put_u64(&mut out_of_range_pair, pairs, 2);
    require_any_rejection(&out_of_range_pair);

    let mut inconsistent_pair_flags = canonical.clone();
    inconsistent_pair_flags[relationships + 9] = 2;
    inconsistent_pair_flags[pairs + 16] = 2;
    require_any_rejection(&inconsistent_pair_flags);

    let mut noncanonical_vertex = canonical.clone();
    let vertices = table_offset(&noncanonical_vertex, 2);
    noncanonical_vertex[vertices + 8..vertices + 16].copy_from_slice(&20_i64.to_le_bytes());
    require_any_rejection(&noncanonical_vertex);

    let mut extreme_arc = canonical.clone();
    let vertices = table_offset(&extreme_arc, 2);
    extreme_arc[vertices + 8..vertices + 16].copy_from_slice(&i64::MIN.to_le_bytes());
    extreme_arc[vertices + 2 * 32 + 8..vertices + 2 * 32 + 16]
        .copy_from_slice(&i64::MAX.to_le_bytes());
    let fragments = table_offset(&extreme_arc, 3);
    extreme_arc[fragments + 16] = 2;
    extreme_arc[fragments + 17] = 1;
    put_u64(&mut extreme_arc, fragments + 24, 1_000_000_000_000);
    require_any_rejection(&extreme_arc);

    let mut unused_source = canonical;
    let mut extra_source = vec![0_u8; 32];
    extra_source[..2].copy_from_slice(&3_u16.to_le_bytes());
    extra_source[8..16].copy_from_slice(&1_u64.to_le_bytes());
    extra_source[16..24].copy_from_slice(&1_u64.to_le_bytes());
    insert_table_record(&mut unused_source, 9, &extra_source);
    require_any_rejection(&unused_source);
}

fn require_rejected(bytes: &[u8], kind: AnalyticPacketErrorKind) {
    let error = decode_analytic_planar_boolean_batch_result_a0_packet(bytes).unwrap_err();
    assert_eq!(error.kind(), kind);
}

fn require_any_rejection(bytes: &[u8]) {
    decode_analytic_planar_boolean_batch_result_a0_packet(bytes).unwrap_err();
}

fn insert_table_record(bytes: &mut Vec<u8>, table: usize, record: &[u8]) {
    let entry = 64 + table * 32;
    let offset = table_offset(bytes, table);
    let byte_length = u64_at(bytes, entry + 16) as usize;
    let old_payload = u64_at(bytes, 48);
    let old_count = table_count(bytes, table);
    bytes.splice(
        offset + byte_length..offset + byte_length,
        record.iter().copied(),
    );
    let total = bytes.len() as u64;
    put_u64(bytes, 16, total);
    put_u64(bytes, 48, old_payload + record.len() as u64);
    put_u64(bytes, entry + 16, byte_length as u64 + record.len() as u64);
    put_u64(bytes, entry + 24, old_count as u64 + 1);
    for following in table + 1..14 {
        let offset_field = 64 + following * 32 + 8;
        let shifted = u64_at(bytes, offset_field) + record.len() as u64;
        put_u64(bytes, offset_field, shifted);
    }
}

fn table_offset(bytes: &[u8], index: usize) -> usize {
    u64::from_le_bytes(
        bytes[64 + index * 32 + 8..64 + index * 32 + 16]
            .try_into()
            .unwrap(),
    ) as usize
}

fn table_count(bytes: &[u8], index: usize) -> usize {
    u64::from_le_bytes(
        bytes[64 + index * 32 + 24..64 + index * 32 + 32]
            .try_into()
            .unwrap(),
    ) as usize
}

fn u16_at(bytes: &[u8], offset: usize) -> u16 {
    u16::from_le_bytes(bytes[offset..offset + 2].try_into().unwrap())
}

fn u64_at(bytes: &[u8], offset: usize) -> u64 {
    u64::from_le_bytes(bytes[offset..offset + 8].try_into().unwrap())
}

fn put_u32(bytes: &mut [u8], offset: usize, value: u32) {
    bytes[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
}

fn put_u64(bytes: &mut [u8], offset: usize, value: u64) {
    bytes[offset..offset + 8].copy_from_slice(&value.to_le_bytes());
}

fn vector(name: &str) -> Vec<u8> {
    let text = std::fs::read_to_string(
        repository_root()
            .join("tests/contracts/vectors/analytic")
            .join(name),
    )
    .unwrap();
    let text = text.trim();
    (0..text.len())
        .step_by(2)
        .map(|index| u8::from_str_radix(&text[index..index + 2], 16).unwrap())
        .collect()
}

fn repository_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../..")
        .canonicalize()
        .unwrap()
}

#[allow(
    clippy::too_many_lines,
    reason = "the fixture deliberately mirrors the governed native exemplar in one place"
)]
fn exemplar_request() -> AnalyticPlanarBooleanBatchRequestA0 {
    use geometer_client::contracts as c;
    let (outer, hole, path) = exemplar_geometry();
    let first = c::AnalyticPlanarBooleanJob {
        job_id: c::JobId::new(10).unwrap(),
        stages: vec![
            c::AnalyticPlanarBooleanStage {
                stage_id: c::StageId::new(100).unwrap(),
                operation: c::StageOperation::UnionStage,
                operands: vec![
                    c::AnalyticPlanarOperand::PlanarRegion(c::PlanarRegionOperand {
                        operand_id: c::OperandId::new(1000).unwrap(),
                        kind: "planar_region".to_owned(),
                        region_id: c::RegionId::new(500).unwrap(),
                        outer,
                        holes: vec![hole],
                    }),
                    c::AnalyticPlanarOperand::Disk(c::DiskOperand {
                        operand_id: c::OperandId::new(1001).unwrap(),
                        kind: "disk".to_owned(),
                        feature_id: c::FeatureId::new(300).unwrap(),
                        center: c::PointNm { x: 1, y: 2 },
                        radius_nm: 250,
                    }),
                    c::AnalyticPlanarOperand::Annulus(c::AnnulusOperand {
                        operand_id: c::OperandId::new(1002).unwrap(),
                        kind: "annulus".to_owned(),
                        feature_id: c::FeatureId::new(310).unwrap(),
                        center: c::PointNm { x: 0, y: 0 },
                        inner_radius_nm: 100,
                        outer_radius_nm: 200,
                    }),
                ],
            },
            c::AnalyticPlanarBooleanStage {
                stage_id: c::StageId::new(101).unwrap(),
                operation: c::StageOperation::Difference,
                operands: vec![
                    c::AnalyticPlanarOperand::Capsule(c::CapsuleOperand {
                        operand_id: c::OperandId::new(1003).unwrap(),
                        kind: "capsule".to_owned(),
                        feature_id: c::FeatureId::new(320).unwrap(),
                        start: c::PointNm { x: 0, y: 0 },
                        end: c::PointNm { x: 1000, y: 0 },
                        width_nm: 50,
                    }),
                    c::AnalyticPlanarOperand::SweptPath(c::SweptPathOperand {
                        operand_id: c::OperandId::new(1004).unwrap(),
                        kind: "swept_path".to_owned(),
                        feature_id: c::FeatureId::new(330).unwrap(),
                        centerline: path,
                        width_nm: 40,
                        cap: "round".to_owned(),
                        join: "round".to_owned(),
                    }),
                ],
            },
        ],
    };
    exemplar_batch(first)
}

fn exemplar_geometry() -> (
    geometer_client::contracts::PlanarRing,
    geometer_client::contracts::PlanarRing,
    geometer_client::contracts::PlanarPath,
) {
    use geometer_client::contracts as c;
    let outer = c::PlanarRing {
        ring_id: c::RingId::new(600).unwrap(),
        vertices: vec![
            vertex(700, 0, 0),
            vertex(701, 10_000, 0),
            vertex(702, 10_000, 10_000),
            vertex(703, 0, 10_000),
        ],
        segments: vec![
            line(800, 900),
            line(801, 901),
            line(802, 902),
            line(803, 903),
        ],
    };
    let hole = c::PlanarRing {
        ring_id: c::RingId::new(601).unwrap(),
        vertices: vec![vertex(704, 4_000, 5_000), vertex(705, 6_000, 5_000)],
        segments: vec![arc(804, 904), arc(805, 904)],
    };
    let path = c::PlanarPath {
        path_id: c::PathId::new(602).unwrap(),
        vertices: vec![vertex(706, 20_000, 20_000), vertex(707, 30_000, 20_000)],
        segments: vec![line(806, 906)],
    };
    (outer, hole, path)
}

fn exemplar_batch(
    first: geometer_client::contracts::AnalyticPlanarBooleanJob,
) -> AnalyticPlanarBooleanBatchRequestA0 {
    use geometer_client::contracts as c;
    let second = c::AnalyticPlanarBooleanJob {
        job_id: c::JobId::new(20).unwrap(),
        stages: vec![c::AnalyticPlanarBooleanStage {
            stage_id: c::StageId::new(102).unwrap(),
            operation: c::StageOperation::UnionStage,
            operands: vec![c::AnalyticPlanarOperand::Disk(c::DiskOperand {
                operand_id: c::OperandId::new(1005).unwrap(),
                kind: "disk".to_owned(),
                feature_id: c::FeatureId::new(301).unwrap(),
                center: c::PointNm { x: -5, y: -6 },
                radius_nm: 1_000_000_000_000,
            })],
        }],
    };
    AnalyticPlanarBooleanBatchRequestA0 {
        jobs: vec![first, second],
        relationship_queries: vec![
            c::PlanarRelationshipQuery {
                query_id: c::QueryId::new(5000).unwrap(),
                left_job_id: c::JobId::new(10).unwrap(),
                right_job_id: c::JobId::new(20).unwrap(),
            },
            c::PlanarRelationshipQuery {
                query_id: c::QueryId::new(5001).unwrap(),
                left_job_id: c::JobId::new(20).unwrap(),
                right_job_id: c::JobId::new(10).unwrap(),
            },
        ],
    }
}

fn vertex(id: u64, x: i64, y: i64) -> geometer_client::contracts::AuthoredVertex {
    use geometer_client::contracts as c;
    c::AuthoredVertex {
        vertex_id: c::VertexId::new(id).unwrap(),
        point: c::PointNm { x, y },
    }
}

fn line(id: u64, curve: u64) -> geometer_client::contracts::AuthoredSegment {
    use geometer_client::contracts as c;
    c::AuthoredSegment::Line(c::AuthoredLineSegment {
        segment_id: c::SegmentId::new(id).unwrap(),
        curve_id: c::CurveId::new(curve).unwrap(),
        kind: "line".to_owned(),
    })
}

fn arc(id: u64, curve: u64) -> geometer_client::contracts::AuthoredSegment {
    use geometer_client::contracts as c;
    c::AuthoredSegment::CircularArc(c::AuthoredCircularArcSegment {
        segment_id: c::SegmentId::new(id).unwrap(),
        curve_id: c::CurveId::new(curve).unwrap(),
        kind: "circular_arc".to_owned(),
        center: c::PointNm { x: 5000, y: 5000 },
        direction: c::ArcDirection::Ccw,
        major_arc: false,
    })
}
