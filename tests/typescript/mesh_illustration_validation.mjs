import assert from "node:assert/strict";

import {
  prepareMeshIllustration,
  renderMeshIllustrationSvg,
} from "../../dist/wasm/demos/mesh_illustration.js";

const positions = new Float32Array([
  -1, -1, -1, 1, -1, -1, 1, 1, -1, -1, 1, -1, -1, -1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1,
]);
const indices = new Uint32Array([
  4, 5, 6, 4, 6, 7, 1, 0, 3, 1, 3, 2, 1, 2, 6, 1, 6, 5, 0, 4, 7, 0, 7, 3, 3, 7, 6, 3, 6, 2, 0, 1, 5,
  0, 5, 4,
]);

const scene = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "cube",
        positions,
        indices,
        materials: [{ color: [0.2, 0.7, 0.62] }],
      },
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
);

assert.equal(scene.schema, "geometry.mesh_illustration.prototype.a0");
assert.equal(scene.stats.sourceMeshes, 1);
assert.equal(scene.stats.sourceTriangles, 12);
assert.equal(scene.stats.projectedTriangles, 12);
assert.equal(scene.warnings.length, 0);
assert.deepEqual(scene.bounds, { minX: -1, minY: -1, maxX: 1, maxY: 1 });
assert.equal(scene.triangles.filter((triangle) => triangle.frontFacing).length, 2);
assert.throws(
  () =>
    prepareMeshIllustration(
      {
        meshes: [
          { id: "limited-cube", positions, indices, materials: [{ color: [0.2, 0.7, 0.62] }] },
        ],
      },
      { direction: [0, 0, 1], up: [0, 1, 0] },
      { maxTriangles: 11 },
    ),
  /configured 11 triangle limit/u,
);

const baseStyle = {
  shading: "toon",
  ambient: 0.25,
  keyIntensity: 0.9,
  lightDirection: [0.4, 0.7, 1],
  bands: 3,
  sourceColors: true,
  fallbackColor: [0.5, 0.5, 0.5],
  background: "#f7f2df",
  transparentBackground: false,
  fuseSurfaces: false,
  showHlrOutline: true,
  showHlrDetail: false,
  showOutlines: true,
  showCreases: true,
  creaseAngleDegrees: 40,
  outlineColor: "#17252c",
  creaseColor: "#33444a",
  outlineWidth: 0.006,
  creaseWidth: 0.003,
  doubleSided: false,
  rimAmount: 0.12,
};

const toon = renderMeshIllustrationSvg(scene, baseStyle, "Cube toon proof");
assert.equal(toon.stats.triangles, 2);
assert.ok(toon.stats.outlines >= 4);
assert.match(toon.svg, /^<\?xml version="1\.0" encoding="UTF-8"\?>/u);
assert.match(toon.svg, /<polygon /u);
assert.match(toon.svg, /<path /u);
assert.match(toon.svg, /Cube toon proof/u);
assert.doesNotMatch(toon.svg, /(?:NaN|Infinity)/u);

const fusedCube = renderMeshIllustrationSvg(scene, {
  ...baseStyle,
  fuseSurfaces: true,
  showHlrOutline: false,
  showOutlines: false,
  showCreases: false,
});
assert.equal(fusedCube.stats.triangles, 2);
assert.equal(fusedCube.stats.surfaceDraws, 1);
assert.match(fusedCube.svg, /<path class="gms0" d="M/u);
assert.doesNotMatch(fusedCube.svg, /<polygon /u);
assert.ok(Buffer.byteLength(fusedCube.svg) < Buffer.byteLength(toon.svg));

const projectedOverlapScene = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "projected-overlap",
        positions: new Float64Array([
          0, 0, 0, 1, 0, 0, 0, 1, 0,
          0, 0, 1, 1, 0, 1, 0, 1, 1,
        ]),
        materials: [{ color: [0.2, 0.7, 0.62] }],
      },
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
);
const projectedOverlap = renderMeshIllustrationSvg(projectedOverlapScene, {
  ...baseStyle,
  fuseSurfaces: true,
  showHlrOutline: false,
  showOutlines: false,
  showCreases: false,
});
assert.equal(projectedOverlap.stats.surfaceDraws, 2);
assert.equal((projectedOverlap.svg.match(/<polygon /gu) ?? []).length, 2);

