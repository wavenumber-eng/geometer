const AO_SYMBOL = Symbol.for("@wavenumber/geometer/experimental-mesh-ambient-occlusion");
const AO_REVISION_SYMBOL = Symbol.for("@wavenumber/geometer/experimental-mesh-ambient-occlusion-revision");
function finite(value, fallback = 0) {
    return Number.isFinite(value) ? value : fallback;
}
function transformPoint(matrix, x, y, z) {
    const at = (index) => finite(Number(matrix[index]), index % 5 === 0 ? 1 : 0);
    const transformed = [
        at(0) * x + at(4) * y + at(8) * z + at(12),
        at(1) * x + at(5) * y + at(9) * z + at(13),
        at(2) * x + at(6) * y + at(10) * z + at(14),
    ];
    return [finite(transformed[0]), finite(transformed[1]), finite(transformed[2])];
}
function flattenedTriangles(input, maxTriangles) {
    let triangleCount = 0;
    const meshRanges = [];
    for (const mesh of input.meshes) {
        const count = Math.trunc((mesh.indices?.length ?? mesh.positions.length / 3) / 3);
        if (count < 0 || !Number.isFinite(count))
            throw new Error(`Invalid triangle count for ${mesh.id}.`);
        meshRanges.push({ id: mesh.id, offset: triangleCount, count });
        triangleCount += count;
        if (triangleCount > maxTriangles)
            throw new Error(`Ambient-occlusion experiment exceeds ${maxTriangles.toLocaleString()} triangles.`);
    }
    const vertices = new Float64Array(triangleCount * 9);
    for (let meshIndex = 0; meshIndex < input.meshes.length; meshIndex += 1) {
        const mesh = input.meshes[meshIndex];
        const range = meshRanges[meshIndex];
        if (!mesh || !range)
            continue;
        const matrix = mesh.matrix ?? [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1];
        const vertexCount = Math.trunc(mesh.positions.length / 3);
        for (let triangleIndex = 0; triangleIndex < range.count; triangleIndex += 1) {
            for (let corner = 0; corner < 3; corner += 1) {
                const elementIndex = triangleIndex * 3 + corner;
                const requested = Number(mesh.indices?.[elementIndex] ?? elementIndex);
                const index = Math.trunc(requested);
                if (!Number.isFinite(requested) || requested !== index || index < 0 || index >= vertexCount)
                    throw new Error(`Ambient-occlusion experiment found an invalid index in ${mesh.id}.`);
                const point = transformPoint(matrix, finite(Number(mesh.positions[index * 3])), finite(Number(mesh.positions[index * 3 + 1])), finite(Number(mesh.positions[index * 3 + 2])));
                const output = (range.offset + triangleIndex) * 9 + corner * 3;
                vertices[output] = point[0];
                vertices[output + 1] = point[1];
                vertices[output + 2] = point[2];
            }
        }
    }
    return { vertices, meshRanges };
}
function ambientOcclusionWorker() {
    const scope = self;
    scope.onmessage = (event) => {
        const started = performance.now();
        const vertices = new Float64Array(event.data.vertices);
        const triangleCount = Math.trunc(vertices.length / 9);
        const samples = event.data.samples;
        if (triangleCount === 0) {
            const accessibility = new Float32Array();
            scope.postMessage({
                accessibility: accessibility.buffer,
                radius: 0,
                milliseconds: performance.now() - started,
                minimumAccessibility: 1,
                meanAccessibility: 1,
            }, [accessibility.buffer]);
            return;
        }
        const bounds = new Float64Array(triangleCount * 6);
        const centers = new Float64Array(triangleCount * 3);
        const normals = new Float64Array(triangleCount * 3);
        let sceneMinX = Number.POSITIVE_INFINITY;
        let sceneMinY = Number.POSITIVE_INFINITY;
        let sceneMinZ = Number.POSITIVE_INFINITY;
        let sceneMaxX = Number.NEGATIVE_INFINITY;
        let sceneMaxY = Number.NEGATIVE_INFINITY;
        let sceneMaxZ = Number.NEGATIVE_INFINITY;
        for (let triangle = 0; triangle < triangleCount; triangle += 1) {
            const offset = triangle * 9;
            const ax = vertices[offset] ?? 0;
            const ay = vertices[offset + 1] ?? 0;
            const az = vertices[offset + 2] ?? 0;
            const bx = vertices[offset + 3] ?? 0;
            const by = vertices[offset + 4] ?? 0;
            const bz = vertices[offset + 5] ?? 0;
            const cx = vertices[offset + 6] ?? 0;
            const cy = vertices[offset + 7] ?? 0;
            const cz = vertices[offset + 8] ?? 0;
            const minX = Math.min(ax, bx, cx);
            const minY = Math.min(ay, by, cy);
            const minZ = Math.min(az, bz, cz);
            const maxX = Math.max(ax, bx, cx);
            const maxY = Math.max(ay, by, cy);
            const maxZ = Math.max(az, bz, cz);
            const boundsOffset = triangle * 6;
            bounds[boundsOffset] = minX;
            bounds[boundsOffset + 1] = minY;
            bounds[boundsOffset + 2] = minZ;
            bounds[boundsOffset + 3] = maxX;
            bounds[boundsOffset + 4] = maxY;
            bounds[boundsOffset + 5] = maxZ;
            centers[triangle * 3] = (ax + bx + cx) / 3;
            centers[triangle * 3 + 1] = (ay + by + cy) / 3;
            centers[triangle * 3 + 2] = (az + bz + cz) / 3;
            const abx = bx - ax;
            const aby = by - ay;
            const abz = bz - az;
            const acx = cx - ax;
            const acy = cy - ay;
            const acz = cz - az;
            const nx = aby * acz - abz * acy;
            const ny = abz * acx - abx * acz;
            const nz = abx * acy - aby * acx;
            const length = Math.hypot(nx, ny, nz);
            normals[triangle * 3] = length > 1e-15 ? nx / length : 0;
            normals[triangle * 3 + 1] = length > 1e-15 ? ny / length : 0;
            normals[triangle * 3 + 2] = length > 1e-15 ? nz / length : 1;
            sceneMinX = Math.min(sceneMinX, minX);
            sceneMinY = Math.min(sceneMinY, minY);
            sceneMinZ = Math.min(sceneMinZ, minZ);
            sceneMaxX = Math.max(sceneMaxX, maxX);
            sceneMaxY = Math.max(sceneMaxY, maxY);
            sceneMaxZ = Math.max(sceneMaxZ, maxZ);
        }
        const diagonal = Math.hypot(sceneMaxX - sceneMinX, sceneMaxY - sceneMinY, sceneMaxZ - sceneMinZ);
        const radius = Math.max(diagonal * event.data.radiusFraction, diagonal * 1e-6, 1e-9);
        const bias = Math.max(radius * 1e-4, diagonal * 1e-7, 1e-10);
        const order = Array.from({ length: triangleCount }, (_, index) => index);
        const nodes = [];
        const buildNode = (start, end) => {
            let minX = Number.POSITIVE_INFINITY;
            let minY = Number.POSITIVE_INFINITY;
            let minZ = Number.POSITIVE_INFINITY;
            let maxX = Number.NEGATIVE_INFINITY;
            let maxY = Number.NEGATIVE_INFINITY;
            let maxZ = Number.NEGATIVE_INFINITY;
            let centerMinX = Number.POSITIVE_INFINITY;
            let centerMinY = Number.POSITIVE_INFINITY;
            let centerMinZ = Number.POSITIVE_INFINITY;
            let centerMaxX = Number.NEGATIVE_INFINITY;
            let centerMaxY = Number.NEGATIVE_INFINITY;
            let centerMaxZ = Number.NEGATIVE_INFINITY;
            for (let cursor = start; cursor < end; cursor += 1) {
                const triangle = order[cursor] ?? 0;
                const bo = triangle * 6;
                const co = triangle * 3;
                minX = Math.min(minX, bounds[bo] ?? 0);
                minY = Math.min(minY, bounds[bo + 1] ?? 0);
                minZ = Math.min(minZ, bounds[bo + 2] ?? 0);
                maxX = Math.max(maxX, bounds[bo + 3] ?? 0);
                maxY = Math.max(maxY, bounds[bo + 4] ?? 0);
                maxZ = Math.max(maxZ, bounds[bo + 5] ?? 0);
                centerMinX = Math.min(centerMinX, centers[co] ?? 0);
                centerMinY = Math.min(centerMinY, centers[co + 1] ?? 0);
                centerMinZ = Math.min(centerMinZ, centers[co + 2] ?? 0);
                centerMaxX = Math.max(centerMaxX, centers[co] ?? 0);
                centerMaxY = Math.max(centerMaxY, centers[co + 1] ?? 0);
                centerMaxZ = Math.max(centerMaxZ, centers[co + 2] ?? 0);
            }
            const nodeIndex = nodes.length;
            nodes.push({ minX, minY, minZ, maxX, maxY, maxZ, start, end, left: -1, right: -1 });
            if (end - start <= 8)
                return nodeIndex;
            const spans = [
                centerMaxX - centerMinX,
                centerMaxY - centerMinY,
                centerMaxZ - centerMinZ,
            ];
            const axis = spans[1] > spans[0] ? (spans[2] > spans[1] ? 2 : 1) : spans[2] > spans[0] ? 2 : 0;
            const sorted = order
                .slice(start, end)
                .sort((left, right) => (centers[left * 3 + axis] ?? 0) - (centers[right * 3 + axis] ?? 0));
            for (let cursor = 0; cursor < sorted.length; cursor += 1)
                order[start + cursor] = sorted[cursor] ?? 0;
            const middle = start + Math.floor((end - start) / 2);
            const node = nodes[nodeIndex];
            if (node) {
                node.left = buildNode(start, middle);
                node.right = buildNode(middle, end);
            }
            return nodeIndex;
        };
        const root = triangleCount > 0 ? buildNode(0, triangleCount) : -1;
        const rayBox = (node, ox, oy, oz, dx, dy, dz) => {
            let near = 0;
            let far = radius;
            const axes = [
                [ox, dx, node.minX, node.maxX],
                [oy, dy, node.minY, node.maxY],
                [oz, dz, node.minZ, node.maxZ],
            ];
            for (const [origin, direction, minimum, maximum] of axes) {
                if (Math.abs(direction) < 1e-15) {
                    if (origin < minimum || origin > maximum)
                        return false;
                    continue;
                }
                const inverse = 1 / direction;
                const first = (minimum - origin) * inverse;
                const second = (maximum - origin) * inverse;
                near = Math.max(near, Math.min(first, second));
                far = Math.min(far, Math.max(first, second));
                if (far < near)
                    return false;
            }
            return true;
        };
        const rayTriangle = (triangle, ox, oy, oz, dx, dy, dz) => {
            const offset = triangle * 9;
            const ax = vertices[offset] ?? 0;
            const ay = vertices[offset + 1] ?? 0;
            const az = vertices[offset + 2] ?? 0;
            const e1x = (vertices[offset + 3] ?? 0) - ax;
            const e1y = (vertices[offset + 4] ?? 0) - ay;
            const e1z = (vertices[offset + 5] ?? 0) - az;
            const e2x = (vertices[offset + 6] ?? 0) - ax;
            const e2y = (vertices[offset + 7] ?? 0) - ay;
            const e2z = (vertices[offset + 8] ?? 0) - az;
            const px = dy * e2z - dz * e2y;
            const py = dz * e2x - dx * e2z;
            const pz = dx * e2y - dy * e2x;
            const determinant = e1x * px + e1y * py + e1z * pz;
            if (Math.abs(determinant) < 1e-14)
                return Number.POSITIVE_INFINITY;
            const inverse = 1 / determinant;
            const tx = ox - ax;
            const ty = oy - ay;
            const tz = oz - az;
            const u = (tx * px + ty * py + tz * pz) * inverse;
            if (u < 0 || u > 1)
                return Number.POSITIVE_INFINITY;
            const qx = ty * e1z - tz * e1y;
            const qy = tz * e1x - tx * e1z;
            const qz = tx * e1y - ty * e1x;
            const v = (dx * qx + dy * qy + dz * qz) * inverse;
            if (v < 0 || u + v > 1)
                return Number.POSITIVE_INFINITY;
            const distance = (e2x * qx + e2y * qy + e2z * qz) * inverse;
            return distance > bias && distance <= radius ? distance : Number.POSITIVE_INFINITY;
        };
        const nearestHit = (ignored, ox, oy, oz, dx, dy, dz) => {
            if (root < 0)
                return Number.POSITIVE_INFINITY;
            let nearest = Number.POSITIVE_INFINITY;
            const stack = [root];
            while (stack.length > 0) {
                const node = nodes[stack.pop() ?? -1];
                if (!node || !rayBox(node, ox, oy, oz, dx, dy, dz))
                    continue;
                if (node.left >= 0) {
                    stack.push(node.left, node.right);
                    continue;
                }
                for (let cursor = node.start; cursor < node.end; cursor += 1) {
                    const candidate = order[cursor] ?? -1;
                    if (candidate === ignored)
                        continue;
                    nearest = Math.min(nearest, rayTriangle(candidate, ox, oy, oz, dx, dy, dz));
                }
            }
            return nearest;
        };
        const reverseBits = (value) => {
            let bits = value >>> 0;
            bits = ((bits << 16) | (bits >>> 16)) >>> 0;
            bits = (((bits & 0x00ff00ff) << 8) | ((bits & 0xff00ff00) >>> 8)) >>> 0;
            bits = (((bits & 0x0f0f0f0f) << 4) | ((bits & 0xf0f0f0f0) >>> 4)) >>> 0;
            bits = (((bits & 0x33333333) << 2) | ((bits & 0xcccccccc) >>> 2)) >>> 0;
            bits = (((bits & 0x55555555) << 1) | ((bits & 0xaaaaaaaa) >>> 1)) >>> 0;
            return bits / 0x1_0000_0000;
        };
        const accessibility = new Float32Array(triangleCount);
        let minimumAccessibility = 1;
        let accessibilitySum = 0;
        for (let triangle = 0; triangle < triangleCount; triangle += 1) {
            const co = triangle * 3;
            const nx = normals[co] ?? 0;
            const ny = normals[co + 1] ?? 0;
            const nz = normals[co + 2] ?? 1;
            const helperX = Math.abs(nz) < 0.9 ? 0 : 1;
            const helperY = 0;
            const helperZ = Math.abs(nz) < 0.9 ? 1 : 0;
            const tangentX0 = helperY * nz - helperZ * ny;
            const tangentY0 = helperZ * nx - helperX * nz;
            const tangentZ0 = helperX * ny - helperY * nx;
            const tangentLength = Math.hypot(tangentX0, tangentY0, tangentZ0) || 1;
            const tx = tangentX0 / tangentLength;
            const ty = tangentY0 / tangentLength;
            const tz = tangentZ0 / tangentLength;
            const bx = ny * tz - nz * ty;
            const by = nz * tx - nx * tz;
            const bz = nx * ty - ny * tx;
            const ox = (centers[co] ?? 0) + nx * bias;
            const oy = (centers[co + 1] ?? 0) + ny * bias;
            const oz = (centers[co + 2] ?? 0) + nz * bias;
            const rotation = ((triangle * 0.6180339887498949) % 1) * Math.PI * 2;
            let occlusion = 0;
            for (let sample = 0; sample < samples; sample += 1) {
                const radial = Math.sqrt((sample + 0.5) / samples);
                const angle = Math.PI * 2 * reverseBits(sample) + rotation;
                const localX = Math.cos(angle) * radial;
                const localY = Math.sin(angle) * radial;
                const localZ = Math.sqrt(Math.max(0, 1 - radial * radial));
                const dx = tx * localX + bx * localY + nx * localZ;
                const dy = ty * localX + by * localY + ny * localZ;
                const dz = tz * localX + bz * localY + nz * localZ;
                const hit = nearestHit(triangle, ox, oy, oz, dx, dy, dz);
                if (Number.isFinite(hit))
                    occlusion += 1 - hit / radius;
            }
            accessibility[triangle] = Math.max(0, Math.min(1, 1 - occlusion / samples));
            minimumAccessibility = Math.min(minimumAccessibility, accessibility[triangle] ?? 1);
            accessibilitySum += accessibility[triangle] ?? 1;
        }
        scope.postMessage({
            accessibility: accessibility.buffer,
            radius,
            milliseconds: performance.now() - started,
            minimumAccessibility,
            meanAccessibility: triangleCount > 0 ? accessibilitySum / triangleCount : 1,
        }, [accessibility.buffer]);
    };
}
export async function prepareExperimentalAmbientOcclusion(input, options) {
    const maxTriangles = Math.max(1, Math.min(100_000, Math.trunc(finite(options.maxTriangles ?? 100_000, 100_000))));
    const samples = Math.max(1, Math.min(32, Math.trunc(finite(options.samples, 16))));
    const radiusFraction = Math.max(0.001, Math.min(0.25, finite(options.radiusFraction, 0.05)));
    if (options.signal?.aborted)
        throw new DOMException("Ambient occlusion canceled.", "AbortError");
    const flattened = flattenedTriangles(input, maxTriangles);
    const workerSource = `(${ambientOcclusionWorker.toString()})();`;
    const workerUrl = URL.createObjectURL(new Blob([workerSource], { type: "text/javascript" }));
    const worker = new Worker(workerUrl);
    try {
        const result = await new Promise((resolve, reject) => {
            const abort = () => {
                worker.terminate();
                reject(new DOMException("Ambient occlusion canceled.", "AbortError"));
            };
            options.signal?.addEventListener("abort", abort, { once: true });
            worker.onerror = (event) => {
                options.signal?.removeEventListener("abort", abort);
                reject(new Error(event.message || "Ambient-occlusion worker failed."));
            };
            worker.onmessage = (event) => {
                options.signal?.removeEventListener("abort", abort);
                resolve(event.data);
            };
            worker.postMessage({ vertices: flattened.vertices.buffer, samples, radiusFraction }, [
                flattened.vertices.buffer,
            ]);
        });
        const flatAccessibility = new Float32Array(result.accessibility);
        const accessibilityByMesh = new Map();
        for (const range of flattened.meshRanges)
            accessibilityByMesh.set(range.id, flatAccessibility.slice(range.offset, range.offset + range.count));
        return {
            accessibilityByMesh,
            stats: {
                triangles: flatAccessibility.length,
                samples,
                radius: result.radius,
                milliseconds: result.milliseconds,
                minimumAccessibility: result.minimumAccessibility,
                meanAccessibility: result.meanAccessibility,
            },
        };
    }
    finally {
        worker.terminate();
        URL.revokeObjectURL(workerUrl);
    }
}
export function applyExperimentalAmbientOcclusion(scene, result) {
    for (const triangle of scene.triangles) {
        const values = result.accessibilityByMesh.get(triangle.meshId);
        const value = values?.[triangle.triangleIndex];
        triangle[AO_SYMBOL] = Number.isFinite(value)
            ? Math.max(0, Math.min(1, value ?? 1))
            : 1;
    }
    const mutableScene = scene;
    mutableScene[AO_REVISION_SYMBOL] = (mutableScene[AO_REVISION_SYMBOL] ?? 0) + 1;
}
