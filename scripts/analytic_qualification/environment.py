"""Machine, toolchain, and process-RSS observations for qualification."""

from __future__ import annotations

import ctypes
import os
import platform
import re
import subprocess
import threading
from pathlib import Path
from typing import Any, Callable

from native_build_attestation import BuildAttestationError, load_and_validate_attestation

from .corpus import QualificationError, file_sha256, identity_sha256


ROOT = Path(__file__).resolve().parents[2]
REFERENCE_SYSTEM = "Windows-11-Pro-build-26200"
REFERENCE_PROCESSOR = "AMD-Ryzen-9-9950X"
REFERENCE_INSTALLED_RAM_BYTES = 66_125_668_352
RSS_SAMPLE_INTERVAL_SECONDS = 0.001


def _installed_memory_bytes() -> int | None:
    if os.name == "nt":
        class MemoryStatus(ctypes.Structure):
            _fields_ = [
                ("length", ctypes.c_ulong),
                ("memory_load", ctypes.c_ulong),
                ("total_physical", ctypes.c_ulonglong),
                ("available_physical", ctypes.c_ulonglong),
                ("total_page_file", ctypes.c_ulonglong),
                ("available_page_file", ctypes.c_ulonglong),
                ("total_virtual", ctypes.c_ulonglong),
                ("available_virtual", ctypes.c_ulonglong),
                ("available_extended_virtual", ctypes.c_ulonglong),
            ]

        status = MemoryStatus()
        status.length = ctypes.sizeof(status)
        if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(status)):
            return int(status.total_physical)
        return None
    if hasattr(os, "sysconf"):
        try:
            return int(os.sysconf("SC_PHYS_PAGES")) * int(os.sysconf("SC_PAGE_SIZE"))
        except (OSError, ValueError):
            return None
    return None


def _windows_cpu() -> dict[str, Any] | None:
    if os.name != "nt":
        return None
    command = (
        "Get-CimInstance Win32_Processor | Select-Object -First 1 Name,NumberOfCores,NumberOfLogicalProcessors "
        "| ConvertTo-Json -Compress"
    )
    try:
        import json

        value = json.loads(
            subprocess.check_output(
                ["powershell", "-NoProfile", "-NonInteractive", "-Command", command],
                text=True,
                stderr=subprocess.DEVNULL,
                timeout=10,
            )
        )
        return {
            "name": value["Name"].strip(),
            "physical_core_count": int(value["NumberOfCores"]),
            "logical_cpu_count": int(value["NumberOfLogicalProcessors"]),
        }
    except (OSError, KeyError, TypeError, ValueError, subprocess.SubprocessError):
        return None


def machine_profile() -> dict[str, Any]:
    cpu = _windows_cpu()
    profile = {
        "system": platform.system(),
        "release": platform.release(),
        "version": platform.version(),
        "machine": platform.machine(),
        "processor": cpu["name"] if cpu else platform.processor(),
        "physical_core_count": None if cpu is None else cpu["physical_core_count"],
        "logical_cpu_count": cpu["logical_cpu_count"] if cpu else os.cpu_count(),
        "installed_ram_bytes": _installed_memory_bytes(),
    }
    return {"profile": profile, "sha256": identity_sha256(profile)}


def _git_revision() -> str:
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True, stderr=subprocess.DEVNULL, timeout=5
        ).strip()
    except (OSError, subprocess.SubprocessError):
        return "unavailable"


def _compiler_identity() -> str:
    for cache in (ROOT / "build" / "CMakeCache.txt", *sorted(ROOT.glob("build-native-*/CMakeCache.txt"))):
        if not cache.is_file():
            continue
        try:
            lines = cache.read_text(encoding="utf-8", errors="replace").splitlines()
        except OSError:
            continue
        compiler = next((line.split("=", 1)[1] for line in lines if line.startswith("CMAKE_CXX_COMPILER:") and "=" in line), None)
        build_type = next((line.split("=", 1)[1] for line in lines if line.startswith("CMAKE_BUILD_TYPE:") and "=" in line), None)
        if compiler:
            return f"{compiler};build_type={build_type or 'unavailable'}"
    return "unavailable"


