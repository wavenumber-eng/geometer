from __future__ import annotations

from collections.abc import Mapping, Sequence
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path
import os
import tempfile
import time
from typing import Any

from ._cli import run_batch as cli_run_batch
from ._cli import version as cli_version
from ._types import HlrOptions, Version


class _UseRootSentinel:
    pass


_USE_ROOT_SENTINEL = _UseRootSentinel()


@dataclass(frozen=True)
class GeometerBatchConfig:
    max_workers: int = 8
    chunk_size: int = 5
    work_dir: Path | None = None
    keep_work_dir: bool = False


@dataclass(frozen=True)
class GeometerBatchResult:
    ok: bool
    jobs: list[dict[str, Any]]
    batches: list[dict[str, Any]]
    wall_seconds: float
    work_dir: Path | None
    version: str | None = None
    abi: int | None = None


class GeometerBatchRunner:
    def __init__(
        self,
        *,
        max_workers: int = 8,
        chunk_size: int = 5,
        work_dir: str | Path | None = None,
        keep_work_dir: bool = False,
    ) -> None:
        if max_workers < 1:
            raise ValueError("max_workers must be at least 1")
        if chunk_size < 1:
            raise ValueError("chunk_size must be at least 1")
        self.config = GeometerBatchConfig(
            max_workers=int(max_workers),
            chunk_size=int(chunk_size),
            work_dir=Path(work_dir) if work_dir is not None else None,
            keep_work_dir=bool(keep_work_dir),
        )

    def run(
        self,
        jobs: Sequence[Mapping[str, Any]],
        *,
        options: HlrOptions | Mapping[str, Any] | None = None,
    ) -> GeometerBatchResult:
        _ensure_exe_backend()
        normalized_jobs = [dict(job) for job in jobs]
        start = time.perf_counter()
        if not normalized_jobs:
            version_info = self.version()
            return GeometerBatchResult(
                ok=True,
                jobs=[],
                batches=[],
                wall_seconds=0.0,
                work_dir=self.config.work_dir,
                version=version_info.string,
                abi=version_info.abi,
            )

        if self.config.work_dir is not None:
            root = self.config.work_dir
            root.mkdir(parents=True, exist_ok=True)
            return self._run_in_root(root, normalized_jobs, options, start)

        if self.config.keep_work_dir:
            root = Path(tempfile.mkdtemp(prefix="geometer-batches-"))
            return self._run_in_root(root, normalized_jobs, options, start)

        with tempfile.TemporaryDirectory(prefix="geometer-batches-") as directory_text:
            root = Path(directory_text)
            return self._run_in_root(root, normalized_jobs, options, start, returned_work_dir=None)

    def version(self) -> Version:
        _ensure_exe_backend()
        return cli_version()

    def _run_in_root(
        self,
        root: Path,
        jobs: list[dict[str, Any]],
        options: HlrOptions | Mapping[str, Any] | None,
        start: float,
        *,
        returned_work_dir: Path | None | _UseRootSentinel = _USE_ROOT_SENTINEL,
    ) -> GeometerBatchResult:
        chunks = _chunks(jobs, self.config.chunk_size)
        indexed_chunks = list(enumerate(chunks))
        if self.config.max_workers == 1 or len(indexed_chunks) == 1:
            batches = [
                self._run_chunk(index, chunk, root, options) for index, chunk in indexed_chunks
            ]
        else:
            batches = []
            with ThreadPoolExecutor(max_workers=self.config.max_workers) as pool:
                futures = [
                    pool.submit(self._run_chunk, index, chunk, root, options)
                    for index, chunk in indexed_chunks
                ]
                for future in as_completed(futures):
                    batches.append(future.result())

        batches.sort(key=lambda batch: int(batch["index"]))
        response_jobs: list[dict[str, Any]] = []
        for batch in batches:
            response_jobs.extend(dict(job) for job in batch["jobs"])
        first_response = next(
            (
                batch["response"]
                for batch in batches
                if isinstance(batch.get("response"), Mapping)
            ),
            {},
        )
        if isinstance(returned_work_dir, _UseRootSentinel):
            work_dir: Path | None = root
        else:
            work_dir = returned_work_dir
        return GeometerBatchResult(
            ok=all(bool(batch["ok"]) for batch in batches),
            jobs=response_jobs,
            batches=batches,
            wall_seconds=time.perf_counter() - start,
            work_dir=work_dir,
            version=(
                first_response.get("version")
                if isinstance(first_response.get("version"), str)
                else None
            ),
            abi=first_response.get("abi") if isinstance(first_response.get("abi"), int) else None,
        )

    def _run_chunk(
        self,
        index: int,
        jobs: list[dict[str, Any]],
        root: Path,
        options: HlrOptions | Mapping[str, Any] | None,
    ) -> dict[str, Any]:
        batch_dir = root / f"batch_{index:04d}"
        started = time.perf_counter()
        response = cli_run_batch(jobs, options=options, work_dir=batch_dir)
        elapsed = time.perf_counter() - started
        response_jobs = response.get("jobs")
        return {
            "index": index,
            "ok": bool(response.get("ok")),
            "job_count": len(jobs),
            "elapsed_seconds": elapsed,
            "work_dir": str(batch_dir),
            "jobs": list(response_jobs) if isinstance(response_jobs, list) else [],
            "response": response,
        }


def _chunks(jobs: list[dict[str, Any]], chunk_size: int) -> list[list[dict[str, Any]]]:
    return [jobs[index : index + chunk_size] for index in range(0, len(jobs), chunk_size)]


def _ensure_exe_backend() -> None:
    configured = os.environ.get("GEOMETER_BACKEND")
    if configured and configured.strip().lower() not in {"exe", "cli"}:
        raise ValueError("Geometer Python only supports the executable backend for now")
    for legacy_name in ("GEOMETER_PYTHON_DIRECT", "GEOMETER_PYTHON_WORKER"):
        if os.environ.get(legacy_name, "").lower() in {"1", "true", "yes", "on"}:
            raise ValueError("Geometer Python only supports the executable backend for now")
