use super::super::error::{AnalyticPacketError, invalid_reference, limit_exceeded};
use super::super::wire::{Table, u32_at};

pub(crate) const MAX_LOGICAL_SOURCE_REFERENCE_EXPANSIONS: usize = 1_048_576;

pub(crate) fn preflight_logical_source_reference_expansions(
    tables: &[Table<'_>],
) -> Result<(), AnalyticPacketError> {
    let vertices = table(tables, 2)?;
    let fragments = table(tables, 3)?;
    let regions = table(tables, 6)?;
    let source_sets = table(tables, 8)?;
    let events = table(tables, 10)?;
    let mut total = 0_usize;
    for index in 0..vertices.count {
        charge_handle(
            &mut total,
            source_sets,
            u32_at(vertices.record(index)?, 24)?,
        )?;
    }
    for index in 0..fragments.count {
        let record = fragments.record(index)?;
        charge_handle(&mut total, source_sets, u32_at(record, 32)?)?;
        charge_handle(&mut total, source_sets, u32_at(record, 36)?)?;
    }
    for index in 0..regions.count {
        charge_handle(&mut total, source_sets, u32_at(regions.record(index)?, 12)?)?;
    }
    for index in 0..events.count {
        charge_handle(&mut total, source_sets, u32_at(events.record(index)?, 20)?)?;
    }
    Ok(())
}

fn table<'a>(tables: &'a [Table<'a>], index: usize) -> Result<&'a Table<'a>, AnalyticPacketError> {
    tables
        .get(index)
        .ok_or_else(|| invalid_reference("result expansion table is missing"))
}

fn charge_handle(
    total: &mut usize,
    source_sets: &Table<'_>,
    handle: u32,
) -> Result<(), AnalyticPacketError> {
    if handle == 0 {
        return Ok(());
    }
    let index = handle as usize - 1;
    if index >= source_sets.count {
        return Err(invalid_reference(
            "logical source-set handle is out of range",
        ));
    }
    let count = u32_at(source_sets.record(index)?, 4)? as usize;
    *total = total
        .checked_add(count)
        .ok_or_else(|| limit_exceeded("logical source-reference expansion count overflow"))?;
    if *total > MAX_LOGICAL_SOURCE_REFERENCE_EXPANSIONS {
        return Err(limit_exceeded(
            "logical source-reference expansion limit exceeded",
        ));
    }
    Ok(())
}

#[cfg(test)]
#[path = "expansion_tests.rs"]
mod tests;
