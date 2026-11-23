//! The real runtime over in-process fake workers: supervision without a host
//! report, retired workers, async hosts and the startup deadline.

use std::path::PathBuf;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

use super::fake::{FakeStarter, FakeWorker};
use super::{ArtifactIdentity, FatalPolicy, GeometerRuntime, RuntimeConfig, RuntimePin, Transport};
use crate::client::{GeometerClientError, OperationOutcome, OperationResponse};
use crate::contracts::{
    self, ModelBoundsResultA0, OperationOutcomeA0, OperationResultValueA0, OperationSuccessA0,
};
use crate::ipc::Attachment;
use crate::model_bounds::ModelBoundsRequest;

const BOUNDS: &str = r#"{"schema":"geometry.model_bounds.a0","units":"mm",
    "source":{"format":"step","hash":"computed:fnv1a64:model"},
    "bounds":{"min":[-1.0,-1.0,0.0],"max":[1.0,1.0,1.0],"size":[2.0,2.0,1.0],
    "center":[0.0,0.0,0.5]},"timings":{"model_read_ms":0,"bounds_ms":0}}"#;

/// Answers model bounds: success, a local operation error for a model
/// `local`, and a simulated process crash for a model `crash`.
#[derive(Default)]
struct Scripted {
    calls: AtomicUsize,
    terminated: AtomicUsize,
    closed: AtomicUsize,
}

impl FakeWorker for Scripted {
    fn execute(
        &self,
        operation: &str,
        _request_json: &[u8],
        attachments: Vec<Attachment>,
    ) -> Result<OperationResponse, GeometerClientError> {
        self.calls.fetch_add(1, Ordering::SeqCst);
        match attachments[0].data.as_slice() {
            b"crash" => Err(GeometerClientError::Process("fake worker exited".into())),
            b"local" => Err(GeometerClientError::Operation {
                operation: operation.into(),
                diagnostics: Vec::new(),
            }),
            _ => {
                let result: ModelBoundsResultA0 =
                    contracts::decode_json(BOUNDS.as_bytes()).expect("valid bounds result");
                Ok(OperationResponse {
                    outcome: OperationOutcome::A0(OperationOutcomeA0::Success(
                        OperationSuccessA0 {
                            operation: operation.into(),
                            ok: true,
                            result: OperationResultValueA0::ModelBounds(result),
                        },
                    )),
                    attachments: Vec::new(),
                })
            }
        }
    }

    fn terminate(&self) {
        self.terminated.fetch_add(1, Ordering::SeqCst);
    }

    fn close(&self) {
        self.closed.fetch_add(1, Ordering::SeqCst);
    }
}

/// Every started fake, in start order.
#[derive(Clone, Default)]
struct Started(Arc<Mutex<Vec<Arc<Scripted>>>>);

impl Started {
    fn starter(&self, release: &'static str) -> FakeStarter {
        let started = Arc::clone(&self.0);
        Arc::new(move || {
            let fake = Arc::new(Scripted::default());
            started.lock().unwrap().push(Arc::clone(&fake));
            let identity = ArtifactIdentity::new(
                PathBuf::from("fake-geometer"),
                release,
                20_260_919,
                format!("sha256:{}", "f".repeat(64)),
            );
            Ok((fake as Arc<dyn FakeWorker>, identity))
        })
    }

    fn count(&self) -> usize {
        self.0.lock().unwrap().len()
    }

    fn get(&self, index: usize) -> Arc<Scripted> {
        Arc::clone(&self.0.lock().unwrap()[index])
    }
}

fn runtime(started: &Started, policy: FatalPolicy) -> GeometerRuntime {
    let mut config = RuntimeConfig::new(Transport::Fake(started.starter("2026.9.19")));
    config.on_fatal = policy;
    GeometerRuntime::new(config).unwrap()
}

fn bounds(runtime: &GeometerRuntime, model: &[u8]) -> Result<f64, super::Failure> {
    let lease = runtime.lease(|| ()).map_err(super::Failure::fatal)?;
    let model = model.to_vec();
    lease
        .worker()
        .run("Geometer bounds", |client| async move {
            client.model_bounds(ModelBoundsRequest::step(model)).await
        })
        .map(|result| result.bounds.size[0])
}

#[test]
fn a_fatal_failure_latches_without_a_host_report_and_the_worker_is_closed() {
    let started = Started::default();
    let runtime = runtime(&started, FatalPolicy::Latch);
    assert_eq!(bounds(&runtime, b"model"), Ok(2.0));
    let local = bounds(&runtime, b"local").unwrap_err();
    assert!(!local.fatal, "{}", local.message);
    let fatal = bounds(&runtime, b"crash").unwrap_err();
    assert!(fatal.fatal);
    assert!(
        fatal.message.contains("fake worker exited"),
        "{}",
        fatal.message
    );
    let fake = started.get(0);
    assert_eq!(fake.terminated.load(Ordering::SeqCst), 1);
    assert_eq!(fake.closed.load(Ordering::SeqCst), 1);
    assert_eq!(runtime.fatal(), Some(fatal.message.as_str()));
    let refused = bounds(&runtime, b"model").unwrap_err();
    assert_eq!(refused.message, fatal.message);
    assert_eq!(started.count(), 1, "a latched runtime starts nothing");
    assert_eq!(fake.calls.load(Ordering::SeqCst), 3);
    assert_eq!(runtime.finish(), Err(fatal.message));
}

