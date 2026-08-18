use std::collections::HashSet;

use super::super::error::{
    AnalyticPacketError, invalid_id, invalid_packet, invalid_reference, limit_exceeded,
};
use super::records::{DiagnosticRecord, ResultRecords, SourceRecord};

const NONE: u32 = u32::MAX;
const MAX_LENGTH_NM: u64 = 1_000_000_000_000;
const MAX_JOBS: usize = 65_535;
const MAX_QUERIES: usize = 1_048_576;

#[derive(Clone, Debug)]
pub(crate) struct Ownership {
    pub vertices: Vec<usize>,
    pub fragments: Vec<usize>,
    pub rings: Vec<usize>,
    pub regions: Vec<usize>,
    pub events: Vec<usize>,
}

pub(crate) fn validate(records: &ResultRecords) -> Result<Ownership, AnalyticPacketError> {
    if records.jobs.len() > MAX_JOBS || records.relationships.len() > MAX_QUERIES {
        return Err(limit_exceeded(
            "result exceeds its job or relationship limit",
        ));
    }
    validate_jobs(records)?;
    validate_diagnostics(records)?;
    validate_sources(records)?;
    validate_source_sets(records)?;
    validate_geometry(records)?;
    let owners = derive_ownership(records)?;
    validate_events(records, &owners)?;
    validate_relationships(records)?;
    validate_source_exhaustion(records)?;
    validate_canonical_order(records, &owners)?;
    validate_job_spans(records, &owners)?;
    Ok(owners)
}

fn validate_jobs(records: &ResultRecords) -> Result<(), AnalyticPacketError> {
    strictly_increasing(records.jobs.iter().map(|value| value.id), "job-result ids")?;
    partition(
        records
            .jobs
            .iter()
            .map(|value| (value.diagnostic_begin, value.diagnostic_count)),
        records.diagnostics.len(),
        "job diagnostics",
    )?;
    partition(
        records
            .jobs
            .iter()
            .map(|value| (value.region_begin, value.region_count)),
        records.regions.len(),
        "job regions",
    )?;
    partition(
        records
            .jobs
            .iter()
            .map(|value| (value.event_begin, value.event_count)),
        records.events.len(),
        "job events",
    )?;
    for job in &records.jobs {
        if job.id == 0 || job.status > 1 {
            return Err(invalid_packet("unknown job-result status"));
        }
        let diagnostics = range(
            &records.diagnostics,
            job.diagnostic_begin,
            job.diagnostic_count,
        )?;
        if diagnostics.iter().any(|value| value.job_id != job.id) {
            return Err(invalid_reference("diagnostic does not belong to its job"));
        }
        let has_error = diagnostics.iter().any(|value| value.severity == 1);
        if (job.status == 1) != has_error
            || (job.status == 1 && (job.region_count != 0 || job.diagnostic_count == 0))
        {
            return Err(invalid_packet("job status and owned records disagree"));
        }
    }
    Ok(())
}

fn validate_diagnostics(records: &ResultRecords) -> Result<(), AnalyticPacketError> {
    let known = [
        0x10003, 0x10004, 0x10005, 0x10007, 0x10008, 0x10009, 0x1000a, 0x1000b,
    ];
    let mut previous = None;
    for value in &records.diagnostics {
        if !diagnostic_fields_valid(value, &known) || !diagnostic_presence_matches(value) {
            return Err(invalid_packet("invalid job diagnostic record"));
        }
        let key = (
            value.job_id,
            value.severity,
            value.code,
            value.presence,
            value.stage_id,
            value.operand_id,
            value.geometry_id,
            value.path_token,
        );
        if previous.as_ref().is_some_and(|old| old >= &key) {
            return Err(invalid_packet("diagnostics are not strictly canonical"));
        }
        previous = Some(key);
    }
    Ok(())
}

fn diagnostic_fields_valid(value: &DiagnosticRecord, known: &[u32]) -> bool {
    [
        known.contains(&value.code),
        (1..=2).contains(&value.severity),
        value.scope == 1,
        value.presence <= 15,
        value.presence & 1 != 0,
        value.path_token <= 26,
        value.detail_token == 0,
    ]
    .into_iter()
    .all(std::convert::identity)
}

