//! Job-local accounting; generation is bookkeeping, never result authority.

use std::fs;
use std::io::Read;
use std::path::Path;
use std::time::SystemTime;

use sha2::{Digest, Sha256};

use super::{DiskCacheLayout, DiskCacheLimits, io, read_record};

const MARKER_BYTES: usize = 51;

#[derive(Default)]
pub(super) struct Memo {
    current: Option<Inventory>,
    #[cfg(test)]
    pub(super) scans: usize,
}

struct Inventory {
    generation: u64,
    fingerprint: SystemTime,
    entries: Vec<io::Entry>,
}

enum Marker {
    Missing,
    Managed { generation: u64, dirty: bool },
    Foreign,
}

pub(super) struct Transaction {
    inventory: Option<Inventory>,
    next_generation: Option<u64>,
}

pub(super) fn begin(root: &Path, layout: &DiskCacheLayout, memo: &mut Memo) -> Option<Transaction> {
    let state = marker_read(root, layout)?;
    let prior = memo.current.take();
    let (generation, dirty) = match state {
        Marker::Missing => (0, true),
        Marker::Managed { generation, dirty } => (generation, dirty),
        Marker::Foreign => {
            return Some(Transaction {
                inventory: None,
                next_generation: None,
            });
        }
    };
    let fingerprint = fs::metadata(root).ok()?.modified().ok()?;
    let inventory = match prior {
        Some(inventory)
            if !dirty
                && inventory.generation == generation
                && inventory.fingerprint == fingerprint =>
        {
            inventory
        }
        _ => scan(root, layout, generation, fingerprint, memo)?,
    };
    write_marker(root, layout, generation, true)?;
    Some(Transaction {
        inventory: Some(inventory),
        next_generation: generation.checked_add(1),
    })
}

fn scan(
    root: &Path,
    layout: &DiskCacheLayout,
    generation: u64,
    fingerprint: SystemTime,
    memo: &mut Memo,
) -> Option<Inventory> {
    #[cfg(test)]
    {
        memo.scans += 1;
    }
    #[cfg(not(test))]
    let _ = memo;
    Some(Inventory {
        generation,
        fingerprint,
        entries: io::entries(root, layout)?,
    })
}

impl Transaction {
    pub(super) fn remove(&mut self, path: &Path) {
        if let Some(inventory) = &mut self.inventory {
            inventory.entries.retain(|entry| entry.path != path);
        }
    }

    pub(super) fn add(&mut self, path: &Path) -> Option<()> {
        if let Some(inventory) = &mut self.inventory {
            let metadata = fs::symlink_metadata(path).ok()?;
            if io::is_link(&metadata) || !metadata.is_file() {
                return None;
            }
            inventory.entries.push(io::Entry {
                path: path.to_path_buf(),
                bytes: metadata.len(),
                modified: metadata.modified().ok()?,
            });
        }
        Some(())
    }

    pub(super) fn reserve(
        &mut self,
        root: &Path,
        incoming: u64,
        layout: &DiskCacheLayout,
        limits: DiskCacheLimits,
    ) -> Option<()> {
        let Some(inventory) = &mut self.inventory else {
            return io::reserve(root, incoming, layout, limits);
        };
        if limits.entries == 0 || incoming > limits.bytes {
            return None;
        }
        inventory
            .entries
            .sort_by(|a, b| a.modified.cmp(&b.modified).then(a.path.cmp(&b.path)));
        let mut bytes = inventory
            .entries
            .iter()
            .try_fold(0u64, |sum, entry| sum.checked_add(entry.bytes))?;
        let mut index = 0;
        while inventory.entries.len() >= limits.entries || bytes > limits.bytes - incoming {
            let entry = inventory.entries.get(index)?;
            if read_record(&entry.path, layout, limits).is_some() {
                io::directory(root, false)?;
                fs::remove_file(&entry.path).ok()?;
                bytes = bytes.checked_sub(entry.bytes)?;
                inventory.entries.remove(index);
            } else {
                index += 1;
            }
        }
        Some(())
    }

    pub(super) fn commit(
        self,
        root: &Path,
        layout: &DiskCacheLayout,
        memo: &mut Memo,
    ) -> Option<()> {
        let Some(mut inventory) = self.inventory else {
            return Some(());
        };
        let Some(next) = self.next_generation else {
            // Overflow stays dirty: every future writer scans, never wraps.
            return Some(());
        };
        write_marker(root, layout, next, false)?;
        inventory.generation = next;
        inventory.fingerprint = fs::metadata(root).ok()?.modified().ok()?;
        memo.current = Some(inventory);
        Some(())
    }
}

fn marker_read(root: &Path, layout: &DiskCacheLayout) -> Option<Marker> {
    let path = root.join(layout.marker_name());
    let metadata = match fs::symlink_metadata(&path) {
        Ok(metadata) => metadata,
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => return Some(Marker::Missing),
        Err(_) => return None,
    };
    if io::is_link(&metadata) || !metadata.is_file() || metadata.len() != MARKER_BYTES as u64 {
        return Some(Marker::Foreign);
    }
    let mut bytes = Vec::new();
    fs::File::open(path)
        .ok()?
        .take(MARKER_BYTES as u64 + 1)
        .read_to_end(&mut bytes)
        .ok()?;
    let Some((generation, dirty)) = decode_marker(&bytes, &layout.marker_magic()) else {
        return Some(Marker::Foreign);
    };
    Some(Marker::Managed { generation, dirty })
}

fn decode_marker(bytes: &[u8], magic: &[u8; 10]) -> Option<(u64, bool)> {
    if bytes.len() != MARKER_BYTES || &bytes[..10] != magic || bytes[18] > 1 {
        return None;
    }
    if !Sha256::digest(&bytes[..19])
        .iter()
        .copied()
        .eq(bytes[19..].iter().copied())
    {
        return None;
    }
    Some((
        u64::from_be_bytes(bytes[10..18].try_into().ok()?),
        bytes[18] == 1,
    ))
}

fn write_marker(root: &Path, layout: &DiskCacheLayout, generation: u64, dirty: bool) -> Option<()> {
    let mut bytes = layout.marker_magic().to_vec();
    bytes.extend_from_slice(&generation.to_be_bytes());
    bytes.push(u8::from(dirty));
    let checksum = Sha256::digest(&bytes);
    bytes.extend(checksum.iter().copied());
    io::marker_write(root, layout, &bytes)
}

#[cfg(test)]
pub(super) fn dirty_for_test(root: &Path, layout: &DiskCacheLayout) {
    let state = marker_read(root, layout).unwrap();
    let Marker::Managed { generation, .. } = state else {
        panic!("expected managed marker");
    };
    write_marker(root, layout, generation, true).unwrap();
}

#[cfg(test)]
pub(super) fn generation_for_test(
    root: &Path,
    layout: &DiskCacheLayout,
    generation: u64,
    dirty: bool,
) {
    write_marker(root, layout, generation, dirty).unwrap();
}

#[cfg(test)]
pub(super) fn marker_for_test(root: &Path, layout: &DiskCacheLayout) -> (u64, bool) {
    match marker_read(root, layout).unwrap() {
        Marker::Managed { generation, dirty } => (generation, dirty),
        _ => panic!("expected managed marker"),
    }
}
