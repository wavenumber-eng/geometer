//! Bounded lazy workers. A lease owns one worker exclusively during a call.
//!
//! A worker that is broken (or whose lease unwinds) never returns to service:
//! the lease's end retires and closes it under both policies. Under
//! [`FatalPolicy::Latch`] the pool also latches with the worker's reason, so a
//! host that never reports the failure still cannot use the pool again.

use std::sync::{Condvar, Mutex, OnceLock};

use super::FatalPolicy;

const UNWOUND: &str = "native worker request unwound";

/// What the pool needs from a worker; implemented by [`super::Worker`] and by
/// test doubles.
pub(crate) trait PoolWorker: Send {
    type Identity: PartialEq + Clone + Send;

    fn identity(&self) -> Self::Identity;
    /// Why the worker must not serve again, if it must not.
    fn broken_reason(&self) -> Option<String>;
    fn close(&mut self) -> Result<(), String>;
}

struct Slot<W> {
    worker: Option<W>,
    busy: bool,
}

struct State<W: PoolWorker> {
    slots: Vec<Slot<W>>,
    identity: Option<W::Identity>,
    closed: bool,
}

impl<W: PoolWorker> Default for State<W> {
    fn default() -> Self {
        Self {
            slots: Vec::new(),
            identity: None,
            closed: false,
        }
    }
}

pub(crate) struct WorkerPool<W: PoolWorker> {
    limit: usize,
    policy: FatalPolicy,
    state: Mutex<State<W>>,
    available: Condvar,
    fatal: OnceLock<String>,
}

pub(crate) struct Lease<'a, W: PoolWorker> {
    pool: &'a WorkerPool<W>,
    slot: usize,
    worker: Option<W>,
    pub(crate) started: bool,
}

impl<W: PoolWorker> WorkerPool<W> {
    pub(crate) fn new(limit: usize, policy: FatalPolicy) -> Self {
        Self {
            limit,
            policy,
            state: Mutex::new(State::default()),
            available: Condvar::new(),
            fatal: OnceLock::new(),
        }
    }

    /// Reuse an idle worker, start one into a free or emptied slot, or wait.
    /// Workers start while the pool lock is held, so starts are serialized;
    /// the transport bounds each start with its startup deadline.
    pub(crate) fn acquire(
        &self,
        start: impl FnOnce() -> Result<W, String>,
    ) -> Result<Lease<'_, W>, String> {
        let mut state = self
            .state
            .lock()
            .map_err(|_| "native worker pool lock poisoned")?;
        loop {
            self.check_open(&state)?;
            let idle = state
                .slots
                .iter()
                .position(|slot| !slot.busy && slot.worker.is_some());
            if let Some(index) = idle {
                state.slots[index].busy = true;
                let worker = state.slots[index].worker.take();
                return Ok(self.lease(index, worker, false));
            }
            let emptied = state.slots.iter().position(|slot| !slot.busy);
            if emptied.is_some() || state.slots.len() < self.limit {
                let mut worker = start()?;
                verify_identity(&mut state, &mut worker)?;
                let slot = emptied.unwrap_or(state.slots.len());
                if slot == state.slots.len() {
                    state.slots.push(Slot {
                        worker: None,
                        busy: true,
                    });
                } else {
                    state.slots[slot].busy = true;
                }
                return Ok(self.lease(slot, Some(worker), true));
            }
            state = self
                .available
                .wait(state)
                .map_err(|_| "native worker pool lock poisoned")?;
        }
    }

    fn lease(&self, slot: usize, worker: Option<W>, started: bool) -> Lease<'_, W> {
        Lease {
            pool: self,
            slot,
            worker,
            started,
        }
    }

    fn check_open(&self, state: &State<W>) -> Result<(), String> {
        if let Some(error) = self.fatal.get() {
            return Err(error.clone());
        }
        if state.closed {
            return Err("native model executor is closed".into());
        }
        Ok(())
    }

    /// Keep the first fatal message; every later lease fails with it.
    pub(crate) fn latch(&self, message: String) {
        let _ = self.fatal.set(message);
        self.wake();
    }

    pub(crate) fn fatal(&self) -> Option<&str> {
        self.fatal.get().map(String::as_str)
    }

    pub(crate) fn wake(&self) {
        self.available.notify_all();
    }

    /// Close the pool, wait for active leases, and close every idle worker.
    /// Reports cleanup failures only; the latched failure is separate.
    pub(crate) fn finish(&self) -> Result<(), String> {
        let mut errors = Vec::new();
        let mut state = match self.state.lock() {
            Ok(state) => state,
            Err(error) => {
                errors.push("native worker pool lock poisoned".to_owned());
                error.into_inner()
            }
        };
        state.closed = true;
        self.available.notify_all();
        while state.slots.iter().any(|slot| slot.busy) {
            state = self
                .available
                .wait(state)
                .unwrap_or_else(std::sync::PoisonError::into_inner);
        }
        for (index, slot) in state.slots.iter_mut().enumerate() {
            if let Some(mut worker) = slot.worker.take()
                && let Err(error) = worker.close()
            {
                errors.push(format!("worker {index}: {error}"));
            }
        }
        if errors.is_empty() {
            Ok(())
        } else {
            Err(errors.join("; "))
        }
    }

    #[cfg(test)]
    pub(crate) fn slot_states(&self) -> Vec<(bool, bool)> {
        self.state
            .lock()
            .unwrap()
            .slots
            .iter()
            .map(|slot| (slot.busy, slot.worker.is_some()))
            .collect()
    }

    #[cfg(test)]
    pub(crate) fn is_closed(&self) -> bool {
        self.state.lock().unwrap().closed
    }
}

fn verify_identity<W: PoolWorker>(state: &mut State<W>, worker: &mut W) -> Result<(), String> {
    let identity = worker.identity();
    if state
        .identity
        .as_ref()
        .is_some_and(|expected| expected != &identity)
    {
        let mut error = "Geometer pool workers have different observed identities".to_owned();
        if let Err(cleanup) = worker.close() {
            error.push_str(&format!("; cleanup failed: {cleanup}"));
        }
        return Err(error);
    }
    state.identity = Some(identity);
    Ok(())
}

impl<W: PoolWorker> Lease<'_, W> {
    pub(crate) fn worker(&self) -> &W {
        self.worker.as_ref().expect("active worker lease")
    }
}

impl<W: PoolWorker> Drop for Lease<'_, W> {
    fn drop(&mut self) {
        let worker = self.worker.take();
        let reason = if std::thread::panicking() {
            Some(UNWOUND.to_owned())
        } else {
            worker.as_ref().and_then(PoolWorker::broken_reason)
        };
        let mut state = self
            .pool
            .state
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner);
        let retired = match reason {
            Some(reason) => {
                if self.pool.policy == FatalPolicy::Latch {
                    let _ = self.pool.fatal.set(reason);
                }
                worker
            }
            None => {
                state.slots[self.slot].worker = worker;
                None
            }
        };
        state.slots[self.slot].busy = false;
        self.pool.available.notify_all();
        drop(state);
        if let Some(mut worker) = retired {
            let _ = worker.close();
        }
    }
}