fn diagnostic_presence_matches(value: &DiagnosticRecord) -> bool {
    [
        (value.presence & 1 != 0) == (value.job_id != 0),
        (value.presence & 2 != 0) == (value.stage_id != 0),
        (value.presence & 4 != 0) == (value.operand_id != 0),
        (value.presence & 8 != 0) == (value.geometry_id != 0),
    ]
    .into_iter()
    .all(std::convert::identity)
}

fn validate_sources(records: &ResultRecords) -> Result<(), AnalyticPacketError> {
    let mut previous: Option<&SourceRecord> = None;
    for value in &records.sources {
        let valid_role = source_shape_valid(value);
        let fields_valid = [
            value.flags == 0,
            value.operand_id != 0,
            value.primary_id != 0,
            valid_role,
        ]
        .into_iter()
        .all(std::convert::identity);
        if !fields_valid || previous.is_some_and(|old| old >= value) {
            return Err(invalid_packet(
                "source reference is invalid or noncanonical",
            ));
        }
        previous = Some(value);
    }
    Ok(())
}

fn source_shape_valid(value: &SourceRecord) -> bool {
    match value.kind {
        1 => matches!(value.role, 1 | 2) && value.secondary_id != 0,
        2 => primitive_source_shape_valid(value),
        3 => value.role == 0 && value.secondary_id == 0,
        _ => false,
    }
}

fn primitive_source_shape_valid(value: &SourceRecord) -> bool {
    let high = value.secondary_id >> 32;
    let low = value.secondary_id & u64::from(u32::MAX);
    match value.role {
        16 | 17 | 32..=35 => value.secondary_id == 0,
        48 | 49 | 50 | 51 | 54 => high != 0 && low == 0,
        52 => high != 0 && low != 0,
        53 => high == 1 && low == 0,
        _ => false,
    }
}

fn validate_source_sets(records: &ResultRecords) -> Result<(), AnalyticPacketError> {
    partition(
        records
            .source_sets
            .iter()
            .map(|value| (value.begin, value.count)),
        records.source_indices.len(),
        "source-set memberships",
    )?;
    let mut previous: Option<Vec<u32>> = None;
    for set in &records.source_sets {
        if set.count == 0 {
            return Err(invalid_packet("empty source set has a table record"));
        }
        let members = range(&records.source_indices, set.begin, set.count)?.to_vec();
        strictly_increasing(members.iter().copied(), "source-set member indices")?;
        if members
            .iter()
            .any(|index| *index as usize >= records.sources.len())
            || previous.as_ref().is_some_and(|old| old >= &members)
        {
            return Err(invalid_reference("source set is invalid or noncanonical"));
        }
        previous = Some(members);
    }
    Ok(())
}

fn validate_geometry(records: &ResultRecords) -> Result<(), AnalyticPacketError> {
    one_based(
        records.vertices.iter().map(|value| value.id),
        "result vertex",
    )?;
    for value in &records.vertices {
        if value.source_set as usize > records.source_sets.len()
            || value.flags > 1
            || value.flags != u32::from(value.source_set != 0)
        {
            return Err(invalid_reference("invalid result vertex"));
        }
    }
    one_based(
        records.fragments.iter().map(|value| value.id),
        "result fragment",
    )?;
    for value in &records.fragments {
        validate_fragment(value, records)?;
    }
    validate_rings(records)?;
    validate_regions(records)?;
    Ok(())
}

fn validate_fragment(
    value: &super::records::FragmentRecord,
    records: &ResultRecords,
) -> Result<(), AnalyticPacketError> {
    let start = records.vertices.get(value.start as usize);
    let end = records.vertices.get(value.end as usize);
    if !fragment_references_valid(value, records) || !fragment_shape_valid(value) {
        return Err(invalid_reference("invalid directed fragment"));
    }
    if value.kind == 2 {
        let start = start.expect("checked start");
        let end = end.expect("checked end");
        let dx = i128::from(end.x) - i128::from(start.x);
        let dy = i128::from(end.y) - i128::from(start.y);
        let chord = dx
            .checked_mul(dx)
            .and_then(|x| dy.checked_mul(dy).and_then(|y| x.checked_add(y)))
            .ok_or_else(|| invalid_packet("arc endpoint distance overflows its exact domain"))?;
        let diameter = 4_i128 * i128::from(value.radius) * i128::from(value.radius);
        if chord > diameter || (chord == diameter && value.major == 1) {
            return Err(invalid_packet("arc radius and branch are incoherent"));
        }
    }
    Ok(())
}