const interleavedScene = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "red-a",
        positions: new Float64Array([0, 0, 0, 1, 0, 0, 0, 1, 0]),
        materials: [{ color: [0.8, 0.1, 0.1] }],
      },
      {
        id: "blue-unrelated",
        positions: new Float64Array([2, 0, 0, 3, 0, 0, 2, 1, 0]),
        materials: [{ color: [0.1, 0.1, 0.8] }],
      },
      {
        id: "red-b",
        positions: new Float64Array([1, 0, 0, 1, 1, 0, 0, 1, 0]),
        materials: [{ color: [0.8, 0.1, 0.1] }],
      },
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
);
const interleaved = renderMeshIllustrationSvg(interleavedScene, {
  ...baseStyle,
  shading: "unlit",
  fuseSurfaces: true,
  showHlrOutline: false,
  showOutlines: false,
  showCreases: false,
});
assert.equal(interleaved.stats.triangles, 3);
assert.equal(interleaved.stats.surfaceDraws, 2);
assert.equal((interleaved.svg.match(/<path class="gms0"/gu) ?? []).length, 1);

const transparentCube = renderMeshIllustrationSvg(scene, {
  ...baseStyle,
  fuseSurfaces: true,
  showHlrOutline: false,
  showOutlines: false,
  showCreases: false,
  sourceColors: true,
});
const transparentScene = {
  ...scene,
  triangles: scene.triangles.map((triangle) => ({ ...triangle, opacity: 0.5 })),
};
const transparentResult = renderMeshIllustrationSvg(transparentScene, {
  ...baseStyle,
  fuseSurfaces: true,
  showHlrOutline: false,
  showOutlines: false,
  showCreases: false,
});
assert.equal(transparentCube.stats.surfaceDraws, 1);
assert.equal(transparentResult.stats.surfaceDraws, 2);
assert.match(transparentResult.svg, /opacity:0\.5/u);

const unlit = renderMeshIllustrationSvg(scene, {
  ...baseStyle,
  shading: "unlit",
  sourceColors: false,
  fallbackColor: [0.8, 0.3, 0.2],
  transparentBackground: true,
});
assert.notEqual(unlit.svg, toon.svg);
assert.doesNotMatch(unlit.svg, /<rect /u);
assert.match(unlit.svg, /rgb\(204,77,51\)/u);

const smoothNormalScene = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "smooth-normal-triangle",
        positions: new Float32Array([-1, -1, 0, 1, -1, 0, 0, 1, 0]),
        normals: new Float32Array([1, 0, 0, 1, 0, 0, 1, 0, 0]),
        materials: [{ color: [0.8, 0.6, 0.2] }],
      },
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
);
const surfaceOnlyStyle = {
  ...baseStyle,
  showHlrOutline: false,
  showOutlines: false,
  showCreases: false,
};
const flatDiffuse = renderMeshIllustrationSvg(smoothNormalScene, {
  ...surfaceOnlyStyle,
  shading: "flat",
});
const lambert = renderMeshIllustrationSvg(smoothNormalScene, {
  ...surfaceOnlyStyle,
  shading: "lambert",
});
assert.notEqual(lambert.svg, flatDiffuse.svg);

const band32 = renderMeshIllustrationSvg(smoothNormalScene, {
  ...surfaceOnlyStyle,
  shading: "banded",
  ambient: 0.49,
  keyIntensity: 0,
  bands: 32,
  sourceColors: false,
  fallbackColor: [1, 1, 1],
});
const band8 = renderMeshIllustrationSvg(smoothNormalScene, {
  ...surfaceOnlyStyle,
  shading: "banded",
  ambient: 0.49,
  keyIntensity: 0,
  bands: 8,
  sourceColors: false,
  fallbackColor: [1, 1, 1],
});
const bandOverLimit = renderMeshIllustrationSvg(smoothNormalScene, {
  ...surfaceOnlyStyle,
  shading: "banded",
  ambient: 0.49,
  keyIntensity: 0,
  bands: 100,
  sourceColors: false,
  fallbackColor: [1, 1, 1],
});
assert.notEqual(band32.svg, band8.svg);
assert.equal(bandOverLimit.svg, band32.svg);

const mirrored = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "mirrored-cube",
        positions,
        indices,
        normals: new Float32Array(Array.from({ length: 8 }, () => [0, 0, 1]).flat()),
        matrix: [-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 3, 0, 0, 1],
        materials: [{ color: [0.2, 0.7, 0.62] }],
      },
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
);
assert.equal(mirrored.triangles.filter((triangle) => triangle.frontFacing).length, 2);
assert.deepEqual(mirrored.bounds, { minX: 2, minY: -1, maxX: 4, maxY: 1 });
for (const triangle of mirrored.triangles)
  assert.ok(
    triangle.normal[2] > 0,
    `reflected source normal should retain its orientation: ${triangle.normal}`,
  );

const backFacingSingleSided = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "back-facing",
        positions: new Float32Array([0, 0, 0, 0, 1, 0, 1, 0, 0]),
        materials: [{ color: [0.4, 0.5, 0.6] }],
      },
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
);
assert.equal(renderMeshIllustrationSvg(backFacingSingleSided, baseStyle).stats.triangles, 0);
const backFacingDoubleSided = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "back-facing-double-sided",
        positions: new Float32Array([0, 0, 0, 0, 1, 0, 1, 0, 0]),
        materials: [{ color: [0.4, 0.5, 0.6] }],
        doubleSided: true,
      },
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
);
assert.equal(renderMeshIllustrationSvg(backFacingDoubleSided, baseStyle).stats.triangles, 1);

