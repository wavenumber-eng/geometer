import type {
  AnalyticPlanarBooleanBatchRequestA0,
  AuthoredVertex,
  PlanarRegionOperand,
} from "@wavenumber/geometer";

const NM_PER_MM = 1_000_000;

export function makeAnalyticPolygonPourRequest(
  slotCenterMm: number,
): AnalyticPlanarBooleanBatchRequestA0 {
  const slotCenter = BigInt(Math.round(slotCenterMm * NM_PER_MM));
  return {
    jobs: [
      {
        job_id: 7n,
        stages: [
          {
            stage_id: 701n,
            operation: "union",
            operands: [
              rectangle(7001n, 7401n, 7501n, 0n, 0n, 20_000_000n, 10_000_000n),
              {
                operand_id: 7002n,
                kind: "disk",
                feature_id: 7602n,
                center: { x: 5_000_000n, y: 5_000_000n },
                radius_nm: 1_000_000n,
              },
              {
                operand_id: 7003n,
                kind: "disk",
                feature_id: 7603n,
                center: { x: 5_000_000n, y: 5_000_000n },
                radius_nm: 1_000_000n,
              },
            ],
          },
          {
            stage_id: 702n,
            operation: "difference",
            operands: [
              rectangle(
                7005n,
                7405n,
                7505n,
                slotCenter - 1_000_000n,
                -1_000_000n,
                slotCenter + 1_000_000n,
                11_000_000n,
              ),
              clearanceDisk(7006n, 7606n, 3_000_000n, 2_500_000n, 650_000n),
              clearanceDisk(7007n, 7607n, 3_000_000n, 7_500_000n, 650_000n),
              clearanceDisk(7008n, 7608n, 17_000_000n, 2_500_000n, 650_000n),
              clearanceDisk(7009n, 7609n, 17_000_000n, 7_500_000n, 650_000n),
            ],
          },
        ],
      },
      {
        job_id: 8n,
        stages: [
          {
            stage_id: 801n,
            operation: "union",
            operands: [
              {
                operand_id: 8001n,
                kind: "swept_path",
                feature_id: 8601n,
                centerline: {
                  path_id: 8701n,
                  vertices: [
                    { vertex_id: 8801n, point: { x: 1_000_000n, y: 1_000_000n } },
                    { vertex_id: 8802n, point: { x: 2_000_000n, y: 1_000_000n } },
                  ],
                  segments: [{ segment_id: 8901n, curve_id: 8951n, kind: "line" }],
                },
                width_nm: 100_000n,
                cap: "round",
                join: "round",
              },
            ],
          },
        ],
      },
    ],
    relationship_queries: [],
  };
}

function rectangle(
  operandId: bigint,
  regionId: bigint,
  ringId: bigint,
  minX: bigint,
  minY: bigint,
  maxX: bigint,
  maxY: bigint,
): PlanarRegionOperand {
  const base = operandId * 10n;
  const vertices: readonly AuthoredVertex[] = [
    { vertex_id: base + 1n, point: { x: minX, y: minY } },
    { vertex_id: base + 2n, point: { x: maxX, y: minY } },
    { vertex_id: base + 3n, point: { x: maxX, y: maxY } },
    { vertex_id: base + 4n, point: { x: minX, y: maxY } },
  ];
  return {
    operand_id: operandId,
    kind: "planar_region",
    region_id: regionId,
    outer: {
      ring_id: ringId,
      vertices,
      segments: [1n, 2n, 3n, 4n].map((offset) => ({
        segment_id: base + 10n + offset,
        curve_id: base + 20n + offset,
        kind: "line" as const,
      })),
    },
    holes: [],
  };
}

function clearanceDisk(operandId: bigint, featureId: bigint, x: bigint, y: bigint, radius: bigint) {
  return {
    operand_id: operandId,
    kind: "disk" as const,
    feature_id: featureId,
    center: { x, y },
    radius_nm: radius,
  };
}
