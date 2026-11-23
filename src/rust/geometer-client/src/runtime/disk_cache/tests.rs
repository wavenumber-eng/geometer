use super::*;
use std::fs;
use std::sync::{Arc, Barrier};
use std::time::{SystemTime, UNIX_EPOCH};

fn layout() -> DiskCacheLayout {
    DiskCacheLayout::new("example.completed-result.v1", "example").unwrap()
}

struct Scratch(PathBuf);

impl Scratch {
    fn new() -> Self {
        let stamp = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos();
        let path = std::env::temp_dir().join(format!(
            "geometer-result-cache-test-{}-{stamp}",
            std::process::id()
        ));
        fs::create_dir(&path).unwrap();
        Self(path)
    }

    fn cache(&self) -> DiskResultCache {
        DiskResultCache::new(
            Some(self.0.join("cache")),
            layout(),
            DiskCacheLimits::default(),
        )
    }
}

impl Drop for Scratch {
    fn drop(&mut self) {
        let root = self.0.canonicalize().unwrap();
        let temporary = std::env::temp_dir().canonicalize().unwrap();
        assert!(root.starts_with(&temporary) && root != temporary);
        assert!(io::directory(&root, false).is_some());
        fs::remove_dir_all(root).unwrap();
    }
}

fn compatibility() -> ResultIdentity {
    ResultIdentity {
        operation: "geometry.model_illustration_geometry.b0".into(),
        result_schema: "geometry.model_illustration_geometry.result.b0".into(),
        geometer_release: "2026.9.19".into(),
        geometer_abi: "geometer.browser_c_abi.v20260919".into(),
        artifact_sha256: format!("sha256:{}", "a".repeat(64)),
    }
}

#[test]
fn completed_bytes_survive_a_separate_cache_host_without_rewriting() {
    let scratch = Scratch::new();
    let identity = compatibility();
    let first = scratch.cache();
    let response = "{\"geometry\":[],\"empty\":true}";
    assert!(first.get("request-a", &identity).is_none());
    assert!(first.insert("request-a", &identity, response));
    let path = first.entry_path("request-a", &identity).unwrap();
    let bytes = fs::read(&path).unwrap();
    assert_eq!(
        scratch.cache().get("request-a", &identity).as_deref(),
        Some(response)
    );
    assert!(first.insert("request-a", &identity, "a different completed result"));
    assert_eq!(fs::read(&path).unwrap(), bytes);
}

#[test]
fn request_and_every_executor_identity_field_separate_entries() {
    let scratch = Scratch::new();
    let cache = scratch.cache();
    let original = compatibility();
    assert!(cache.insert("request-a", &original, "completed"));
    assert!(cache.get("request-b", &original).is_none());
    let mut changes = Vec::new();
    for field in 0..5 {
        let mut changed = original.clone();
        match field {
            0 => changed.operation.push_str("-other"),
            1 => changed.result_schema.push_str("-other"),
            2 => changed.geometer_release.push_str("-other"),
            3 => changed.geometer_abi.push_str("-other"),
            _ => changed.artifact_sha256 = format!("sha256:{}", "b".repeat(64)),
        }
        changes.push(changed);
    }
    for changed in changes {
        assert!(cache.get("request-a", &changed).is_none());
        assert_ne!(
            cache.entry_path("request-a", &original),
            cache.entry_path("request-a", &changed)
        );
    }
}

#[test]
fn corrupt_truncated_and_partial_staging_records_never_hit() {
    let scratch = Scratch::new();
    let cache = scratch.cache();
    let identity = compatibility();
    assert!(cache.insert("request-a", &identity, "completed"));
    let path = cache.entry_path("request-a", &identity).unwrap();
    let good = fs::read(&path).unwrap();
    let mut bad = good.clone();
    *bad.last_mut().unwrap() ^= 1;
    fs::write(&path, bad).unwrap();
    assert!(cache.get("request-a", &identity).is_none());
    fs::write(&path, &good[..good.len() - 1]).unwrap();
    assert!(cache.get("request-a", &identity).is_none());
    assert!(cache.insert("request-a", &identity, "healed"));
    assert_eq!(cache.get("request-a", &identity).as_deref(), Some("healed"));
    assert!(cache.invalidate("request-a", &identity));
    fs::write(path.with_extension("tmp"), &good).unwrap();
    assert!(cache.get("request-a", &identity).is_none());
    assert!(cache.insert("request-a", &identity, "completed"));
    assert_eq!(
        cache.get("request-a", &identity).as_deref(),
        Some("completed")
    );
}

