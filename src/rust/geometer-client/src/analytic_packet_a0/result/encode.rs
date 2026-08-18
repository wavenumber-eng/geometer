use super::super::error::AnalyticPacketError;
use super::super::wire::{TableBuilder, encode_tables, put_i64, put_u16, put_u32, put_u64};
use super::records::ResultRecords;

const RESULT_MAGIC: &[u8; 8] = b"GMABRS01";
pub(crate) const RECORD_BYTES: [usize; 14] = [48, 56, 32, 48, 32, 4, 24, 8, 8, 32, 48, 32, 32, 4];

pub(crate) fn encode(records: &ResultRecords) -> Result<Vec<u8>, AnalyticPacketError> {
    let mut tables: Vec<_> = RECORD_BYTES
        .iter()
        .enumerate()
        .map(|(index, bytes)| TableBuilder::new(101 + index as u16, *bytes))
        .collect();
    encode_job_tables(records, &mut tables)?;
    encode_geometry_tables(records, &mut tables)?;
    encode_provenance_tables(records, &mut tables)?;
    encode_output_tables(records, &mut tables)?;
    encode_tables(
        RESULT_MAGIC,
        &tables,
        records.jobs.len(),
        records.relationships.len(),
    )
}

fn encode_job_tables(
    records: &ResultRecords,
    tables: &mut [TableBuilder],
) -> Result<(), AnalyticPacketError> {
    for value in &records.jobs {
        let v = add(&mut tables[0])?;
        put_u64(v, 0, value.id)?;
        v[8] = value.status;
        put_u32(v, 16, value.diagnostic_begin)?;
        put_u32(v, 20, value.diagnostic_count)?;
        put_u32(v, 24, value.region_begin)?;
        put_u32(v, 28, value.region_count)?;
        put_u32(v, 32, value.event_begin)?;
        put_u32(v, 36, value.event_count)?;
    }
    for value in &records.diagnostics {
        let v = add(&mut tables[1])?;
        put_u32(v, 0, value.code)?;
        v[4] = value.severity;
        v[5] = value.scope;
        put_u16(v, 6, value.presence)?;
        put_u64(v, 8, value.job_id)?;
        put_u64(v, 16, value.stage_id)?;
        put_u64(v, 24, value.operand_id)?;
        put_u64(v, 32, value.geometry_id)?;
        put_u32(v, 40, value.path_token)?;
        put_u32(v, 44, value.detail_token)?;
    }
    Ok(())
}

fn encode_geometry_tables(
    records: &ResultRecords,
    tables: &mut [TableBuilder],
) -> Result<(), AnalyticPacketError> {
    for value in &records.vertices {
        let v = add(&mut tables[2])?;
        put_u64(v, 0, value.id)?;
        put_i64(v, 8, value.x)?;
        put_i64(v, 16, value.y)?;
        put_u32(v, 24, value.source_set)?;
        put_u32(v, 28, value.flags)?;
    }
    for value in &records.fragments {
        let v = add(&mut tables[3])?;
        put_u64(v, 0, value.id)?;
        put_u32(v, 8, value.start)?;
        put_u32(v, 12, value.end)?;
        v[16] = value.kind;
        v[17] = value.direction;
        v[18] = value.major;
        put_u64(v, 24, value.radius)?;
        put_u32(v, 32, value.positive_set)?;
        put_u32(v, 36, value.subtraction_set)?;
    }
    for value in &records.rings {
        let v = add(&mut tables[4])?;
        put_u64(v, 0, value.id)?;
        put_u32(v, 8, value.reference_begin)?;
        put_u32(v, 12, value.reference_count)?;
        put_u32(v, 16, value.parent)?;
        put_u32(v, 20, value.depth)?;
        put_u32(v, 24, value.flags)?;
    }
    for value in &records.fragment_references {
        put_u32(add(&mut tables[5])?, 0, *value)?;
    }
    for value in &records.regions {
        let v = add(&mut tables[6])?;
        put_u64(v, 0, value.id)?;
        put_u32(v, 8, value.outer)?;
        put_u32(v, 12, value.positive_set)?;
        put_u32(v, 16, value.flags)?;
    }
    for value in &records.ring_region_references {
        put_u64(add(&mut tables[7])?, 0, *value)?;
    }
    Ok(())
}

fn encode_provenance_tables(
    records: &ResultRecords,
    tables: &mut [TableBuilder],
) -> Result<(), AnalyticPacketError> {
    for value in &records.source_sets {
        let v = add(&mut tables[8])?;
        put_u32(v, 0, value.begin)?;
        put_u32(v, 4, value.count)?;
    }
    for value in &records.sources {
        let v = add(&mut tables[9])?;
        put_u16(v, 0, value.kind)?;
        put_u16(v, 2, value.role)?;
        put_u32(v, 4, value.flags)?;
        put_u64(v, 8, value.operand_id)?;
        put_u64(v, 16, value.primary_id)?;
        put_u64(v, 24, value.secondary_id)?;
    }
    for value in &records.events {
        let v = add(&mut tables[10])?;
        put_u64(v, 0, value.operand_id)?;
        put_u16(v, 8, value.kind)?;
        put_u16(v, 10, value.flags)?;
        put_u32(v, 12, value.reference_begin)?;
        put_u32(v, 16, value.reference_count)?;
        put_u32(v, 20, value.source_set)?;
    }
    Ok(())
}

fn encode_output_tables(
    records: &ResultRecords,
    tables: &mut [TableBuilder],
) -> Result<(), AnalyticPacketError> {
    for value in &records.relationships {
        let v = add(&mut tables[11])?;
        put_u64(v, 0, value.query_id)?;
        v[8] = value.status;
        v[9] = value.dimension;
        put_u16(v, 10, value.flags)?;
        put_u32(v, 12, value.pair_begin)?;
        put_u32(v, 16, value.pair_count)?;
    }
    for value in &records.pairs {
        let v = add(&mut tables[12])?;
        put_u64(v, 0, value.left)?;
        put_u64(v, 8, value.right)?;
        v[16] = value.dimension;
        v[17] = value.equality;
        v[18] = value.left_contains;
        v[19] = value.right_contains;
    }
    for value in &records.source_indices {
        put_u32(add(&mut tables[13])?, 0, *value)?;
    }
    Ok(())
}

fn add(table: &mut TableBuilder) -> Result<&mut [u8], AnalyticPacketError> {
    let index = table.reserve_record()?;
    table.record_mut(index)
}
