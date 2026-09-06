use std::io;
use std::sync::Arc;
use std::sync::atomic::Ordering;

use tokio::time::Instant;

use crate::client::{GeometerClientError, Inner};
use crate::process::GeometerProcessExit;

const PROCESS_POLL_INTERVAL: std::time::Duration = std::time::Duration::from_millis(10);

struct TaskJoinGuard<'a> {
    inner: &'a Inner,
    tasks: Option<crate::client::ClientTasks>,
    complete: bool,
}

impl TaskJoinGuard<'_> {
    fn finish(mut self) {
        self.inner.tasks_complete.store(true, Ordering::SeqCst);
        self.complete = true;
    }
}

impl Drop for TaskJoinGuard<'_> {
    fn drop(&mut self) {
        if self.complete {
            return;
        }
        if let Some(tasks) = self.tasks.as_ref() {
            tasks.reader.abort();
            tasks.stderr.abort();
        }
        // A cancelled cleanup future must not leave task ownership detached or
        // make every later cleanup caller wait forever for an absent owner.
        self.inner.tasks_complete.store(true, Ordering::SeqCst);
    }
}

pub(crate) fn terminate_process(inner: &Inner) -> io::Result<()> {
    inner
        .process
        .lock()
        .unwrap_or_else(|poison| poison.into_inner())
        .terminate()
}

pub(crate) async fn terminate_and_reap_until(
    inner: &Arc<Inner>,
    deadline: Instant,
) -> io::Result<()> {
    let mut first_error = match try_wait_process(inner) {
        Ok(Some(_)) => return Ok(()),
        Ok(None) => None,
        Err(error) => Some(error),
    };
    if let Err(error) = terminate_process(inner) {
        first_error.get_or_insert(error);
    }
    loop {
        match try_wait_process(inner) {
            Ok(Some(_)) => return Ok(()),
            Ok(None) => {}
            Err(error) => {
                first_error.get_or_insert(error);
            }
        }
        let now = Instant::now();
        if now >= deadline {
            return Err(first_error.unwrap_or_else(|| {
                io::Error::new(
                    io::ErrorKind::TimedOut,
                    "Geometer process cleanup exceeded its deadline",
                )
            }));
        }
        tokio::time::sleep((deadline - now).min(PROCESS_POLL_INTERVAL)).await;
    }
}

pub(crate) fn try_wait_process(inner: &Inner) -> io::Result<Option<GeometerProcessExit>> {
    let status = inner
        .process
        .lock()
        .unwrap_or_else(|poison| poison.into_inner())
        .try_wait()?;
    if status.is_some() {
        inner.process_reaped.store(true, Ordering::SeqCst);
    }
    Ok(status)
}

pub(crate) async fn wait_for_process_exit(
    inner: &Inner,
    deadline: Instant,
) -> io::Result<Option<GeometerProcessExit>> {
    loop {
        if let Some(status) = try_wait_process(inner)? {
            return Ok(Some(status));
        }
        let now = Instant::now();
        if now >= deadline {
            return Ok(None);
        }
        tokio::time::sleep((deadline - now).min(PROCESS_POLL_INTERVAL)).await;
    }
}

pub(crate) async fn join_tasks_until(inner: &Inner, deadline: Instant) -> io::Result<()> {
    if inner.tasks_complete.load(Ordering::SeqCst) {
        return Ok(());
    }
    let tasks = inner
        .tasks
        .lock()
        .unwrap_or_else(|poison| poison.into_inner())
        .take();
    let Some(tasks) = tasks else {
        while !inner.tasks_complete.load(Ordering::SeqCst) {
            let now = Instant::now();
            if now >= deadline {
                return Err(task_timeout());
            }
            tokio::time::sleep((deadline - now).min(PROCESS_POLL_INTERVAL)).await;
        }
        return Ok(());
    };
    let mut guard = TaskJoinGuard {
        inner,
        tasks: Some(tasks),
        complete: false,
    };
    let tasks = guard.tasks.as_mut().expect("task guard owns task handles");
    let joined = tokio::time::timeout_at(deadline, async {
        tokio::join!(&mut tasks.reader, &mut tasks.stderr)
    })
    .await;
    let result = match joined {
        Ok((Ok(()), Ok(()))) => Ok(()),
        Ok((reader, stderr)) => Err(io::Error::other(format!(
            "Geometer stream task failed (reader: {reader:?}, stderr: {stderr:?})"
        ))),
        Err(_) => {
            tasks.reader.abort();
            tasks.stderr.abort();
            if !tasks.reader.is_finished() {
                let _ = (&mut tasks.reader).await;
            }
            if !tasks.stderr.is_finished() {
                let _ = (&mut tasks.stderr).await;
            }
            Err(task_timeout())
        }
    };
    guard.finish();
    result
}

pub(crate) fn mark_cleanup_complete(inner: &Inner) {
    if inner.process_reaped.load(Ordering::SeqCst) && inner.tasks_complete.load(Ordering::SeqCst) {
        inner.cleanup_complete.store(true, Ordering::SeqCst);
    }
}

pub(crate) fn start_cleanup_supervisor(inner: &Arc<Inner>, deadline: Instant) {
    if inner
        .cleanup_supervisor_started
        .swap(true, Ordering::SeqCst)
    {
        return;
    }
    let inner = Arc::clone(inner);
    tokio::spawn(async move {
        let _ = join_tasks_until(&inner, deadline).await;
        mark_cleanup_complete(&inner);
    });
}

pub(crate) fn cleanup_error(error: io::Error) -> GeometerClientError {
    GeometerClientError::Process(error.to_string())
}

pub(crate) fn combine_cleanup_results(
    process: Result<(), GeometerClientError>,
    tasks: io::Result<()>,
) -> Result<(), GeometerClientError> {
    match (process, tasks) {
        (Ok(()), Ok(())) => Ok(()),
        (Err(error), Ok(())) => Err(error),
        (Ok(()), Err(error)) => Err(cleanup_error(error)),
        (Err(primary), Err(cleanup)) => Err(GeometerClientError::Process(format!(
            "{primary}; additional cleanup failure: {cleanup}"
        ))),
    }
}

fn task_timeout() -> io::Error {
    io::Error::new(
        io::ErrorKind::TimedOut,
        "Geometer stream task cleanup exceeded its deadline",
    )
}
