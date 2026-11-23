//! Advisory host storage for results the caller has already completed.
//! Reading an entry never replaces the caller's own completion checks.

mod inventory;
mod io;
#[cfg(test)]
mod tests;

use std::fs::File;
use std::io::{BufRead, BufReader, Read};
use std::path::{Path, PathBuf};
use std::sync::Mutex;

use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};

use super::ResultIdentity;

const MAX_HEADER_BYTES: usize = 64 * 1024;

/// Names the stored format and the cache's bookkeeping files, so caches of
/// different applications never adopt each other's records or markers.
///
/// The layout is an on-disk compatibility shape: every name below, the record
/// file name (the SHA-256 of the length-prefixed format, request digest and
/// identity fields), the record header's field order and the marker bytes are
/// derived from `format` and `namespace` alone. An application that keeps its
/// two strings keeps reading the records and markers its earlier releases
/// wrote; changing either string starts a separate cache. Hosts with an
/// established directory should pin these derived names in their own tests.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct DiskCacheLayout {
    format: String,
    namespace: String,
}

impl DiskCacheLayout {
    /// `format` is written into every record header (for example
    /// `app.completed-model-result.v1`). `namespace` (1 to 32 lowercase ASCII
    /// letters, digits or hyphens) names the writer lock
    /// `.{namespace}-write-lock`, the generation marker
    /// `.{namespace}-directory-generation` and its staging files
    /// `.{namespace}-generation.{pid}.{sequence}.tmp`. The marker is 51 bytes:
    /// a 10-byte magic (the first four namespace characters, uppercased and
    /// padded with `_`, then `CDIR01`; for example `APP_CDIR01`), the
    /// generation as a big-endian `u64`, a dirty byte, and the SHA-256 of
    /// those 19 bytes.
    pub fn new(format: impl Into<String>, namespace: impl Into<String>) -> Result<Self, String> {
        let format = format.into();
        let namespace = namespace.into();
        if format.is_empty() || format.len() > 256 || format.contains('\n') {
            return Err("disk cache format must be 1 to 256 bytes on one line".into());
        }
        let valid = |byte: u8| byte.is_ascii_lowercase() || byte.is_ascii_digit() || byte == b'-';
        if namespace.is_empty() || namespace.len() > 32 || !namespace.bytes().all(valid) {
            return Err("disk cache namespace must be 1 to 32 of [a-z0-9-]".into());
        }
        Ok(Self { format, namespace })
    }

    pub fn format(&self) -> &str {
        &self.format
    }

    pub fn namespace(&self) -> &str {
        &self.namespace
    }

    fn lock_name(&self) -> String {
        format!(".{}-write-lock", self.namespace)
    }

    fn marker_name(&self) -> String {
        format!(".{}-directory-generation", self.namespace)
    }

    fn marker_staging_prefix(&self) -> String {
        format!(".{}-generation", self.namespace)
    }

    fn marker_magic(&self) -> [u8; 10] {
        let mut magic = *b"____CDIR01";
        for (slot, byte) in magic.iter_mut().zip(self.namespace.bytes().take(4)) {
            *slot = byte.to_ascii_uppercase();
        }
        magic
    }
}

/// Storage bounds. Entries over `entry_bytes` are never stored; publishing
/// evicts the oldest complete records to stay within `bytes` and `entries`.
/// Bounds may be added, so the type is non-exhaustive: start from
/// [`DiskCacheLimits::default`] and adjust fields.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[non_exhaustive]
pub struct DiskCacheLimits {
    pub bytes: u64,
    pub entries: usize,
    pub entry_bytes: usize,
}

impl Default for DiskCacheLimits {
    fn default() -> Self {
        Self {
            bytes: 1024 * 1024 * 1024,
            entries: 4096,
            entry_bytes: 32 * 1024 * 1024,
        }
    }
}

/// A private, rebuildable result store in one host-chosen directory.
///
/// Corrupt, partial, oversized, foreign and unreadable records are misses.
/// IO or capacity failures never fail the caller's request.
pub struct DiskResultCache {
    directory: Option<PathBuf>,
    layout: DiskCacheLayout,
    limits: DiskCacheLimits,
    inventory: Mutex<inventory::Memo>,
}