const tinyScene = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "micron-scale-triangle",
        positions: new Float64Array([0, 0, 0, 0.000002, 0, 0, 0, 0.000002, 0]),
        materials: [{ color: [0.4, 0.5, 0.6] }],
      },
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
  { weldTolerance: 1e-9 },
);
const tinySvg = renderMeshIllustrationSvg(tinyScene, { ...baseStyle, doubleSided: true }).svg;
assert.match(tinySvg, /viewBox="0 0 \d+ \d+"/u);
assert.doesNotMatch(tinySvg, /viewBox="0 0 0 0"/u);

// Average triangle depth gives the wrong painter order here: the large red
// triangle has a greater average depth, while the overlapping blue patch is
// closer at every point in their actual overlap.
const overlapScene = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "sloped-background",
        positions: new Float64Array([0, 0, 0, 10, 0, 100, 0, 10, 100]),
        materials: [{ color: [0.8, 0.1, 0.1] }],
      },
      {
        id: "near-patch",
        positions: new Float64Array([0.1, 0.1, 20, 1, 0.1, 20, 0.1, 1, 20]),
        materials: [{ color: [0.1, 0.1, 0.8] }],
      },
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
);
const overlapSvg = renderMeshIllustrationSvg(overlapScene, {
  ...baseStyle,
  shading: "unlit",
  showHlrOutline: false,
  showOutlines: false,
  showCreases: false,
});
const overlapPolygons = overlapSvg.svg.match(/<polygon [^>]+>/gu) ?? [];
assert.equal(overlapPolygons.length, 2);
assert.match(overlapSvg.svg, /\.gms0\{fill:rgb\(204,26,26\)/u);
assert.match(overlapSvg.svg, /\.gms1\{fill:rgb\(26,26,204\)/u);
assert.match(overlapPolygons[1], /class="gms1"/u);

const hlrOverlay = renderMeshIllustrationSvg(
  {
    ...scene,
    outlineSegments: [
      {
        points: [
          [-1, -1],
          [1, -1],
        ],
      },
    ],
    detailSegments: [
      {
        points: [
          [-0.5, 0],
          [0, 0],
        ],
      },
      {
        points: [
          [0, 0],
          [0.5, 0],
        ],
      },
    ],
  },
  {
    ...baseStyle,
    showOutlines: false,
    showCreases: false,
    showHlrOutline: true,
    showHlrDetail: true,
  },
);
assert.equal(hlrOverlay.stats.outlines, 1);
assert.equal(hlrOverlay.stats.details, 2);
assert.equal((hlrOverlay.svg.match(/<path /gu) ?? []).length, 2);
assert.match(hlrOverlay.svg, /<style>\.gms0\{/u);
assert.doesNotMatch(hlrOverlay.svg, /data-(?:surface|linework|triangles)=/u);

const sanitizedStyle = renderMeshIllustrationSvg(scene, {
  ...baseStyle,
  outlineColor: "red;}<script>alert(1)</script>",
});
assert.doesNotMatch(sanitizedStyle.svg, /<script>/u);

const gridColumns = 32;
const gridRows = 16;
const gridPositions = [];
for (let row = 0; row <= gridRows; row += 1)
  for (let column = 0; column <= gridColumns; column += 1)
    gridPositions.push(column, row, 0);
const gridIndices = [];
for (let row = 0; row < gridRows; row += 1) {
  for (let column = 0; column < gridColumns; column += 1) {
    const a = row * (gridColumns + 1) + column;
    const b = a + 1;
    const d = (row + 1) * (gridColumns + 1) + column;
    const c = d + 1;
    gridIndices.push(a, b, c, a, c, d);
  }
}
const pcbPlaneScene = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "pcb-plane-grid",
        positions: new Float64Array(gridPositions),
        indices: new Uint32Array(gridIndices),
        materials: [{ color: [0.1, 0.5, 0.2] }],
      },
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
);
const pcbPlane = renderMeshIllustrationSvg(pcbPlaneScene, {
  ...surfaceOnlyStyle,
  shading: "unlit",
  fuseSurfaces: true,
});
assert.equal(pcbPlane.stats.triangles, gridColumns * gridRows * 2);
assert.equal(pcbPlane.stats.surfaceDraws, 1);
assert.ok(Buffer.byteLength(pcbPlane.svg) < 1_000);

console.log(
  JSON.stringify({
    triangles: toon.stats.triangles,
    outlines: toon.stats.outlines,
    details: toon.stats.details,
    creases: toon.stats.creases,
    svgBytes: Buffer.byteLength(toon.svg),
  }),
);
