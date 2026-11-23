//! Cache-local guarded staging and bounded oldest-write eviction.

use std::fs::{self, File, OpenOptions};
use std::io::Write;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::SystemTime;

use super::{
    Compatibility, DiskCacheLayout, DiskCacheLimits, inventory, owned_header, read_record,
    valid_filename,
};
use crate::runtime::ResultIdentity;

static SEQUENCE: AtomicU64 = AtomicU64::new(0);
const MAX_DIRECTORY_ROWS: usize = 8192;

/// One complete record ready to publish.
pub(super) struct Record<'a> {
    pub(super) key: &'a str,
    pub(super) header: &'a [u8],
    pub(super) response: &'a str,
}

pub(super) fn is_link(metadata: &fs::Metadata) -> bool {
    #[cfg(windows)]
    {
        use std::os::windows::fs::MetadataExt;
        metadata.file_attributes() & 0x400 != 0
    }
    #[cfg(not(windows))]
    {
        metadata.file_type().is_symlink()
    }
}

fn no_links(path: &Path) -> Option<()> {
    for ancestor in path.ancestors() {
        match fs::symlink_metadata(ancestor) {
            Ok(metadata) if is_link(&metadata) || !metadata.is_dir() => return None,
            Ok(_) => {}
            Err(error) if error.kind() == std::io::ErrorKind::NotFound => {}
            Err(_) => return None,
        }
    }
    Some(())
}

pub(super) fn directory(path: &Path, create: bool) -> Option<PathBuf> {
    no_links(path)?;
    if create {
        fs::create_dir_all(path).ok()?;
    }
    no_links(path)?;
    let root = path.canonicalize().ok()?;
    fs::metadata(&root).ok()?.is_dir().then_some(root)
}

pub(super) struct OwnedFile {
    pub(super) path: PathBuf,
    pub(super) file: File,
}

impl OwnedFile {
    pub(super) fn create(path: PathBuf) -> Option<Self> {
        let file = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(&path)
            .ok()?;
        Some(Self { path, file })
    }
}

impl Drop for OwnedFile {
    fn drop(&mut self) {
        if no_links(self.path.parent().unwrap_or(Path::new(""))).is_none() {
            return;
        }
        let same = self.file.try_clone().ok().and_then(|file| {
            let held = same_file::Handle::from_file(file).ok()?;
            Some(held == same_file::Handle::from_path(&self.path).ok()?)
        });
        if same == Some(true) {
            let _ = fs::remove_file(&self.path);
        }
    }
}

pub(super) fn publish(
    root: &Path,
    record: Record<'_>,
    layout: &DiskCacheLayout,
    limits: DiskCacheLimits,
    memo: &mut inventory::Memo,
) -> Option<()> {
    let _lock = writer_lock(root, layout)?;
    let target = root.join(format!("{}.cache", record.key));
    if target.exists() && read_record(&target, layout, limits).is_some() {
        return Some(());
    }
    let mut transaction = inventory::begin(root, layout, memo)?;
    if target.exists() {
        // Heal a corrupt body only when the complete header still proves that
        // this exact key belongs to the cache. Unknown/truncated headers stay.
        owned_header(&target, layout)?;
        fs::remove_file(&target).ok()?;
        transaction.remove(&target);
    }
    let bytes = record
        .header
        .len()
        .checked_add(1)?
        .checked_add(record.response.len())? as u64;
    transaction.reserve(root, bytes, layout, limits)?;
    let sequence = SEQUENCE.fetch_add(1, Ordering::Relaxed);
    let mut staged = OwnedFile::create(root.join(format!(
        ".{}.{}.{}.tmp",
        record.key,
        std::process::id(),
        sequence
    )))?;
    staged.file.write_all(record.header).ok()?;
    staged.file.write_all(b"\n").ok()?;
    staged.file.write_all(record.response.as_bytes()).ok()?;
    staged.file.sync_all().ok()?;
    no_links(root)?;
    // All cooperating publishers hold this OS file lock. An independently
    // existing destination is never replaced by the cache.
    if fs::symlink_metadata(&target).is_ok() {
        return None;
    }
    fs::rename(&staged.path, &target).ok()?;
    transaction.add(&target)?;
    transaction.commit(root, layout, memo)
}

