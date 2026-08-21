use geometer_client::contracts::{
    AnalyticPlanarBooleanJobResult, AnalyticPlanarOperand, ArcDirection,
    AuthoredCircularArcSegment, AuthoredLineSegment, AuthoredPathSegment, CurveId, DiskOperand,
    FailedJobResult, FeatureId, JobId, OperandId, PointNm, SegmentId, StageId, Validate,
};

#[test]
fn analytic_logical_dtos_preserve_integer_domains_and_closed_unions() {
    assert!(JobId::new(0).is_none());
    assert_eq!(JobId::new(1).unwrap().get(), 1);
    assert_eq!(JobId::new(u64::MAX).unwrap().get(), u64::MAX);
    assert!(StageId::new(0).is_none());

    let point = PointNm {
        x: i64::MIN,
        y: i64::MAX,
    };
    assert_eq!((point.x, point.y), (i64::MIN, i64::MAX));

    let disk = DiskOperand {
        operand_id: OperandId::new(1).unwrap(),
        kind: "disk".to_owned(),
        feature_id: FeatureId::new(1).unwrap(),
        center: point,
        radius_nm: 200_000,
    };
    disk.validate_at("").unwrap();
    let operand = AnalyticPlanarOperand::Disk(disk);
    assert!(matches!(operand, AnalyticPlanarOperand::Disk(_)));

    let failed = AnalyticPlanarBooleanJobResult::Failure(FailedJobResult {
        job_id: JobId::new(1).unwrap(),
        status: "failure".to_owned(),
        diagnostics: Vec::new(),
        digest_sha256: "0".repeat(64),
    });
    assert!(matches!(failed, AnalyticPlanarBooleanJobResult::Failure(_)));

    let path_segments = [
        AuthoredPathSegment::Line(AuthoredLineSegment {
            segment_id: SegmentId::new(1).unwrap(),
            curve_id: CurveId::new(1).unwrap(),
            kind: "line".to_owned(),
        }),
        AuthoredPathSegment::CircularArc(AuthoredCircularArcSegment {
            segment_id: SegmentId::new(2).unwrap(),
            curve_id: CurveId::new(2).unwrap(),
            kind: "circular_arc".to_owned(),
            center: PointNm { x: 0, y: 0 },
            direction: ArcDirection::Ccw,
            major_arc: false,
        }),
    ];
    for segment in path_segments {
        match segment {
            AuthoredPathSegment::Line(value) => value.validate_at("").unwrap(),
            AuthoredPathSegment::CircularArc(value) => value.validate_at("").unwrap(),
        }
    }
}

#[test]
fn analytic_logical_validation_rejects_invalid_literals_and_bounds() {
    let mut disk = DiskOperand {
        operand_id: OperandId::new(1).unwrap(),
        kind: "not-disk".to_owned(),
        feature_id: FeatureId::new(1).unwrap(),
        center: PointNm { x: 0, y: 0 },
        radius_nm: 1,
    };
    assert!(disk.validate_at("").is_err());
    disk.kind = "disk".to_owned();
    disk.radius_nm = 1_000_000_000_001;
    assert!(disk.validate_at("").is_err());
}
