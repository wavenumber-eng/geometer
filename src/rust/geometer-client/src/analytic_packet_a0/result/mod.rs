mod encode;
mod expansion;
mod project;
mod records;
mod standalone;
mod validate;

use crate::generated::contracts::{AnalyticPlanarBooleanBatchResultA0, Validate};

use super::error::{AnalyticPacketError, invalid_packet};
use super::wire::{decode_directory, u32_at};

const RESULT_MAGIC: &[u8; 8] = b"GMABRS01";

/// Strictly decode GMABRS01 and project it to the public logical DTO graph.
pub fn decode_analytic_planar_boolean_batch_result_a0_packet(
    bytes: &[u8],
) -> Result<AnalyticPlanarBooleanBatchResultA0, AnalyticPacketError> {
    let tables = decode_directory(bytes, RESULT_MAGIC, 101, &encode::RECORD_BYTES)?;
    expansion::preflight_logical_source_reference_expansions(&tables)?;
    let records = records::parse(&tables)?;
    if u32_at(bytes, 36)? as usize != records.jobs.len()
        || u32_at(bytes, 40)? as usize != records.relationships.len()
    {
        return Err(invalid_packet(
            "result header counts do not match their tables",
        ));
    }
    let owners = validate::validate(&records)?;
    if encode::encode(&records)? != bytes {
        return Err(invalid_packet("result packet is not canonically encoded"));
    }
    let selections = standalone::selections(&records, &owners)?;
    let digests = selections
        .iter()
        .enumerate()
        .map(|(index, selection)| standalone::digest(&records, index, selection))
        .collect::<Result<Vec<_>, _>>()?;
    let result = project::project(&records, &selections, &digests)?;
    result
        .validate_at("")
        .map_err(|error| invalid_packet(error.to_string()))?;
    Ok(result)
}