pub(super) fn invalidate(
    root: &Path,
    key: &str,
    request: &str,
    identity: &ResultIdentity,
    layout: &DiskCacheLayout,
    memo: &mut inventory::Memo,
) -> Option<()> {
    let _lock = writer_lock(root, layout)?;
    let target = root.join(format!("{key}.cache"));
    let header = owned_header(&target, layout)?;
    if header.request_id != request || header.compatibility != Compatibility::from(identity) {
        return None;
    }
    let mut transaction = inventory::begin(root, layout, memo)?;
    no_links(root)?;
    fs::remove_file(&target).ok()?;
    transaction.remove(&target);
    transaction.commit(root, layout, memo)
}

fn writer_lock(root: &Path, layout: &DiskCacheLayout) -> Option<File> {
    let path = root.join(layout.lock_name());
    if let Ok(metadata) = fs::symlink_metadata(&path)
        && (is_link(&metadata) || !metadata.is_file() || metadata.len() != 0)
    {
        return None;
    }
    let file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .truncate(false)
        .open(&path)
        .ok()?;
    let metadata = fs::symlink_metadata(&path).ok()?;
    if is_link(&metadata) || metadata.len() != 0 {
        return None;
    }
    file.try_lock().ok()?;
    Some(file)
}

#[derive(Clone)]
pub(super) struct Entry {
    pub(super) path: PathBuf,
    pub(super) bytes: u64,
    pub(super) modified: SystemTime,
}

pub(super) fn entries(root: &Path, layout: &DiskCacheLayout) -> Option<Vec<Entry>> {
    let mut entries = Vec::new();
    for (index, row) in fs::read_dir(root).ok()?.enumerate() {
        if index >= MAX_DIRECTORY_ROWS {
            return None;
        }
        let row = row.ok()?;
        let path = row.path();
        if valid_filename(&path).is_none() {
            continue;
        }
        let metadata = fs::symlink_metadata(&path).ok()?;
        if is_link(&metadata) || !metadata.is_file() {
            continue;
        }
        if owned_header(&path, layout).is_none() {
            continue;
        }
        entries.push(Entry {
            path,
            bytes: metadata.len(),
            modified: metadata.modified().ok()?,
        });
    }
    entries.sort_by(|a, b| a.modified.cmp(&b.modified).then(a.path.cmp(&b.path)));
    Some(entries)
}

pub(super) fn reserve(
    root: &Path,
    incoming: u64,
    layout: &DiskCacheLayout,
    limits: DiskCacheLimits,
) -> Option<()> {
    if limits.entries == 0 || incoming > limits.bytes {
        return None;
    }
    let entries = entries(root, layout)?;
    let mut bytes = entries
        .iter()
        .try_fold(0_u64, |sum, entry| sum.checked_add(entry.bytes))?;
    let mut count = entries.len();
    for entry in entries {
        if count < limits.entries && bytes <= limits.bytes - incoming {
            return Some(());
        }
        // Only a fully verified cache envelope is eligible for eviction.
        if read_record(&entry.path, layout, limits).is_some() {
            no_links(root)?;
            fs::remove_file(&entry.path).ok()?;
            bytes = bytes.checked_sub(entry.bytes)?;
            count -= 1;
        }
    }
    (count < limits.entries && bytes <= limits.bytes - incoming).then_some(())
}

pub(super) fn marker_write(root: &Path, layout: &DiskCacheLayout, bytes: &[u8]) -> Option<()> {
    let sequence = SEQUENCE.fetch_add(1, Ordering::Relaxed);
    let mut staged = OwnedFile::create(root.join(format!(
        "{}.{}.{}.tmp",
        layout.marker_staging_prefix(),
        std::process::id(),
        sequence
    )))?;
    staged.file.write_all(bytes).ok()?;
    staged.file.sync_all().ok()?;
    no_links(root)?;
    fs::rename(&staged.path, root.join(layout.marker_name())).ok()
}
