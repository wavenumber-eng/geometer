use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, OnceLock, mpsc};
use std::time::Duration;

use super::FatalPolicy;
use super::pool::{PoolWorker, WorkerPool};

struct Fake {
    identity: &'static str,
    broken: OnceLock<String>,
    closed: Arc<AtomicUsize>,
}

impl Fake {
    fn new(identity: &'static str, closed: &Arc<AtomicUsize>) -> Self {
        Self {
            identity,
            broken: OnceLock::new(),
            closed: Arc::clone(closed),
        }
    }
}

impl PoolWorker for Fake {
    type Identity = &'static str;

    fn identity(&self) -> Self::Identity {
        self.identity
    }

    fn broken_reason(&self) -> Option<String> {
        self.broken.get().cloned()
    }

    fn close(&mut self) -> Result<(), String> {
        self.closed.fetch_add(1, Ordering::SeqCst);
        Ok(())
    }
}

#[test]
fn closed_or_fatal_pool_never_starts_another_process() {
    let pool = WorkerPool::<Fake>::new(4, FatalPolicy::Latch);
    pool.latch("actual fatal cause".into());
    let starts = AtomicUsize::new(0);
    let attempt = |pool: &WorkerPool<Fake>| {
        pool.acquire(|| {
            starts.fetch_add(1, Ordering::SeqCst);
            Err("must not start".into())
        })
        .map(|_| ())
    };
    assert_eq!(attempt(&pool), Err("actual fatal cause".into()));
    pool.finish().unwrap();
    let closed = WorkerPool::<Fake>::new(4, FatalPolicy::Latch);
    closed.finish().unwrap();
    assert_eq!(
        attempt(&closed),
        Err("native model executor is closed".into())
    );
    assert_eq!(starts.load(Ordering::SeqCst), 0);
}

#[test]
fn leases_reuse_idle_workers_and_finish_drains_the_active_lease() {
    let closed = Arc::new(AtomicUsize::new(0));
    let pool = WorkerPool::new(2, FatalPolicy::Latch);
    let leases = [0, 1].map(|_| pool.acquire(|| Ok(Fake::new("a", &closed))).unwrap());
    assert!(leases.iter().all(|lease| lease.started));
    assert_eq!(pool.slot_states(), vec![(true, false), (true, false)]);
    drop(leases);
    let lease = pool
        .acquire(|| Err("must reuse an idle worker".into()))
        .unwrap();
    assert!(!lease.started);
    std::thread::scope(|scope| {
        let (done_tx, done_rx) = mpsc::channel();
        let pool_ref = &pool;
        scope.spawn(move || done_tx.send(pool_ref.finish()).unwrap());
        assert!(
            done_rx.recv_timeout(Duration::from_millis(200)).is_err(),
            "finish must wait for the active lease"
        );
        drop(lease);
        done_rx
            .recv_timeout(Duration::from_secs(5))
            .unwrap()
            .unwrap();
    });
    assert!(pool.is_closed());
    assert_eq!(pool.slot_states(), vec![(false, false), (false, false)]);
    assert_eq!(closed.load(Ordering::SeqCst), 2);
}

#[test]
fn workers_must_agree_on_identity_and_a_mismatch_is_closed() {
    let closed = Arc::new(AtomicUsize::new(0));
    let pool = WorkerPool::new(2, FatalPolicy::Latch);
    let _first = pool.acquire(|| Ok(Fake::new("a", &closed))).unwrap();
    let error = pool
        .acquire(|| Ok(Fake::new("b", &closed)))
        .map(|_| ())
        .unwrap_err();
    assert_eq!(
        error,
        "Geometer pool workers have different observed identities"
    );
    assert_eq!(closed.load(Ordering::SeqCst), 1);
    assert_eq!(pool.slot_states(), vec![(true, false)]);
}

#[test]
fn latch_retires_a_broken_worker_and_latches_its_reason_without_a_host_report() {
    let closed = Arc::new(AtomicUsize::new(0));
    let pool = WorkerPool::new(2, FatalPolicy::Latch);
    let lease = pool.acquire(|| Ok(Fake::new("a", &closed))).unwrap();
    lease
        .worker()
        .broken
        .set("result failed admission".into())
        .unwrap();
    drop(lease);
    assert_eq!(pool.slot_states(), vec![(false, false)]);
    assert_eq!(closed.load(Ordering::SeqCst), 1);
    assert_eq!(pool.fatal(), Some("result failed admission"));
    let starts = AtomicUsize::new(0);
    let next = pool.acquire(|| {
        starts.fetch_add(1, Ordering::SeqCst);
        Ok(Fake::new("a", &closed))
    });
    assert_eq!(next.map(|_| ()), Err("result failed admission".into()));
    assert_eq!(starts.load(Ordering::SeqCst), 0);
    pool.finish().unwrap();
}

#[test]
fn respawn_retires_a_broken_worker_and_starts_a_verified_replacement() {
    let closed = Arc::new(AtomicUsize::new(0));
    let pool = WorkerPool::new(2, FatalPolicy::Respawn);
    let first = pool.acquire(|| Ok(Fake::new("a", &closed))).unwrap();
    let healthy = pool.acquire(|| Ok(Fake::new("a", &closed))).unwrap();
    first.worker().broken.set("crashed".into()).unwrap();
    drop(first);
    assert_eq!(closed.load(Ordering::SeqCst), 1);
    assert_eq!(pool.slot_states(), vec![(false, false), (true, false)]);
    assert!(pool.fatal().is_none());
    drop(healthy);
    let reused = pool
        .acquire(|| Err("an idle healthy worker comes first".into()))
        .unwrap();
    assert!(!reused.started);
    let error = pool
        .acquire(|| Ok(Fake::new("b", &closed)))
        .map(|_| ())
        .unwrap_err();
    assert!(error.contains("different observed identities"));
    let replacement = pool.acquire(|| Ok(Fake::new("a", &closed))).unwrap();
    assert!(replacement.started);
    drop((reused, replacement));
    assert_eq!(pool.slot_states(), vec![(false, true), (false, true)]);
    pool.finish().unwrap();
}

#[test]
fn a_panicking_lease_latches_a_latched_pool_and_retires_the_worker_under_both_policies() {
    for (policy, latched) in [(FatalPolicy::Latch, true), (FatalPolicy::Respawn, false)] {
        let closed = Arc::new(AtomicUsize::new(0));
        let pool = WorkerPool::new(1, policy);
        let unwound = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            let _lease = pool.acquire(|| Ok(Fake::new("a", &closed))).unwrap();
            panic!("injected request panic");
        }));
        assert!(unwound.is_err());
        assert_eq!(closed.load(Ordering::SeqCst), 1, "{policy:?}");
        let next = pool.acquire(|| Ok(Fake::new("a", &closed)));
        assert_eq!(next.is_err(), latched, "{policy:?}");
        if latched {
            assert_eq!(pool.fatal(), Some("native worker request unwound"));
        } else {
            assert!(next.unwrap().started);
        }
    }
}
