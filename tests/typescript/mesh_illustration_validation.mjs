import assert from "node:assert/strict";
import { createFastHlrIllustrator } from "../../dist/wasm/npm/geometer/illustrated-hlr.js";
import {
  createIllustrator,
  illustrateMesh,
  prepareMeshIllustration,
  renderMeshIllustrationSvg,
  toMeshIllustrationStyleA0,
} from "../../dist/wasm/npm/geometer/mesh-illustration.js";

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

assert.equal(scene.stats.sourceMeshes, 1);
assert.equal(scene.stats.sourceTriangles, 12);
assert.equal(scene.stats.projectedTriangles, 12);
assert.equal(scene.warnings.length, 0);

const governedInput = {
  schema: "geometry.mesh_illustration.input.a0",
  meshes: [
    {
      id: "governed-cube",
      positions: [...positions],
      indices: [...indices],
      materials: [{ color: [0.2, 0.7, 0.62] }],
    },
  ],
  view: { direction: [0, 0, 1], up: [0, 1, 0] },
  style: { shading: "toon", show_hlr_detail: false },
  svg: { title: "Governed cube" },
};
const oneShot = illustrateMesh(governedInput);
assert.equal(oneShot.schema, "geometry.mesh_illustration.result.a0");
assert.match(oneShot.svg, /<metadata>geometry\.mesh_illustration\.result\.a0<\/metadata>/u);
assert.match(oneShot.svg, /Governed cube/u);
const reusable = createIllustrator(governedInput);
assert.equal(reusable.renderSvg({ shading: "unlit" }).schema, oneShot.schema);
reusable.dispose();
assert.equal(reusable.disposed, true);
assert.throws(() => reusable.renderSvg(), /disposed/u);

let observedHlrOptions;
const composed = await createFastHlrIllustrator(
  {
    async meshHlrProjection(request) {
      observedHlrOptions = request.options;
      return {
        views: [
          {
            modes: {
              outline: { segments: [] },
              detail: { segments: [] },
            },
          },
        ],
      };
    },
  },
  {
    illustration: governedInput,
    hlr: {
      output_detail: false,
      model_transform: [2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1],
      strip_root_placement: true,
      unknown_option: "discard me",
    },
  },
);
assert.equal(observedHlrOptions.output_detail, false);
assert.equal(observedHlrOptions.projection_algorithm, "fast");
assert.equal(observedHlrOptions.outline_algorithm, "fast-mesh-shadow");
assert.equal("model_transform" in observedHlrOptions, false);
assert.equal("strip_root_placement" in observedHlrOptions, false);
assert.equal("unknown_option" in observedHlrOptions, false);
composed.illustrator.dispose();
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
  layerCoplanarMaterials: false,
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
const governedStyle = toMeshIllustrationStyleA0(baseStyle);
assert.equal(governedStyle.key_intensity, baseStyle.keyIntensity);
assert.deepEqual(governedStyle.light_direction, baseStyle.lightDirection);
assert.equal(governedStyle.layer_coplanar_materials, baseStyle.layerCoplanarMaterials);
assert.equal(governedStyle.crease_angle_degrees, baseStyle.creaseAngleDegrees);
assert.equal("keyIntensity" in governedStyle, false);

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
        positions: new Float64Array([0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1]),
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