fn fragment_references_valid(
    value: &super::records::FragmentRecord,
    records: &ResultRecords,
) -> bool {
    [
        records.vertices.get(value.start as usize).is_some(),
        records.vertices.get(value.end as usize).is_some(),
        value.start != value.end,
        value.positive_set as usize <= records.source_sets.len(),
        value.subtraction_set as usize <= records.source_sets.len(),
    ]
    .into_iter()
    .all(std::convert::identity)
}

fn fragment_shape_valid(value: &super::records::FragmentRecord) -> bool {
    let fields = match value.kind {
        1 => [value.direction == 0, value.major == 0, value.radius == 0],
        2 => [
            (1..=2).contains(&value.direction),
            value.major <= 1,
            (1..=MAX_LENGTH_NM).contains(&value.radius),
        ],
        _ => return false,
    };
    fields.into_iter().all(std::convert::identity)
}

fn validate_rings(records: &ResultRecords) -> Result<(), AnalyticPacketError> {
    one_based(records.rings.iter().map(|value| value.id), "result ring")?;
    partition(
        records
            .rings
            .iter()
            .map(|value| (value.reference_begin, value.reference_count)),
        records.fragment_references.len(),
        "ring fragment references",
    )?;
    let mut used = HashSet::with_capacity(records.fragments.len());
    for (index, value) in records.rings.iter().enumerate() {
        if !ring_fields_valid(value) {
            return Err(invalid_packet("invalid result ring flags or closure"));
        }
        validate_ring_parent(records, index)?;
        let references = range(
            &records.fragment_references,
            value.reference_begin,
            value.reference_count,
        )?;
        for reference in references {
            if *reference as usize >= records.fragments.len() {
                return Err(invalid_reference("ring references an unknown fragment"));
            }
            if !used.insert(*reference) {
                return Err(invalid_reference(
                    "fragment is referenced by multiple rings",
                ));
            }
        }
        validate_ring_topology(records, references)?;
    }
    if used.len() != records.fragments.len() {
        return Err(invalid_reference(
            "result contains an unreferenced fragment",
        ));
    }
    Ok(())
}

fn validate_ring_topology(
    records: &ResultRecords,
    references: &[u32],
) -> Result<(), AnalyticPacketError> {
    for (index, reference) in references.iter().enumerate() {
        let current = &records.fragments[*reference as usize];
        let next = &records.fragments[references[(index + 1) % references.len()] as usize];
        if current.end != next.start {
            return Err(invalid_reference("ring fragment topology is disconnected"));
        }
    }
    Ok(())
}

fn ring_fields_valid(value: &super::records::RingRecord) -> bool {
    [
        value.reference_count >= 2,
        value.flags & !1 == 0,
        (value.flags & 1 != 0) == (value.depth % 2 == 1),
    ]
    .into_iter()
    .all(std::convert::identity)
}

fn validate_ring_parent(records: &ResultRecords, index: usize) -> Result<(), AnalyticPacketError> {
    let value = &records.rings[index];
    if value.parent == NONE {
        if value.depth == 0 {
            return Ok(());
        }
        return Err(invalid_reference("root ring has nonzero depth"));
    }
    let parent = records
        .rings
        .get(value.parent as usize)
        .ok_or_else(|| invalid_reference("ring parent is out of range"))?;
    if value.parent as usize >= index || parent.depth + 1 != value.depth {
        return Err(invalid_reference("ring parent/depth is invalid"));
    }
    Ok(())
}

