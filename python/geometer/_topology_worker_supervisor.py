"""Internal hard-containment runner for experimental native topology workers.

This module deliberately does not define a topology wire protocol. It provides
the process boundary that a later generated client can reuse once that protocol
has been approved.
"""

from __future__ import annotations

import ctypes
import math
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Sequence, cast


_MINIMUM_MEMORY_LIMIT = 32 * 1024 * 1024


class TopologyWorkerError(RuntimeError):
    """Base error for an experimental contained topology worker."""


class TopologyWorkerDeadlineExceeded(TopologyWorkerError):
    """The worker was forcibly replaced after its hard deadline."""

    def __init__(self, generation: int, stdout: bytes, stderr: bytes, temporary_directory: Path) -> None:
        super().__init__(f"topology worker generation {generation} exceeded its hard deadline and was killed")
        self.generation = generation
        self.stdout = stdout
        self.stderr = stderr
        self.temporary_directory = temporary_directory


class TopologyWorkerCancelled(TopologyWorkerError):
    """The active worker was forcibly cancelled and must not be reused."""

    def __init__(self, outcome: TopologyWorkerOutcome) -> None:
        super().__init__(f"topology worker generation {outcome.generation} was forcibly cancelled")
        self.outcome = outcome


class TopologyWorkerProcessError(TopologyWorkerError):
    """The contained worker exited unsuccessfully."""

    def __init__(self, outcome: TopologyWorkerOutcome) -> None:
        super().__init__(f"topology worker generation {outcome.generation} exited with code {outcome.return_code}")
        self.outcome = outcome


@dataclass(frozen=True, slots=True)
class TopologyWorkerOutcome:
    generation: int
    process_id: int
    return_code: int
    stdout: bytes
    stderr: bytes
    temporary_directory: Path


class _WindowsIoCounters(ctypes.Structure):
    _fields_ = [
        ("ReadOperationCount", ctypes.c_ulonglong),
        ("WriteOperationCount", ctypes.c_ulonglong),
        ("OtherOperationCount", ctypes.c_ulonglong),
        ("ReadTransferCount", ctypes.c_ulonglong),
        ("WriteTransferCount", ctypes.c_ulonglong),
        ("OtherTransferCount", ctypes.c_ulonglong),
    ]


class _WindowsBasicLimitInformation(ctypes.Structure):
    _fields_ = [
        ("PerProcessUserTimeLimit", ctypes.c_longlong),
        ("PerJobUserTimeLimit", ctypes.c_longlong),
        ("LimitFlags", ctypes.c_uint32),
        ("MinimumWorkingSetSize", ctypes.c_size_t),
        ("MaximumWorkingSetSize", ctypes.c_size_t),
        ("ActiveProcessLimit", ctypes.c_uint32),
        ("Affinity", ctypes.c_size_t),
        ("PriorityClass", ctypes.c_uint32),
        ("SchedulingClass", ctypes.c_uint32),
    ]


class _WindowsExtendedLimitInformation(ctypes.Structure):
    _fields_ = [
        ("BasicLimitInformation", _WindowsBasicLimitInformation),
        ("IoInfo", _WindowsIoCounters),
        ("ProcessMemoryLimit", ctypes.c_size_t),
        ("JobMemoryLimit", ctypes.c_size_t),
        ("PeakProcessMemoryUsed", ctypes.c_size_t),
        ("PeakJobMemoryUsed", ctypes.c_size_t),
    ]