#[test]
fn respawn_replaces_a_crashed_worker_without_a_host_report() {
    let started = Started::default();
    let runtime = runtime(&started, FatalPolicy::Respawn);
    assert!(bounds(&runtime, b"crash").unwrap_err().fatal);
    assert_eq!(started.get(0).closed.load(Ordering::SeqCst), 1);
    assert!(runtime.fatal().is_none());
    assert_eq!(bounds(&runtime, b"model"), Ok(2.0));
    assert_eq!(started.count(), 2);
    assert_eq!(started.get(0).calls.load(Ordering::SeqCst), 1);
    runtime.finish().unwrap();
    assert_eq!(started.get(1).closed.load(Ordering::SeqCst), 1);
}

#[test]
fn a_host_retired_worker_refuses_calls_and_never_returns_to_the_pool() {
    for (policy, latched) in [(FatalPolicy::Latch, true), (FatalPolicy::Respawn, false)] {
        let started = Started::default();
        let runtime = runtime(&started, policy);
        let lease = runtime.lease(|| ()).unwrap();
        lease.worker().mark_broken("host rejected a result");
        lease.worker().mark_broken("a later reason is ignored");
        let refused = lease
            .worker()
            .run("Geometer bounds", |client| async move {
                client
                    .model_bounds(ModelBoundsRequest::step(b"m".to_vec()))
                    .await
            })
            .unwrap_err();
        assert!(refused.fatal);
        assert!(refused.message.ends_with("host rejected a result"));
        drop(lease);
        assert_eq!(started.get(0).calls.load(Ordering::SeqCst), 0);
        assert_eq!(started.get(0).closed.load(Ordering::SeqCst), 1);
        let next = runtime.lease(|| ());
        assert_eq!(next.is_err(), latched, "{policy:?}");
        if latched {
            assert_eq!(runtime.fatal(), Some("host rejected a result"));
        } else {
            assert!(next.unwrap().started());
            assert_eq!(started.count(), 2);
        }
    }
}

#[test]
fn a_pin_mismatch_refuses_and_closes_the_fake_worker() {
    let started = Started::default();
    let mut config = RuntimeConfig::new(Transport::Fake(started.starter("2026.1.1")));
    config.pin = Some(RuntimePin::new("2026.9.19", 20_260_919));
    let runtime = GeometerRuntime::new(config).unwrap();
    let error = runtime.lease(|| ()).map(|_| ()).unwrap_err();
    assert!(
        error.contains("is incompatible; expected 2026.9.19/20260919"),
        "{error}"
    );
    assert_eq!(started.get(0).closed.load(Ordering::SeqCst), 1);
}

#[test]
fn startup_deadline_is_validated() {
    let started = Started::default();
    for deadline in [
        Duration::ZERO,
        super::MAX_STARTUP_DEADLINE + Duration::from_secs(1),
    ] {
        let mut config = RuntimeConfig::new(Transport::Fake(started.starter("2026.9.19")));
        config.startup_deadline = deadline;
        assert!(GeometerRuntime::new(config).is_err());
    }
}

#[tokio::test(flavor = "current_thread")]
async fn calls_close_and_drop_inside_an_async_task_do_not_panic() {
    let started = Started::default();
    let runtime = runtime(&started, FatalPolicy::Respawn);
    assert_eq!(bounds(&runtime, b"model"), Ok(2.0));
    assert!(bounds(&runtime, b"crash").unwrap_err().fatal);
    assert_eq!(bounds(&runtime, b"model"), Ok(2.0));
    runtime.finish().unwrap();
    drop(runtime);
    assert_eq!(started.count(), 2);
    assert!((0..2).all(|index| started.get(index).closed.load(Ordering::SeqCst) == 1));
}

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn spawn_blocking_hosts_run_calls_and_drop_workers() {
    let started = Started::default();
    let runtime = Arc::new(runtime(&started, FatalPolicy::Latch));
    let shared = Arc::clone(&runtime);
    let value = tokio::task::spawn_blocking(move || bounds(&shared, b"model"))
        .await
        .unwrap();
    assert_eq!(value, Ok(2.0));
    let dropped = tokio::task::spawn_blocking(move || {
        runtime.finish().unwrap();
        drop(runtime);
    });
    dropped.await.unwrap();
    assert_eq!(started.get(0).closed.load(Ordering::SeqCst), 1);
}

#[tokio::test(flavor = "current_thread")]
async fn a_start_that_never_finishes_is_dropped_at_its_deadline() {
    let started = Instant::now();
    let error = super::worker::bounded_start::<()>(
        Duration::from_millis(50),
        std::future::pending::<Result<(), GeometerClientError>>(),
    )
    .await
    .unwrap_err();
    assert_eq!(error, "Geometer worker start exceeded its 50 ms deadline");
    assert!(started.elapsed() < Duration::from_secs(10));
}
