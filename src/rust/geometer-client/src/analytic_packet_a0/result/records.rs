use super::super::error::AnalyticPacketError;
use super::super::wire::{Table, i64_at, u16_at, u32_at, u64_at};

#[derive(Clone, Debug, PartialEq)]
pub(crate) struct JobRecord {
    pub id: u64,
    pub status: u8,
    pub diagnostic_begin: u32,
    pub diagnostic_count: u32,
    pub region_begin: u32,
    pub region_count: u32,
    pub event_begin: u32,
    pub event_count: u32,
}

#[derive(Clone, Debug, PartialEq)]
pub(crate) struct DiagnosticRecord {
    pub code: u32,
    pub severity: u8,
    pub scope: u8,
    pub presence: u16,
    pub job_id: u64,
    pub stage_id: u64,
    pub operand_id: u64,
    pub geometry_id: u64,
    pub path_token: u32,
    pub detail_token: u32,
}

#[derive(Clone, Debug, PartialEq)]
pub(crate) struct VertexRecord {
    pub id: u64,
    pub x: i64,
    pub y: i64,
    pub source_set: u32,
    pub flags: u32,
}

#[derive(Clone, Debug, PartialEq)]
pub(crate) struct FragmentRecord {
    pub id: u64,
    pub start: u32,
    pub end: u32,
    pub kind: u8,
    pub direction: u8,
    pub major: u8,
    pub radius: u64,
    pub positive_set: u32,
    pub subtraction_set: u32,
}

#[derive(Clone, Debug, PartialEq)]
pub(crate) struct RingRecord {
    pub id: u64,
    pub reference_begin: u32,
    pub reference_count: u32,
    pub parent: u32,
    pub depth: u32,
    pub flags: u32,
}

#[derive(Clone, Debug, PartialEq)]
pub(crate) struct RegionRecord {
    pub id: u64,
    pub outer: u32,
    pub positive_set: u32,
    pub flags: u32,
}

#[derive(Clone, Debug, PartialEq)]
pub(crate) struct SourceSetRecord {
    pub begin: u32,
    pub count: u32,
}

#[derive(Clone, Debug, PartialEq, Eq, Ord, PartialOrd)]
pub(crate) struct SourceRecord {
    pub kind: u16,
    pub role: u16,
    pub flags: u32,
    pub operand_id: u64,
    pub primary_id: u64,
    pub secondary_id: u64,
}

#[derive(Clone, Debug, PartialEq)]
pub(crate) struct EventRecord {
    pub operand_id: u64,
    pub kind: u16,
    pub flags: u16,
    pub reference_begin: u32,
    pub reference_count: u32,
    pub source_set: u32,
}

#[derive(Clone, Debug, PartialEq)]
pub(crate) struct RelationshipRecord {
    pub query_id: u64,
    pub status: u8,
    pub dimension: u8,
    pub flags: u16,
    pub pair_begin: u32,
    pub pair_count: u32,
}

#[derive(Clone, Debug, PartialEq, Eq, Ord, PartialOrd)]
pub(crate) struct PairRecord {
    pub left: u64,
    pub right: u64,
    pub dimension: u8,
    pub equality: u8,
    pub left_contains: u8,
    pub right_contains: u8,
}

#[derive(Clone, Debug, PartialEq)]
pub(crate) struct ResultRecords {
    pub jobs: Vec<JobRecord>,
    pub diagnostics: Vec<DiagnosticRecord>,
    pub vertices: Vec<VertexRecord>,
    pub fragments: Vec<FragmentRecord>,
    pub rings: Vec<RingRecord>,
    pub fragment_references: Vec<u32>,
    pub regions: Vec<RegionRecord>,
    pub ring_region_references: Vec<u64>,
    pub source_sets: Vec<SourceSetRecord>,
    pub sources: Vec<SourceRecord>,
    pub events: Vec<EventRecord>,
    pub relationships: Vec<RelationshipRecord>,
    pub pairs: Vec<PairRecord>,
    pub source_indices: Vec<u32>,
}

