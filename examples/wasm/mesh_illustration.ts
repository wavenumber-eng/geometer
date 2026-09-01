export type Vec2 = readonly [number, number];
export type Vec3 = readonly [number, number, number];
export type Rgb = readonly [number, number, number];

export interface MeshIllustrationMaterial {
  /** Source color expressed as sRGB channel values in the inclusive range [0, 1]. */
  color: Rgb;
  opacity?: number;
  name?: string;
}

export interface MeshIllustrationMesh {
  id: string;
  positions: ArrayLike<number>;
  normals?: ArrayLike<number>;
  indices?: ArrayLike<number>;
  matrix?: ArrayLike<number>;
  materials: readonly MeshIllustrationMaterial[];
  triangleMaterialIndices?: ArrayLike<number>;
  doubleSided?: boolean;
}

export interface MeshIllustrationInput {
  meshes: readonly MeshIllustrationMesh[];
}

export interface MeshIllustrationView {
  direction: Vec3;
  up: Vec3;
  mirrorX?: boolean;
}

export interface MeshIllustrationPrepareOptions {
  maxTriangles?: number;
  weldTolerance?: number;
}

export type MeshIllustrationShading = "unlit" | "flat" | "lambert" | "banded" | "toon";

export interface MeshIllustrationStyle {
  shading: MeshIllustrationShading;
  ambient: number;
  keyIntensity: number;
  lightDirection: Vec3;
  bands: number;
  sourceColors: boolean;
  fallbackColor: Rgb;
  background: string;
  transparentBackground: boolean;
  showHlrOutline: boolean;
  showHlrDetail: boolean;
  showOutlines: boolean;
  showCreases: boolean;
  creaseAngleDegrees: number;
  outlineColor: string;
  creaseColor: string;
  outlineWidth: number;
  creaseWidth: number;
  doubleSided: boolean;
  rimAmount: number;
}

export interface IllustrationTriangle {
  points: readonly [Vec2, Vec2, Vec2];
  depths: readonly [number, number, number];
  depth: number;
  normal: Vec3;
  geometricNormal: Vec3;
  baseColor: Rgb;
  opacity: number;
  frontFacing: boolean;
  doubleSided: boolean;
  meshId: string;
  triangleIndex: number;
}

export interface IllustrationEdge {
  points: readonly [Vec2, Vec2];
  depth: number;
  frontA: boolean;
  frontB: boolean | null;
  normalA: Vec3;
  normalB: Vec3 | null;
  meshId: string;
}

export interface IllustrationLineSegment {
  points: readonly [Vec2, Vec2];
}

export interface MeshIllustrationScene {
  schema: "geometry.mesh_illustration.prototype.a0";
  view: {
    direction: Vec3;
    up: Vec3;
    right: Vec3;
    mirrorX: boolean;
  };
  bounds: { minX: number; minY: number; maxX: number; maxY: number };
  triangles: readonly IllustrationTriangle[];
  edges: readonly IllustrationEdge[];
  /** Optional CAD-derived linework, such as Geometer's HLR mesh-shadow outline. */
  outlineSegments?: readonly IllustrationLineSegment[];
  /** Optional visible-edge detail produced by Geometer HLR. */
  detailSegments?: readonly IllustrationLineSegment[];
  stats: {
    sourceMeshes: number;
    sourceTriangles: number;
    projectedTriangles: number;
    edges: number;
  };
  warnings: readonly string[];
}

export interface IllustrationRenderStats {
  triangles: number;
  outlines: number;
  details: number;
  creases: number;
  commands: number;
}

interface EdgeAccumulator {
  points: readonly [Vec2, Vec2];
  depth: number;
  frontA: boolean;
  frontB: boolean | null;
  normalA: Vec3;
  normalB: Vec3 | null;
  meshId: string;
}

interface TriangleCommand {
  kind: "triangle";
  depth: number;
  order: number;
  triangle: IllustrationTriangle;
  fill: string;
  opacity: number;
}

interface StrokeCommand {
  kind: "outline" | "crease";
  depth: number;
  order: number;
  edge: IllustrationEdge;
  color: string;
  width: number;
}

interface HlrLineCommand {
  kind: "hlr-outline" | "hlr-detail";
  depth: number;
  order: number;
  points: readonly [Vec2, Vec2];
  color: string;
  width: number;
}

type RenderCommand = TriangleCommand | StrokeCommand | HlrLineCommand;