fn validate_regions(records: &ResultRecords) -> Result<(), AnalyticPacketError> {
    one_based(
        records.regions.iter().map(|value| value.id),
        "result region",
    )?;
    let mut named = HashSet::new();
    for value in &records.regions {
        let ring = records
            .rings
            .get(value.outer as usize)
            .ok_or_else(|| invalid_reference("region outer ring is out of range"))?;
        if ring.depth % 2 != 0
            || value.flags != 0
            || value.positive_set == 0
            || value.positive_set as usize > records.source_sets.len()
            || !named.insert(value.outer)
        {
            return Err(invalid_reference("invalid result region"));
        }
    }
    if records
        .rings
        .iter()
        .enumerate()
        .any(|(index, ring)| ring.depth % 2 == 0 && !named.contains(&(index as u32)))
    {
        return Err(invalid_reference("even-depth ring lacks a result region"));
    }
    Ok(())
}

fn derive_ownership(records: &ResultRecords) -> Result<Ownership, AnalyticPacketError> {
    let mut regions = vec![usize::MAX; records.regions.len()];
    let mut events = vec![usize::MAX; records.events.len()];
    for (owner, job) in records.jobs.iter().enumerate() {
        assign(&mut regions, job.region_begin, job.region_count, owner)?;
        assign(&mut events, job.event_begin, job.event_count, owner)?;
    }
    let rings = derive_ring_ownership(records, &regions)?;
    let mut fragments = vec![usize::MAX; records.fragments.len()];
    for (ring_index, ring) in records.rings.iter().enumerate() {
        for reference in range(
            &records.fragment_references,
            ring.reference_begin,
            ring.reference_count,
        )? {
            set_owner(&mut fragments, *reference, rings[ring_index], "fragment")?;
        }
    }
    let mut vertices = vec![usize::MAX; records.vertices.len()];
    for (fragment_index, fragment) in records.fragments.iter().enumerate() {
        let owner = fragments[fragment_index];
        set_owner(&mut vertices, fragment.start, owner, "vertex")?;
        set_owner(&mut vertices, fragment.end, owner, "vertex")?;
    }
    if vertices
        .iter()
        .chain(&fragments)
        .chain(&rings)
        .chain(&regions)
        .chain(&events)
        .any(|owner| *owner == usize::MAX)
    {
        return Err(invalid_reference("result contains an unowned record"));
    }
    Ok(Ownership {
        vertices,
        fragments,
        rings,
        regions,
        events,
    })
}

fn derive_ring_ownership(
    records: &ResultRecords,
    regions: &[usize],
) -> Result<Vec<usize>, AnalyticPacketError> {
    let mut outer_regions = vec![usize::MAX; records.rings.len()];
    for (region_index, region) in records.regions.iter().enumerate() {
        outer_regions[region.outer as usize] = region_index;
    }
    let mut rings = vec![usize::MAX; records.rings.len()];
    for index in 0..records.rings.len() {
        let parent = records.rings[index].parent;
        rings[index] = if parent == NONE {
            let region = outer_regions[index];
            if region == usize::MAX {
                return Err(invalid_reference("root ring lacks a result region"));
            }
            regions[region]
        } else {
            rings[parent as usize]
        };
    }
    for (index, region) in records.regions.iter().enumerate() {
        if rings[region.outer as usize] != regions[index] {
            return Err(invalid_reference(
                "result region and outer ring have different job owners",
            ));
        }
    }
    Ok(rings)
}

fn validate_events(records: &ResultRecords, owners: &Ownership) -> Result<(), AnalyticPacketError> {
    partition(
        records
            .events
            .iter()
            .map(|value| (value.reference_begin, value.reference_count)),
        records.ring_region_references.len(),
        "operand-event references",
    )?;
    for (index, value) in records.events.iter().enumerate() {
        if value.operand_id == 0
            || !(1..=7).contains(&value.kind)
            || value.flags != 0
            || value.source_set as usize > records.source_sets.len()
        {
            return Err(invalid_packet("invalid operand outcome event"));
        }
        let references = range(
            &records.ring_region_references,
            value.reference_begin,
            value.reference_count,
        )?;
        strictly_increasing(references.iter().copied(), "operand-event references")?;
        for reference in references {
            let kind = reference >> 32;
            let target = (*reference & u64::from(u32::MAX)) as usize;
            let owner = match kind {
                1 => owners.rings.get(target),
                2 => owners.regions.get(target),
                _ => None,
            }
            .ok_or_else(|| invalid_reference("event result reference is invalid"))?;
            if *owner != owners.events[index] {
                return Err(invalid_reference("event references another job closure"));
            }
        }
    }
    Ok(())
}