class _WindowsJob:
    _JOB_OBJECT_EXTENDED_LIMIT_INFORMATION = 9
    _JOB_OBJECT_LIMIT_PROCESS_MEMORY = 0x00000100
    _JOB_OBJECT_LIMIT_JOB_MEMORY = 0x00000200
    _JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x00002000

    def __init__(self, process: subprocess.Popen[bytes], memory_limit_bytes: int) -> None:
        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel32.CreateJobObjectW.argtypes = (ctypes.c_void_p, ctypes.c_wchar_p)
        kernel32.CreateJobObjectW.restype = ctypes.c_void_p
        kernel32.SetInformationJobObject.argtypes = (
            ctypes.c_void_p,
            ctypes.c_int,
            ctypes.c_void_p,
            ctypes.c_uint32,
        )
        kernel32.SetInformationJobObject.restype = ctypes.c_int
        kernel32.AssignProcessToJobObject.argtypes = (ctypes.c_void_p, ctypes.c_void_p)
        kernel32.AssignProcessToJobObject.restype = ctypes.c_int
        kernel32.TerminateJobObject.argtypes = (ctypes.c_void_p, ctypes.c_uint32)
        kernel32.TerminateJobObject.restype = ctypes.c_int
        kernel32.CloseHandle.argtypes = (ctypes.c_void_p,)
        kernel32.CloseHandle.restype = ctypes.c_int
        handle = kernel32.CreateJobObjectW(None, None)
        if not handle:
            raise ctypes.WinError(ctypes.get_last_error())
        self._kernel32 = kernel32
        self._handle = handle
        try:
            limits = _WindowsExtendedLimitInformation()
            limits.BasicLimitInformation.LimitFlags = (
                self._JOB_OBJECT_LIMIT_PROCESS_MEMORY
                | self._JOB_OBJECT_LIMIT_JOB_MEMORY
                | self._JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
            )
            limits.ProcessMemoryLimit = memory_limit_bytes
            limits.JobMemoryLimit = memory_limit_bytes
            if not kernel32.SetInformationJobObject(
                handle,
                self._JOB_OBJECT_EXTENDED_LIMIT_INFORMATION,
                ctypes.byref(limits),
                ctypes.sizeof(limits),
            ):
                raise ctypes.WinError(ctypes.get_last_error())
            process_handle = cast(Any, process)._handle
            if not kernel32.AssignProcessToJobObject(handle, ctypes.c_void_p(process_handle)):
                raise ctypes.WinError(ctypes.get_last_error())
        except BaseException:
            self.close()
            raise

    def terminate(self) -> bool:
        if self._handle:
            return bool(self._kernel32.TerminateJobObject(self._handle, 137))
        return True

    def close(self) -> None:
        if self._handle:
            self._kernel32.CloseHandle(self._handle)
            self._handle = None


