use super::{derive_ownership, least_rotation};
use crate::analytic_packet_a0::result::records::{
    JobRecord, RegionRecord, ResultRecords, RingRecord,
};

#[test]
fn least_rotation_handles_repeated_prefixes_and_long_adversarial_inputs() {
    assert_eq!(least_rotation(&[]), 0);
    assert_eq!(least_rotation(&[1, 2, 1, 2, 1, 1]), 4);
    assert_eq!(least_rotation(&[2, 2, 2, 1, 2, 2, 2, 1]), 3);
    let mut repeated = vec![1_u32; 20_000];
    repeated[19_999] = 0;
    assert_eq!(least_rotation(&repeated), 19_999);
}

#[test]
fn nested_region_cannot_cross_job_ownership() {
    let mut records = empty_records();
    records.jobs = vec![job(1, 0), job(2, 1)];
    records.rings = vec![ring(1, u32::MAX, 0), ring(2, 0, 1), ring(3, 1, 2)];
    records.regions = vec![region(1, 0), region(2, 2)];
    assert!(derive_ownership(&records).is_err());
}

fn empty_records() -> ResultRecords {
    ResultRecords {
        jobs: Vec::new(),
        diagnostics: Vec::new(),
        vertices: Vec::new(),
        fragments: Vec::new(),
        rings: Vec::new(),
        fragment_references: Vec::new(),
        regions: Vec::new(),
        ring_region_references: Vec::new(),
        source_sets: Vec::new(),
        sources: Vec::new(),
        events: Vec::new(),
        relationships: Vec::new(),
        pairs: Vec::new(),
        source_indices: Vec::new(),
    }
}

fn job(id: u64, region_begin: u32) -> JobRecord {
    JobRecord {
        id,
        status: 0,
        diagnostic_begin: 0,
        diagnostic_count: 0,
        region_begin,
        region_count: 1,
        event_begin: 0,
        event_count: 0,
    }
}

fn ring(id: u64, parent: u32, depth: u32) -> RingRecord {
    RingRecord {
        id,
        reference_begin: 0,
        reference_count: 0,
        parent,
        depth,
        flags: depth % 2,
    }
}

fn region(id: u64, outer: u32) -> RegionRecord {
    RegionRecord {
        id,
        outer,
        positive_set: 1,
        flags: 0,
    }
}