def toolchain_profile(executable: Path) -> dict[str, Any]:
    try:
        version = subprocess.check_output([str(executable), "--version"], text=True, timeout=10).strip()
    except (OSError, subprocess.SubprocessError) as error:
        raise QualificationError(f"could not query Geometer version: {error}") from error
    try:
        attestation = load_and_validate_attestation(executable)
    except BuildAttestationError as error:
        raise QualificationError(f"native build attestation validation failed: {error}") from error
    if attestation is not None:
        attested = bool(attestation["build_provenance_attested"])
        profile = {
            "geometer_executable_sha256": attestation["artifact"]["sha256"],
            "geometer_version": version,
            "build_attestation_schema": attestation["schema"],
            "build_attestation_sha256": attestation["sidecar_sha256"],
            "build_attestation_producer": attestation["producer"],
            "build": attestation["build"],
            "hint_authority": (
                "validated_executable_bound_clean_source_attestation"
                if attested
                else "validated_executable_bound_nonpromotable_source_attestation"
            ),
            "build_provenance_attested": attested,
            "python": platform.python_version(),
        }
        return {"profile": profile, "sha256": identity_sha256(profile)}
    try:
        from dependency_versions import OCCT_TAG
    except ImportError:
        OCCT_TAG = "unavailable"
    profile = {
        "geometer_executable_sha256": file_sha256(executable),
        "geometer_version": version,
        "workspace_revision_hint": _git_revision(),
        "native_compiler_hint": _compiler_identity(),
        "configured_occt_tag_hint": OCCT_TAG,
        "hint_authority": "current_workspace_not_embedded_executable_provenance",
        "build_provenance_attested": False,
        "python": platform.python_version(),
    }
    return {"profile": profile, "sha256": identity_sha256(profile)}


def reference_machine_match(machine: dict[str, Any]) -> bool:
    profile = machine["profile"]
    system = "-".join((str(profile["system"]), str(profile["release"]), str(profile["version"]))).lower()
    processor = re.sub(r"[^a-z0-9]", "", str(profile["processor"]).lower())
    return (
        "windows" in system
        and "26200" in system
        and "amdryzen99950x" in processor
        and profile["physical_core_count"] == 16
        and profile["logical_cpu_count"] == 32
        and profile["installed_ram_bytes"] == REFERENCE_INSTALLED_RAM_BYTES
    )


def _read_process_memory_windows(pid: int) -> tuple[int, int] | None:
    class ProcessMemoryCounters(ctypes.Structure):
        _fields_ = [
            ("cb", ctypes.c_ulong),
            ("page_fault_count", ctypes.c_ulong),
            ("peak_working_set_size", ctypes.c_size_t),
            ("working_set_size", ctypes.c_size_t),
            ("quota_peak_paged_pool_usage", ctypes.c_size_t),
            ("quota_paged_pool_usage", ctypes.c_size_t),
            ("quota_peak_non_paged_pool_usage", ctypes.c_size_t),
            ("quota_non_paged_pool_usage", ctypes.c_size_t),
            ("pagefile_usage", ctypes.c_size_t),
            ("peak_pagefile_usage", ctypes.c_size_t),
        ]

    windll = getattr(ctypes, "windll", None)
    if windll is None:
        return None
    kernel32 = windll.kernel32
    psapi = windll.psapi
    kernel32.OpenProcess.argtypes = (ctypes.c_ulong, ctypes.c_int, ctypes.c_ulong)
    kernel32.OpenProcess.restype = ctypes.c_void_p
    kernel32.CloseHandle.argtypes = (ctypes.c_void_p,)
    kernel32.CloseHandle.restype = ctypes.c_int
    psapi.GetProcessMemoryInfo.argtypes = (ctypes.c_void_p, ctypes.c_void_p, ctypes.c_ulong)
    psapi.GetProcessMemoryInfo.restype = ctypes.c_int
    handle = kernel32.OpenProcess(0x1000 | 0x0010, False, pid)
    if not handle:
        return None
    try:
        counters = ProcessMemoryCounters()
        counters.cb = ctypes.sizeof(counters)
        if not psapi.GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb):
            return None
        return int(counters.working_set_size), int(counters.peak_working_set_size)
    finally:
        kernel32.CloseHandle(handle)


