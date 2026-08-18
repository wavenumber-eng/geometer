#!/usr/bin/env python3
"""Replay the TypeScript PCB demo packet through the current native IPC server."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from geometer._analytic_packet_a0 import decode_analytic_planar_boolean_batch_result_a0_packet
from geometer._generated.contracts.models import (
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
    GeometerIpcClient,
)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("packet", type=Path)
    parser.add_argument("--executable", type=Path, required=True)
    args = parser.parse_args()
    projection = PackedAttachmentProjectionA0(
        schema=ANALYTIC_REQUEST_CONTRACT,
        packet=PackedAttachmentReferenceA0(
            attachment=ANALYTIC_REQUEST_ATTACHMENT,
            format=ANALYTIC_PACKET_FORMAT,
        ),
    )
    with GeometerIpcClient(args.executable, client_name="typescript-pcb-demo-validation") as client:
        response = client.execute(
            ANALYTIC_OPERATION,
            projection,
            (
                Attachment(
                    name=ANALYTIC_REQUEST_ATTACHMENT,
                    media_type=ANALYTIC_REQUEST_MEDIA_TYPE,
                    data=args.packet.read_bytes(),
                ),
            ),
            timeout=30,
        )
    if isinstance(response.outcome, OperationFailureA0):
        raise SystemExit("operation failed before packet result")
    result = decode_analytic_planar_boolean_batch_result_a0_packet(response.attachments[0].data)
    summary = [
        {
            "diagnostics": [diagnostic.code for diagnostic in job.diagnostics],
            "digest": job.digest_sha256 if job.status == "success" else "",
            "job_id": job.job_id,
            "status": job.status,
        }
        for job in result.job_results
    ]
    print(json.dumps(summary, separators=(",", ":")))
    if not all(item["status"] == "success" for item in summary):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