fn validate_relationships(records: &ResultRecords) -> Result<(), AnalyticPacketError> {
    strictly_increasing(
        records.relationships.iter().map(|value| value.query_id),
        "relationship ids",
    )?;
    partition(
        records
            .relationships
            .iter()
            .map(|value| (value.pair_begin, value.pair_count)),
        records.pairs.len(),
        "relationship pairs",
    )?;
    for value in &records.relationships {
        if !relationship_valid(value) {
            return Err(invalid_packet("invalid relationship result"));
        }
        let pairs = range(&records.pairs, value.pair_begin, value.pair_count)?;
        if pairs.windows(2).any(|items| items[0] >= items[1]) {
            return Err(invalid_packet("relationship pairs are not canonical"));
        }
        let aggregate = pairs.iter().map(|pair| pair.dimension).max().unwrap_or(0);
        if value.status == 0 && value.dimension != aggregate {
            return Err(invalid_packet(
                "relationship aggregate dimension is invalid",
            ));
        }
    }
    for pair in &records.pairs {
        if !relationship_pair_valid(pair, records.regions.len()) {
            return Err(invalid_packet("invalid relationship pair"));
        }
    }
    Ok(())
}

fn relationship_valid(value: &super::records::RelationshipRecord) -> bool {
    let base = [
        value.query_id != 0,
        value.status <= 1,
        value.dimension <= 3,
        value.flags == 0,
    ]
    .into_iter()
    .all(std::convert::identity);
    let rejected_shape = [
        value.dimension == 0,
        value.pair_begin == 0,
        value.pair_count == 0,
    ]
    .into_iter()
    .all(std::convert::identity);
    base && (value.status != 1 || rejected_shape)
}

fn relationship_pair_valid(value: &super::records::PairRecord, region_count: usize) -> bool {
    let fields = [
        (1..=region_count as u64).contains(&value.left),
        (1..=region_count as u64).contains(&value.right),
        value.dimension <= 3,
        value.equality <= 1,
        value.left_contains <= 1,
        value.right_contains <= 1,
    ]
    .into_iter()
    .all(std::convert::identity);
    let containment = value.equality != 0 || value.left_contains != 0 || value.right_contains != 0;
    let flags = (!containment || value.dimension == 3)
        && (value.equality == 0 || (value.left_contains == 1 && value.right_contains == 1));
    fields && flags
}

fn validate_source_exhaustion(records: &ResultRecords) -> Result<(), AnalyticPacketError> {
    let mut sets = HashSet::new();
    sets.extend(
        records
            .vertices
            .iter()
            .filter_map(|value| handle_index(value.source_set)),
    );
    for value in &records.fragments {
        sets.extend(
            [
                handle_index(value.positive_set),
                handle_index(value.subtraction_set),
            ]
            .into_iter()
            .flatten(),
        );
    }
    sets.extend(
        records
            .regions
            .iter()
            .filter_map(|value| handle_index(value.positive_set)),
    );
    sets.extend(
        records
            .events
            .iter()
            .filter_map(|value| handle_index(value.source_set)),
    );
    let expected_sets: HashSet<_> = (0..records.source_sets.len()).collect();
    let used_sources: HashSet<_> = records
        .source_indices
        .iter()
        .map(|value| *value as usize)
        .collect();
    let expected_sources: HashSet<_> = (0..records.sources.len()).collect();
    if sets != expected_sets || used_sources != expected_sources {
        return Err(invalid_reference("result contains unused source content"));
    }
    Ok(())
}

fn handle_index(value: u32) -> Option<usize> {
    (value != 0).then(|| value as usize - 1)
}

