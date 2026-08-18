use std::collections::{BTreeSet, HashMap};

use sha2::{Digest, Sha256};

use super::super::error::{AnalyticPacketError, invalid_reference, limit_exceeded};
use super::encode;
use super::records::ResultRecords;
use super::validate::{Ownership, range};

const NONE: u32 = u32::MAX;
type SparseMap = HashMap<usize, u32>;

#[derive(Clone, Debug)]
pub(crate) struct Selection {
    pub vertices: Vec<usize>,
    pub fragments: Vec<usize>,
    pub rings: Vec<usize>,
    pub regions: Vec<usize>,
    pub events: Vec<usize>,
    pub sets: Vec<usize>,
    pub sources: Vec<usize>,
}

pub(crate) fn selections(
    records: &ResultRecords,
    owners: &Ownership,
) -> Result<Vec<Selection>, AnalyticPacketError> {
    let mut output: Vec<_> = records
        .jobs
        .iter()
        .map(|_| Selection {
            vertices: Vec::new(),
            fragments: Vec::new(),
            rings: Vec::new(),
            regions: Vec::new(),
            events: Vec::new(),
            sets: Vec::new(),
            sources: Vec::new(),
        })
        .collect();
    append_owned(&mut output, &owners.vertices, |value| &mut value.vertices)?;
    append_owned(&mut output, &owners.fragments, |value| &mut value.fragments)?;
    append_owned(&mut output, &owners.rings, |value| &mut value.rings)?;
    append_owned(&mut output, &owners.regions, |value| &mut value.regions)?;
    append_owned(&mut output, &owners.events, |value| &mut value.events)?;
    let mut sets: Vec<BTreeSet<usize>> = records.jobs.iter().map(|_| BTreeSet::new()).collect();
    for (index, value) in records.vertices.iter().enumerate() {
        mark_set(&mut sets[owners.vertices[index]], value.source_set);
    }
    for (index, value) in records.fragments.iter().enumerate() {
        mark_set(&mut sets[owners.fragments[index]], value.positive_set);
        mark_set(&mut sets[owners.fragments[index]], value.subtraction_set);
    }
    for (index, value) in records.regions.iter().enumerate() {
        mark_set(&mut sets[owners.regions[index]], value.positive_set);
    }
    for (index, value) in records.events.iter().enumerate() {
        mark_set(&mut sets[owners.events[index]], value.source_set);
    }
    for (job, selected_sets) in sets.into_iter().enumerate() {
        output[job].sets = selected_sets.into_iter().collect();
        let mut sources = BTreeSet::new();
        for set_index in &output[job].sets {
            let set = &records.source_sets[*set_index];
            sources.extend(
                range(&records.source_indices, set.begin, set.count)?
                    .iter()
                    .map(|value| *value as usize),
            );
        }
        output[job].sources = sources.into_iter().collect();
    }
    Ok(output)
}

pub(crate) fn digest(
    records: &ResultRecords,
    job_index: usize,
    selection: &Selection,
) -> Result<String, AnalyticPacketError> {
    let packet = encode::encode(&rebase(records, job_index, selection)?)?;
    Ok(Sha256::digest(packet)
        .iter()
        .map(|value| format!("{value:02x}"))
        .collect())
}

fn rebase(
    input: &ResultRecords,
    job_index: usize,
    selected: &Selection,
) -> Result<ResultRecords, AnalyticPacketError> {
    let vertex_map = sparse_map(&selected.vertices, input.vertices.len())?;
    let fragment_map = sparse_map(&selected.fragments, input.fragments.len())?;
    let ring_map = sparse_map(&selected.rings, input.rings.len())?;
    let region_map = sparse_map(&selected.regions, input.regions.len())?;
    let set_map = sparse_map(&selected.sets, input.source_sets.len())?;
    let source_map = sparse_map(&selected.sources, input.sources.len())?;
    let job = input
        .jobs
        .get(job_index)
        .ok_or_else(|| invalid_reference("standalone job index is out of range"))?;
    let mut output = ResultRecords {
        jobs: Vec::new(),
        diagnostics: range(
            &input.diagnostics,
            job.diagnostic_begin,
            job.diagnostic_count,
        )?
        .to_vec(),
        vertices: Vec::new(),
        fragments: Vec::new(),
        rings: Vec::new(),
        fragment_references: Vec::new(),
        regions: Vec::new(),
        ring_region_references: Vec::new(),
        source_sets: Vec::new(),
        sources: selected
            .sources
            .iter()
            .map(|index| input.sources[*index].clone())
            .collect(),
        events: Vec::new(),
        relationships: Vec::new(),
        pairs: Vec::new(),
        source_indices: Vec::new(),
    };
    rebase_geometry(
        input,
        selected,
        &mut output,
        &vertex_map,
        &fragment_map,
        &ring_map,
        &set_map,
    )?;
    rebase_regions(input, selected, &mut output, &ring_map, &set_map)?;
    rebase_sets(input, selected, &mut output, &source_map)?;
    rebase_events(
        input,
        selected,
        &mut output,
        &ring_map,
        &region_map,
        &set_map,
    )?;
    let mut standalone_job = job.clone();
    standalone_job.diagnostic_begin = 0;
    standalone_job.region_begin = 0;
    standalone_job.event_begin = 0;
    output.jobs.push(standalone_job);
    Ok(output)
}

