const EXPERIMENTAL_AO_SYMBOL = Symbol.for("@wavenumber/geometer/experimental-mesh-ambient-occlusion");
const EXPERIMENTAL_AO_REVISION_SYMBOL = Symbol.for("@wavenumber/geometer/experimental-mesh-ambient-occlusion-revision");
const IDENTITY_MATRIX = Object.freeze([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
const VISIBILITY_ORDER_CACHE = new WeakMap();
const RENDER_COMMAND_CACHE = new WeakMap();
function finite(value, fallback = 0) {
    return Number.isFinite(value) ? value : fallback;
}
function clamp(value, minimum = 0, maximum = 1) {
    return Math.min(maximum, Math.max(minimum, finite(value)));
}
function add(a, b) {
    return [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
}
function subtract(a, b) {
    return [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
}
function dot(a, b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
function cross(a, b) {
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
}
function length(value) {
    return Math.hypot(value[0], value[1], value[2]);
}
function normalize(value, label) {
    const magnitude = length(value);
    if (!Number.isFinite(magnitude) || magnitude < 1e-12)
        throw new Error(`${label} must be a finite non-zero vector.`);
    return [value[0] / magnitude, value[1] / magnitude, value[2] / magnitude];
}
function matrixValue(matrix, index) {
    return finite(Number(matrix[index]), IDENTITY_MATRIX[index] ?? 0);
}
function transformPoint(matrix, point) {
    const x = point[0];
    const y = point[1];
    const z = point[2];
    const w = matrixValue(matrix, 3) * x +
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
function matrixDeterminantSign(matrix) {
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
function transformNormal(matrix, normal) {
    const a = matrixValue(matrix, 0);
    const b = matrixValue(matrix, 4);
    const c = matrixValue(matrix, 8);
    const d = matrixValue(matrix, 1);
    const e = matrixValue(matrix, 5);
    const f = matrixValue(matrix, 9);
    const g = matrixValue(matrix, 2);
    const h = matrixValue(matrix, 6);
    const i = matrixValue(matrix, 10);
    const transformed = [
        (e * i - f * h) * normal[0] + (f * g - d * i) * normal[1] + (d * h - e * g) * normal[2],
        (c * h - b * i) * normal[0] + (a * i - c * g) * normal[1] + (b * g - a * h) * normal[2],
        (b * f - c * e) * normal[0] + (c * d - a * f) * normal[1] + (a * e - b * d) * normal[2],
    ];
    const orientation = matrixDeterminantSign(matrix);
    return normalize([transformed[0] * orientation, transformed[1] * orientation, transformed[2] * orientation], "Transformed normal");
}
function positionAt(positions, index) {
    const offset = index * 3;
    return [
        finite(Number(positions[offset])),
        finite(Number(positions[offset + 1])),
        finite(Number(positions[offset + 2])),
    ];
}
function averagedSourceNormal(normals, indices, matrix, fallback) {
    if (!normals || normals.length === 0)
        return fallback;
    const average = indices.reduce((sum, index) => add(sum, positionAt(normals, index)), [0, 0, 0]);
    if (length(average) < 1e-12)
        return fallback;
    return transformNormal(matrix, normalize(average, "Source normal"));
}
function indexAt(indices, index) {
    return indices ? Math.trunc(finite(Number(indices[index]))) : index;
}
function projectPoint(point, right, up, direction, mirrorX) {
    return {
        point: [(mirrorX ? -1 : 1) * dot(point, right), dot(point, up)],
        depth: dot(point, direction),
    };
}
function quantizedPointKey(point, tolerance) {
    const scale = 1 / tolerance;
    return `${Math.round(point[0] * scale)},${Math.round(point[1] * scale)},${Math.round(point[2] * scale)}`;
}
function edgeKey(a, b, tolerance) {
    const ka = quantizedPointKey(a, tolerance);
    const kb = quantizedPointKey(b, tolerance);
    return ka < kb ? `${ka}|${kb}` : `${kb}|${ka}`;
}
function materialForTriangle(mesh, triangleIndex) {
    const requested = Math.trunc(Number(mesh.triangleMaterialIndices?.[triangleIndex] ?? 0));
    return mesh.materials[requested] ?? mesh.materials[0] ?? { color: [0.72, 0.74, 0.78] };
}
function validateMesh(mesh) {
    if (!mesh.id)
        throw new Error("Every mesh needs a non-empty id.");
    if (mesh.positions.length % 3 !== 0)
        throw new Error(`Mesh ${mesh.id} position length must be divisible by 3.`);
    const elementCount = mesh.indices?.length ?? mesh.positions.length / 3;
    if (elementCount % 3 !== 0)
        throw new Error(`Mesh ${mesh.id} index/vertex element count must be divisible by 3.`);
    if (mesh.matrix && mesh.matrix.length !== 16)
        throw new Error(`Mesh ${mesh.id} matrix must contain 16 column-major values.`);
    return elementCount / 3;
}
export function prepareMeshIllustration(input, view, options = {}) {
    const direction = normalize(view.direction, "View direction");
    const requestedUp = normalize(view.up, "View up");
    const right = normalize(cross(requestedUp, direction), "View right");
    const up = normalize(cross(direction, right), "Orthogonal view up");
    const mirrorX = view.mirrorX === true;
    const maxTriangles = options.maxTriangles === undefined
        ? Number.POSITIVE_INFINITY
        : Math.max(1, Math.trunc(finite(options.maxTriangles, 1)));
    const weldTolerance = Math.max(1e-9, finite(options.weldTolerance ?? 1e-6, 1e-6));
    const triangles = [];
    const edgeMaps = new Map();
    const warnings = [];
    let suppressedWarnings = 0;
    const addWarning = (warning) => {
        if (warnings.length < 255)
            warnings.push(warning);
        else
            suppressedWarnings += 1;
    };
    let sourceTriangles = 0;
    let minX = Infinity;
    let minY = Infinity;
    let maxX = -Infinity;
    let maxY = -Infinity;
    for (const mesh of input.meshes) {
        const triangleCount = validateMesh(mesh);
        sourceTriangles += triangleCount;
        if (sourceTriangles > maxTriangles)
            throw new Error(`Mesh illustration exceeds the configured ${maxTriangles.toLocaleString()} triangle limit.`);
        const matrix = mesh.matrix ?? IDENTITY_MATRIX;
        const orientation = matrixDeterminantSign(matrix);
        for (let triangleIndex = 0; triangleIndex < triangleCount; triangleIndex += 1) {
            const indices = [
                indexAt(mesh.indices, triangleIndex * 3),
                indexAt(mesh.indices, triangleIndex * 3 + 1),
                indexAt(mesh.indices, triangleIndex * 3 + 2),
            ];
            const vertexCount = mesh.positions.length / 3;
            if (indices.some((index) => index < 0 || index >= vertexCount)) {
                addWarning(`Skipped out-of-range triangle ${triangleIndex} in ${mesh.id}.`);
                continue;
            }
            const world = indices.map((index) => transformPoint(matrix, positionAt(mesh.positions, index)));
            const crossNormal = cross(subtract(world[1], world[0]), subtract(world[2], world[0]));
            const magnitude = length(crossNormal);
            if (magnitude < 1e-12) {
                addWarning(`Skipped degenerate triangle ${triangleIndex} in ${mesh.id}.`);
                continue;
            }
            const normal = normalize([crossNormal[0] * orientation, crossNormal[1] * orientation, crossNormal[2] * orientation], "Triangle normal");
            const shadingNormal = averagedSourceNormal(mesh.normals, indices, matrix, normal);
            const frontFacing = dot(normal, direction) > 1e-9;
            const projected = world.map((point) => projectPoint(point, right, up, direction, mirrorX));
            for (const item of projected) {
                minX = Math.min(minX, item.point[0]);
                minY = Math.min(minY, item.point[1]);
                maxX = Math.max(maxX, item.point[0]);
                maxY = Math.max(maxY, item.point[1]);
            }
            const material = materialForTriangle(mesh, triangleIndex);
            const triangle = {
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
            ]) {
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
                }
                else if (existing.normalB === null) {
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
    if (suppressedWarnings > 0)
        warnings.push(`${String(suppressedWarnings)} additional warnings suppressed.`);
    return {
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
function srgbToLinear(value) {
    const v = clamp(value);
    return v <= 0.04045 ? v / 12.92 : ((v + 0.055) / 1.055) ** 2.4;
}
function linearToSrgb(value) {
    const v = clamp(value);
    return v <= 0.0031308 ? v * 12.92 : 1.055 * v ** (1 / 2.4) - 0.055;
}
function rgbCss(color, intensity, rim, darkLift = 0) {
    const channels = color.map((value) => {
        const lit = srgbToLinear(value) * Math.max(0, intensity) + clamp(darkLift, 0, 0.08);
        const withRim = lit + (1 - lit) * clamp(rim);
        return Math.round(clamp(linearToSrgb(withRim)) * 255);
    });
    return `rgb(${channels[0]},${channels[1]},${channels[2]})`;
}
function triangleFill(triangle, scene, style) {
    const base = style.sourceColors ? triangle.baseColor : style.fallbackColor;
    if (style.shading === "unlit")
        return rgbCss(base, 1, 0);
    const light = normalize(style.lightDirection, "Light direction");
    const activeNormal = style.shading === "flat" ? triangle.geometricNormal : triangle.normal;
    const diffuse = Math.max(0, dot(activeNormal, light));
    const experimentalStyle = style;
    const accessibility = Number(triangle[EXPERIMENTAL_AO_SYMBOL] ?? 1);
    const aoBands = Math.max(2, Math.min(32, Math.trunc(experimentalStyle.experimentalAmbientOcclusionBands ?? 5)));
    const quantizedAccessibility = Math.round(clamp(accessibility) * (aoBands - 1)) / (aoBands - 1);
    const aoStrength = clamp(experimentalStyle.experimentalAmbientOcclusionStrength ?? 0);
    const ambientAccessibility = 1 - aoStrength * (1 - quantizedAccessibility);
    let intensity = clamp(style.ambient) * ambientAccessibility + clamp(style.keyIntensity, 0, 4) * diffuse;
    intensity = clamp(intensity);
    if (style.shading === "banded" || style.shading === "toon") {
        const bands = Math.max(2, Math.min(32, Math.trunc(style.bands)));
        intensity = Math.round(intensity * (bands - 1)) / (bands - 1);
    }
    const rim = style.shading === "toon"
        ? clamp(style.rimAmount) *
            clamp((1 - Math.abs(dot(activeNormal, scene.view.direction)) - 0.45) / 0.55)
        : 0;
    return rgbCss(base, intensity, rim, style.shading === "toon" ? 0.012 : 0);
}
function edgeKind(edge, style) {
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
        if (dot(edge.normalA, edge.normalB) < threshold)
            return "crease";
    }
    return null;
}
function projectedBounds(triangle) {
    return {
        minX: Math.min(...triangle.points.map((point) => point[0])),
        minY: Math.min(...triangle.points.map((point) => point[1])),
        maxX: Math.max(...triangle.points.map((point) => point[0])),
        maxY: Math.max(...triangle.points.map((point) => point[1])),
    };
}
function boundsOverlap(a, b, epsilon) {
    return !(a.maxX <= b.minX + epsilon ||
        b.maxX <= a.minX + epsilon ||
        a.maxY <= b.minY + epsilon ||
        b.maxY <= a.minY + epsilon);
}
function cross2(a, b, point) {
    return (b[0] - a[0]) * (point[1] - a[1]) - (b[1] - a[1]) * (point[0] - a[0]);
}
function polygonSignedArea(points) {
    let twiceArea = 0;
    for (let index = 0; index < points.length; index += 1) {
        const current = points[index];
        const next = points[(index + 1) % points.length];
        twiceArea += current[0] * next[1] - next[0] * current[1];
    }
    return twiceArea * 0.5;
}
function clipConvexPolygon(subject, clip, epsilon) {
    let output = [...subject];
    const orientation = polygonSignedArea(clip) >= 0 ? 1 : -1;
    for (let edgeIndex = 0; edgeIndex < clip.length && output.length > 0; edgeIndex += 1) {
        const edgeStart = clip[edgeIndex];
        const edgeEnd = clip[(edgeIndex + 1) % clip.length];
        const input = output;
        output = [];
        let previous = input[input.length - 1];
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
            if (currentInside)
                output.push(current);
            previous = current;
            previousDistance = currentDistance;
        }
    }
    return output;
}
function depthAt(triangle, point) {
    const [a, b, c] = triangle.points;
    const denominator = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1]);
    if (Math.abs(denominator) < 1e-20)
        return triangle.depth;
    const weightA = ((b[1] - c[1]) * (point[0] - c[0]) + (c[0] - b[0]) * (point[1] - c[1])) / denominator;
    const weightB = ((c[1] - a[1]) * (point[0] - c[0]) + (a[0] - c[0]) * (point[1] - c[1])) / denominator;
    return (weightA * triangle.depths[0] +
        weightB * triangle.depths[1] +
        (1 - weightA - weightB) * triangle.depths[2]);
}
function significantTriangleOverlap(overlap, a, b, coordinateEpsilon) {
    if (overlap.length < 3)
        return false;
    const boundsA = projectedBounds(a);
    const boundsB = projectedBounds(b);
    const scale = Math.max(boundsA.maxX - boundsA.minX, boundsA.maxY - boundsA.minY, boundsB.maxX - boundsB.minX, boundsB.maxY - boundsB.minY, coordinateEpsilon);
    // Clipping two triangles that merely share an edge can produce a
    // floating-point sliver. Compare area with a coordinate-tolerance strip,
    // rather than epsilon squared, so adjacency does not become a false depth
    // blocker in oblique projections.
    return Math.abs(polygonSignedArea(overlap)) > coordinateEpsilon * scale * 4;
}
function overlapDepthOrder(a, b, coordinateEpsilon) {
    const overlap = clipConvexPolygon(a.points, b.points, coordinateEpsilon);
    if (!significantTriangleOverlap(overlap, a, b, coordinateEpsilon))
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
    if (minimum >= -depthEpsilon && maximum > depthEpsilon)
        return 1;
    if (maximum <= depthEpsilon && minimum < -depthEpsilon)
        return -1;
    return 0;
}
function heapPush(heap, value, compare) {
    heap.push(value);
    let index = heap.length - 1;
    while (index > 0) {
        const parent = Math.floor((index - 1) / 2);
        const parentValue = heap[parent];
        if (compare(parentValue, value) <= 0)
            break;
        heap[index] = parentValue;
        index = parent;
    }
    heap[index] = value;
}
function heapPop(heap, compare) {
    const first = heap[0];
    const last = heap.pop();
    if (first === undefined || last === undefined || heap.length === 0)
        return first;
    let index = 0;
    while (true) {
        const left = index * 2 + 1;
        if (left >= heap.length)
            break;
        const right = left + 1;
        let child = left;
        const leftValue = heap[left];
        if (right < heap.length && compare(heap[right], leftValue) < 0)
            child = right;
        const childValue = heap[child];
        if (compare(childValue, last) >= 0)
            break;
        heap[index] = childValue;
        index = child;
    }
    heap[index] = last;
    return first;
}
function triangleCommandOrder(a, b) {
    return a.depth - b.depth || a.order - b.order;
}
function stronglyConnectedComponents(outgoing, reverse) {
    const visited = new Uint8Array(outgoing.length);
    const finishOrder = [];
    for (let start = 0; start < outgoing.length; start += 1) {
        if (visited[start] === 1)
            continue;
        const nodes = [start];
        const offsets = [0];
        visited[start] = 1;
        while (nodes.length > 0) {
            const stackIndex = nodes.length - 1;
            const node = nodes[stackIndex];
            const offset = offsets[stackIndex];
            const neighbors = outgoing[node];
            if (offset < neighbors.length) {
                const next = neighbors[offset];
                offsets[stackIndex] = offset + 1;
                if (visited[next] === 0) {
                    visited[next] = 1;
                    nodes.push(next);
                    offsets.push(0);
                }
            }
            else {
                finishOrder.push(node);
                nodes.pop();
                offsets.pop();
            }
        }
    }
    const componentOf = new Int32Array(outgoing.length);
    componentOf.fill(-1);
    const components = [];
    for (let orderIndex = finishOrder.length - 1; orderIndex >= 0; orderIndex -= 1) {
        const start = finishOrder[orderIndex];
        if (componentOf[start] !== -1)
            continue;
        const componentIndex = components.length;
        const members = [];
        const stack = [start];
        componentOf[start] = componentIndex;
        while (stack.length > 0) {
            const node = stack.pop();
            members.push(node);
            for (const next of reverse[node]) {
                if (componentOf[next] !== -1)
                    continue;
                componentOf[next] = componentIndex;
                stack.push(next);
            }
        }
        components.push(members);
    }
    return { components, componentOf };
}
function orderTriangleCommands(commands, bounds) {
    if (commands.length < 2)
        return [...commands];
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
    const buckets = new Map();
    const broadTriangles = [];
    const previousTriangles = [];
    const marks = new Int32Array(commands.length);
    const outgoing = Array.from({ length: commands.length }, () => []);
    const reverse = Array.from({ length: commands.length }, () => []);
    const cellRange = (box) => [
        Math.max(0, Math.min(gridSize - 1, Math.floor((box.minX - bounds.minX) / cellWidth))),
        Math.max(0, Math.min(gridSize - 1, Math.floor((box.maxX - bounds.minX) / cellWidth))),
        Math.max(0, Math.min(gridSize - 1, Math.floor((box.minY - bounds.minY) / cellHeight))),
        Math.max(0, Math.min(gridSize - 1, Math.floor((box.maxY - bounds.minY) / cellHeight))),
    ];
    const addConstraint = (index, candidate) => {
        const box = boxes[index];
        const candidateBox = boxes[candidate];
        if (!boundsOverlap(box, candidateBox, coordinateEpsilon))
            return;
        const order = overlapDepthOrder(commands[index].triangle, commands[candidate].triangle, coordinateEpsilon);
        if (order === 0)
            return;
        const behind = order > 0 ? candidate : index;
        const front = order > 0 ? index : candidate;
        outgoing[behind].push(front);
        reverse[front].push(behind);
    };
    for (let index = 0; index < commands.length; index += 1) {
        const box = boxes[index];
        const [minCellX, maxCellX, minCellY, maxCellY] = cellRange(box);
        const coveredCells = (maxCellX - minCellX + 1) * (maxCellY - minCellY + 1);
        const broad = coveredCells > Math.max(64, gridSize * 2);
        const stamp = index + 1;
        if (broad) {
            for (const candidate of previousTriangles)
                addConstraint(index, candidate);
        }
        else {
            for (const candidate of broadTriangles)
                addConstraint(index, candidate);
            for (let cellY = minCellY; cellY <= maxCellY; cellY += 1) {
                for (let cellX = minCellX; cellX <= maxCellX; cellX += 1) {
                    const bucket = buckets.get(cellY * gridSize + cellX);
                    if (!bucket)
                        continue;
                    for (const candidate of bucket) {
                        if (marks[candidate] === stamp)
                            continue;
                        marks[candidate] = stamp;
                        addConstraint(index, candidate);
                    }
                }
            }
            for (let cellY = minCellY; cellY <= maxCellY; cellY += 1) {
                for (let cellX = minCellX; cellX <= maxCellX; cellX += 1) {
                    const key = cellY * gridSize + cellX;
                    const bucket = buckets.get(key);
                    if (bucket)
                        bucket.push(index);
                    else
                        buckets.set(key, [index]);
                }
            }
        }
        if (broad)
            broadTriangles.push(index);
        previousTriangles.push(index);
    }
    const { components, componentOf } = stronglyConnectedComponents(outgoing, reverse);
    const componentOutgoing = components.map(() => new Set());
    const componentIncoming = new Uint32Array(components.length);
    for (let behind = 0; behind < outgoing.length; behind += 1) {
        const behindComponent = componentOf[behind];
        for (const front of outgoing[behind]) {
            const frontComponent = componentOf[front];
            if (behindComponent === frontComponent)
                continue;
            const targets = componentOutgoing[behindComponent];
            if (targets.has(frontComponent))
                continue;
            targets.add(frontComponent);
            componentIncoming[frontComponent] = (componentIncoming[frontComponent] ?? 0) + 1;
        }
    }
    const componentDepths = components.map((members) => members.reduce((sum, member) => sum + commands[member].depth, 0) /
        members.length);
    const componentOrders = components.map((members) => members.reduce((minimum, member) => Math.min(minimum, commands[member].order), Infinity));
    const compareComponents = (a, b) => componentDepths[a] - componentDepths[b] ||
        componentOrders[a] - componentOrders[b];
    const ready = [];
    for (let component = 0; component < components.length; component += 1) {
        if (componentIncoming[component] === 0)
            heapPush(ready, component, compareComponents);
    }
    const ordered = [];
    while (ready.length > 0) {
        const component = heapPop(ready, compareComponents);
        const members = [...components[component]].sort((a, b) => triangleCommandOrder(commands[a], commands[b]));
        for (const member of members)
            ordered.push(commands[member]);
        for (const front of componentOutgoing[component]) {
            const count = componentIncoming[front] ?? 0;
            if (count > 0)
                componentIncoming[front] = count - 1;
            if ((componentIncoming[front] ?? 0) === 0)
                heapPush(ready, front, compareComponents);
        }
    }
    return ordered;
}
function pointFusionKey(point, depth, coordinateTolerance, depthTolerance) {
    return `${Math.round(point[0] / coordinateTolerance)},${Math.round(point[1] / coordinateTolerance)},${Math.round(depth / depthTolerance)}`;
}
function orientedFusionVertices(command) {
    const { points, depths } = command.triangle;
    return polygonSignedArea(points) >= 0
        ? [
            { point: points[0], depth: depths[0] },
            { point: points[1], depth: depths[1] },
            { point: points[2], depth: depths[2] },
        ]
        : [
            { point: points[0], depth: depths[0] },
            { point: points[2], depth: depths[2] },
            { point: points[1], depth: depths[1] },
        ];
}
function segmentsIntersect(a, b, c, d, epsilon) {
    const abC = cross2(a, b, c);
    const abD = cross2(a, b, d);
    const cdA = cross2(c, d, a);
    const cdB = cross2(c, d, b);
    if (((abC > epsilon && abD < -epsilon) || (abC < -epsilon && abD > epsilon)) &&
        ((cdA > epsilon && cdB < -epsilon) || (cdA < -epsilon && cdB > epsilon)))
        return true;
    const within = (value, first, second) => value >= Math.min(first, second) - epsilon && value <= Math.max(first, second) + epsilon;
    const onSegment = (point, first, second, area) => Math.abs(area) <= epsilon &&
        within(point[0], first[0], second[0]) &&
        within(point[1], first[1], second[1]);
    return (onSegment(c, a, b, abC) ||
        onSegment(d, a, b, abD) ||
        onSegment(a, c, d, cdA) ||
        onSegment(b, c, d, cdB));
}
function validFusionRings(rings, epsilon) {
    const segments = [];
    for (let ring = 0; ring < rings.length; ring += 1) {
        const points = rings[ring];
        if (points.length < 3 || Math.abs(polygonSignedArea(points)) <= epsilon ** 2)
            return false;
        for (let edge = 0; edge < points.length; edge += 1) {
            segments.push({
                a: points[edge],
                b: points[(edge + 1) % points.length],
                ring,
                edge,
                count: points.length,
            });
        }
    }
    const intersects = (first, second) => {
        const a = segments[first];
        const b = segments[second];
        if (a.ring === b.ring &&
            (a.edge === b.edge || (a.edge + 1) % a.count === b.edge || (b.edge + 1) % b.count === a.edge))
            return false;
        return segmentsIntersect(a.a, a.b, b.a, b.b, epsilon);
    };
    let minX = Infinity;
    let minY = Infinity;
    let maxX = -Infinity;
    let maxY = -Infinity;
    for (const segment of segments) {
        minX = Math.min(minX, segment.a[0], segment.b[0]);
        minY = Math.min(minY, segment.a[1], segment.b[1]);
        maxX = Math.max(maxX, segment.a[0], segment.b[0]);
        maxY = Math.max(maxY, segment.a[1], segment.b[1]);
    }
    const gridSize = Math.max(4, Math.min(256, Math.ceil(Math.sqrt(segments.length / 2))));
    const cellWidth = Math.max((maxX - minX) / gridSize, epsilon);
    const cellHeight = Math.max((maxY - minY) / gridSize, epsilon);
    const buckets = new Map();
    const broadSegments = [];
    const previousSegments = [];
    const marks = new Int32Array(segments.length);
    for (let index = 0; index < segments.length; index += 1) {
        const segment = segments[index];
        const minCellX = Math.max(0, Math.min(gridSize - 1, Math.floor((Math.min(segment.a[0], segment.b[0]) - minX) / cellWidth)));
        const maxCellX = Math.max(0, Math.min(gridSize - 1, Math.floor((Math.max(segment.a[0], segment.b[0]) - minX) / cellWidth)));
        const minCellY = Math.max(0, Math.min(gridSize - 1, Math.floor((Math.min(segment.a[1], segment.b[1]) - minY) / cellHeight)));
        const maxCellY = Math.max(0, Math.min(gridSize - 1, Math.floor((Math.max(segment.a[1], segment.b[1]) - minY) / cellHeight)));
        const broad = (maxCellX - minCellX + 1) * (maxCellY - minCellY + 1) > Math.max(32, gridSize);
        const stamp = index + 1;
        if (broad) {
            for (const candidate of previousSegments)
                if (intersects(index, candidate))
                    return false;
        }
        else {
            for (const candidate of broadSegments)
                if (intersects(index, candidate))
                    return false;
            for (let cellY = minCellY; cellY <= maxCellY; cellY += 1) {
                for (let cellX = minCellX; cellX <= maxCellX; cellX += 1) {
                    for (const candidate of buckets.get(cellY * gridSize + cellX) ?? []) {
                        if (marks[candidate] === stamp)
                            continue;
                        marks[candidate] = stamp;
                        if (intersects(index, candidate))
                            return false;
                    }
                }
            }
            for (let cellY = minCellY; cellY <= maxCellY; cellY += 1) {
                for (let cellX = minCellX; cellX <= maxCellX; cellX += 1) {
                    const key = cellY * gridSize + cellX;
                    const bucket = buckets.get(key);
                    if (bucket)
                        bucket.push(index);
                    else
                        buckets.set(key, [index]);
                }
            }
        }
        if (broad)
            broadSegments.push(index);
        previousSegments.push(index);
    }
    return true;
}
function simplifyFusionRing(points, tolerance) {
    const simplified = [...points];
    let changed = true;
    while (changed && simplified.length > 3) {
        changed = false;
        for (let index = 0; index < simplified.length; index += 1) {
            const previous = simplified[(index + simplified.length - 1) % simplified.length];
            const current = simplified[index];
            const next = simplified[(index + 1) % simplified.length];
            const baseline = Math.hypot(next[0] - previous[0], next[1] - previous[1]);
            if (baseline <= tolerance ||
                Math.abs(cross2(previous, next, current)) / baseline <= tolerance) {
                simplified.splice(index, 1);
                changed = true;
                break;
            }
        }
    }
    return simplified;
}
function fusedComponent(commands, members, coordinateTolerance, depthTolerance) {
    const edgeMap = new Map();
    for (const member of members) {
        const vertices = orientedFusionVertices(commands[member]);
        for (let edgeIndex = 0; edgeIndex < 3; edgeIndex += 1) {
            const startVertex = vertices[edgeIndex];
            const endVertex = vertices[(edgeIndex + 1) % 3];
            const thirdVertex = vertices[(edgeIndex + 2) % 3];
            const start = startVertex.point;
            const end = endVertex.point;
            const third = thirdVertex.point;
            const startKey = pointFusionKey(start, startVertex.depth, coordinateTolerance, depthTolerance);
            const endKey = pointFusionKey(end, endVertex.depth, coordinateTolerance, depthTolerance);
            const key = startKey < endKey ? `${startKey}|${endKey}` : `${endKey}|${startKey}`;
            const edge = { triangle: member, start, end, third, startKey, endKey, key };
            const entries = edgeMap.get(key);
            if (entries)
                entries.push(edge);
            else
                edgeMap.set(key, [edge]);
        }
    }
    const boundary = [];
    for (const entries of edgeMap.values()) {
        if (entries.length === 1) {
            boundary.push(entries[0]);
            continue;
        }
        if (entries.length !== 2 ||
            entries[0]?.startKey !== entries[1]?.endKey ||
            entries[0]?.endKey !== entries[1]?.startKey)
            return null;
    }
    if (boundary.length < 3)
        return null;
    const outgoing = new Map();
    const incoming = new Map();
    for (const edge of boundary) {
        if (outgoing.has(edge.startKey))
            return null;
        outgoing.set(edge.startKey, edge);
        incoming.set(edge.endKey, (incoming.get(edge.endKey) ?? 0) + 1);
    }
    if ([...outgoing.keys()].some((key) => (incoming.get(key) ?? 0) !== 1) ||
        [...incoming.keys()].some((key) => !outgoing.has(key)))
        return null;
    const edgeId = (edge) => `${edge.key}|${edge.startKey}`;
    const unused = new Set(boundary.map(edgeId));
    const rings = [];
    for (const first of boundary) {
        if (!unused.has(edgeId(first)))
            continue;
        const ring = [];
        let current = first;
        for (let step = 0; step <= boundary.length; step += 1) {
            if (!current || !unused.delete(edgeId(current)))
                return null;
            ring.push(current.start);
            if (current.endKey === first.startKey)
                break;
            current = outgoing.get(current.endKey);
            if (step === boundary.length)
                return null;
        }
        rings.push(ring);
    }
    const simplifiedRings = rings.map((ring) => simplifyFusionRing(ring, coordinateTolerance));
    if (unused.size > 0 || !validFusionRings(simplifiedRings, coordinateTolerance))
        return null;
    const first = commands[members[0]];
    return {
        kind: "fused-surface",
        depth: first.depth,
        order: first.order,
        rings: simplifiedRings,
        fill: first.fill,
        opacity: first.opacity,
        triangleCount: members.length,
    };
}
function projectedTrianglesOverlap(a, b, epsilon) {
    const overlap = clipConvexPolygon(a.points, b.points, epsilon);
    return significantTriangleOverlap(overlap, a, b, epsilon);
}
function fusionMobilityIntervals(commands, bounds) {
    const low = new Int32Array(commands.length);
    const high = new Int32Array(commands.length);
    const sameStyleOverlaps = [];
    for (let index = 0; index < commands.length; index += 1)
        high[index] = commands.length - 1;
    if (commands.length < 2)
        return { low, high, sameStyleOverlaps };
    const width = Math.max(bounds.maxX - bounds.minX, 1e-12);
    const height = Math.max(bounds.maxY - bounds.minY, 1e-12);
    const epsilon = Math.max(width, height) * 1e-10;
    const gridSize = Math.max(8, Math.min(192, Math.ceil(Math.sqrt(commands.length / 4))));
    const cellWidth = width / gridSize;
    const cellHeight = height / gridSize;
    const boxes = commands.map((command) => projectedBounds(command.triangle));
    const buckets = new Map();
    const broadTriangles = [];
    const previousTriangles = [];
    const marks = new Int32Array(commands.length);
    const cellRange = (box) => [
        Math.max(0, Math.min(gridSize - 1, Math.floor((box.minX - bounds.minX) / cellWidth))),
        Math.max(0, Math.min(gridSize - 1, Math.floor((box.maxX - bounds.minX) / cellWidth))),
        Math.max(0, Math.min(gridSize - 1, Math.floor((box.minY - bounds.minY) / cellHeight))),
        Math.max(0, Math.min(gridSize - 1, Math.floor((box.maxY - bounds.minY) / cellHeight))),
    ];
    const addBlocker = (front, back) => {
        const frontCommand = commands[front];
        const backCommand = commands[back];
        if (!boundsOverlap(boxes[front], boxes[back], epsilon) ||
            !projectedTrianglesOverlap(frontCommand.triangle, backCommand.triangle, epsilon))
            return;
        if (frontCommand.fill === backCommand.fill &&
            Math.abs(frontCommand.opacity - backCommand.opacity) <= 1e-12) {
            sameStyleOverlaps.push([front, back]);
            return;
        }
        low[front] = Math.max(low[front], back + 1);
        high[back] = Math.min(high[back], front - 1);
    };
    for (let index = 0; index < commands.length; index += 1) {
        const box = boxes[index];
        const [minCellX, maxCellX, minCellY, maxCellY] = cellRange(box);
        const coveredCells = (maxCellX - minCellX + 1) * (maxCellY - minCellY + 1);
        const broad = coveredCells > Math.max(64, gridSize * 2);
        const stamp = index + 1;
        if (broad) {
            for (const candidate of previousTriangles)
                addBlocker(index, candidate);
        }
        else {
            for (const candidate of broadTriangles)
                addBlocker(index, candidate);
            for (let cellY = minCellY; cellY <= maxCellY; cellY += 1) {
                for (let cellX = minCellX; cellX <= maxCellX; cellX += 1) {
                    const bucket = buckets.get(cellY * gridSize + cellX);
                    if (!bucket)
                        continue;
                    for (const candidate of bucket) {
                        if (marks[candidate] === stamp)
                            continue;
                        marks[candidate] = stamp;
                        addBlocker(index, candidate);
                    }
                }
            }
            for (let cellY = minCellY; cellY <= maxCellY; cellY += 1) {
                for (let cellX = minCellX; cellX <= maxCellX; cellX += 1) {
                    const key = cellY * gridSize + cellX;
                    const bucket = buckets.get(key);
                    if (bucket)
                        bucket.push(index);
                    else
                        buckets.set(key, [index]);
                }
            }
        }
        if (broad)
            broadTriangles.push(index);
        previousTriangles.push(index);
    }
    return { low, high, sameStyleOverlaps };
}
function surfaceStyleKey(command) {
    return `${command.fill}\u0000${command.opacity.toFixed(12)}`;
}
function layeredSurfaceCommand(commands, members, sameStyleAdjacency, coordinateTolerance, depthTolerance) {
    const referenceTriangle = commands[members[0]].triangle;
    const referenceNormal = referenceTriangle.geometricNormal;
    const planeTolerance = Math.max(coordinateTolerance, depthTolerance) * 8;
    if (members.some((member) => {
        const triangle = commands[member].triangle;
        return (dot(triangle.geometricNormal, referenceNormal) < 1 - 1e-10 ||
            triangle.points.some((point, index) => Math.abs(triangle.depths[index] - depthAt(referenceTriangle, point)) >
                planeTolerance));
    }))
        return null;
    const sourceMaterials = new Set(members.map((member) => {
        const triangle = commands[member].triangle;
        return `${triangle.baseColor.map((value) => value.toFixed(12)).join(",")}|${triangle.opacity.toFixed(12)}`;
    }));
    if (sourceMaterials.size < 2)
        return null;
    const styleAreas = new Map();
    for (const member of members) {
        const command = commands[member];
        const key = surfaceStyleKey(command);
        const area = Math.abs(polygonSignedArea(command.triangle.points));
        const entry = styleAreas.get(key);
        if (entry)
            entry.area += area;
        else
            styleAreas.set(key, { area, first: member, command });
    }
    if (styleAreas.size < 2)
        return null;
    const footprint = fusedComponent(commands, members, coordinateTolerance, depthTolerance);
    if (!footprint)
        return null;
    const base = [...styleAreas.entries()].sort((a, b) => b[1].area - a[1].area || a[1].first - b[1].first)[0];
    if (!base)
        return null;
    const [baseKey, baseStyle] = base;
    const localIndex = new Map(members.map((member, index) => [member, index]));
    const parent = Int32Array.from(members, (_member, index) => index);
    const find = (value) => {
        let root = value;
        while (parent[root] !== root)
            root = parent[root];
        while (parent[value] !== value) {
            const next = parent[value];
            parent[value] = root;
            value = next;
        }
        return root;
    };
    for (let index = 0; index < members.length; index += 1) {
        const member = members[index];
        for (const neighbor of sameStyleAdjacency[member] ?? []) {
            const neighborIndex = localIndex.get(neighbor);
            if (neighborIndex === undefined || neighborIndex <= index)
                continue;
            const rootA = find(index);
            const rootB = find(neighborIndex);
            if (rootA !== rootB)
                parent[rootB] = rootA;
        }
    }
    const components = new Map();
    for (let index = 0; index < members.length; index += 1) {
        const member = members[index];
        const root = find(index);
        const component = components.get(root);
        if (component)
            component.push(member);
        else
            components.set(root, [member]);
    }
    const layers = [
        {
            rings: footprint.rings,
            fill: baseStyle.command.fill,
            opacity: baseStyle.command.opacity,
            triangleCount: members.length,
        },
    ];
    for (const component of [...components.values()].sort((a, b) => a[0] - b[0])) {
        const first = commands[component[0]];
        if (surfaceStyleKey(first) === baseKey)
            continue;
        if (component.length === 1) {
            layers.push({
                rings: [orientedFusionVertices(first).map((vertex) => vertex.point)],
                fill: first.fill,
                opacity: first.opacity,
                triangleCount: 1,
            });
            continue;
        }
        const fused = fusedComponent(commands, component, coordinateTolerance, depthTolerance);
        if (!fused)
            return null;
        layers.push({
            rings: fused.rings,
            fill: fused.fill,
            opacity: fused.opacity,
            triangleCount: fused.triangleCount,
        });
    }
    if (layers.length < 2)
        return null;
    return {
        kind: "layered-surface",
        depth: members.reduce((sum, member) => sum + commands[member].depth, 0) /
            members.length,
        order: members.reduce((minimum, member) => Math.min(minimum, commands[member].order), Infinity),
        layers,
        triangleCount: members.length,
    };
}
function fuseTriangleCommands(commands, bounds, layerCoplanarMaterials) {
    if (commands.length < 2)
        return [...commands];
    const span = Math.max(bounds.maxX - bounds.minX, bounds.maxY - bounds.minY, 1e-9);
    const coordinateTolerance = Math.max(span * 1e-9, 1e-12);
    const depths = commands.flatMap((command) => [...command.triangle.depths]);
    const minimumDepth = depths.reduce((minimum, depth) => Math.min(minimum, depth), Infinity);
    const maximumDepth = depths.reduce((maximum, depth) => Math.max(maximum, depth), -Infinity);
    const depthTolerance = Math.max((maximumDepth - minimumDepth) * 1e-9, 1e-12);
    const { low, high, sameStyleOverlaps } = fusionMobilityIntervals(commands, bounds);
    const parent = commands.map((_command, index) => index);
    const groupLow = Int32Array.from(low);
    const groupHigh = Int32Array.from(high);
    const find = (value) => {
        let root = value;
        while (parent[root] !== root)
            root = parent[root];
        while (parent[value] !== value) {
            const next = parent[value];
            parent[value] = root;
            value = next;
        }
        return root;
    };
    const candidates = [];
    const coplanarCandidates = [];
    const edgeMap = new Map();
    for (let triangle = 0; triangle < commands.length; triangle += 1) {
        const command = commands[triangle];
        if (command.opacity < 0.999)
            continue;
        const vertices = orientedFusionVertices(command);
        for (let edgeIndex = 0; edgeIndex < 3; edgeIndex += 1) {
            const startVertex = vertices[edgeIndex];
            const endVertex = vertices[(edgeIndex + 1) % 3];
            const thirdVertex = vertices[(edgeIndex + 2) % 3];
            const startKey = pointFusionKey(startVertex.point, startVertex.depth, coordinateTolerance, depthTolerance);
            const endKey = pointFusionKey(endVertex.point, endVertex.depth, coordinateTolerance, depthTolerance);
            const key = startKey < endKey ? `${startKey}|${endKey}` : `${endKey}|${startKey}`;
            const edge = {
                triangle,
                start: startVertex.point,
                end: endVertex.point,
                third: thirdVertex.point,
                startKey,
                endKey,
                key,
            };
            const entries = edgeMap.get(key);
            if (entries)
                entries.push(edge);
            else
                edgeMap.set(key, [edge]);
        }
    }
    const sideEpsilon = coordinateTolerance ** 2;
    for (const entries of edgeMap.values()) {
        if (entries.length !== 2)
            continue;
        const [a, b] = entries;
        const commandA = commands[a.triangle];
        const commandB = commands[b.triangle];
        const sharedBoundary = a.startKey === b.endKey &&
            a.endKey === b.startKey &&
            cross2(a.start, a.end, a.third) * cross2(a.start, a.end, b.third) < -sideEpsilon;
        if (!sharedBoundary)
            continue;
        if (commandA.fill === commandB.fill && Math.abs(commandA.opacity - commandB.opacity) <= 1e-12)
            candidates.push([a.triangle, b.triangle]);
        if (layerCoplanarMaterials &&
            commandA.opacity >= 1 - 1e-12 &&
            commandB.opacity >= 1 - 1e-12 &&
            dot(commandA.triangle.geometricNormal, commandB.triangle.geometricNormal) >= 1 - 1e-10)
            coplanarCandidates.push([a.triangle, b.triangle]);
    }
    candidates.sort((a, b) => Math.abs(a[0] - a[1]) - Math.abs(b[0] - b[1]));
    for (const [a, b] of candidates) {
        const rootA = find(a);
        const rootB = find(b);
        if (rootA === rootB)
            continue;
        const mergedLow = Math.max(groupLow[rootA], groupLow[rootB]);
        const mergedHigh = Math.min(groupHigh[rootA], groupHigh[rootB]);
        if (mergedLow > mergedHigh)
            continue;
        parent[rootB] = rootA;
        groupLow[rootA] = mergedLow;
        groupHigh[rootA] = mergedHigh;
    }
    const layerParent = commands.map((_command, index) => index);
    const findLayer = (value) => {
        let root = value;
        while (layerParent[root] !== root)
            root = layerParent[root];
        while (layerParent[value] !== value) {
            const next = layerParent[value];
            layerParent[value] = root;
            value = next;
        }
        return root;
    };
    coplanarCandidates.sort((a, b) => Math.abs(a[0] - a[1]) - Math.abs(b[0] - b[1]));
    for (const [a, b] of coplanarCandidates) {
        const rootA = findLayer(a);
        const rootB = findLayer(b);
        if (rootA !== rootB)
            layerParent[rootB] = rootA;
    }
    const components = new Map();
    for (let triangle = 0; triangle < commands.length; triangle += 1) {
        const root = find(triangle);
        const members = components.get(root);
        if (members)
            members.push(triangle);
        else
            components.set(root, [triangle]);
    }
    const unsafeRoots = new Set();
    for (const [a, b] of sameStyleOverlaps) {
        const rootA = find(a);
        if (rootA === find(b))
            unsafeRoots.add(rootA);
    }
    const sameStyleAdjacency = Array.from({ length: commands.length }, () => []);
    for (const [a, b] of candidates) {
        sameStyleAdjacency[a].push(b);
        sameStyleAdjacency[b].push(a);
    }
    const placements = [];
    const consumedByLayers = new Set();
    const layerComponents = new Map();
    const layerTriangles = new Set();
    for (const [first, second] of coplanarCandidates) {
        layerTriangles.add(first);
        layerTriangles.add(second);
    }
    for (const triangle of layerTriangles) {
        const root = findLayer(triangle);
        const members = layerComponents.get(root);
        if (members)
            members.push(triangle);
        else
            layerComponents.set(root, [triangle]);
    }
    for (const members of layerComponents.values()) {
        if (members.length < 2)
            continue;
        const commonLow = members.reduce((value, member) => Math.max(value, low[member]), 0);
        const commonHigh = members.reduce((value, member) => Math.min(value, high[member]), commands.length - 1);
        if (commonLow > commonHigh)
            continue;
        const layered = layeredSurfaceCommand(commands, members, sameStyleAdjacency, coordinateTolerance, depthTolerance);
        if (!layered)
            continue;
        for (const member of members)
            consumedByLayers.add(member);
        const average = members.reduce((sum, member) => sum + member, 0) / members.length;
        const position = Math.max(commonLow, Math.min(commonHigh, Math.round(average)));
        placements.push({ position, order: members[0], commands: [layered] });
    }
    for (const [root, members] of components) {
        const available = members.filter((member) => !consumedByLayers.has(member));
        if (available.length === 0)
            continue;
        if (available.length === 1 || unsafeRoots.has(root)) {
            for (const member of available)
                placements.push({
                    position: member,
                    order: member,
                    commands: [commands[member]],
                });
            continue;
        }
        const fused = fusedComponent(commands, available, coordinateTolerance, depthTolerance);
        if (!fused) {
            for (const member of available)
                placements.push({
                    position: member,
                    order: member,
                    commands: [commands[member]],
                });
            continue;
        }
        const average = available.reduce((sum, member) => sum + member, 0) / available.length;
        const position = Math.max(groupLow[root], Math.min(groupHigh[root], Math.round(average)));
        placements.push({ position, order: available[0], commands: [fused] });
    }
    placements.sort((a, b) => a.position - b.position || a.order - b.order);
    return placements.flatMap((placement) => placement.commands);
}
function renderCommands(scene, style) {
    const styleKey = JSON.stringify(style);
    const experimentalAmbientOcclusionRevision = Number(scene[EXPERIMENTAL_AO_REVISION_SYMBOL] ?? 0);
    const cached = RENDER_COMMAND_CACHE.get(scene);
    if (cached?.styleKey === styleKey &&
        cached.triangles === scene.triangles &&
        cached.edges === scene.edges &&
        cached.outlines === scene.outlineSegments &&
        cached.details === scene.detailSegments &&
        cached.experimentalAmbientOcclusionRevision === experimentalAmbientOcclusionRevision)
        return cached.result;
    const span = Math.max(scene.bounds.maxX - scene.bounds.minX, scene.bounds.maxY - scene.bounds.minY, 1e-9);
    const triangleCommands = [];
    const strokeCommands = [];
    const hlrCommands = [];
    let order = 0;
    let triangleCount = 0;
    let outlines = 0;
    let details = 0;
    let creases = 0;
    for (const triangle of scene.triangles) {
        if (!triangle.frontFacing && !(style.doubleSided || triangle.doubleSided))
            continue;
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
        if (!kind)
            continue;
        strokeCommands.push({
            kind,
            depth: edge.depth,
            order: order++,
            edge,
            color: kind === "outline" ? style.outlineColor : style.creaseColor,
            width: span * (kind === "outline" ? style.outlineWidth : style.creaseWidth),
        });
        if (kind === "outline")
            outlines += 1;
        else
            creases += 1;
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
    const commandByTriangle = new Map(triangleCommands.map((command) => [command.triangle, command]));
    let orderedTriangles;
    if (cachedTriangles?.length === triangleCommands.length &&
        cachedTriangles.every((triangle) => commandByTriangle.has(triangle))) {
        orderedTriangles = cachedTriangles.map((triangle) => commandByTriangle.get(triangle));
    }
    else {
        orderedTriangles = orderTriangleCommands(triangleCommands, scene.bounds);
        sceneCache.set(style.doubleSided, orderedTriangles.map((command) => command.triangle));
    }
    const surfaceCommands = style.fuseSurfaces
        ? fuseTriangleCommands(orderedTriangles, scene.bounds, style.layerCoplanarMaterials)
        : orderedTriangles;
    const surfaceDraws = surfaceCommands.reduce((count, command) => count + (command.kind === "layered-surface" ? command.layers.length : 1), 0);
    const layeredSurfaces = surfaceCommands.filter((command) => command.kind === "layered-surface").length;
    const commands = [
        ...surfaceCommands,
        // Generic mesh-derived linework is experimental and currently hidden by
        // the lab. Keep it above filled surfaces; CAD-derived HLR remains last.
        ...strokeCommands.sort((a, b) => a.depth - b.depth || a.order - b.order),
        ...hlrCommands.sort((a, b) => Number(a.kind === "hlr-outline") - Number(b.kind === "hlr-outline") || a.order - b.order),
    ];
    const result = {
        commands,
        stats: {
            triangles: triangleCount,
            surfaceDraws,
            layeredSurfaces,
            outlines,
            details,
            creases,
            commands: surfaceDraws + strokeCommands.length + hlrCommands.length,
        },
    };
    RENDER_COMMAND_CACHE.set(scene, {
        styleKey,
        triangles: scene.triangles,
        edges: scene.edges,
        outlines: scene.outlineSegments,
        details: scene.detailSegments,
        experimentalAmbientOcclusionRevision,
        result,
    });
    return result;
}
function numberText(value) {
    const rounded = Number(value.toPrecision(12));
    return Object.is(rounded, -0) ? "0" : String(rounded);
}
function escapeXml(value) {
    return value.replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll('"', "&quot;");
}
function safeCssColor(value) {
    const color = value.trim();
    return /^(?:#[0-9a-f]{3,8}|[a-z]+|(?:rgb|rgba|hsl|hsla)\([0-9.,%+\-\s]+\))$/iu.test(color)
        ? color
        : "#000000";
}
function lineCommandPoints(command) {
    return "points" in command ? command.points : command.edge.points;
}
function chainedSvgLinePath(commands, mapPoint) {
    const pointKey = (point) => `${point[0]},${point[1]}`;
    const segments = commands
        .map((command) => {
        const [sourceA, sourceB] = lineCommandPoints(command);
        const a = mapPoint(sourceA);
        const b = mapPoint(sourceB);
        return { a, b, aKey: pointKey(a), bKey: pointKey(b) };
    })
        .filter((segment) => segment.aKey !== segment.bKey);
    const adjacency = new Map();
    const points = new Map();
    for (let index = 0; index < segments.length; index += 1) {
        const segment = segments[index];
        points.set(segment.aKey, segment.a);
        points.set(segment.bKey, segment.b);
        for (const key of [segment.aKey, segment.bKey]) {
            const entries = adjacency.get(key);
            if (entries)
                entries.push(index);
            else
                adjacency.set(key, [index]);
        }
    }
    const used = new Uint8Array(segments.length);
    const polylines = [];
    const walk = (startKey, firstEdge) => {
        const polyline = [points.get(startKey)];
        let key = startKey;
        let edge = firstEdge;
        while (used[edge] === 0) {
            used[edge] = 1;
            const segment = segments[edge];
            key = segment.aKey === key ? segment.bKey : segment.aKey;
            polyline.push(points.get(key));
            const incident = adjacency.get(key) ?? [];
            const available = incident.filter((candidate) => used[candidate] === 0);
            if (incident.length !== 2 || available.length === 0)
                break;
            edge = available[0];
        }
        polylines.push(polyline);
    };
    for (const [key, incident] of adjacency) {
        if (incident.length === 2)
            continue;
        for (const edge of incident)
            if (used[edge] === 0)
                walk(key, edge);
    }
    for (let edge = 0; edge < segments.length; edge += 1) {
        if (used[edge] === 0)
            walk(segments[edge].aKey, edge);
    }
    return polylines
        .map((polyline) => {
        const closed = polyline.length > 2 && pointKey(polyline[0]) === pointKey(polyline.at(-1));
        const pointsToWrite = closed ? polyline.slice(0, -1) : polyline;
        const [first, ...rest] = pointsToWrite;
        const tail = rest.map((point) => `${numberText(point[0])} ${numberText(point[1])}`).join(" ");
        return `M${numberText(first[0])} ${numberText(first[1])}${tail ? `L${tail}` : ""}${closed ? "Z" : ""}`;
    })
        .join("");
}
export function renderMeshIllustrationSvg(scene, style, title = "Geometer mesh illustration", options = {}) {
    const width = Math.max(scene.bounds.maxX - scene.bounds.minX, 1e-9);
    const height = Math.max(scene.bounds.maxY - scene.bounds.minY, 1e-9);
    const pad = Math.max(width, height) * 0.06;
    const sourceMinX = scene.bounds.minX - pad;
    const sourceMinY = -scene.bounds.maxY - pad;
    const viewWidth = width + pad * 2;
    const viewHeight = height + pad * 2;
    // Normalize illustration output onto a high-resolution integer grid. Canvas
    // retains model coordinates; SVG avoids long metre-scale
    // decimals while preserving one part per million across the larger axis.
    const coordinateSpan = Math.round(clamp(options.coordinateSpan ?? 1_000_000, 10_000, 1_000_000_000));
    const coordinateScale = coordinateSpan / Math.max(width, height);
    const svgWidth = Math.max(1, Math.round(viewWidth * coordinateScale));
    const svgHeight = Math.max(1, Math.round(viewHeight * coordinateScale));
    const mapPoint = (point) => [
        Math.round((point[0] - sourceMinX) * coordinateScale),
        Math.round((-point[1] - sourceMinY) * coordinateScale),
    ];
    const surfacePath = (rings) => rings
        .map((ring) => `M${ring
        .map(mapPoint)
        .map((point) => `${numberText(point[0])} ${numberText(point[1])}`)
        .join("L")}Z`)
        .join("");
    const seamOverlap = Math.max(1, Math.round(Math.max(width, height) * 0.003 * coordinateScale));
    const { commands, stats } = renderCommands(scene, style);
    const body = [];
    const styleRules = [];
    const surfaceClasses = new Map();
    const lineClasses = new Map();
    const surfaceClass = (fill, opacity) => {
        const color = safeCssColor(fill);
        const key = `${color}|${numberText(opacity)}`;
        const cached = surfaceClasses.get(key);
        if (cached)
            return cached;
        const name = `gms${surfaceClasses.size}`;
        surfaceClasses.set(key, name);
        styleRules.push(`.${name}{fill:${color};fill-rule:evenodd;stroke:${color};stroke-width:${numberText(seamOverlap)};stroke-linejoin:round${opacity < 0.999 ? `;opacity:${numberText(opacity)}` : ""}}`);
        return name;
    };
    const lineClass = (color, lineWidth) => {
        const safeColor = safeCssColor(color);
        const scaledWidth = Math.max(1, Math.round(lineWidth * coordinateScale));
        const key = `${safeColor}|${scaledWidth}`;
        const cached = lineClasses.get(key);
        if (cached)
            return cached;
        const name = `gml${lineClasses.size}`;
        lineClasses.set(key, name);
        styleRules.push(`.${name}{fill:none;stroke:${safeColor};stroke-width:${scaledWidth};stroke-linecap:round;stroke-linejoin:round}`);
        return name;
    };
    if (!style.transparentBackground) {
        body.push(`<rect width="${svgWidth}" height="${svgHeight}" fill="${escapeXml(safeCssColor(style.background))}"/>`);
    }
    for (let commandIndex = 0; commandIndex < commands.length; commandIndex += 1) {
        const command = commands[commandIndex];
        if (command.kind === "triangle") {
            const points = command.triangle.points
                .map(mapPoint)
                .map((point) => `${numberText(point[0])},${numberText(point[1])}`)
                .join(" ");
            body.push(`<polygon class="${surfaceClass(command.fill, command.opacity)}" points="${points}"/>`);
        }
        else if (command.kind === "fused-surface") {
            const path = surfacePath(command.rings);
            body.push(`<path class="${surfaceClass(command.fill, command.opacity)}" d="${path}"/>`);
        }
        else if (command.kind === "layered-surface") {
            for (const layer of command.layers) {
                const path = surfacePath(layer.rings);
                body.push(`<path class="${surfaceClass(layer.fill, layer.opacity)}" d="${path}"/>`);
            }
        }
        else {
            const lineCommands = [command];
            while (commandIndex + 1 < commands.length) {
                const next = commands[commandIndex + 1];
                if (next.kind === "triangle" ||
                    next.kind === "fused-surface" ||
                    next.kind === "layered-surface" ||
                    next.color !== command.color ||
                    Math.abs(next.width - command.width) > 1e-15)
                    break;
                lineCommands.push(next);
                commandIndex += 1;
            }
            const path = chainedSvgLinePath(lineCommands, mapPoint);
            if (path)
                body.push(`<path class="${lineClass(command.color, command.width)}" d="${path}"/>`);
        }
    }
    const svg = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${svgWidth} ${svgHeight}" role="img">`,
        `<title>${escapeXml(title)}</title>`,
        `<metadata>geometry.mesh_illustration.result.a0</metadata>`,
        `<style>${styleRules.map(escapeXml).join("")}</style>`,
        ...body,
        "</svg>",
        "",
    ].join("\n");
    return { svg, stats };
}
export function renderMeshIllustrationCanvas(context, scene, style) {
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
        }
        else if (command.kind === "fused-surface") {
            context.beginPath();
            for (const ring of command.rings) {
                const first = ring[0];
                context.moveTo(offsetX + first[0] * scale, offsetY - first[1] * scale);
                for (let index = 1; index < ring.length; index += 1) {
                    const point = ring[index];
                    context.lineTo(offsetX + point[0] * scale, offsetY - point[1] * scale);
                }
                context.closePath();
            }
            context.globalAlpha = command.opacity;
            context.fillStyle = command.fill;
            context.fill("evenodd");
            context.strokeStyle = command.fill;
            context.lineWidth = Math.max(0.7, Math.min(1.5, scale * 0.0008));
            context.stroke();
        }
        else if (command.kind === "layered-surface") {
            for (const layer of command.layers) {
                context.beginPath();
                for (const ring of layer.rings) {
                    const first = ring[0];
                    context.moveTo(offsetX + first[0] * scale, offsetY - first[1] * scale);
                    for (let index = 1; index < ring.length; index += 1) {
                        const point = ring[index];
                        context.lineTo(offsetX + point[0] * scale, offsetY - point[1] * scale);
                    }
                    context.closePath();
                }
                context.globalAlpha = layer.opacity;
                context.fillStyle = layer.fill;
                context.fill("evenodd");
                context.strokeStyle = layer.fill;
                context.lineWidth = Math.max(0.7, Math.min(1.5, scale * 0.0008));
                context.stroke();
            }
        }
        else {
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
const DEFAULT_ILLUSTRATION_STYLE = Object.freeze({
    shading: "toon",
    ambient: 0.25,
    keyIntensity: 0.9,
    lightDirection: [0.4, 0.7, 1],
    bands: 3,
    sourceColors: true,
    fallbackColor: [0.72, 0.74, 0.78],
    background: "#ffffff",
    transparentBackground: false,
    fuseSurfaces: true,
    layerCoplanarMaterials: true,
    showHlrOutline: true,
    showHlrDetail: false,
    showOutlines: true,
    showCreases: true,
    creaseAngleDegrees: 30,
    outlineColor: "#17252c",
    creaseColor: "#33444a",
    outlineWidth: 0.006,
    creaseWidth: 0.003,
    doubleSided: false,
    rimAmount: 0.12,
});
/** Resolve a governed presence-preserving style patch to package defaults. */
export function resolveMeshIllustrationStyle(style = {}) {
    return {
        shading: style.shading ?? DEFAULT_ILLUSTRATION_STYLE.shading,
        ambient: style.ambient ?? DEFAULT_ILLUSTRATION_STYLE.ambient,
        keyIntensity: style.key_intensity ?? DEFAULT_ILLUSTRATION_STYLE.keyIntensity,
        lightDirection: style.light_direction ?? DEFAULT_ILLUSTRATION_STYLE.lightDirection,
        bands: style.bands ?? DEFAULT_ILLUSTRATION_STYLE.bands,
        sourceColors: style.source_colors ?? DEFAULT_ILLUSTRATION_STYLE.sourceColors,
        fallbackColor: style.fallback_color ?? DEFAULT_ILLUSTRATION_STYLE.fallbackColor,
        background: style.background ?? DEFAULT_ILLUSTRATION_STYLE.background,
        transparentBackground: style.transparent_background ?? DEFAULT_ILLUSTRATION_STYLE.transparentBackground,
        fuseSurfaces: style.fuse_surfaces ?? DEFAULT_ILLUSTRATION_STYLE.fuseSurfaces,
        layerCoplanarMaterials: style.layer_coplanar_materials ?? DEFAULT_ILLUSTRATION_STYLE.layerCoplanarMaterials,
        showHlrOutline: style.show_hlr_outline ?? DEFAULT_ILLUSTRATION_STYLE.showHlrOutline,
        showHlrDetail: style.show_hlr_detail ?? DEFAULT_ILLUSTRATION_STYLE.showHlrDetail,
        showOutlines: style.show_outlines ?? DEFAULT_ILLUSTRATION_STYLE.showOutlines,
        showCreases: style.show_creases ?? DEFAULT_ILLUSTRATION_STYLE.showCreases,
        creaseAngleDegrees: style.crease_angle_degrees ?? DEFAULT_ILLUSTRATION_STYLE.creaseAngleDegrees,
        outlineColor: style.outline_color ?? DEFAULT_ILLUSTRATION_STYLE.outlineColor,
        creaseColor: style.crease_color ?? DEFAULT_ILLUSTRATION_STYLE.creaseColor,
        outlineWidth: style.outline_width ?? DEFAULT_ILLUSTRATION_STYLE.outlineWidth,
        creaseWidth: style.crease_width ?? DEFAULT_ILLUSTRATION_STYLE.creaseWidth,
        doubleSided: style.double_sided ?? DEFAULT_ILLUSTRATION_STYLE.doubleSided,
        rimAmount: style.rim_amount ?? DEFAULT_ILLUSTRATION_STYLE.rimAmount,
    };
}
/** Convert a resolved ergonomic style into the governed standalone A0 DTO. */
export function toMeshIllustrationStyleA0(style) {
    return {
        shading: style.shading,
        ambient: style.ambient,
        key_intensity: style.keyIntensity,
        light_direction: style.lightDirection,
        bands: style.bands,
        source_colors: style.sourceColors,
        fallback_color: style.fallbackColor,
        background: style.background,
        transparent_background: style.transparentBackground,
        fuse_surfaces: style.fuseSurfaces,
        layer_coplanar_materials: style.layerCoplanarMaterials,
        show_hlr_outline: style.showHlrOutline,
        show_hlr_detail: style.showHlrDetail,
        show_outlines: style.showOutlines,
        show_creases: style.showCreases,
        crease_angle_degrees: style.creaseAngleDegrees,
        outline_color: style.outlineColor,
        crease_color: style.creaseColor,
        outline_width: style.outlineWidth,
        crease_width: style.creaseWidth,
        double_sided: style.doubleSided,
        rim_amount: style.rimAmount,
    };
}
/** Prepare one governed illustration input once, then render multiple styles or targets. */
export function createIllustrator(input, linework = {}) {
    const baseStyle = { ...input.style };
    const baseSvg = { ...input.svg };
    let scene = prepareMeshIllustration({
        meshes: input.meshes.map((mesh) => ({
            id: mesh.id,
            positions: mesh.positions,
            ...(mesh.normals === undefined ? {} : { normals: mesh.normals }),
            ...(mesh.indices === undefined ? {} : { indices: mesh.indices }),
            ...(mesh.matrix === undefined ? {} : { matrix: mesh.matrix }),
            materials: mesh.materials,
            ...(mesh.triangle_material_indices === undefined
                ? {}
                : { triangleMaterialIndices: mesh.triangle_material_indices }),
            ...(mesh.double_sided === undefined ? {} : { doubleSided: mesh.double_sided }),
        })),
    }, {
        direction: input.view.direction,
        up: input.view.up,
        ...(input.view.mirror_x === undefined ? {} : { mirrorX: input.view.mirror_x }),
    }, {
        maxTriangles: input.prepare?.max_triangles ?? 750_000,
        weldTolerance: input.prepare?.weld_tolerance ?? 0.0000001,
    });
    if (linework.outlineSegments !== undefined)
        scene.outlineSegments = linework.outlineSegments;
    if (linework.detailSegments !== undefined)
        scene.detailSegments = linework.detailSegments;
    let disposed = false;
    const requireScene = () => {
        if (scene === null)
            throw new Error("Mesh illustrator has been disposed.");
        return scene;
    };
    const mergedStyle = (patch = {}) => resolveMeshIllustrationStyle({ ...baseStyle, ...patch });
    return {
        get disposed() {
            return disposed;
        },
        renderSvg(style = {}, svg = {}) {
            const options = { ...baseSvg, ...svg };
            const rendered = renderMeshIllustrationSvg(requireScene(), mergedStyle(style), options.title ?? "Geometer mesh illustration", options.coordinate_span === undefined ? {} : { coordinateSpan: options.coordinate_span });
            return {
                schema: "geometry.mesh_illustration.result.a0",
                svg: rendered.svg,
                stats: illustrationStatsA0(rendered.stats),
                warnings: [...requireScene().warnings],
            };
        },
        renderCanvas(context, style = {}) {
            return illustrationStatsA0(renderMeshIllustrationCanvas(context, requireScene(), mergedStyle(style)));
        },
        dispose() {
            if (disposed)
                return;
            const current = requireScene();
            VISIBILITY_ORDER_CACHE.delete(current);
            RENDER_COMMAND_CACHE.delete(current);
            scene = null;
            disposed = true;
        },
    };
}
/** Prepare and render one governed mesh-illustration A0 input to SVG. */
export function illustrateMesh(input) {
    const illustrator = createIllustrator(input);
    try {
        return illustrator.renderSvg();
    }
    finally {
        illustrator.dispose();
    }
}
function illustrationStatsA0(stats) {
    return {
        triangles: stats.triangles,
        surface_draws: stats.surfaceDraws,
        layered_surfaces: stats.layeredSurfaces,
        outlines: stats.outlines,
        details: stats.details,
        creases: stats.creases,
        commands: stats.commands,
    };
}
