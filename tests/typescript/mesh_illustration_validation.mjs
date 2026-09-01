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

const mirrored = prepareMeshIllustration(
  {
    meshes: [
      {
        id: "mirrored-cube",
        positions,
        indices,
        matrix: [-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 3, 0, 0, 1],
        materials: [{ color: [0.2, 0.7, 0.62] }],
      },
    ],
  },
  { direction: [0, 0, 1], up: [0, 1, 0] },
);
assert.equal(mirrored.triangles.filter((triangle) => triangle.frontFacing).length, 2);
assert.deepEqual(mirrored.bounds, { minX: 2, minY: -1, maxX: 4, maxY: 1 });

console.log(
  JSON.stringify({
    triangles: toon.stats.triangles,
    outlines: toon.stats.outlines,
    creases: toon.stats.creases,
    svgBytes: Buffer.byteLength(toon.svg),
  }),
);
