//! Per-user, digest-addressed helper closure with crash-released OS locking.

use crate::archive;
use std::fs::{self, File, OpenOptions};
use std::io::{Read, Write};
use std::path::{Path, PathBuf};

pub fn materialize(
    root: &Path,
    bytes: &[u8],
    expected: &str,
    platform: archive::Platform,
) -> Result<PathBuf, String> {
    Bundle::admit(bytes, expected, platform)?.materialize(root)
}

/// Admitted immutable payload, reusable without decoding the archive for every worker.
/// Materialization still verifies/repairs the actual closure on every invocation.
pub struct Bundle {
    contents: archive::Contents,
    expected: String,
    platform: archive::Platform,
}

impl Bundle {
    pub fn admit(
        bytes: &[u8],
        expected: &str,
        platform: archive::Platform,
    ) -> Result<Self, String> {
        if archive::digest(bytes) != expected {
            return Err("Embedded Geometer archive digest mismatch".into());
        }
        // Admission precedes all filesystem changes; archive names never select paths.
        let contents = archive::decode(bytes, platform)?;
        Ok(Self {
            contents,
            expected: expected.into(),
            platform,
        })
    }

    pub fn materialize(&self, root: &Path) -> Result<PathBuf, String> {
        let expected = &self.expected;
        directory(root)?;
        let lock_path = root.join(format!("{expected}.lock"));
        regular_file(&lock_path)?;
        let lock = OpenOptions::new()
            .read(true)
            .write(true)
            .create(true)
            .truncate(false)
            .open(&lock_path)
            .map_err(|error| error.to_string())?;
        lock.lock().map_err(|error| error.to_string())?;
        let target = root.join(expected);
        directory(&target)?;
        directory(&target.join("licenses"))?;
        for (name, content) in &self.contents {
            let path = target.join(&name);
            if !matches(&path, &content)? {
                publish(&path, content, name == self.platform.executable)?;
            }
        }
        let executable = target.join(self.platform.executable);
        executable_mode(&executable)?;
        Ok(executable)
    }
}

fn link(metadata: &fs::Metadata) -> bool {
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

fn directory(path: &Path) -> Result<(), String> {
    if !path.is_absolute() {
        return Err("Geometer cache directory must be absolute".into());
    }
    check_ancestors(path)?;
    fs::create_dir_all(path).map_err(|error| error.to_string())?;
    check_ancestors(path)
}

fn check_ancestors(path: &Path) -> Result<(), String> {
    for ancestor in path.ancestors() {
        match fs::symlink_metadata(ancestor) {
            Ok(metadata) if link(&metadata) || !metadata.is_dir() => {
                return Err(format!(
                    "Geometer cache requires ordinary directories: {}",
                    ancestor.display()
                ));
            }
            Ok(_) => {}
            Err(error) if error.kind() == std::io::ErrorKind::NotFound => {}
            Err(error) => return Err(error.to_string()),
        }
    }
    Ok(())
}

fn regular_file(path: &Path) -> Result<bool, String> {
    match fs::symlink_metadata(path) {
        Ok(metadata) if link(&metadata) || !metadata.is_file() => Err(format!(
            "Geometer cache requires an ordinary file: {}",
            path.display()
        )),
        Ok(_) => Ok(true),
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => Ok(false),
        Err(error) => Err(error.to_string()),
    }
}

fn matches(path: &Path, wanted: &[u8]) -> Result<bool, String> {
    if !regular_file(path)? {
        return Ok(false);
    }
    let mut actual = Vec::new();
    File::open(path)
        .map_err(|error| error.to_string())?
        .take(wanted.len() as u64 + 1)
        .read_to_end(&mut actual)
        .map_err(|error| error.to_string())?;
    Ok(archive::digest(&actual) == archive::digest(wanted))
}

struct Staged(PathBuf);

impl Drop for Staged {
    fn drop(&mut self) {
        let _ = fs::remove_file(&self.0);
    }
}

fn publish(path: &Path, bytes: &[u8], executable: bool) -> Result<(), String> {
    let stage = path.with_extension("pcbe-stage");
    // The fixed stage name is owned by this exact entry and protected by the
    // OS lock. Recover only an ordinary incomplete file left by a killed writer.
    if regular_file(&stage)? {
        fs::remove_file(&stage).map_err(|error| error.to_string())?;
    }
    let staged = Staged(stage);
    let mut file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&staged.0)
        .map_err(|error| error.to_string())?;
    file.write_all(bytes)
        .and_then(|()| file.sync_all())
        .map_err(|error| error.to_string())?;
    drop(file);
    if executable {
        executable_mode(&staged.0)?;
    }
    if regular_file(path)? {
        fs::remove_file(path).map_err(|error| error.to_string())?;
    }
    fs::rename(&staged.0, path).map_err(|error| error.to_string())
}

fn executable_mode(path: &Path) -> Result<(), String> {
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        let mode = fs::metadata(path)
            .map_err(|error| error.to_string())?
            .permissions()
            .mode();
        if mode & 0o7777 != 0o755 {
            fs::set_permissions(path, fs::Permissions::from_mode(0o755))
                .map_err(|error| error.to_string())?;
        }
    }
    #[cfg(not(unix))]
    let _ = path;
    Ok(())
}