impl DiskResultCache {
    /// `None` disables the store. A relative directory is made absolute now;
    /// if that fails the store is disabled. Nothing is created until a write.
    pub fn new(
        directory: Option<PathBuf>,
        layout: DiskCacheLayout,
        limits: DiskCacheLimits,
    ) -> Self {
        Self {
            directory: directory.and_then(|path| std::path::absolute(path).ok()),
            layout,
            limits,
            inventory: Mutex::default(),
        }
    }

    /// The absolute directory, when enabled.
    pub fn directory(&self) -> Option<&Path> {
        self.directory.as_deref()
    }

    /// Advisory preflight: whether the guarded directory exists or can be made.
    pub fn prepare_write_directory(&self) -> bool {
        self.directory
            .as_deref()
            .is_some_and(|path| io::directory(path, true).is_some())
    }

    /// A stored response for exactly this request digest and identity.
    pub fn get(&self, request: &str, identity: &ResultIdentity) -> Option<String> {
        let root = io::directory(self.directory.as_deref()?, false)?;
        let key = storage_key(&self.layout, request, identity)?;
        let record = read_record(
            &root.join(format!("{key}.cache")),
            &self.layout,
            self.limits,
        )?;
        (record.header.request_id == request
            && record.header.compatibility == Compatibility::from(identity))
        .then_some(record.response)
    }

    /// Store a response only after the caller's completion succeeded. An
    /// existing complete record is kept. Returns whether a record is present.
    pub fn insert(&self, request: &str, identity: &ResultIdentity, response: &str) -> bool {
        self.insert_record(request, identity, response).is_some()
    }

    /// Remove this exact record after the caller rejected it. Files the cache
    /// cannot prove it owns are preserved.
    pub fn invalidate(&self, request: &str, identity: &ResultIdentity) -> bool {
        let Some(root) = self
            .directory
            .as_deref()
            .and_then(|path| io::directory(path, false))
        else {
            return false;
        };
        let Some(key) = storage_key(&self.layout, request, identity) else {
            return false;
        };
        let Ok(mut memo) = self.inventory.lock() else {
            return false;
        };
        io::invalidate(&root, &key, request, identity, &self.layout, &mut memo).is_some()
    }

    /// Where this record would be stored; no IO.
    pub fn entry_path(&self, request: &str, identity: &ResultIdentity) -> Option<PathBuf> {
        let key = storage_key(&self.layout, request, identity)?;
        Some(self.directory.as_ref()?.join(format!("{key}.cache")))
    }

    fn insert_record(
        &self,
        request: &str,
        identity: &ResultIdentity,
        response: &str,
    ) -> Option<()> {
        if response.is_empty() || response.len() > self.limits.entry_bytes {
            return None;
        }
        let key = storage_key(&self.layout, request, identity)?;
        let header = Header {
            format: self.layout.format.clone(),
            request_id: request.into(),
            compatibility: Compatibility::from(identity),
            response_sha256: digest(response.as_bytes()),
            response_bytes: response.len() as u64,
        };
        let header_bytes = serde_json::to_vec(&header).ok()?;
        let bytes = header_bytes
            .len()
            .checked_add(1)?
            .checked_add(response.len())?;
        if header_bytes.len() > MAX_HEADER_BYTES || bytes > self.limits.entry_bytes {
            return None;
        }
        let root = io::directory(self.directory.as_deref()?, true)?;
        let mut memo = self.inventory.lock().ok()?;
        let record = io::Record {
            key: &key,
            header: &header_bytes,
            response,
        };
        io::publish(&root, record, &self.layout, self.limits, &mut memo)
    }
}

#[derive(Serialize, Deserialize, PartialEq)]
#[serde(deny_unknown_fields)]
struct Compatibility {
    operation: String,
    result_schema: String,
    geometer_release: String,
    geometer_abi: String,
    artifact_sha256: String,
}

impl From<&ResultIdentity> for Compatibility {
    fn from(value: &ResultIdentity) -> Self {
        Self {
            operation: value.operation.clone(),
            result_schema: value.result_schema.clone(),
            geometer_release: value.geometer_release.clone(),
            geometer_abi: value.geometer_abi.clone(),
            artifact_sha256: value.artifact_sha256.clone(),
        }
    }
}

