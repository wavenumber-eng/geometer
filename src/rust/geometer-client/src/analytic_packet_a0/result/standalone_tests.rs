use super::{mapped, sparse_map};

#[test]
fn sparse_maps_scale_with_each_tiny_selection() {
    for selected in 0..10_000_usize {
        let mapping = sparse_map(&[selected], 10_000).unwrap();
        assert_eq!(mapping.len(), 1);
        assert_eq!(mapped(&mapping, selected as u32).unwrap(), 0);
    }
}

#[test]
fn sparse_maps_reject_out_of_range_and_duplicate_indexes() {
    assert!(sparse_map(&[10], 10).is_err());
    assert!(sparse_map(&[1, 1], 10).is_err());
}
