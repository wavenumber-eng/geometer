//! Observed executable identity and the result identity derived from it.

use std::fs::{File, OpenOptions};
use std::io::{Read, Seek};
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};

/// The executable a worker actually started, as observed during startup.
///
/// `artifact_sha256` is `sha256:<hex>` of the bytes that were opened with
/// writes denied, spawned, and hashed again after the handshake.
///
/// Further transports may add identity fields, so the type is
/// non-exhaustive; construct it with [`ArtifactIdentity::new`].
#[derive(Clone, Debug, PartialEq, Eq)]
#[non_exhaustive]
pub struct ArtifactIdentity {
    /// Canonical path of the started executable.
    pub path: PathBuf,
    /// Release version reported by the welcome.
    pub release_version: String,
    /// C ABI generation reported by the welcome.
    pub c_abi_generation: u32,
    /// `sha256:<hex>` digest of the executable bytes.
    pub artifact_sha256: String,
}

impl ArtifactIdentity {
    /// An identity from its observed parts.
    pub fn new(
        path: PathBuf,
        release_version: impl Into<String>,
        c_abi_generation: u32,
        artifact_sha256: impl Into<String>,
    ) -> Self {
        Self {
            path,
            release_version: release_version.into(),
            c_abi_generation,
            artifact_sha256: artifact_sha256.into(),
        }
    }

    /// The ABI token used in result identities: `geometer.browser_c_abi.v<generation>`.
    pub fn abi(&self) -> String {
        format!("geometer.browser_c_abi.v{}", self.c_abi_generation)
    }

    /// The identity of results this artifact produces for one operation.
    pub fn result_identity(&self, operation: &str, result_schema: &str) -> ResultIdentity {
        ResultIdentity {
            operation: operation.to_owned(),
            result_schema: result_schema.to_owned(),
            geometer_release: self.release_version.clone(),
            geometer_abi: self.abi(),
            artifact_sha256: self.artifact_sha256.clone(),
        }
    }

    /// Whether two workers run the same release, ABI and executable bytes.
    /// The path is not part of the comparison.
    pub fn same_artifact(&self, other: &Self) -> bool {
        self.release_version == other.release_version
            && self.c_abi_generation == other.c_abi_generation
            && self.artifact_sha256 == other.artifact_sha256
    }
}

/// Everything a cached result depends on besides the caller's request digest:
/// the operation, its result schema, and the producing artifact.
#[derive(Clone, Debug, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct ResultIdentity {
    pub operation: String,
    pub result_schema: String,
    pub geometer_release: String,
    pub geometer_abi: String,
    pub artifact_sha256: String,
}

/// Open the executable for reading while denying writers (Windows share mode).
pub(crate) fn open_artifact(path: &Path) -> Result<File, String> {
    let mut options = OpenOptions::new();
    options.read(true);
    #[cfg(windows)]
    {
        use std::os::windows::fs::OpenOptionsExt;
        options.share_mode(1);
    }
    options.open(path).map_err(|error| error.to_string())
}

/// Lowercase hexadecimal SHA-256 of the whole file, read from its start.
pub(crate) fn file_digest(file: &mut File) -> Result<String, String> {
    file.rewind().map_err(|error| error.to_string())?;
    let mut digest = Sha256::new();
    let mut buffer = [0_u8; 64 * 1024];
    loop {
        let count = file.read(&mut buffer).map_err(|error| error.to_string())?;
        if count == 0 {
            break;
        }
        digest.update(&buffer[..count]);
    }
    Ok(digest
        .finalize()
        .iter()
        .map(|byte| format!("{byte:02x}"))
        .collect())
}
