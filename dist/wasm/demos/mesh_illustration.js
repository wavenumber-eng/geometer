const IDENTITY_MATRIX = Object.freeze([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
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
    const maxTriangles = Math.max(1, Math.trunc(options.maxTriangles ?? 120_000));
    const weldTolerance = Math.max(1e-9, finite(options.weldTolerance ?? 1e-6, 1e-6));
    const triangles = [];
    const edgeMaps = new Map();
    const warnings = [];
    let sourceTriangles = 0;
    let minX = Infinity;
    let minY = Infinity;
    let maxX = -Infinity;
    let maxY = -Infinity;
    for (const mesh of input.meshes) {
        const triangleCount = validateMesh(mesh);
        sourceTriangles += triangleCount;
        if (sourceTriangles > maxTriangles)
            throw new Error(`Mesh illustration exceeds the ${maxTriangles.toLocaleString()} triangle prototype limit.`);
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
                warnings.push(`Skipped out-of-range triangle ${triangleIndex} in ${mesh.id}.`);
                continue;
            }
            const world = indices.map((index) => transformPoint(matrix, positionAt(mesh.positions, index)));
            const crossNormal = cross(subtract(world[1], world[0]), subtract(world[2], world[0]));
            const magnitude = length(crossNormal);
            if (magnitude < 1e-12) {
                warnings.push(`Skipped degenerate triangle ${triangleIndex} in ${mesh.id}.`);
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
    let intensity = clamp(style.ambient) + clamp(style.keyIntensity, 0, 4) * diffuse;
    intensity = clamp(intensity);
    if (style.shading === "banded" || style.shading === "toon") {
        const bands = Math.max(2, Math.min(8, Math.trunc(style.bands)));
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
function renderCommands(scene, style) {
    const span = Math.max(scene.bounds.maxX - scene.bounds.minX, scene.bounds.maxY - scene.bounds.minY, 1e-9);
    const commands = [];
    let order = 0;
    let triangleCount = 0;
    let outlines = 0;
    let creases = 0;
    for (const triangle of scene.triangles) {
        if (!triangle.frontFacing && !(style.doubleSided || triangle.doubleSided))
            continue;
        commands.push({
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
        commands.push({
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
    commands.sort((a, b) => {
        const depthOrder = a.depth - b.depth;
        if (depthOrder !== 0)
            return depthOrder;
        const kindOrder = Number(a.kind !== "triangle") - Number(b.kind !== "triangle");
        return kindOrder || a.order - b.order;
    });
    return {
        commands,
        stats: { triangles: triangleCount, outlines, creases, commands: commands.length },
    };
}
function numberText(value) {
    const rounded = Number(value.toPrecision(12));
    return Object.is(rounded, -0) ? "0" : String(rounded);
}
function escapeXml(value) {
    return value.replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll('"', "&quot;");
}
export function renderMeshIllustrationSvg(scene, style, title = "Geometer mesh illustration") {
    const width = Math.max(scene.bounds.maxX - scene.bounds.minX, 1e-9);
    const height = Math.max(scene.bounds.maxY - scene.bounds.minY, 1e-9);
    const pad = Math.max(width, height) * 0.06;
    const minX = scene.bounds.minX - pad;
    const minY = -scene.bounds.maxY - pad;
    const viewWidth = width + pad * 2;
    const viewHeight = height + pad * 2;
    const seamOverlap = Math.max(width, height) * 0.003;
    const { commands, stats } = renderCommands(scene, style);
    const body = [];
    if (!style.transparentBackground) {
        body.push(`<rect x="${numberText(minX)}" y="${numberText(minY)}" width="${numberText(viewWidth)}" height="${numberText(viewHeight)}" fill="${escapeXml(style.background)}"/>`);
    }
    for (const command of commands) {
        if (command.kind === "triangle") {
            const points = command.triangle.points
                .map((point) => `${numberText(point[0])},${numberText(-point[1])}`)
                .join(" ");
            const opacity = command.opacity < 0.999 ? ` fill-opacity="${numberText(command.opacity)}"` : "";
            body.push(`<polygon points="${points}" fill="${command.fill}" stroke="${command.fill}" stroke-width="${numberText(seamOverlap)}" stroke-linejoin="round"${opacity}/>`);
        }
        else {
            const [a, b] = command.edge.points;
            body.push(`<path d="M ${numberText(a[0])} ${numberText(-a[1])} L ${numberText(b[0])} ${numberText(-b[1])}" fill="none" stroke="${escapeXml(command.color)}" stroke-width="${numberText(command.width)}" stroke-linecap="round" stroke-linejoin="round"/>`);
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
        else {
            const [a, b] = command.edge.points;
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