fn validate_canonical_order(
    records: &ResultRecords,
    owners: &Ownership,
) -> Result<(), AnalyticPacketError> {
    validate_vertex_order(records, owners)?;
    for (index, pair) in records.fragments.windows(2).enumerate() {
        let left = fragment_key(records, &pair[0], owners.fragments[index]);
        let right = fragment_key(records, &pair[1], owners.fragments[index + 1]);
        if left >= right {
            return Err(invalid_packet("directed fragments are not canonical"));
        }
    }
    let mut previous_ring = None;
    for (index, ring) in records.rings.iter().enumerate() {
        let refs = range(
            &records.fragment_references,
            ring.reference_begin,
            ring.reference_count,
        )?;
        if least_rotation(refs) != 0 {
            return Err(invalid_packet("ring fragment rotation is not canonical"));
        }
        let key = (
            records.jobs[owners.rings[index]].id,
            ring.depth,
            refs.to_vec(),
            ring.parent,
        );
        require_key_after(&mut previous_ring, key, "result rings")?;
    }
    let mut previous_region = None;
    for (index, region) in records.regions.iter().enumerate() {
        let key = (
            records.jobs[owners.regions[index]].id,
            region.outer,
            region.positive_set,
        );
        require_key_after(&mut previous_region, key, "result regions")?;
    }
    for job in &records.jobs {
        let events = range(&records.events, job.event_begin, job.event_count)?;
        let mut previous_event = None;
        for event in events {
            require_key_after(
                &mut previous_event,
                event_key(records, event)?,
                "operand events",
            )?;
        }
    }
    Ok(())
}

type IncidentKey = (u8, i64, i64, u8, u8, u8, u64);

fn validate_vertex_order(
    records: &ResultRecords,
    owners: &Ownership,
) -> Result<(), AnalyticPacketError> {
    let mut incidents: Vec<Vec<IncidentKey>> = vec![Vec::new(); records.vertices.len()];
    for fragment in &records.fragments {
        let start = &records.vertices[fragment.start as usize];
        let end = &records.vertices[fragment.end as usize];
        incidents[fragment.start as usize].push((
            0,
            end.x,
            end.y,
            fragment.kind,
            fragment.direction,
            fragment.major,
            fragment.radius,
        ));
        incidents[fragment.end as usize].push((
            1,
            start.x,
            start.y,
            fragment.kind,
            fragment.direction,
            fragment.major,
            fragment.radius,
        ));
    }
    let mut previous = None;
    for (index, vertex) in records.vertices.iter().enumerate() {
        incidents[index].sort_unstable();
        let key = (
            records.jobs[owners.vertices[index]].id,
            vertex.x,
            vertex.y,
            incidents[index].clone(),
            vertex.source_set,
        );
        require_key_after(&mut previous, key, "result vertices")?;
    }
    Ok(())
}

fn require_key_after<T: Ord>(
    previous: &mut Option<T>,
    value: T,
    label: &str,
) -> Result<(), AnalyticPacketError> {
    if previous.as_ref().is_some_and(|old| old >= &value) {
        return Err(invalid_packet(format!(
            "{label} are not strictly canonical"
        )));
    }
    *previous = Some(value);
    Ok(())
}

fn fragment_key(
    records: &ResultRecords,
    value: &super::records::FragmentRecord,
    owner: usize,
) -> (u64, u32, u32, u8, u8, u8, u64, u32, u32) {
    (
        records.jobs[owner].id,
        value.start,
        value.end,
        value.kind,
        value.direction,
        value.major,
        value.radius,
        value.positive_set,
        value.subtraction_set,
    )
}

fn event_key(
    records: &ResultRecords,
    value: &super::records::EventRecord,
) -> Result<(u64, u16, Vec<u64>, u32), AnalyticPacketError> {
    Ok((
        value.operand_id,
        value.kind,
        range(
            &records.ring_region_references,
            value.reference_begin,
            value.reference_count,
        )?
        .to_vec(),
        value.source_set,
    ))
}

fn validate_job_spans(
    records: &ResultRecords,
    owners: &Ownership,
) -> Result<(), AnalyticPacketError> {
    let mut bounds: Vec<Option<(i64, i64, i64, i64)>> = vec![None; records.jobs.len()];
    for (index, vertex) in records.vertices.iter().enumerate() {
        let target = &mut bounds[owners.vertices[index]];
        *target = Some(match *target {
            None => (vertex.x, vertex.x, vertex.y, vertex.y),
            Some((min_x, max_x, min_y, max_y)) => (
                min_x.min(vertex.x),
                max_x.max(vertex.x),
                min_y.min(vertex.y),
                max_y.max(vertex.y),
            ),
        });
    }
    if bounds
        .into_iter()
        .flatten()
        .any(|(min_x, max_x, min_y, max_y)| {
            i128::from(max_x) - i128::from(min_x) > i128::from(MAX_LENGTH_NM)
                || i128::from(max_y) - i128::from(min_y) > i128::from(MAX_LENGTH_NM)
        })
    {
        return Err(limit_exceeded(
            "job result coordinate span exceeds the governed maximum",
        ));
    }
    Ok(())
}

