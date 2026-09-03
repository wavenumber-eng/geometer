import assert from "node:assert/strict";

import * as THREE from "three";

import { RasterHlrModel } from "../../dist/wasm/npm/geometer/raster-hlr.js";

const cubeRoot = new THREE.Group();
cubeRoot.add(new THREE.Mesh(new THREE.BoxGeometry(2, 2, 2), new THREE.MeshBasicMaterial()));
const cube = new RasterHlrModel(cubeRoot, { creaseAngleDegrees: 25 });
assert.equal(cube.stats.meshes, 1);
assert.equal(cube.stats.triangles, 12);
assert.equal(cube.stats.candidateEdges, 12);
assert.ok(cube.stats.buildMs >= 0);

let generatedLines = 0;
cube.root.traverse((node) => {
  if (node instanceof THREE.LineSegments && node.name === "geometer-fast-hlr-edges")
    generatedLines += 1;
});
assert.equal(generatedLines, 1);
cube.dispose();

// Geometer's present GLB path can duplicate vertices at face boundaries. The
// prototype must join coincident face-local endpoints rather than expose the
// diagonal between these two coplanar, non-indexed triangles.
const splitPlane = new THREE.BufferGeometry();
splitPlane.setAttribute(
  "position",
  new THREE.Float32BufferAttribute([0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0], 3),
);
const planeRoot = new THREE.Group();
planeRoot.add(new THREE.Mesh(splitPlane, new THREE.MeshBasicMaterial()));
const plane = new RasterHlrModel(planeRoot, { creaseAngleDegrees: 25 });
assert.equal(plane.stats.triangles, 2);
assert.equal(plane.stats.candidateEdges, 4);
plane.dispose();

console.log(JSON.stringify({ cube: cube.stats, splitPlane: plane.stats }));