#[test]
fn disabled_oversized_and_unwritable_paths_bypass_without_mutation() {
    let scratch = Scratch::new();
    let identity = compatibility();
    let disabled_path = scratch.0.join("disabled");
    let disabled = DiskResultCache::new(None, layout(), DiskCacheLimits::default());
    assert!(!disabled.insert("request", &identity, "completed"));
    assert!(disabled.get("request", &identity).is_none());
    assert!(!disabled_path.exists());
    let mut small = scratch.cache();
    small.limits.entry_bytes = 1024;
    assert!(!small.insert("request", &identity, &"x".repeat(2048)));
    assert!(!small.directory.as_ref().unwrap().exists());
    let blocked = scratch.0.join("ordinary-file");
    fs::write(&blocked, b"untouched").unwrap();
    let cache = DiskResultCache::new(
        Some(blocked.join("cache")),
        layout(),
        DiskCacheLimits::default(),
    );
    assert!(!cache.insert("request", &identity, "completed"));
    assert!(cache.get("request", &identity).is_none());
    assert_eq!(fs::read(blocked).unwrap(), b"untouched");
}

#[test]
fn bounded_eviction_preserves_unknown_records_and_unrelated_paths() {
    let scratch = Scratch::new();
    let mut cache = scratch.cache();
    cache.limits = DiskCacheLimits {
        bytes: 2048,
        entries: 2,
        entry_bytes: 1024,
    };
    let identity = compatibility();
    for request in ["one", "two", "three", "four"] {
        assert!(cache.insert(request, &identity, "completed"));
    }
    let root = cache.directory.as_ref().unwrap();
    let own = io::entries(root, &layout()).unwrap();
    assert!(own.len() <= 2);
    assert!(own.iter().map(|entry| entry.bytes).sum::<u64>() <= 2048);
    let foreign = root.join(format!("{}.cache", "f".repeat(64)));
    fs::write(&foreign, b"not a cache envelope").unwrap();
    let unrelated = root.join("source.txt");
    fs::write(&unrelated, b"preserve").unwrap();
    let _ = cache.insert("five", &identity, "completed");
    assert_eq!(fs::read(&foreign).unwrap(), b"not a cache envelope");
    assert_eq!(fs::read(&unrelated).unwrap(), b"preserve");
}

#[test]
fn concurrent_publication_and_eviction_keep_only_complete_bounded_records() {
    let scratch = Scratch::new();
    let mut cache = scratch.cache();
    cache.limits = DiskCacheLimits {
        bytes: 2048,
        entries: 2,
        entry_bytes: 1024,
    };
    let cache = Arc::new(cache);
    let barrier = Arc::new(Barrier::new(8));
    let workers = (0..8)
        .map(|index| {
            let cache = Arc::clone(&cache);
            let barrier = Arc::clone(&barrier);
            std::thread::spawn(move || {
                barrier.wait();
                cache.insert(&format!("request-{index}"), &compatibility(), "completed")
            })
        })
        .collect::<Vec<_>>();
    let successes = workers
        .into_iter()
        .map(|worker| worker.join().unwrap())
        .filter(|success| *success)
        .count();
    assert!(successes > 0);
    let entries = io::entries(cache.directory.as_ref().unwrap(), &layout()).unwrap();
    assert!(entries.len() <= 2);
    assert!(entries.iter().map(|entry| entry.bytes).sum::<u64>() <= 2048);
    for entry in entries {
        assert!(read_record(&entry.path, &layout(), cache.limits).is_some());
    }
    assert!(
        !fs::read_dir(cache.directory.as_ref().unwrap())
            .unwrap()
            .any(|row| row
                .unwrap()
                .path()
                .extension()
                .is_some_and(|ext| ext == "tmp"))
    );
}