impl Compatibility {
    fn identity(&self) -> ResultIdentity {
        ResultIdentity {
            operation: self.operation.clone(),
            result_schema: self.result_schema.clone(),
            geometer_release: self.geometer_release.clone(),
            geometer_abi: self.geometer_abi.clone(),
            artifact_sha256: self.artifact_sha256.clone(),
        }
    }
}

#[derive(Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
struct Header {
    format: String,
    request_id: String,
    compatibility: Compatibility,
    response_sha256: String,
    response_bytes: u64,
}

struct StoredRecord {
    header: Header,
    response: String,
}

fn storage_key(
    layout: &DiskCacheLayout,
    request: &str,
    identity: &ResultIdentity,
) -> Option<String> {
    let values = [
        layout.format.as_str(),
        request,
        &identity.operation,
        &identity.result_schema,
        &identity.geometer_release,
        &identity.geometer_abi,
        &identity.artifact_sha256,
    ];
    if values
        .iter()
        .any(|value| value.is_empty() || value.len() > 4096)
    {
        return None;
    }
    let mut hash = Sha256::new();
    for value in values {
        hash.update((value.len() as u64).to_be_bytes());
        hash.update(value.as_bytes());
    }
    Some(hex(hash.finalize()))
}

fn digest(bytes: &[u8]) -> String {
    hex(Sha256::digest(bytes))
}

fn hex(bytes: impl AsRef<[u8]>) -> String {
    bytes
        .as_ref()
        .iter()
        .map(|byte| format!("{byte:02x}"))
        .collect()
}

fn valid_filename(path: &Path) -> Option<&str> {
    let stem = path.file_name()?.to_str()?.strip_suffix(".cache")?;
    (stem.len() == 64 && stem.bytes().all(|byte| byte.is_ascii_hexdigit())).then_some(stem)
}

fn read_record(
    path: &Path,
    layout: &DiskCacheLayout,
    limits: DiskCacheLimits,
) -> Option<StoredRecord> {
    let key = valid_filename(path)?;
    let metadata = std::fs::symlink_metadata(path).ok()?;
    if io::is_link(&metadata) || !metadata.is_file() || metadata.len() > limits.entry_bytes as u64 {
        return None;
    }
    let mut bytes = Vec::new();
    File::open(path)
        .ok()?
        .take(limits.entry_bytes as u64 + 1)
        .read_to_end(&mut bytes)
        .ok()?;
    if bytes.len() > limits.entry_bytes {
        return None;
    }
    let split = bytes.iter().position(|byte| *byte == b'\n')?;
    if split > MAX_HEADER_BYTES {
        return None;
    }
    let header: Header = serde_json::from_slice(&bytes[..split]).ok()?;
    let response = String::from_utf8(bytes[split + 1..].to_vec()).ok()?;
    let expected_key = storage_key(layout, &header.request_id, &header.compatibility.identity())?;
    (header.format == layout.format
        && key == expected_key
        && header.response_bytes == response.len() as u64
        && !response.is_empty()
        && header.response_sha256 == digest(response.as_bytes()))
    .then_some(StoredRecord { header, response })
}

/// A complete header proves cache ownership of its key even when the stored
/// body was corrupted or truncated. It does not qualify the body.
fn owned_header(path: &Path, layout: &DiskCacheLayout) -> Option<Header> {
    let key = valid_filename(path)?;
    let metadata = std::fs::symlink_metadata(path).ok()?;
    if io::is_link(&metadata) || !metadata.is_file() {
        return None;
    }
    let mut bytes = Vec::new();
    BufReader::new(File::open(path).ok()?.take(MAX_HEADER_BYTES as u64 + 1))
        .read_until(b'\n', &mut bytes)
        .ok()?;
    if bytes.len() > MAX_HEADER_BYTES || bytes.pop()? != b'\n' {
        return None;
    }
    let header: Header = serde_json::from_slice(&bytes).ok()?;
    let expected = storage_key(layout, &header.request_id, &header.compatibility.identity())?;
    (header.format == layout.format && key == expected).then_some(header)
}
