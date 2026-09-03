import { createIllustrator, } from "./mesh-illustration.js";
/** Prepare Fast vector linework and the colorized scene once for repeated rendering. */
export async function createFastHlrIllustrator(projector, request) {
    const input = request.illustration;
    const hlr = await projector.meshHlrProjection({
        mesh: indexedMeshFromIllustrationInput(input),
        options: {
            ...request.hlr,
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
    if (view === undefined)
        throw new Error("Fast HLR returned no illustration view.");
    return {
        hlr,
        illustrator: createIllustrator(input, lineworkFromHlr(view, input.view.mirror_x === true)),
    };
}
/** Project Fast vector linework, colorize the mesh, and return one SVG result. */
export async function illustrateMeshWithFastHlr(projector, request) {
    const prepared = await createFastHlrIllustrator(projector, request);
    try {
        return { hlr: prepared.hlr, illustration: prepared.illustrator.renderSvg() };
    }
    finally {
        prepared.illustrator.dispose();
    }
}
/** Flatten governed illustration meshes and column-major transforms into one HLR mesh. */
export function indexedMeshFromIllustrationInput(input) {
    const positions = [];
    const indices = [];
    for (const mesh of input.meshes) {
        if (mesh.positions.length % 3 !== 0) {
            throw new Error(`Mesh ${mesh.id} position length must be divisible by three.`);
        }
        const vertexOffset = positions.length / 3;
        for (let offset = 0; offset < mesh.positions.length; offset += 3) {
            const point = transformPoint([
                mesh.positions[offset] ?? Number.NaN,
                mesh.positions[offset + 1] ?? Number.NaN,
                mesh.positions[offset + 2] ?? Number.NaN,
            ], mesh.matrix);
            positions.push(...point);
        }
        const sourceIndices = mesh.indices ?? sequentialIndices(mesh.positions.length / 3);
        if (sourceIndices.length % 3 !== 0) {
            throw new Error(`Mesh ${mesh.id} index length must be divisible by three.`);
        }
        for (const index of sourceIndices)
            indices.push(vertexOffset + index);
    }
    return { positions, indices };
}
function sequentialIndices(count) {
    return Array.from({ length: count }, (_, index) => index);
}
function transformPoint(point, matrix) {
    if (matrix === undefined)
        return point;
    const [x, y, z] = point;
    const w = matrix[3] * x + matrix[7] * y + matrix[11] * z + matrix[15];
    const divisor = Math.abs(w) > 1e-15 ? w : 1;
    return [
        (matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12]) / divisor,
        (matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13]) / divisor,
        (matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14]) / divisor,
    ];
}
function lineworkFromHlr(view, mirrorX) {
    const convert = (segments) => segments.map(([x1, y1, x2, y2]) => ({
        points: [
            [mirrorX ? -x1 : x1, y1],
            [mirrorX ? -x2 : x2, y2],
        ],
    }));
    return {
        outlineSegments: convert(view.modes.outline.segments),
        detailSegments: convert(view.modes.detail.segments),
    };
}