#[test]
fn semantic_rejection_invalidation_is_exact_and_unknown_files_survive() {
    let scratch = Scratch::new();
    let cache = scratch.cache();
    let identity = compatibility();
    assert!(cache.insert(
        "request-a",
        &identity,
        "complete but semantically rejected by caller"
    ));
    assert!(cache.insert("request-b", &identity, "unaffected complete result"));
    assert!(cache.invalidate("request-a", &identity));
    assert!(cache.get("request-a", &identity).is_none());
    assert_eq!(
        cache.get("request-b", &identity).as_deref(),
        Some("unaffected complete result")
    );
    assert!(cache.insert("request-a", &identity, "fresh completed result"));
    assert_eq!(
        cache.get("request-a", &identity).as_deref(),
        Some("fresh completed result")
    );
    let path = cache.entry_path("foreign", &identity).unwrap();
    fs::write(&path, b"ordinary file, not our envelope").unwrap();
    assert!(!cache.invalidate("foreign", &identity));
    assert!(!cache.insert("foreign", &identity, "completed"));
    assert_eq!(fs::read(path).unwrap(), b"ordinary file, not our envelope");
}

fn small_cache(scratch: &Scratch) -> DiskResultCache {
    let mut cache = scratch.cache();
    cache.limits = DiskCacheLimits {
        bytes: 2048,
        entries: 2,
        entry_bytes: 1024,
    };
    cache
}

#[test]
fn one_job_tracks_many_publications_without_repeated_directory_scans() {
    let scratch = Scratch::new();
    let cache = small_cache(&scratch);
    for index in 0..20 {
        assert!(cache.insert(&format!("request-{index}"), &compatibility(), "completed"));
        let rows = io::entries(cache.directory.as_ref().unwrap(), &layout()).unwrap();
        assert!(rows.len() <= 2);
        assert!(rows.iter().map(|entry| entry.bytes).sum::<u64>() <= 2048);
    }
    assert_eq!(cache.inventory.lock().unwrap().scans, 1);
    assert_eq!(
        cache.get("request-19", &compatibility()).as_deref(),
        Some("completed")
    );
}

#[test]
fn alternating_jobs_rescan_generation_and_preserve_combined_quota() {
    let scratch = Scratch::new();
    let first = small_cache(&scratch);
    let second = small_cache(&scratch);
    for index in 0..10 {
        let cache = if index % 2 == 0 { &first } else { &second };
        assert!(cache.insert(&format!("request-{index}"), &compatibility(), "completed"));
        let rows = io::entries(cache.directory.as_ref().unwrap(), &layout()).unwrap();
        assert!(rows.len() <= 2);
        assert!(rows.iter().map(|entry| entry.bytes).sum::<u64>() <= 2048);
    }
    assert_eq!(first.inventory.lock().unwrap().scans, 5);
    assert_eq!(second.inventory.lock().unwrap().scans, 5);
    assert!(second.invalidate("request-9", &compatibility()));
    assert!(first.insert("after-invalidation", &compatibility(), "completed"));
    assert_eq!(first.inventory.lock().unwrap().scans, 6);
}

#[test]
fn dirty_crash_recovery_accounts_before_and_after_file_mutation() {
    for boundary in ["before-mutation", "after-eviction", "after-publication"] {
        let scratch = Scratch::new();
        let cache = small_cache(&scratch);
        let identity = compatibility();
        assert!(cache.insert("one", &identity, "completed-one"));
        let root = cache.directory.as_ref().unwrap();
        inventory::dirty_for_test(root, &layout());
        if boundary == "after-eviction" {
            fs::remove_file(cache.entry_path("one", &identity).unwrap()).unwrap();
        }
        if boundary == "after-publication" {
            let other_scratch = Scratch::new();
            let other = other_scratch.cache();
            assert!(other.insert("two", &identity, "completed-two"));
            fs::copy(
                other.entry_path("two", &identity).unwrap(),
                cache.entry_path("two", &identity).unwrap(),
            )
            .unwrap();
            assert_eq!(
                cache.get("two", &identity).as_deref(),
                Some("completed-two")
            );
        }
        assert!(cache.insert("next", &identity, "completed-next"));
        let rows = io::entries(root, &layout()).unwrap();
        assert!(rows.len() <= 2);
        assert!(rows.iter().map(|entry| entry.bytes).sum::<u64>() <= 2048);
        assert_eq!(cache.inventory.lock().unwrap().scans, 2);
        assert!(!inventory::marker_for_test(root, &layout()).1);
    }
}