pub(crate) fn range<T>(values: &[T], begin: u32, count: u32) -> Result<&[T], AnalyticPacketError> {
    let begin = begin as usize;
    let end = begin
        .checked_add(count as usize)
        .ok_or_else(|| limit_exceeded("range overflow"))?;
    values
        .get(begin..end)
        .ok_or_else(|| invalid_reference("record range is out of bounds"))
}

fn partition(
    values: impl IntoIterator<Item = (u32, u32)>,
    length: usize,
    label: &str,
) -> Result<(), AnalyticPacketError> {
    let mut cursor = 0_usize;
    for (begin, count) in values {
        if count == 0 {
            if begin != 0 {
                return Err(invalid_reference(format!(
                    "empty {label} range has a nonzero begin"
                )));
            }
            continue;
        }
        if begin as usize != cursor {
            return Err(invalid_reference(format!(
                "{label} ranges are not a gapless partition"
            )));
        }
        cursor = cursor
            .checked_add(count as usize)
            .ok_or_else(|| limit_exceeded("partition overflow"))?;
    }
    if cursor != length {
        return Err(invalid_reference(format!(
            "{label} partition does not cover its table"
        )));
    }
    Ok(())
}

fn one_based(
    values: impl IntoIterator<Item = u64>,
    label: &str,
) -> Result<(), AnalyticPacketError> {
    for (index, value) in values.into_iter().enumerate() {
        if value != index as u64 + 1 {
            return Err(invalid_id(format!(
                "{label} ids are not dense one-based ordinals"
            )));
        }
    }
    Ok(())
}

fn strictly_increasing<T: Ord>(
    values: impl IntoIterator<Item = T>,
    label: &str,
) -> Result<(), AnalyticPacketError> {
    let mut previous = None;
    for value in values {
        if previous.as_ref().is_some_and(|old| old >= &value) {
            return Err(invalid_id(format!("{label} are not strictly increasing")));
        }
        previous = Some(value);
    }
    Ok(())
}

fn assign(
    values: &mut [usize],
    begin: u32,
    count: u32,
    owner: usize,
) -> Result<(), AnalyticPacketError> {
    for value in values
        .get_mut(begin as usize..begin as usize + count as usize)
        .ok_or_else(|| invalid_reference("owner range is out of bounds"))?
    {
        if *value != usize::MAX {
            return Err(invalid_reference("record has multiple owners"));
        }
        *value = owner;
    }
    Ok(())
}

fn set_owner(
    values: &mut [usize],
    index: u32,
    owner: usize,
    label: &str,
) -> Result<(), AnalyticPacketError> {
    let value = values
        .get_mut(index as usize)
        .ok_or_else(|| invalid_reference(format!("{label} index is out of range")))?;
    if *value != usize::MAX && *value != owner {
        return Err(invalid_reference(format!(
            "{label} has multiple job owners"
        )));
    }
    *value = owner;
    Ok(())
}

fn least_rotation(values: &[u32]) -> usize {
    let count = values.len();
    if count == 0 {
        return 0;
    }
    let mut left = 0;
    let mut right = 1;
    let mut offset = 0;
    while left < count && right < count && offset < count {
        let a = values[(left + offset) % count];
        let b = values[(right + offset) % count];
        match a.cmp(&b) {
            std::cmp::Ordering::Equal => offset += 1,
            std::cmp::Ordering::Greater => {
                left += offset + 1;
                if left == right {
                    left += 1;
                }
                offset = 0;
            }
            std::cmp::Ordering::Less => {
                right += offset + 1;
                if left == right {
                    right += 1;
                }
                offset = 0;
            }
        }
    }
    left.min(right)
}

#[cfg(test)]
#[path = "validate_tests.rs"]
mod tests;