const coplanarMeshes = [
  {
    id: "white-inlay",
    positions: new Float64Array([1, 1, 0, 3, 1, 0, 3, 2, 0, 1, 1, 0, 3, 2, 0, 1, 2, 0]),
    materials: [{ color: [1, 1, 1] }],
  },
];
for (const [x0, x1] of [
  [0, 1],
  [1, 3],
  [3, 4],
]) {
  for (const [y0, y1] of [
    [0, 1],
    [1, 2],
    [2, 3],
  ]) {
    if (x0 === 1 && x1 === 3 && y0 === 1 && y1 === 2) continue;
    coplanarMeshes.push({
      id: `black-base-${x0}-${y0}`,
      positions: new Float64Array([
        x0,
        y0,
        0,
        x1,
        y0,
        0,
        x1,
        y1,
        0,
        x0,
        y0,
        0,
        x1,
        y1,
        0,
        x0,
        y1,
        0,
      ]),
      materials: [{ color: [0.05, 0.05, 0.05] }],
    });
  }
}
const coplanarScene = prepareMeshIllustration(
  { meshes: coplanarMeshes },
  { direction: [0.4, 0.3, 1], up: [0, 1, 0] },
);
const coplanarStyle = {
  ...baseStyle,
  shading: "unlit",
  fuseSurfaces: true,
  layerCoplanarMaterials: true,
  showHlrOutline: false,
  showOutlines: false,
  showCreases: false,
};
const coplanarLayered = renderMeshIllustrationSvg(coplanarScene, coplanarStyle);
assert.equal(coplanarLayered.stats.layeredSurfaces, 1);
assert.equal(coplanarLayered.stats.surfaceDraws, 2);
const coplanarPaths = [...coplanarLayered.svg.matchAll(/<path class="(gms\d+)" d="([^"]+)"/gu)];
assert.equal(coplanarPaths.length, 2);
assert.equal((coplanarPaths[0]?.[2].match(/M/gu) ?? []).length, 1);
assert.match(coplanarLayered.svg, /\.gms0\{fill:rgb\(13,13,13\)/u);
assert.match(coplanarLayered.svg, /\.gms1\{fill:rgb\(255,255,255\)/u);
const coplanarUnlayered = renderMeshIllustrationSvg(coplanarScene, {
  ...coplanarStyle,
  layerCoplanarMaterials: false,
});
assert.equal(coplanarUnlayered.stats.layeredSurfaces, 0);
const transparentCoplanar = renderMeshIllustrationSvg(
  {
    ...coplanarScene,
    triangles: coplanarScene.triangles.map((triangle) => ({ ...triangle, opacity: 0.5 })),
  },
  coplanarStyle,
);
assert.equal(transparentCoplanar.stats.layeredSurfaces, 0);
const foldedCoplanarCandidate = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "flat-material-a",
        positions: new Float64Array([0, 0, 0, 1, 0, 0, 0, 1, 0]),
        materials: [{ color: [0.05, 0.05, 0.05] }],
      },
      {
        id: "tilted-material-b",
        positions: new Float64Array([1, 0, 0, 1, 1, 0.02, 0, 1, 0]),
        materials: [{ color: [1, 1, 1] }],
      },
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
);
const foldedCoplanarResult = renderMeshIllustrationSvg(foldedCoplanarCandidate, coplanarStyle);
assert.equal(foldedCoplanarResult.stats.layeredSurfaces, 0);
const varyingNormalScene = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "one-material-normal-a",
        positions: new Float64Array([0, 0, 0, 1, 0, 0, 0, 1, 0]),
        normals: new Float64Array([0, 0, 1, 0, 0, 1, 0, 0, 1]),
        materials: [{ color: [0.6, 0.6, 0.6] }],
      },
      {
        id: "one-material-normal-b",
        positions: new Float64Array([1, 0, 0, 1, 1, 0, 0, 1, 0]),
        normals: new Float64Array([0, 1, 0, 0, 1, 0, 0, 1, 0]),
        materials: [{ color: [0.6, 0.6, 0.6] }],
      },
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
);
const varyingNormalResult = renderMeshIllustrationSvg(varyingNormalScene, {
  ...coplanarStyle,
  shading: "lambert",
});
assert.equal(varyingNormalResult.stats.layeredSurfaces, 0);
const occludedCoplanarScene = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "front-occluder",
        positions: new Float64Array([1.5, 0.5, 1, 3.5, 0.5, 1, 2.5, 2.5, 1]),
        materials: [{ color: [0.1, 0.2, 0.8] }],
      },
      ...coplanarMeshes,
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
);
const occludedCoplanar = renderMeshIllustrationSvg(occludedCoplanarScene, coplanarStyle);
assert.equal(occludedCoplanar.stats.layeredSurfaces, 1);
assert.equal(occludedCoplanar.stats.surfaceDraws, 3);
assert.ok(occludedCoplanar.svg.lastIndexOf("<polygon") > occludedCoplanar.svg.lastIndexOf("<path"));

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
  background: "red;}<script>alert(2)</script>",
});
assert.doesNotMatch(sanitizedStyle.svg, /<script>/u);

const warningBoundInput = {
  schema: "geometry.mesh_illustration.input.a0",
  meshes: [
    {
      id: "degenerate-warning-bound",
      positions: Array.from({ length: 300 * 9 }, () => 0),
      materials: [{ color: [0.5, 0.5, 0.5] }],
    },
  ],
  view: { direction: [0, 0, 1], up: [0, 1, 0] },
};
const boundedWarnings = illustrateMesh(warningBoundInput).warnings;
assert.equal(boundedWarnings.length, 256);
assert.match(boundedWarnings.at(-1), /additional warnings suppressed/u);

const gridColumns = 32;
const gridRows = 16;
const gridPositions = [];
for (let row = 0; row <= gridRows; row += 1)
  for (let column = 0; column <= gridColumns; column += 1) gridPositions.push(column, row, 0);
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