class TopologyWorkerSupervisor:
    """Runs one native worker generation at a time behind hard OS containment."""

    def __init__(self, executable: str | Path, *, memory_limit_bytes: int) -> None:
        if memory_limit_bytes < _MINIMUM_MEMORY_LIMIT:
            raise ValueError(f"memory_limit_bytes must be at least {_MINIMUM_MEMORY_LIMIT}")
        command = Path(executable).resolve()
        if not command.is_file():
            raise ValueError(f"topology worker executable does not exist: {command}")
        self._executable = command
        self._memory_limit_bytes = memory_limit_bytes
        self._lock = threading.RLock()
        self._process: subprocess.Popen[bytes] | None = None
        self._job: _WindowsJob | None = None
        self._generation = 0
        self._cancelled_generation: int | None = None

    @property
    def active_process_id(self) -> int | None:
        with self._lock:
            return None if self._process is None else self._process.pid

    def run(
        self,
        arguments: Sequence[str | Path],
        *,
        timeout: float,
        stdin: bytes = b"",
    ) -> TopologyWorkerOutcome:
        if not math.isfinite(timeout) or not (timeout > 0.0):
            raise ValueError("timeout must be positive and finite")
        with self._lock:
            if self._process is not None:
                raise TopologyWorkerError("a topology worker generation is already active")
            self._generation += 1
            generation = self._generation
            temporary_directory = Path(tempfile.mkdtemp(prefix="geometer-topology-worker-"))
            temporary_directory.chmod(0o700)
            environment = os.environ.copy()
            for name in ("TMP", "TEMP", "TMPDIR"):
                environment[name] = str(temporary_directory)
            environment["GEOMETER_TOPOLOGY_WORKER_TEMP"] = str(temporary_directory)
            start_gate = temporary_directory / "supervisor.ready"
            environment["GEOMETER_TOPOLOGY_WORKER_START_GATE"] = str(start_gate)
            popen_options: dict[str, Any] = {}
            command = [str(self._executable), *(str(value) for value in arguments)]
            if os.name == "posix":
                popen_options["start_new_session"] = True
                command = [
                    sys.executable,
                    "-m",
                    "geometer._topology_worker_posix_launcher",
                    str(self._memory_limit_bytes),
                    *command,
                ]
            try:
                process = cast(
                    "subprocess.Popen[bytes]",
                    subprocess.Popen(
                        command,
                        stdin=subprocess.PIPE,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        env=environment,
                        text=False,
                        **popen_options,
                    ),
                )
            except BaseException:
                shutil.rmtree(temporary_directory)
                raise
            self._process = process
            try:
                if os.name == "nt":
                    self._job = _WindowsJob(process, self._memory_limit_bytes)
                start_gate.touch(exist_ok=False)
            except BaseException:
                self._terminate_and_collect(process)
                if self._job is not None:
                    self._job.close()
                    self._job = None
                self._process = None
                shutil.rmtree(temporary_directory)
                raise

        timed_out = False
        try:
            try:
                stdout, stderr = process.communicate(stdin, timeout=timeout)
            except subprocess.TimeoutExpired:
                timed_out = True
                stdout, stderr = self._terminate_and_collect(process)
            with self._lock:
                cancelled = self._cancelled_generation == generation
            outcome = TopologyWorkerOutcome(
                generation=generation,
                process_id=process.pid,
                return_code=process.returncode,
                stdout=stdout,
                stderr=stderr,
                temporary_directory=temporary_directory,
            )
        except BaseException:
            self._terminate_and_collect(process)
            raise
        finally:
            with self._lock:
                if self._job is not None:
                    self._job.close()
                    self._job = None
                self._process = None
                if self._cancelled_generation == generation:
                    self._cancelled_generation = None
            shutil.rmtree(temporary_directory)

        if timed_out:
            raise TopologyWorkerDeadlineExceeded(generation, stdout, stderr, temporary_directory)
        if cancelled:
            raise TopologyWorkerCancelled(outcome)
        if outcome.return_code != 0:
            raise TopologyWorkerProcessError(outcome)
        return outcome

    def cancel(self) -> bool:
        with self._lock:
            process = self._process
            if process is None or process.poll() is not None:
                return False
            self._cancelled_generation = self._generation
            self._terminate_generation(process)
            return True

    def _terminate_generation(self, process: subprocess.Popen[bytes]) -> None:
        with self._lock:
            if process.poll() is not None:
                return
            if os.name == "nt" and self._job is not None:
                if not self._job.terminate():
                    process.kill()
            elif os.name == "posix":
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
            else:
                process.kill()

    def _terminate_and_collect(self, process: subprocess.Popen[bytes]) -> tuple[bytes, bytes]:
        self._terminate_generation(process)
        try:
            return process.communicate(timeout=2)
        except subprocess.TimeoutExpired as error:
            process.kill()
            try:
                return process.communicate(timeout=2)
            except subprocess.TimeoutExpired as second_error:
                for stream in (process.stdin, process.stdout, process.stderr):
                    if stream is not None:
                        stream.close()
                try:
                    process.wait(timeout=2)
                except subprocess.TimeoutExpired as wait_error:
                    raise TopologyWorkerError(
                        "topology worker could not be reaped after forced termination"
                    ) from wait_error
                stdout = b"" if error.output is None else error.output
                stderr = b"" if second_error.stderr is None else second_error.stderr
                return stdout, stderr