const IDENTITY_MATRIX = Object.freeze([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
const VISIBILITY_ORDER_CACHE = new WeakMap<
  MeshIllustrationScene,
  Map<boolean, readonly IllustrationTriangle[]>
>();

function finite(value: number, fallback = 0): number {
  return Number.isFinite(value) ? value : fallback;
}

function clamp(value: number, minimum = 0, maximum = 1): number {
  return Math.min(maximum, Math.max(minimum, finite(value)));
}

function add(a: Vec3, b: Vec3): Vec3 {
  return [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
}

function subtract(a: Vec3, b: Vec3): Vec3 {
  return [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
}

function dot(a: Vec3, b: Vec3): number {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

function cross(a: Vec3, b: Vec3): Vec3 {
  return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
}

function length(value: Vec3): number {
  return Math.hypot(value[0], value[1], value[2]);
}

function normalize(value: Vec3, label: string): Vec3 {
  const magnitude = length(value);
  if (!Number.isFinite(magnitude) || magnitude < 1e-12)
    throw new Error(`${label} must be a finite non-zero vector.`);
  return [value[0] / magnitude, value[1] / magnitude, value[2] / magnitude];
}

function matrixValue(matrix: ArrayLike<number>, index: number): number {
  return finite(Number(matrix[index]), IDENTITY_MATRIX[index] ?? 0);
}

function transformPoint(matrix: ArrayLike<number>, point: Vec3): Vec3 {
  const x = point[0];
  const y = point[1];
  const z = point[2];
  const w =
    matrixValue(matrix, 3) * x +
      matrixValue(matrix, 7) * y +
      matrixValue(matrix, 11) * z +
      matrixValue(matrix, 15) || 1;
  return [
    (matrixValue(matrix, 0) * x +
      matrixValue(matrix, 4) * y +
      matrixValue(matrix, 8) * z +
      matrixValue(matrix, 12)) /
      w,
    (matrixValue(matrix, 1) * x +
      matrixValue(matrix, 5) * y +
      matrixValue(matrix, 9) * z +
      matrixValue(matrix, 13)) /
      w,
    (matrixValue(matrix, 2) * x +
      matrixValue(matrix, 6) * y +
      matrixValue(matrix, 10) * z +
      matrixValue(matrix, 14)) /
      w,
  ];
}

function matrixDeterminantSign(matrix: ArrayLike<number>): number {
  const a = matrixValue(matrix, 0);
  const b = matrixValue(matrix, 4);
  const c = matrixValue(matrix, 8);
  const d = matrixValue(matrix, 1);
  const e = matrixValue(matrix, 5);
  const f = matrixValue(matrix, 9);
  const g = matrixValue(matrix, 2);
  const h = matrixValue(matrix, 6);
  const i = matrixValue(matrix, 10);
  const determinant = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
  return determinant < 0 ? -1 : 1;
}

function transformNormal(matrix: ArrayLike<number>, normal: Vec3): Vec3 {
  const a = matrixValue(matrix, 0);
  const b = matrixValue(matrix, 4);
  const c = matrixValue(matrix, 8);
  const d = matrixValue(matrix, 1);
  const e = matrixValue(matrix, 5);
  const f = matrixValue(matrix, 9);
  const g = matrixValue(matrix, 2);
  const h = matrixValue(matrix, 6);
  const i = matrixValue(matrix, 10);
  const transformed: Vec3 = [
    (e * i - f * h) * normal[0] + (f * g - d * i) * normal[1] + (d * h - e * g) * normal[2],
    (c * h - b * i) * normal[0] + (a * i - c * g) * normal[1] + (b * g - a * h) * normal[2],
    (b * f - c * e) * normal[0] + (c * d - a * f) * normal[1] + (a * e - b * d) * normal[2],
  ];
  const orientation = matrixDeterminantSign(matrix);
  return normalize(
    [transformed[0] * orientation, transformed[1] * orientation, transformed[2] * orientation],
    "Transformed normal",
  );
}

function positionAt(positions: ArrayLike<number>, index: number): Vec3 {
  const offset = index * 3;
  return [
    finite(Number(positions[offset])),
    finite(Number(positions[offset + 1])),
    finite(Number(positions[offset + 2])),
  ];
}

function averagedSourceNormal(
  normals: ArrayLike<number> | undefined,
  indices: readonly [number, number, number],
  matrix: ArrayLike<number>,
  fallback: Vec3,
): Vec3 {
  if (!normals || normals.length === 0) return fallback;
  const average = indices.reduce<Vec3>(
    (sum, index) => add(sum, positionAt(normals, index)),
    [0, 0, 0],
  );
  if (length(average) < 1e-12) return fallback;
  return transformNormal(matrix, normalize(average, "Source normal"));
}

function indexAt(indices: ArrayLike<number> | undefined, index: number): number {
  return indices ? Math.trunc(finite(Number(indices[index]))) : index;
}

function projectPoint(point: Vec3, right: Vec3, up: Vec3, direction: Vec3, mirrorX: boolean) {
  return {
    point: [(mirrorX ? -1 : 1) * dot(point, right), dot(point, up)] as Vec2,
    depth: dot(point, direction),
  };
}

function quantizedPointKey(point: Vec3, tolerance: number): string {
  const scale = 1 / tolerance;
  return `${Math.round(point[0] * scale)},${Math.round(point[1] * scale)},${Math.round(point[2] * scale)}`;
}

function edgeKey(a: Vec3, b: Vec3, tolerance: number): string {
  const ka = quantizedPointKey(a, tolerance);
  const kb = quantizedPointKey(b, tolerance);
  return ka < kb ? `${ka}|${kb}` : `${kb}|${ka}`;
}

function materialForTriangle(
  mesh: MeshIllustrationMesh,
  triangleIndex: number,
): MeshIllustrationMaterial {
  const requested = Math.trunc(Number(mesh.triangleMaterialIndices?.[triangleIndex] ?? 0));
  return mesh.materials[requested] ?? mesh.materials[0] ?? { color: [0.72, 0.74, 0.78] };
}

function validateMesh(mesh: MeshIllustrationMesh): number {
  if (!mesh.id) throw new Error("Every mesh needs a non-empty id.");
  if (mesh.positions.length % 3 !== 0)
    throw new Error(`Mesh ${mesh.id} position length must be divisible by 3.`);
  const elementCount = mesh.indices?.length ?? mesh.positions.length / 3;
  if (elementCount % 3 !== 0)
    throw new Error(`Mesh ${mesh.id} index/vertex element count must be divisible by 3.`);
  if (mesh.matrix && mesh.matrix.length !== 16)
    throw new Error(`Mesh ${mesh.id} matrix must contain 16 column-major values.`);
  return elementCount / 3;
}

export function prepareMeshIllustration(
  input: MeshIllustrationInput,
  view: MeshIllustrationView,
  options: MeshIllustrationPrepareOptions = {},
): MeshIllustrationScene {
  const direction = normalize(view.direction, "View direction");
  const requestedUp = normalize(view.up, "View up");
  const right = normalize(cross(requestedUp, direction), "View right");
  const up = normalize(cross(direction, right), "Orthogonal view up");
  const mirrorX = view.mirrorX === true;
  const maxTriangles = Math.max(1, Math.trunc(options.maxTriangles ?? 120_000));
  const weldTolerance = Math.max(1e-9, finite(options.weldTolerance ?? 1e-6, 1e-6));
  const triangles: IllustrationTriangle[] = [];
  const edgeMaps = new Map<string, EdgeAccumulator>();
  const warnings: string[] = [];
  let sourceTriangles = 0;
  let minX = Infinity;
  let minY = Infinity;
  let maxX = -Infinity;
  let maxY = -Infinity;

  for (const mesh of input.meshes) {
    const triangleCount = validateMesh(mesh);
    sourceTriangles += triangleCount;
    if (sourceTriangles > maxTriangles)
      throw new Error(
        `Mesh illustration exceeds the ${maxTriangles.toLocaleString()} triangle prototype limit.`,
      );
    const matrix = mesh.matrix ?? IDENTITY_MATRIX;
    const orientation = matrixDeterminantSign(matrix);

    for (let triangleIndex = 0; triangleIndex < triangleCount; triangleIndex += 1) {
      const indices = [
        indexAt(mesh.indices, triangleIndex * 3),
        indexAt(mesh.indices, triangleIndex * 3 + 1),
        indexAt(mesh.indices, triangleIndex * 3 + 2),
      ] as const;
      const vertexCount = mesh.positions.length / 3;
      if (indices.some((index) => index < 0 || index >= vertexCount)) {
        warnings.push(`Skipped out-of-range triangle ${triangleIndex} in ${mesh.id}.`);
        continue;
      }

      const world = indices.map((index) =>
        transformPoint(matrix, positionAt(mesh.positions, index)),
      ) as [Vec3, Vec3, Vec3];
      const crossNormal = cross(subtract(world[1], world[0]), subtract(world[2], world[0]));
      const magnitude = length(crossNormal);
      if (magnitude < 1e-12) {
        warnings.push(`Skipped degenerate triangle ${triangleIndex} in ${mesh.id}.`);
        continue;
      }
      const normal = normalize(
        [crossNormal[0] * orientation, crossNormal[1] * orientation, crossNormal[2] * orientation],
        "Triangle normal",
      );
      const shadingNormal = averagedSourceNormal(mesh.normals, indices, matrix, normal);
      const frontFacing = dot(normal, direction) > 1e-9;
      const projected = world.map((point) =>
        projectPoint(point, right, up, direction, mirrorX),
      ) as [
        { point: Vec2; depth: number },
        { point: Vec2; depth: number },
        { point: Vec2; depth: number },
      ];
      for (const item of projected) {
        minX = Math.min(minX, item.point[0]);
        minY = Math.min(minY, item.point[1]);
        maxX = Math.max(maxX, item.point[0]);
        maxY = Math.max(maxY, item.point[1]);
      }
      const material = materialForTriangle(mesh, triangleIndex);
      const triangle: IllustrationTriangle = {
        points: [projected[0].point, projected[1].point, projected[2].point],
        depths: [projected[0].depth, projected[1].depth, projected[2].depth],
        depth: (projected[0].depth + projected[1].depth + projected[2].depth) / 3,
        normal: shadingNormal,
        geometricNormal: normal,
        baseColor: [
          clamp(Number(material.color[0])),
          clamp(Number(material.color[1])),
          clamp(Number(material.color[2])),
        ],
        opacity: clamp(material.opacity ?? 1),
        frontFacing,
        doubleSided: mesh.doubleSided === true,
        meshId: mesh.id,
        triangleIndex,
      };
      triangles.push(triangle);

      for (const [start, end] of [
        [0, 1],
        [1, 2],
        [2, 0],
      ] as const) {
        // OCCT GLB export may split adjacent CAD faces into separate mesh
        // primitives. Weld in world space across those primitives so their
        // shared tessellation edges do not become false silhouettes.
        const key = edgeKey(world[start], world[end], weldTolerance);
        const existing = edgeMaps.get(key);
        if (!existing) {
          edgeMaps.set(key, {
            points: [projected[start].point, projected[end].point],
            depth: (projected[start].depth + projected[end].depth) / 2,
            frontA: frontFacing,
            frontB: null,
            normalA: normal,
            normalB: null,
            meshId: mesh.id,
          });
        } else if (existing.normalB === null) {
          existing.frontB = frontFacing;
          existing.normalB = normal;
        }
      }
    }
  }

  if (!Number.isFinite(minX)) {
    minX = -1;
    minY = -1;
    maxX = 1;
    maxY = 1;
  }

  return {
    schema: "geometry.mesh_illustration.prototype.a0",
    view: { direction, up, right, mirrorX },
    bounds: { minX, minY, maxX, maxY },
    triangles,
    edges: [...edgeMaps.values()],
    stats: {
      sourceMeshes: input.meshes.length,
      sourceTriangles,
      projectedTriangles: triangles.length,
      edges: edgeMaps.size,
    },
    warnings,
  };
}

function srgbToLinear(value: number): number {
  const v = clamp(value);
  return v <= 0.04045 ? v / 12.92 : ((v + 0.055) / 1.055) ** 2.4;
}

function linearToSrgb(value: number): number {
  const v = clamp(value);
  return v <= 0.0031308 ? v * 12.92 : 1.055 * v ** (1 / 2.4) - 0.055;
}

function rgbCss(color: Rgb, intensity: number, rim: number, darkLift = 0): string {
  const channels = color.map((value) => {
    const lit = srgbToLinear(value) * Math.max(0, intensity) + clamp(darkLift, 0, 0.08);
    const withRim = lit + (1 - lit) * clamp(rim);
    return Math.round(clamp(linearToSrgb(withRim)) * 255);
  });
  return `rgb(${channels[0]},${channels[1]},${channels[2]})`;
}

function triangleFill(
  triangle: IllustrationTriangle,
  scene: MeshIllustrationScene,
  style: MeshIllustrationStyle,
): string {
  const base = style.sourceColors ? triangle.baseColor : style.fallbackColor;
  if (style.shading === "unlit") return rgbCss(base, 1, 0);
  const light = normalize(style.lightDirection, "Light direction");
  const activeNormal = style.shading === "flat" ? triangle.geometricNormal : triangle.normal;
  const diffuse = Math.max(0, dot(activeNormal, light));
  let intensity = clamp(style.ambient) + clamp(style.keyIntensity, 0, 4) * diffuse;
  intensity = clamp(intensity);
  if (style.shading === "banded" || style.shading === "toon") {
    const bands = Math.max(2, Math.min(32, Math.trunc(style.bands)));
    intensity = Math.round(intensity * (bands - 1)) / (bands - 1);
  }
  const rim =
    style.shading === "toon"
      ? clamp(style.rimAmount) *
        clamp((1 - Math.abs(dot(activeNormal, scene.view.direction)) - 0.45) / 0.55)
      : 0;
  return rgbCss(base, intensity, rim, style.shading === "toon" ? 0.012 : 0);
}

function edgeKind(
  edge: IllustrationEdge,
  style: MeshIllustrationStyle,
): "outline" | "crease" | null {
  const frontCount = Number(edge.frontA) + Number(edge.frontB === true);
  const boundary = edge.frontB === null;
  // The STEP-to-GLB compatibility path can expose a triangulated CAD face as
  // triangle soup. Treating every unmatched edge as an open-boundary outline
  // reveals the tessellation. The prototype therefore draws closed-mesh
  // front/back silhouettes and shared creases; explicit open-mesh boundary
  // policy belongs in the reviewed contract.
  if (style.showOutlines && !boundary && frontCount === 1 && edge.frontA !== edge.frontB)
    return "outline";
  if (style.showCreases && edge.frontA && edge.frontB && edge.normalB) {
    const threshold = Math.cos((clamp(style.creaseAngleDegrees, 0, 180) * Math.PI) / 180);
    if (dot(edge.normalA, edge.normalB) < threshold) return "crease";
  }
  return null;
}

interface ProjectedBounds {
  minX: number;
  minY: number;
  maxX: number;
  maxY: number;
}

function projectedBounds(triangle: IllustrationTriangle): ProjectedBounds {
  return {
    minX: Math.min(...triangle.points.map((point) => point[0])),
    minY: Math.min(...triangle.points.map((point) => point[1])),
    maxX: Math.max(...triangle.points.map((point) => point[0])),
    maxY: Math.max(...triangle.points.map((point) => point[1])),
  };
}

function boundsOverlap(a: ProjectedBounds, b: ProjectedBounds, epsilon: number): boolean {
  return !(
    a.maxX <= b.minX + epsilon ||
    b.maxX <= a.minX + epsilon ||
    a.maxY <= b.minY + epsilon ||
    b.maxY <= a.minY + epsilon
  );
}

function cross2(a: Vec2, b: Vec2, point: Vec2): number {
  return (b[0] - a[0]) * (point[1] - a[1]) - (b[1] - a[1]) * (point[0] - a[0]);
}

function polygonSignedArea(points: readonly Vec2[]): number {
  let twiceArea = 0;
  for (let index = 0; index < points.length; index += 1) {
    const current = points[index] as Vec2;
    const next = points[(index + 1) % points.length] as Vec2;
    twiceArea += current[0] * next[1] - next[0] * current[1];
  }
  return twiceArea * 0.5;
}

function clipConvexPolygon(
  subject: readonly Vec2[],
  clip: readonly [Vec2, Vec2, Vec2],
  epsilon: number,
): Vec2[] {
  let output = [...subject];
  const orientation = polygonSignedArea(clip) >= 0 ? 1 : -1;
  for (let edgeIndex = 0; edgeIndex < clip.length && output.length > 0; edgeIndex += 1) {
    const edgeStart = clip[edgeIndex] as Vec2;
    const edgeEnd = clip[(edgeIndex + 1) % clip.length] as Vec2;
    const input = output;
    output = [];
    let previous = input[input.length - 1] as Vec2;
    let previousDistance = orientation * cross2(edgeStart, edgeEnd, previous);
    for (const current of input) {
      const currentDistance = orientation * cross2(edgeStart, edgeEnd, current);
      const previousInside = previousDistance >= -epsilon;
      const currentInside = currentDistance >= -epsilon;
      if (previousInside !== currentInside) {
        const denominator = previousDistance - currentDistance;
        if (Math.abs(denominator) > epsilon) {
          const ratio = previousDistance / denominator;
          output.push([
            previous[0] + (current[0] - previous[0]) * ratio,
            previous[1] + (current[1] - previous[1]) * ratio,
          ]);
        }
      }
      if (currentInside) output.push(current);
      previous = current;
      previousDistance = currentDistance;
    }
  }
  return output;
}

function depthAt(triangle: IllustrationTriangle, point: Vec2): number {
  const [a, b, c] = triangle.points;
  const denominator = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1]);
  if (Math.abs(denominator) < 1e-20) return triangle.depth;
  const weightA =
    ((b[1] - c[1]) * (point[0] - c[0]) + (c[0] - b[0]) * (point[1] - c[1])) / denominator;
  const weightB =
    ((c[1] - a[1]) * (point[0] - c[0]) + (a[0] - c[0]) * (point[1] - c[1])) / denominator;
  return (
    weightA * triangle.depths[0] +
    weightB * triangle.depths[1] +
    (1 - weightA - weightB) * triangle.depths[2]
  );
}

function overlapDepthOrder(
  a: IllustrationTriangle,
  b: IllustrationTriangle,
  coordinateEpsilon: number,
): -1 | 0 | 1 {
  const overlap = clipConvexPolygon(a.points, b.points, coordinateEpsilon);
  if (overlap.length < 3 || Math.abs(polygonSignedArea(overlap)) <= coordinateEpsilon ** 2)
    return 0;
  let minimum = Infinity;
  let maximum = -Infinity;
  let maximumDepth = 1;
  for (const point of overlap) {
    const depthA = depthAt(a, point);
    const depthB = depthAt(b, point);
    const difference = depthA - depthB;
    minimum = Math.min(minimum, difference);
    maximum = Math.max(maximum, difference);
    maximumDepth = Math.max(maximumDepth, Math.abs(depthA), Math.abs(depthB));
  }
  const depthEpsilon = maximumDepth * 1e-10;
  if (minimum >= -depthEpsilon && maximum > depthEpsilon) return 1;
  if (maximum <= depthEpsilon && minimum < -depthEpsilon) return -1;
  return 0;
}

function heapPush(heap: number[], value: number, compare: (a: number, b: number) => number): void {
  heap.push(value);
  let index = heap.length - 1;
  while (index > 0) {
    const parent = Math.floor((index - 1) / 2);
    const parentValue = heap[parent] as number;
    if (compare(parentValue, value) <= 0) break;
    heap[index] = parentValue;
    index = parent;
  }
  heap[index] = value;
}

function heapPop(heap: number[], compare: (a: number, b: number) => number): number | undefined {
  const first = heap[0];
  const last = heap.pop();
  if (first === undefined || last === undefined || heap.length === 0) return first;
  let index = 0;
  while (true) {
    const left = index * 2 + 1;
    if (left >= heap.length) break;
    const right = left + 1;
    let child = left;
    const leftValue = heap[left] as number;
    if (right < heap.length && compare(heap[right] as number, leftValue) < 0) child = right;
    const childValue = heap[child] as number;
    if (compare(childValue, last) >= 0) break;
    heap[index] = childValue;
    index = child;
  }
  heap[index] = last;
  return first;
}

function triangleCommandOrder(a: TriangleCommand, b: TriangleCommand): number {
  return a.depth - b.depth || a.order - b.order;
}

function stronglyConnectedComponents(
  outgoing: readonly (readonly number[])[],
  reverse: readonly (readonly number[])[],
): { components: number[][]; componentOf: Int32Array } {
  const visited = new Uint8Array(outgoing.length);
  const finishOrder: number[] = [];
  for (let start = 0; start < outgoing.length; start += 1) {
    if (visited[start] === 1) continue;
    const nodes = [start];
    const offsets = [0];
    visited[start] = 1;
    while (nodes.length > 0) {
      const stackIndex = nodes.length - 1;
      const node = nodes[stackIndex] as number;
      const offset = offsets[stackIndex] as number;
      const neighbors = outgoing[node] as readonly number[];
      if (offset < neighbors.length) {
        const next = neighbors[offset] as number;
        offsets[stackIndex] = offset + 1;
        if (visited[next] === 0) {
          visited[next] = 1;
          nodes.push(next);
          offsets.push(0);
        }
      } else {
        finishOrder.push(node);
        nodes.pop();
        offsets.pop();
      }
    }
  }

  const componentOf = new Int32Array(outgoing.length);
  componentOf.fill(-1);
  const components: number[][] = [];
  for (let orderIndex = finishOrder.length - 1; orderIndex >= 0; orderIndex -= 1) {
    const start = finishOrder[orderIndex] as number;
    if (componentOf[start] !== -1) continue;
    const componentIndex = components.length;
    const members: number[] = [];
    const stack = [start];
    componentOf[start] = componentIndex;
    while (stack.length > 0) {
      const node = stack.pop() as number;
      members.push(node);
      for (const next of reverse[node] as readonly number[]) {
        if (componentOf[next] !== -1) continue;
        componentOf[next] = componentIndex;
        stack.push(next);
      }
    }
    components.push(members);
  }
  return { components, componentOf };
}

function orderTriangleCommands(
  commands: readonly TriangleCommand[],
  bounds: MeshIllustrationScene["bounds"],
): TriangleCommand[] {
  if (commands.length < 2) return [...commands];
  // Orthographic triangle depth is affine in projected X/Y. Clipping each
  // candidate pair to its convex overlap and checking that polygon's vertices
  // therefore proves a constant behind/in-front relation over the whole
  // overlap. The grid avoids an O(n^2) scan on dense CAD tessellations.
  const width = Math.max(bounds.maxX - bounds.minX, 1e-12);
  const height = Math.max(bounds.maxY - bounds.minY, 1e-12);
  const coordinateEpsilon = Math.max(width, height) * 1e-10;
  const gridSize = Math.max(8, Math.min(192, Math.ceil(Math.sqrt(commands.length / 4))));
  const cellWidth = width / gridSize;
  const cellHeight = height / gridSize;
  const boxes = commands.map((command) => projectedBounds(command.triangle));
  const buckets = new Map<number, number[]>();
  const broadTriangles: number[] = [];
  const previousTriangles: number[] = [];
  const marks = new Int32Array(commands.length);
  const outgoing: number[][] = Array.from({ length: commands.length }, () => []);
  const reverse: number[][] = Array.from({ length: commands.length }, () => []);
  const cellRange = (box: ProjectedBounds): [number, number, number, number] => [
    Math.max(0, Math.min(gridSize - 1, Math.floor((box.minX - bounds.minX) / cellWidth))),
    Math.max(0, Math.min(gridSize - 1, Math.floor((box.maxX - bounds.minX) / cellWidth))),
    Math.max(0, Math.min(gridSize - 1, Math.floor((box.minY - bounds.minY) / cellHeight))),
    Math.max(0, Math.min(gridSize - 1, Math.floor((box.maxY - bounds.minY) / cellHeight))),
  ];
  const addConstraint = (index: number, candidate: number): void => {
    const box = boxes[index] as ProjectedBounds;
    const candidateBox = boxes[candidate] as ProjectedBounds;
    if (!boundsOverlap(box, candidateBox, coordinateEpsilon)) return;
    const order = overlapDepthOrder(
      (commands[index] as TriangleCommand).triangle,
      (commands[candidate] as TriangleCommand).triangle,
      coordinateEpsilon,
    );
    if (order === 0) return;
    const behind = order > 0 ? candidate : index;
    const front = order > 0 ? index : candidate;
    (outgoing[behind] as number[]).push(front);
    (reverse[front] as number[]).push(behind);
  };

  for (let index = 0; index < commands.length; index += 1) {
    const box = boxes[index] as ProjectedBounds;
    const [minCellX, maxCellX, minCellY, maxCellY] = cellRange(box);
    const coveredCells = (maxCellX - minCellX + 1) * (maxCellY - minCellY + 1);
    const broad = coveredCells > Math.max(64, gridSize * 2);
    const stamp = index + 1;
    if (broad) {
      for (const candidate of previousTriangles) addConstraint(index, candidate);
    } else {
      for (const candidate of broadTriangles) addConstraint(index, candidate);
      for (let cellY = minCellY; cellY <= maxCellY; cellY += 1) {
        for (let cellX = minCellX; cellX <= maxCellX; cellX += 1) {
          const bucket = buckets.get(cellY * gridSize + cellX);
          if (!bucket) continue;
          for (const candidate of bucket) {
            if (marks[candidate] === stamp) continue;
            marks[candidate] = stamp;
            addConstraint(index, candidate);
          }
        }
      }
      for (let cellY = minCellY; cellY <= maxCellY; cellY += 1) {
        for (let cellX = minCellX; cellX <= maxCellX; cellX += 1) {
          const key = cellY * gridSize + cellX;
          const bucket = buckets.get(key);
          if (bucket) bucket.push(index);
          else buckets.set(key, [index]);
        }
      }
    }
    if (broad) broadTriangles.push(index);
    previousTriangles.push(index);
  }

  const { components, componentOf } = stronglyConnectedComponents(outgoing, reverse);
  const componentOutgoing = components.map(() => new Set<number>());
  const componentIncoming = new Uint32Array(components.length);
  for (let behind = 0; behind < outgoing.length; behind += 1) {
    const behindComponent = componentOf[behind] as number;
    for (const front of outgoing[behind] as number[]) {
      const frontComponent = componentOf[front] as number;
      if (behindComponent === frontComponent) continue;
      const targets = componentOutgoing[behindComponent] as Set<number>;
      if (targets.has(frontComponent)) continue;
      targets.add(frontComponent);
      componentIncoming[frontComponent] = (componentIncoming[frontComponent] ?? 0) + 1;
    }
  }

  const componentDepths = components.map(
    (members) =>
      members.reduce((sum, member) => sum + (commands[member] as TriangleCommand).depth, 0) /
      members.length,
  );
  const componentOrders = components.map((members) =>
    members.reduce(
      (minimum, member) => Math.min(minimum, (commands[member] as TriangleCommand).order),
      Infinity,
    ),
  );
  const compareComponents = (a: number, b: number): number =>
    (componentDepths[a] as number) - (componentDepths[b] as number) ||
    (componentOrders[a] as number) - (componentOrders[b] as number);
  const ready: number[] = [];
  for (let component = 0; component < components.length; component += 1) {
    if (componentIncoming[component] === 0) heapPush(ready, component, compareComponents);
  }
  const ordered: TriangleCommand[] = [];
  while (ready.length > 0) {
    const component = heapPop(ready, compareComponents) as number;
    const members = [...(components[component] as number[])].sort((a, b) =>
      triangleCommandOrder(commands[a] as TriangleCommand, commands[b] as TriangleCommand),
    );
    for (const member of members) ordered.push(commands[member] as TriangleCommand);
    for (const front of componentOutgoing[component] as Set<number>) {
      const count = componentIncoming[front] ?? 0;
      if (count > 0) componentIncoming[front] = count - 1;
      if ((componentIncoming[front] ?? 0) === 0) heapPush(ready, front, compareComponents);
    }
  }
  return ordered;
}

function renderCommands(
  scene: MeshIllustrationScene,
  style: MeshIllustrationStyle,
): { commands: RenderCommand[]; stats: IllustrationRenderStats } {
  const span = Math.max(
    scene.bounds.maxX - scene.bounds.minX,
    scene.bounds.maxY - scene.bounds.minY,
    1e-9,
  );
  const triangleCommands: TriangleCommand[] = [];
  const strokeCommands: StrokeCommand[] = [];
  const hlrCommands: HlrLineCommand[] = [];
  let order = 0;
  let triangleCount = 0;
  let outlines = 0;
  let details = 0;
  let creases = 0;
  for (const triangle of scene.triangles) {
    if (!triangle.frontFacing && !(style.doubleSided || triangle.doubleSided)) continue;
    triangleCommands.push({
      kind: "triangle",
      depth: triangle.depth,
      order: order++,
      triangle,
      fill: triangleFill(triangle, scene, style),
      opacity: triangle.opacity,
    });
    triangleCount += 1;
  }
  for (const edge of scene.edges) {
    const kind = edgeKind(edge, style);
    if (!kind) continue;
    strokeCommands.push({
      kind,
      depth: edge.depth,
      order: order++,
      edge,
      color: kind === "outline" ? style.outlineColor : style.creaseColor,
      width: span * (kind === "outline" ? style.outlineWidth : style.creaseWidth),
    });
    if (kind === "outline") outlines += 1;
    else creases += 1;
  }
  if (style.showHlrOutline) {
    for (const segment of scene.outlineSegments ?? []) {
      hlrCommands.push({
        kind: "hlr-outline",
        depth: Number.MAX_VALUE,
        order: order++,
        points: segment.points,
        color: style.outlineColor,
        width: span * style.outlineWidth,
      });
      outlines += 1;
    }
  }
  if (style.showHlrDetail) {
    for (const segment of scene.detailSegments ?? []) {
      hlrCommands.push({
        kind: "hlr-detail",
        depth: Number.MAX_VALUE,
        order: order++,
        points: segment.points,
        color: style.creaseColor,
        width: span * style.creaseWidth,
      });
      details += 1;
    }
  }
  let sceneCache = VISIBILITY_ORDER_CACHE.get(scene);
  if (!sceneCache) {
    sceneCache = new Map();
    VISIBILITY_ORDER_CACHE.set(scene, sceneCache);
  }
  const cachedTriangles = sceneCache.get(style.doubleSided);
  const commandByTriangle = new Map(
    triangleCommands.map((command) => [command.triangle, command] as const),
  );
  let orderedTriangles: TriangleCommand[];
  if (
    cachedTriangles?.length === triangleCommands.length &&
    cachedTriangles.every((triangle) => commandByTriangle.has(triangle))
  ) {
    orderedTriangles = cachedTriangles.map(
      (triangle) => commandByTriangle.get(triangle) as TriangleCommand,
    );
  } else {
    orderedTriangles = orderTriangleCommands(triangleCommands, scene.bounds);
    sceneCache.set(
      style.doubleSided,
      orderedTriangles.map((command) => command.triangle),
    );
  }
  const commands: RenderCommand[] = [
    ...orderedTriangles,
    // Generic mesh-derived linework is experimental and currently hidden by
    // the lab. Keep it above filled surfaces; CAD-derived HLR remains last.
    ...strokeCommands.sort((a, b) => a.depth - b.depth || a.order - b.order),
    ...hlrCommands.sort(
      (a, b) =>
        Number(a.kind === "hlr-outline") - Number(b.kind === "hlr-outline") || a.order - b.order,
    ),
  ];
  return {
    commands,
    stats: { triangles: triangleCount, outlines, details, creases, commands: commands.length },
  };
}

function numberText(value: number): string {
  const rounded = Number(value.toPrecision(12));
  return Object.is(rounded, -0) ? "0" : String(rounded);
}

function escapeXml(value: string): string {
  return value.replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll('"', "&quot;");
}

export function renderMeshIllustrationSvg(
  scene: MeshIllustrationScene,
  style: MeshIllustrationStyle,
  title = "Geometer mesh illustration",
): { svg: string; stats: IllustrationRenderStats } {
  const width = Math.max(scene.bounds.maxX - scene.bounds.minX, 1e-9);
  const height = Math.max(scene.bounds.maxY - scene.bounds.minY, 1e-9);
  const pad = Math.max(width, height) * 0.06;
  const minX = scene.bounds.minX - pad;
  const minY = -scene.bounds.maxY - pad;
  const viewWidth = width + pad * 2;
  const viewHeight = height + pad * 2;
  const seamOverlap = Math.max(width, height) * 0.003;
  const { commands, stats } = renderCommands(scene, style);
  const body: string[] = [];
  if (!style.transparentBackground) {
    body.push(
      `<rect x="${numberText(minX)}" y="${numberText(minY)}" width="${numberText(viewWidth)}" height="${numberText(viewHeight)}" fill="${escapeXml(style.background)}"/>`,
    );
  }
  for (const command of commands) {
    if (command.kind === "triangle") {
      const points = command.triangle.points
        .map((point) => `${numberText(point[0])},${numberText(-point[1])}`)
        .join(" ");
      const opacity =
        command.opacity < 0.999 ? ` fill-opacity="${numberText(command.opacity)}"` : "";
      body.push(
        `<polygon points="${points}" fill="${command.fill}" stroke="${command.fill}" stroke-width="${numberText(seamOverlap)}" stroke-linejoin="round"${opacity}/>`,
      );
    } else {
      const [a, b] = "points" in command ? command.points : command.edge.points;
      body.push(
        `<path d="M ${numberText(a[0])} ${numberText(-a[1])} L ${numberText(b[0])} ${numberText(-b[1])}" fill="none" stroke="${escapeXml(command.color)}" stroke-width="${numberText(command.width)}" stroke-linecap="round" stroke-linejoin="round"/>`,
      );
    }
  }
  const svg = [
    '<?xml version="1.0" encoding="UTF-8"?>',
    `<svg xmlns="http://www.w3.org/2000/svg" viewBox="${numberText(minX)} ${numberText(minY)} ${numberText(viewWidth)} ${numberText(viewHeight)}" role="img">`,
    `<title>${escapeXml(title)}</title>`,
    `<metadata>geometry.mesh_illustration.prototype.a0</metadata>`,
    ...body,
    "</svg>",
    "",
  ].join("\n");
  return { svg, stats };
}

export function renderMeshIllustrationCanvas(
  context: CanvasRenderingContext2D,
  scene: MeshIllustrationScene,
  style: MeshIllustrationStyle,
): IllustrationRenderStats {
  const canvas = context.canvas;
  const width = Math.max(scene.bounds.maxX - scene.bounds.minX, 1e-9);
  const height = Math.max(scene.bounds.maxY - scene.bounds.minY, 1e-9);
  const pad = Math.max(width, height) * 0.06;
  const scale = Math.min(canvas.width / (width + pad * 2), canvas.height / (height + pad * 2));
  const offsetX = (canvas.width - width * scale) / 2 - scene.bounds.minX * scale;
  const offsetY = (canvas.height - height * scale) / 2 + scene.bounds.maxY * scale;
  const { commands, stats } = renderCommands(scene, style);
  context.save();
  context.setTransform(1, 0, 0, 1, 0, 0);
  context.clearRect(0, 0, canvas.width, canvas.height);
  if (!style.transparentBackground) {
    context.fillStyle = style.background;
    context.fillRect(0, 0, canvas.width, canvas.height);
  }
  context.lineCap = "round";
  context.lineJoin = "round";
  for (const command of commands) {
    if (command.kind === "triangle") {
      const [a, b, c] = command.triangle.points;
      context.beginPath();
      context.moveTo(offsetX + a[0] * scale, offsetY - a[1] * scale);
      context.lineTo(offsetX + b[0] * scale, offsetY - b[1] * scale);
      context.lineTo(offsetX + c[0] * scale, offsetY - c[1] * scale);
      context.closePath();
      context.globalAlpha = command.opacity;
      context.fillStyle = command.fill;
      context.fill();
      context.globalAlpha = command.opacity;
      context.strokeStyle = command.fill;
      context.lineWidth = Math.max(0.7, Math.min(1.5, scale * 0.0008));
      context.stroke();
    } else {
      const [a, b] = "points" in command ? command.points : command.edge.points;
      context.beginPath();
      context.moveTo(offsetX + a[0] * scale, offsetY - a[1] * scale);
      context.lineTo(offsetX + b[0] * scale, offsetY - b[1] * scale);
      context.globalAlpha = 1;
      context.strokeStyle = command.color;
      context.lineWidth = Math.max(0.6, command.width * scale);
      context.stroke();
    }
  }
  context.restore();
  return stats;
}