def read_process_rss(pid: int) -> int | None:
    if os.name == "nt":
        value = _read_process_memory_windows(pid)
        return None if value is None else value[0]
    status = Path(f"/proc/{pid}/status")
    if status.is_file():
        try:
            match = re.search(r"^VmRSS:\s+(\d+)\s+kB$", status.read_text(encoding="ascii"), re.MULTILINE)
            return None if match is None else int(match.group(1)) * 1024
        except OSError:
            return None
    try:
        value = subprocess.check_output(["ps", "-o", "rss=", "-p", str(pid)], text=True, timeout=1).strip()
        return int(value) * 1024 if value else None
    except (OSError, ValueError, subprocess.SubprocessError):
        return None


def read_process_peak_rss(pid: int) -> int | None:
    """Read the OS-maintained process-lifetime RSS high-water mark."""

    if os.name == "nt":
        value = _read_process_memory_windows(pid)
        return None if value is None else value[1]
    status = Path(f"/proc/{pid}/status")
    if status.is_file():
        try:
            match = re.search(r"^VmHWM:\s+(\d+)\s+kB$", status.read_text(encoding="ascii"), re.MULTILINE)
            return None if match is None else int(match.group(1)) * 1024
        except OSError:
            return None
    return None


def process_rss_measurement_method() -> dict[str, Any]:
    if os.name == "nt":
        source = "GetProcessMemoryInfo.PeakWorkingSetSize"
    elif Path("/proc/self/status").is_file():
        source = "proc_status.VmHWM"
    else:
        source = "unavailable"
    return {
        "metric": "os_process_lifetime_peak_resident_set_bytes",
        "source": source,
        "scope": "process_lifetime; later cases conservatively include earlier process peaks",
        "sample_interval_seconds": RSS_SAMPLE_INTERVAL_SECONDS if source != "unavailable" else None,
        "transient_peak_can_be_missed": False if source != "unavailable" else None,
        "includes_solver_internal_allocations_only": False,
    }


class PeakRssSampler:
    def __init__(
        self,
        pid: int,
        peak_reader: Callable[[int], int | None] = read_process_peak_rss,
        rss_reader: Callable[[int], int | None] = read_process_rss,
    ) -> None:
        self._pid = pid
        self._peak_reader = peak_reader
        self._rss_reader = rss_reader
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._peak: int | None = None
        self._thread = threading.Thread(target=self._sample_loop, name="geometer-qualification-rss", daemon=True)

    def start(self) -> None:
        self.sample_once()
        self._thread.start()

    def reset(self) -> int | None:
        baseline = self._rss_reader(self._pid)
        value = self._peak_reader(self._pid)
        with self._lock:
            self._peak = value
        return baseline

    def peak(self) -> int | None:
        self.sample_once()
        with self._lock:
            return self._peak

    def close(self) -> None:
        self._stop.set()
        self._thread.join(timeout=1)

    def sample_once(self) -> None:
        """Take one synchronous RSS sample, primarily for boundary coverage."""

        value = self._peak_reader(self._pid)
        if value is None:
            return
        with self._lock:
            self._peak = value if self._peak is None else max(self._peak, value)

    def _sample_loop(self) -> None:
        while not self._stop.wait(RSS_SAMPLE_INTERVAL_SECONDS):
            self.sample_once()