#[test]
fn foreign_generation_is_preserved_and_overflow_never_wraps() {
    let scratch = Scratch::new();
    let cache = small_cache(&scratch);
    let identity = compatibility();
    assert!(cache.insert("one", &identity, "completed"));
    let root = cache.directory.as_ref().unwrap();
    fs::write(root.join(layout().marker_name()), b"unknown user file").unwrap();
    assert!(cache.insert("two", &identity, "completed"));
    assert_eq!(
        fs::read(root.join(layout().marker_name())).unwrap(),
        b"unknown user file"
    );
    // Explicitly authored bookkeeping-only failure witness: no result content
    // or geometry is changed, and overflow permanently selects safe rescans.
    fs::remove_file(root.join(layout().marker_name())).unwrap();
    inventory::generation_for_test(root, &layout(), u64::MAX, false);
    assert!(cache.insert("three", &identity, "completed"));
    assert!(cache.insert("four", &identity, "completed"));
    assert_eq!(
        inventory::marker_for_test(root, &layout()),
        (u64::MAX, true)
    );
    assert_eq!(io::entries(root, &layout()).unwrap().len(), 2);
}

#[cfg(unix)]
#[test]
fn linked_directory_and_record_never_read_or_delete_external_content() {
    let scratch = Scratch::new();
    let external = scratch.0.join("external");
    fs::create_dir(&external).unwrap();
    let linked = scratch.0.join("linked");
    std::os::unix::fs::symlink(&external, &linked).unwrap();
    let cache = DiskResultCache::new(Some(linked.clone()), layout(), DiskCacheLimits::default());
    assert!(!cache.insert("request", &compatibility(), "completed"));
    assert!(cache.get("request", &compatibility()).is_none());
    assert_eq!(fs::read_dir(&external).unwrap().count(), 0);
    fs::remove_file(linked).unwrap();
}

#[test]
fn the_layout_derives_names_magic_and_header_order_from_its_two_strings() {
    let layout = layout();
    assert_eq!(layout.lock_name(), ".example-write-lock");
    assert_eq!(layout.marker_name(), ".example-directory-generation");
    assert_eq!(layout.marker_staging_prefix(), ".example-generation");
    assert_eq!(&layout.marker_magic(), b"EXAMCDIR01");
    assert_eq!(
        &DiskCacheLayout::new("x", "ab").unwrap().marker_magic(),
        b"AB__CDIR01"
    );
    assert!(DiskCacheLayout::new("x", "Upper").is_err());
    assert!(DiskCacheLayout::new("", "ok").is_err());
    let scratch = Scratch::new();
    let cache = scratch.cache();
    assert!(cache.insert("request-a", &compatibility(), "completed"));
    let bytes = fs::read(cache.entry_path("request-a", &compatibility()).unwrap()).unwrap();
    let split = bytes.iter().position(|byte| *byte == b'\n').unwrap();
    let header = std::str::from_utf8(&bytes[..split]).unwrap();
    assert!(header.starts_with(concat!(
        r#"{"format":"example.completed-result.v1","request_id":"request-a","#,
        r#""compatibility":{"operation":"geometry.model_illustration_geometry.b0","#,
        r#""result_schema":"#
    )));
    let root = cache.directory().unwrap().canonicalize().unwrap();
    assert!(root.join(".example-write-lock").is_file());
    assert_eq!(inventory::marker_for_test(&root, &layout), (1, false));
}