pub(crate) fn parse(tables: &[Table<'_>]) -> Result<ResultRecords, AnalyticPacketError> {
    let (jobs, diagnostics) = parse_job_tables(tables)?;
    let (vertices, fragments, rings, fragment_references, regions, ring_region_references) =
        parse_geometry_tables(tables)?;
    let (source_sets, sources, events) = parse_provenance_tables(tables)?;
    let (relationships, pairs, source_indices) = parse_output_tables(tables)?;
    Ok(ResultRecords {
        jobs,
        diagnostics,
        vertices,
        fragments,
        rings,
        fragment_references,
        regions,
        ring_region_references,
        source_sets,
        sources,
        events,
        relationships,
        pairs,
        source_indices,
    })
}

fn parse_job_tables(
    tables: &[Table<'_>],
) -> Result<(Vec<JobRecord>, Vec<DiagnosticRecord>), AnalyticPacketError> {
    Ok((
        map(&tables[0], |v| {
            Ok(JobRecord {
                id: u64_at(v, 0)?,
                status: v[8],
                diagnostic_begin: u32_at(v, 16)?,
                diagnostic_count: u32_at(v, 20)?,
                region_begin: u32_at(v, 24)?,
                region_count: u32_at(v, 28)?,
                event_begin: u32_at(v, 32)?,
                event_count: u32_at(v, 36)?,
            })
        })?,
        map(&tables[1], |v| {
            Ok(DiagnosticRecord {
                code: u32_at(v, 0)?,
                severity: v[4],
                scope: v[5],
                presence: u16_at(v, 6)?,
                job_id: u64_at(v, 8)?,
                stage_id: u64_at(v, 16)?,
                operand_id: u64_at(v, 24)?,
                geometry_id: u64_at(v, 32)?,
                path_token: u32_at(v, 40)?,
                detail_token: u32_at(v, 44)?,
            })
        })?,
    ))
}

#[allow(
    clippy::type_complexity,
    reason = "the tuple mirrors six adjacent wire tables"
)]
fn parse_geometry_tables(
    tables: &[Table<'_>],
) -> Result<
    (
        Vec<VertexRecord>,
        Vec<FragmentRecord>,
        Vec<RingRecord>,
        Vec<u32>,
        Vec<RegionRecord>,
        Vec<u64>,
    ),
    AnalyticPacketError,
> {
    Ok((
        map(&tables[2], |v| {
            Ok(VertexRecord {
                id: u64_at(v, 0)?,
                x: i64_at(v, 8)?,
                y: i64_at(v, 16)?,
                source_set: u32_at(v, 24)?,
                flags: u32_at(v, 28)?,
            })
        })?,
        map(&tables[3], |v| {
            Ok(FragmentRecord {
                id: u64_at(v, 0)?,
                start: u32_at(v, 8)?,
                end: u32_at(v, 12)?,
                kind: v[16],
                direction: v[17],
                major: v[18],
                radius: u64_at(v, 24)?,
                positive_set: u32_at(v, 32)?,
                subtraction_set: u32_at(v, 36)?,
            })
        })?,
        map(&tables[4], |v| {
            Ok(RingRecord {
                id: u64_at(v, 0)?,
                reference_begin: u32_at(v, 8)?,
                reference_count: u32_at(v, 12)?,
                parent: u32_at(v, 16)?,
                depth: u32_at(v, 20)?,
                flags: u32_at(v, 24)?,
            })
        })?,
        map(&tables[5], |v| u32_at(v, 0))?,
        map(&tables[6], |v| {
            Ok(RegionRecord {
                id: u64_at(v, 0)?,
                outer: u32_at(v, 8)?,
                positive_set: u32_at(v, 12)?,
                flags: u32_at(v, 16)?,
            })
        })?,
        map(&tables[7], |v| u64_at(v, 0))?,
    ))
}

#[allow(
    clippy::type_complexity,
    reason = "the tuple mirrors three adjacent wire tables"
)]
fn parse_provenance_tables(
    tables: &[Table<'_>],
) -> Result<(Vec<SourceSetRecord>, Vec<SourceRecord>, Vec<EventRecord>), AnalyticPacketError> {
    Ok((
        map(&tables[8], |v| {
            Ok(SourceSetRecord {
                begin: u32_at(v, 0)?,
                count: u32_at(v, 4)?,
            })
        })?,
        map(&tables[9], |v| {
            Ok(SourceRecord {
                kind: u16_at(v, 0)?,
                role: u16_at(v, 2)?,
                flags: u32_at(v, 4)?,
                operand_id: u64_at(v, 8)?,
                primary_id: u64_at(v, 16)?,
                secondary_id: u64_at(v, 24)?,
            })
        })?,
        map(&tables[10], |v| {
            Ok(EventRecord {
                operand_id: u64_at(v, 0)?,
                kind: u16_at(v, 8)?,
                flags: u16_at(v, 10)?,
                reference_begin: u32_at(v, 12)?,
                reference_count: u32_at(v, 16)?,
                source_set: u32_at(v, 20)?,
            })
        })?,
    ))
}

#[allow(
    clippy::type_complexity,
    reason = "the tuple mirrors three adjacent wire tables"
)]
fn parse_output_tables(
    tables: &[Table<'_>],
) -> Result<(Vec<RelationshipRecord>, Vec<PairRecord>, Vec<u32>), AnalyticPacketError> {
    Ok((
        map(&tables[11], |v| {
            Ok(RelationshipRecord {
                query_id: u64_at(v, 0)?,
                status: v[8],
                dimension: v[9],
                flags: u16_at(v, 10)?,
                pair_begin: u32_at(v, 12)?,
                pair_count: u32_at(v, 16)?,
            })
        })?,
        map(&tables[12], |v| {
            Ok(PairRecord {
                left: u64_at(v, 0)?,
                right: u64_at(v, 8)?,
                dimension: v[16],
                equality: v[17],
                left_contains: v[18],
                right_contains: v[19],
            })
        })?,
        map(&tables[13], |v| u32_at(v, 0))?,
    ))
}

fn map<T>(
    table: &Table<'_>,
    mut decode: impl FnMut(&[u8]) -> Result<T, AnalyticPacketError>,
) -> Result<Vec<T>, AnalyticPacketError> {
    (0..table.count)
        .map(|index| decode(table.record(index)?))
        .collect()
}
