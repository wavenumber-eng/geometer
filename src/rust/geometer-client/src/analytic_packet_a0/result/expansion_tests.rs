use super::{
    MAX_LOGICAL_SOURCE_REFERENCE_EXPANSIONS, preflight_logical_source_reference_expansions,
};
use crate::analytic_packet_a0::result::encode::RECORD_BYTES;
use crate::analytic_packet_a0::wire::{Table, TableBuilder, encode_tables};
use crate::analytic_packet_a0::{
    AnalyticPacketError, AnalyticPacketErrorKind,
    decode_analytic_planar_boolean_batch_result_a0_packet,
};

type Use = (usize, usize, usize, u32);

fn preflight(count: u32, uses: &[Use]) -> Result<(), AnalyticPacketError> {
    let mut storage: [Vec<u8>; RECORD_BYTES.len()] = std::array::from_fn(|_| Vec::new());
    for &(table, record, offset, handle) in uses {
        storage[table].resize((record + 1) * RECORD_BYTES[table], 0);
        let begin = record * RECORD_BYTES[table] + offset;
        storage[table][begin..begin + 4].copy_from_slice(&handle.to_le_bytes());
    }
    storage[8].resize(RECORD_BYTES[8], 0);
    storage[8][4..8].copy_from_slice(&count.to_le_bytes());
    let tables: [Table<'_>; RECORD_BYTES.len()] = std::array::from_fn(|index| Table {
        kind: 101 + index as u16,
        record_bytes: RECORD_BYTES[index],
        count: storage[index].len() / RECORD_BYTES[index],
        data: &storage[index],
    });
    preflight_logical_source_reference_expansions(&tables)
}

fn compact_packet(count: u32, uses: &[Use]) -> Vec<u8> {
    let mut tables: Vec<_> = RECORD_BYTES
        .iter()
        .enumerate()
        .map(|(index, bytes)| TableBuilder::new(101 + index as u16, *bytes))
        .collect();
    for &(table, record, offset, handle) in uses {
        while tables[table].count().unwrap() <= record {
            tables[table].reserve_record().unwrap();
        }
        tables[table].record_mut(record).unwrap()[offset..offset + 4]
            .copy_from_slice(&handle.to_le_bytes());
    }
    tables[8].reserve_record().unwrap();
    tables[8].record_mut(0).unwrap()[4..8].copy_from_slice(&count.to_le_bytes());
    encode_tables(b"GMABRS01", &tables, 0, 0).unwrap()
}

#[test]
fn compact_expansion_preflight_covers_every_use_and_repeated_handles() {
    const AUTHORITATIVE_LIMIT: usize = 1_048_576;
    assert_eq!(MAX_LOGICAL_SOURCE_REFERENCE_EXPANSIONS, AUTHORITATIVE_LIMIT);
    let uses = [
        (2, 0, 24, 1),
        (3, 0, 32, 1),
        (3, 0, 36, 1),
        (6, 0, 12, 1),
        (10, 0, 20, 1),
    ];
    for use_slot in uses {
        preflight(AUTHORITATIVE_LIMIT as u32, &[use_slot]).unwrap();
        assert_eq!(
            preflight(AUTHORITATIVE_LIMIT as u32 + 1, &[use_slot])
                .unwrap_err()
                .kind(),
            AnalyticPacketErrorKind::LimitExceeded
        );
    }

    let repeated = [(2, 0, 24, 1), (10, 0, 20, 1)];
    preflight((AUTHORITATIVE_LIMIT / 2) as u32, &repeated).unwrap();
    assert_eq!(
        preflight((AUTHORITATIVE_LIMIT / 2 + 1) as u32, &repeated)
            .unwrap_err()
            .kind(),
        AnalyticPacketErrorKind::LimitExceeded
    );
    assert_eq!(
        preflight(1, &[(2, 0, 24, 2)]).unwrap_err().kind(),
        AnalyticPacketErrorKind::InvalidReference
    );
}

#[test]
fn public_decoder_runs_expansion_preflight_before_record_projection() {
    const AUTHORITATIVE_LIMIT: u32 = 1_048_576;
    let use_slot = [(2, 0, 24, 1)];
    let exact = decode_analytic_planar_boolean_batch_result_a0_packet(&compact_packet(
        AUTHORITATIVE_LIMIT,
        &use_slot,
    ))
    .unwrap_err();
    assert_ne!(exact.kind(), AnalyticPacketErrorKind::LimitExceeded);
    assert_eq!(
        decode_analytic_planar_boolean_batch_result_a0_packet(&compact_packet(
            AUTHORITATIVE_LIMIT + 1,
            &use_slot,
        ))
        .unwrap_err()
        .kind(),
        AnalyticPacketErrorKind::LimitExceeded
    );
    assert_eq!(
        decode_analytic_planar_boolean_batch_result_a0_packet(
            &compact_packet(1, &[(2, 0, 24, 2)],)
        )
        .unwrap_err()
        .kind(),
        AnalyticPacketErrorKind::InvalidReference
    );
}
