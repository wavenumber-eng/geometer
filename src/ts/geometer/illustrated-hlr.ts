import type {
  HlrProjectionOptionsA0,
  HlrProjectionResultA0,
  IllustrationMatrix4x4,
  MeshIllustrationInputA0,
  MeshIllustrationResultA0,
} from "./generated/contracts.js";
import type { IndexedTriangleMeshA0 } from "./indexed-mesh-packet-a0.js";
import {
  createIllustrator,
  type MeshIllustrationLinework,
  type MeshIllustratorA0,
} from "./mesh-illustration.js";

export interface MeshHlrProjector {
  meshHlrProjection(request: {
    readonly mesh: IndexedTriangleMeshA0 | Uint8Array;
    readonly options?: HlrProjectionOptionsA0;
  }): Promise<HlrProjectionResultA0>;
}

export interface FastHlrIllustrationRequest {
  /** Illustration positions and transforms are interpreted as millimeters. */
  readonly illustration: MeshIllustrationInputA0;
  /** Fast controls and output layers; the facade owns views, algorithms, and transforms. */
  readonly hlr?: Omit<
    HlrProjectionOptionsA0,
    | "views"
    | "projection_algorithm"
    | "outline_algorithm"
    | "model_transform"
    | "strip_root_placement"
  >;
}

export interface PreparedFastHlrIllustration {
  readonly hlr: HlrProjectionResultA0;
  readonly illustrator: MeshIllustratorA0;
}

export interface FastHlrIllustrationResult {
  readonly hlr: HlrProjectionResultA0;
  readonly illustration: MeshIllustrationResultA0;
}

const ILLUSTRATION_HLR_OPTION_KEYS = [
  "output_outline",
  "output_detail",
  "output_bbox",
  "curve_mode",
  "samples_per_curve",
  "round_digits",
  "edge_v_sharp",
  "edge_v_outline",
  "edge_v_smooth",
  "edge_v_sewn",
  "edge_v_iso",
  "edge_h_sharp",
  "edge_h_outline",
  "edge_h_smooth",
  "edge_h_sewn",
  "edge_h_iso",
  "union_outline_polygons",
  "mesh_linear_deflection",
  "mesh_angular_deflection",
  "mesh_relative",
  "mesh_deflection_mode",
  "mesh_deflection_coefficient",
  "hlr_angle_tolerance",
  "fast",
] as const satisfies readonly (keyof HlrProjectionOptionsA0)[];

function illustrationHlrOptions(
  options: FastHlrIllustrationRequest["hlr"],
): HlrProjectionOptionsA0 {
  if (options === undefined) return {};
  return Object.fromEntries(
    ILLUSTRATION_HLR_OPTION_KEYS.flatMap((key) =>
      options[key] === undefined ? [] : [[key, options[key]]],
    ),
  ) as HlrProjectionOptionsA0;
}

/** Prepare Fast vector linework and the colorized scene once for repeated rendering. */
export async function createFastHlrIllustrator(
  projector: MeshHlrProjector,
  request: FastHlrIllustrationRequest,
): Promise<PreparedFastHlrIllustration> {
  const input = request.illustration;
  const hlr = await projector.meshHlrProjection({
    mesh: indexedMeshFromIllustrationInput(input),
    options: {
      ...illustrationHlrOptions(request.hlr),
      views: [
        {
          id: "illustration",
          direction: input.view.direction,
          up: input.view.up,
        },
      ],
      projection_algorithm: "fast",
      outline_algorithm: "fast-mesh-shadow",
    },
  });
  const view = hlr.views[0];
  if (view === undefined) throw new Error("Fast HLR returned no illustration view.");
  return {
    hlr,
    illustrator: createIllustrator(input, lineworkFromHlr(view, input.view.mirror_x === true)),
  };
}

/** Project Fast vector linework, colorize the mesh, and return one SVG result. */
export async function illustrateMeshWithFastHlr(
  projector: MeshHlrProjector,
  request: FastHlrIllustrationRequest,
): Promise<FastHlrIllustrationResult> {
  const prepared = await createFastHlrIllustrator(projector, request);
  try {
    return { hlr: prepared.hlr, illustration: prepared.illustrator.renderSvg() };
  } finally {
    prepared.illustrator.dispose();
  }
}

/** Flatten governed illustration meshes and column-major transforms into one HLR mesh. */
export function indexedMeshFromIllustrationInput(
  input: MeshIllustrationInputA0,
): IndexedTriangleMeshA0 {
  const positions: number[] = [];
  const indices: number[] = [];
  for (const mesh of input.meshes) {
    if (mesh.positions.length % 3 !== 0) {
      throw new Error(`Mesh ${mesh.id} position length must be divisible by three.`);
    }
    const vertexOffset = positions.length / 3;
    for (let offset = 0; offset < mesh.positions.length; offset += 3) {
      const point = transformPoint(
        [
          mesh.positions[offset] ?? Number.NaN,
          mesh.positions[offset + 1] ?? Number.NaN,
          mesh.positions[offset + 2] ?? Number.NaN,
        ],
        mesh.matrix,
      );
      positions.push(...point);
    }
    const sourceIndices = mesh.indices ?? sequentialIndices(mesh.positions.length / 3);
    if (sourceIndices.length % 3 !== 0) {
      throw new Error(`Mesh ${mesh.id} index length must be divisible by three.`);
    }
    for (const index of sourceIndices) indices.push(vertexOffset + index);
  }
  return { positions, indices };
}

function sequentialIndices(count: number): number[] {
  return Array.from({ length: count }, (_, index) => index);
}

function transformPoint(
  point: readonly [number, number, number],
  matrix: IllustrationMatrix4x4 | undefined,
): readonly [number, number, number] {
  if (matrix === undefined) return point;
  const [x, y, z] = point;
  const w = matrix[3] * x + matrix[7] * y + matrix[11] * z + matrix[15];
  const divisor = Math.abs(w) > 1e-15 ? w : 1;
  return [
    (matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12]) / divisor,
    (matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13]) / divisor,
    (matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14]) / divisor,
  ];
}

function lineworkFromHlr(
  view: HlrProjectionResultA0["views"][number],
  mirrorX: boolean,
): MeshIllustrationLinework {
  const convert = (segments: readonly (readonly [number, number, number, number])[]) =>
    segments.map(([x1, y1, x2, y2]) => ({
      points: [
        [mirrorX ? -x1 : x1, y1],
        [mirrorX ? -x2 : x2, y2],
      ] as const,
    }));
  return {
    outlineSegments: convert(view.modes.outline.segments),
    detailSegments: convert(view.modes.detail.segments),
  };
}
