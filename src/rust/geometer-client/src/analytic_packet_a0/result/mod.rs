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

#[cfg(test)]
mod resolution_warning_codec_tests {
    use super::decode_analytic_planar_boolean_batch_result_a0_packet;
    use super::encode::encode;
    use super::records::{DiagnosticRecord, JobRecord, ResultRecords};
    use crate::generated::contracts::{AnalyticPlanarBooleanJobResult, JobDiagnosticCode};

    #[test]
    fn successful_resolution_warning_round_trips_through_public_decoder() {
        let records = ResultRecords {
            jobs: vec![JobRecord {
                id: 1,
                status: 0,
                diagnostic_begin: 0,
                diagnostic_count: 1,
                region_begin: 0,
                region_count: 0,
                event_begin: 0,
                event_count: 0,
            }],
            diagnostics: vec![DiagnosticRecord {
                code: 0x1000c,
                severity: 2,
                scope: 1,
                presence: 15,
                job_id: 1,
                stage_id: 2,
                operand_id: 3,
                geometry_id: 4,
                path_token: 8,
                detail_token: 0,
            }],
            vertices: vec![],
            fragments: vec![],
            rings: vec![],
            fragment_references: vec![],
            regions: vec![],
            ring_region_references: vec![],
            source_sets: vec![],
            sources: vec![],
            events: vec![],
            relationships: vec![],
            pairs: vec![],
            source_indices: vec![],
        };
        let decoded = decode_analytic_planar_boolean_batch_result_a0_packet(
            &encode(&records).expect("encode resolution warning"),
        )
        .expect("decode resolution warning");
        assert_eq!(decoded.job_results.len(), 1);
        let AnalyticPlanarBooleanJobResult::Success(job) = &decoded.job_results[0] else {
            panic!("resolution warning changed successful job status");
        };
        assert_eq!(
            job.diagnostics[0].code,
            JobDiagnosticCode::ResolutionCoalesced
        );
    }
}
