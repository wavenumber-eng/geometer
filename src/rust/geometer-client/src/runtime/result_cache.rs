//! Bounded in-memory reuse of successful opaque operation results.
//!
//! The caller supplies a request digest that already covers every request
//! input, and the [`ResultIdentity`] of the producing artifact. The cache
//! neither executes nor validates; callers insert only after their own
//! completion of the result succeeded, and re-complete every hit.

use std::collections::VecDeque;
use std::sync::Arc;

use super::ResultIdentity;

const DEFAULT_MAX_ENTRIES: usize = 128;
const DEFAULT_MAX_BYTES: usize = 32 * 1024 * 1024;

/// Operational counters only.
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct ResultCacheStats {
    pub hits: u64,
    pub misses: u64,
    pub evictions: u64,
    /// Insertions bypassed because the limits prohibit retention.
    pub oversized_bypasses: u64,
    pub entries: usize,
    /// UTF-8 bytes of retained request digests, identity fields and results.
    pub retained_bytes: usize,
}

#[derive(Debug)]
struct Entry {
    request: Box<str>,
    identity: ResultIdentity,
    response: Arc<str>,
    retained_bytes: usize,
}

/// A deterministic least-recently-used cache with no process-global state.
///
/// The byte budget counts retained string contents; container headers are
/// bounded by the entry limit. Lookup is linear in the bounded entry count.
#[derive(Debug)]
pub struct ResultCache {
    entries: VecDeque<Entry>,
    max_entries: usize,
    max_bytes: usize,
    stats: ResultCacheStats,
}

impl Default for ResultCache {
    fn default() -> Self {
        Self::new(DEFAULT_MAX_ENTRIES, DEFAULT_MAX_BYTES)
    }
}

impl ResultCache {
    /// An empty cache. A zero limit disables retention.
    pub fn new(max_entries: usize, max_bytes: usize) -> Self {
        Self {
            entries: VecDeque::new(),
            max_entries,
            max_bytes,
            stats: ResultCacheStats::default(),
        }
    }

    /// Reuse only an exact request digest and identity; a hit becomes most recent.
    pub fn get(&mut self, request: &str, identity: &ResultIdentity) -> Option<Arc<str>> {
        let index = self
            .entries
            .iter()
            .position(|entry| entry.request.as_ref() == request && &entry.identity == identity);
        if let Some(entry) = index.and_then(|index| self.entries.remove(index)) {
            let response = Arc::clone(&entry.response);
            self.entries.push_back(entry);
            self.stats.hits = self.stats.hits.saturating_add(1);
            Some(response)
        } else {
            self.stats.misses = self.stats.misses.saturating_add(1);
            None
        }
    }

    /// Retain a successful response. Returns false, without evicting anything,
    /// when it cannot fit. Replacing the same key is not a capacity eviction.
    pub fn insert(&mut self, request: &str, identity: &ResultIdentity, response: &str) -> bool {
        let Some(retained_bytes) = entry_bytes(request, identity, response) else {
            self.stats.oversized_bypasses = self.stats.oversized_bypasses.saturating_add(1);
            return false;
        };
        if self.max_entries == 0 || self.max_bytes == 0 || retained_bytes > self.max_bytes {
            self.stats.oversized_bypasses = self.stats.oversized_bypasses.saturating_add(1);
            return false;
        }
        let existing = self
            .entries
            .iter()
            .position(|entry| entry.request.as_ref() == request && &entry.identity == identity);
        if let Some(entry) = existing.and_then(|index| self.entries.remove(index)) {
            self.stats.retained_bytes -= entry.retained_bytes;
        }
        while self.entries.len() >= self.max_entries
            || self.stats.retained_bytes > self.max_bytes - retained_bytes
        {
            if let Some(entry) = self.entries.pop_front() {
                self.stats.retained_bytes -= entry.retained_bytes;
                self.stats.evictions = self.stats.evictions.saturating_add(1);
            }
        }
        self.entries.push_back(Entry {
            request: request.into(),
            identity: identity.clone(),
            response: Arc::from(response),
            retained_bytes,
        });
        self.stats.retained_bytes += retained_bytes;
        true
    }

    /// Snapshot the counters.
    pub fn stats(&self) -> ResultCacheStats {
        ResultCacheStats {
            entries: self.entries.len(),
            ..self.stats
        }
    }
}

fn entry_bytes(request: &str, identity: &ResultIdentity, response: &str) -> Option<usize> {
    [
        request,
        identity.operation.as_str(),
        identity.geometer_release.as_str(),
        identity.geometer_abi.as_str(),
        identity.result_schema.as_str(),
        identity.artifact_sha256.as_str(),
        response,
    ]
    .into_iter()
    .try_fold(0_usize, |bytes, value| bytes.checked_add(value.len()))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn identity(artifact: &str) -> ResultIdentity {
        ResultIdentity {
            operation: "geometry.model_hlr_projection.a0".into(),
            result_schema: "geometry.hlr_projection.result.a0".into(),
            geometer_release: "2026.9.19".into(),
            geometer_abi: "geometer.browser_c_abi.v20260919".into(),
            artifact_sha256: artifact.into(),
        }
    }

    #[test]
    fn exact_key_reuses_the_retained_response_and_identity_separates_entries() {
        let mut cache = ResultCache::default();
        let a = identity("sha256:a");
        let b = identity("sha256:b");
        assert!(cache.insert("request", &a, "{\"a\":1}"));
        let first = cache.get("request", &a).unwrap();
        let second = cache.get("request", &a).unwrap();
        assert!(Arc::ptr_eq(&first, &second));
        assert!(cache.get("request", &b).is_none());
        assert!(cache.get("other", &a).is_none());
        let stats = cache.stats();
        assert_eq!((stats.hits, stats.misses, stats.entries), (2, 2, 1));
    }

    #[test]
    fn least_recent_entries_are_evicted_and_oversized_responses_bypass() {
        let identity = identity("sha256:a");
        let one = entry_bytes("1", &identity, "x").unwrap();
        let mut cache = ResultCache::new(2, one * 2);
        assert!(cache.insert("1", &identity, "x"));
        assert!(cache.insert("2", &identity, "y"));
        assert!(cache.get("1", &identity).is_some());
        assert!(cache.insert("3", &identity, "z"));
        assert!(cache.get("2", &identity).is_none());
        assert!(cache.get("1", &identity).is_some());
        assert!(!cache.insert("4", &identity, &"w".repeat(one * 2)));
        let stats = cache.stats();
        assert_eq!((stats.evictions, stats.oversized_bypasses), (1, 1));
        assert_eq!(stats.retained_bytes, one * 2);
        assert!(!ResultCache::new(0, 1024).insert("1", &identity, "x"));
    }

    #[test]
    fn replacing_a_key_keeps_one_entry_without_counting_an_eviction() {
        let identity = identity("sha256:a");
        let mut cache = ResultCache::default();
        assert!(cache.insert("1", &identity, "old"));
        assert!(cache.insert("1", &identity, "new"));
        assert_eq!(cache.get("1", &identity).as_deref(), Some("new"));
        let stats = cache.stats();
        assert_eq!((stats.entries, stats.evictions), (1, 0));
    }
}