fn rebase_geometry(
    input: &ResultRecords,
    selected: &Selection,
    output: &mut ResultRecords,
    vertices: &SparseMap,
    fragments: &SparseMap,
    rings: &SparseMap,
    sets: &SparseMap,
) -> Result<(), AnalyticPacketError> {
    for index in &selected.vertices {
        let mut value = input.vertices[*index].clone();
        value.id = output.vertices.len() as u64 + 1;
        value.source_set = remap_handle(value.source_set, sets)?;
        output.vertices.push(value);
    }
    for index in &selected.fragments {
        let mut value = input.fragments[*index].clone();
        value.id = output.fragments.len() as u64 + 1;
        value.start = mapped(vertices, value.start)?;
        value.end = mapped(vertices, value.end)?;
        value.positive_set = remap_handle(value.positive_set, sets)?;
        value.subtraction_set = remap_handle(value.subtraction_set, sets)?;
        output.fragments.push(value);
    }
    for index in &selected.rings {
        let mut value = input.rings[*index].clone();
        let original_begin = value.reference_begin;
        value.id = output.rings.len() as u64 + 1;
        value.reference_begin = output.fragment_references.len() as u32;
        for reference in range(
            &input.fragment_references,
            original_begin,
            value.reference_count,
        )? {
            output
                .fragment_references
                .push(mapped(fragments, *reference)?);
        }
        value.parent = if value.parent == NONE {
            NONE
        } else {
            mapped(rings, value.parent)?
        };
        output.rings.push(value);
    }
    Ok(())
}

fn rebase_regions(
    input: &ResultRecords,
    selected: &Selection,
    output: &mut ResultRecords,
    rings: &SparseMap,
    sets: &SparseMap,
) -> Result<(), AnalyticPacketError> {
    for index in &selected.regions {
        let mut value = input.regions[*index].clone();
        value.id = output.regions.len() as u64 + 1;
        value.outer = mapped(rings, value.outer)?;
        value.positive_set = remap_handle(value.positive_set, sets)?;
        output.regions.push(value);
    }
    Ok(())
}

fn rebase_sets(
    input: &ResultRecords,
    selected: &Selection,
    output: &mut ResultRecords,
    sources: &SparseMap,
) -> Result<(), AnalyticPacketError> {
    for index in &selected.sets {
        let mut value = input.source_sets[*index].clone();
        let original_begin = value.begin;
        value.begin = output.source_indices.len() as u32;
        for source in range(&input.source_indices, original_begin, value.count)? {
            output.source_indices.push(mapped(sources, *source)?);
        }
        output.source_sets.push(value);
    }
    Ok(())
}

fn rebase_events(
    input: &ResultRecords,
    selected: &Selection,
    output: &mut ResultRecords,
    rings: &SparseMap,
    regions: &SparseMap,
    sets: &SparseMap,
) -> Result<(), AnalyticPacketError> {
    for index in &selected.events {
        let mut value = input.events[*index].clone();
        let original_begin = value.reference_begin;
        value.reference_begin = if value.reference_count == 0 {
            0
        } else {
            output.ring_region_references.len() as u32
        };
        for reference in range(
            &input.ring_region_references,
            original_begin,
            value.reference_count,
        )? {
            let kind = reference >> 32;
            let target = (*reference & u64::from(u32::MAX)) as u32;
            let mapped_target = match kind {
                1 => mapped(rings, target)?,
                2 => mapped(regions, target)?,
                _ => {
                    return Err(invalid_reference(
                        "standalone event reference kind is invalid",
                    ));
                }
            };
            output
                .ring_region_references
                .push((kind << 32) | u64::from(mapped_target));
        }
        value.source_set = remap_handle(value.source_set, sets)?;
        output.events.push(value);
    }
    Ok(())
}

fn append_owned(
    selections: &mut [Selection],
    owners: &[usize],
    field: impl Fn(&mut Selection) -> &mut Vec<usize>,
) -> Result<(), AnalyticPacketError> {
    for (index, owner) in owners.iter().copied().enumerate() {
        field(
            selections
                .get_mut(owner)
                .ok_or_else(|| invalid_reference("owner job is out of range"))?,
        )
        .push(index);
    }
    Ok(())
}

fn mark_set(sets: &mut BTreeSet<usize>, handle: u32) {
    if handle != 0 {
        sets.insert(handle as usize - 1);
    }
}

fn sparse_map(indexes: &[usize], length: usize) -> Result<SparseMap, AnalyticPacketError> {
    let mut output = HashMap::with_capacity(indexes.len());
    for (new, old) in indexes.iter().copied().enumerate() {
        if old >= length {
            return Err(invalid_reference("selected index is out of range"));
        }
        let mapped =
            u32::try_from(new).map_err(|_| limit_exceeded("standalone index exceeds u32"))?;
        if output.insert(old, mapped).is_some() {
            return Err(invalid_reference("selected index is duplicated"));
        }
    }
    Ok(output)
}

fn mapped(mapping: &SparseMap, index: u32) -> Result<u32, AnalyticPacketError> {
    mapping
        .get(&(index as usize))
        .copied()
        .ok_or_else(|| invalid_reference("standalone closure contains an unmapped reference"))
}

fn remap_handle(handle: u32, mapping: &SparseMap) -> Result<u32, AnalyticPacketError> {
    if handle == 0 {
        Ok(0)
    } else {
        mapped(mapping, handle - 1)?
            .checked_add(1)
            .ok_or_else(|| limit_exceeded("source-set handle overflow"))
    }
}

#[cfg(test)]
#[path = "standalone_tests.rs"]
mod tests;
