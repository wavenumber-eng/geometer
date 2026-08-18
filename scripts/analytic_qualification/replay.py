"""Exact GMABRQ01 production-IPC replay and result projection."""

from __future__ import annotations

from typing import Any

from geometer._analytic_packet_a0 import (
    AnalyticPacketError,
    decode_analytic_planar_boolean_batch_result_a0_packet,
)
from geometer._generated.contracts.models import (
    FailedJobResult,
    OperationFailureA0,
    PackedAttachmentProjectionA0,
    PackedAttachmentReferenceA0,
)
from geometer._ipc_a0 import Attachment
from geometer._ipc_client import (
    ANALYTIC_OPERATION,
    ANALYTIC_PACKET_FORMAT,
    ANALYTIC_REQUEST_ATTACHMENT,
    ANALYTIC_REQUEST_CONTRACT,
    ANALYTIC_REQUEST_MEDIA_TYPE,
    ANALYTIC_RESULT_ATTACHMENT,
    ANALYTIC_RESULT_CONTRACT,
    GeometerIpcClient,
    GeometerOperationError,
)

from .corpus import QualificationError


def _diagnostic_json(value: Any) -> dict[str, Any]:
    return {
        "code": value.code.value,
        "severity": value.severity.value,
        "job_id": value.job_id,
        "stage_id": value.stage_id,
        "operand_id": value.operand_id,
        "geometry_id": value.geometry_id,
        "path": None if value.path_identity is None else value.path_identity.value,
    }


def execute_packet(client: GeometerIpcClient, packet: bytes, timeout: float) -> tuple[bytes, dict[str, Any]]:
    projection = PackedAttachmentProjectionA0(
        schema=ANALYTIC_REQUEST_CONTRACT,
        packet=PackedAttachmentReferenceA0(
            attachment=ANALYTIC_REQUEST_ATTACHMENT,
            format=ANALYTIC_PACKET_FORMAT,
        ),
    )
    response = client.execute(
        ANALYTIC_OPERATION,
        projection,
        (
            Attachment(
                name=ANALYTIC_REQUEST_ATTACHMENT,
                media_type=ANALYTIC_REQUEST_MEDIA_TYPE,
                data=packet,
            ),
        ),
        timeout=timeout,
    )
    if isinstance(response.outcome, OperationFailureA0):
        raise GeometerOperationError(response.outcome.operation, response.outcome.diagnostics)
    expected_projection = PackedAttachmentProjectionA0(
        schema=ANALYTIC_RESULT_CONTRACT,
        packet=PackedAttachmentReferenceA0(
            attachment=ANALYTIC_RESULT_ATTACHMENT,
            format=ANALYTIC_PACKET_FORMAT,
        ),
    )
    if response.outcome.result != expected_projection or len(response.attachments) != 1:
        raise QualificationError("production executable returned an incompatible analytic result projection")
    result_packet = response.attachments[0].data
    try:
        result = decode_analytic_planar_boolean_batch_result_a0_packet(result_packet)
    except AnalyticPacketError as error:
        raise QualificationError(f"production executable returned an invalid analytic result packet: {error}") from error
    diagnostics = [item for job in result.job_results for item in job.diagnostics]
    return result_packet, {
        "job_count": len(result.job_results),
        "failed_job_count": sum(isinstance(job, FailedJobResult) for job in result.job_results),
        "result_region_count": sum(len(getattr(job, "result_regions", ())) for job in result.job_results),
        "result_segment_count": sum(len(getattr(job, "directed_fragments", ())) for job in result.job_results),
        "normalization_failure_count": sum(item.code.value.startswith("normalization_") for item in diagnostics),
        "job_digests": [{"job_id": job.job_id, "sha256": job.digest_sha256} for job in result.job_results],
        "solver_diagnostics": [_diagnostic_json(item) for item in diagnostics],
    }
