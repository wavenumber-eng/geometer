import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join, resolve } from "node:path";
import { illustrateMesh } from "../../dist/wasm/npm/geometer/mesh-illustration.js";
import { nativeIllustrationFixtures } from "./native_illustration_fixtures.mjs";

const executable = resolve(
  process.argv[2] ??
    process.env.GEOMETER_ILLUSTRATION_TEST ??
    `build/tests/cpp/geometer_mesh_illustration_test${process.platform === "win32" ? ".exe" : ""}`,
);
const temporary = mkdtempSync(join(tmpdir(), "geometer-illustration-parity-"));
const fixtures = nativeIllustrationFixtures();
function native(args) {
  const child = spawnSync(executable, args, {
    encoding: "utf8",
    maxBuffer: 64 * 1024 * 1024,
    timeout: 30000,
  });
  assert.equal(child.status, 0, child.error?.message ?? child.stderr);
  return JSON.parse(child.stdout);
}
const meshCollection = native([
  "--step",
  resolve("tests/fixtures/step/embedded_models/SOT-23.STEP"),
]);
for (const direction of [
  [0, 0, 1],
  [0.4, 0.7, 1],
  [-1, 0.3, -0.5],
]) {
  fixtures.push({
    name: `STEP-${direction.join("_")}`,
    input: {
      schema: "geometry.mesh_illustration.input.a0",
      meshes: meshCollection.meshes,
      view: { direction, up: [0, 1, 0] },
    },
  });
}
try {
  const documents = [];
  for (const { name, input } of fixtures) {
    const inputPath = join(temporary, `${name}.json`);
    writeFileSync(inputPath, JSON.stringify(input));
    const expected = illustrateMesh(input);
    const actual = native([inputPath]);
    writeFileSync(join(temporary, `${name}.expected.json`), JSON.stringify(expected, null, 2));
    writeFileSync(join(temporary, `${name}.actual.json`), JSON.stringify(actual, null, 2));
    assert.deepEqual(
      actual,
      expected,
      `${name}: native illustration differs from TypeScript baseline`,
    );
    assert.deepEqual(native([inputPath]), actual, `${name}: native output is not deterministic`);
    documents.push(actual.svg);
  }
  const python = process.env.GEOMETER_PARITY_PYTHON;
  const xml = spawnSync(
    python ?? "uv",
    [
      ...(python ? [] : ["run", "python"]),
      "-c",
      "import json,sys,xml.etree.ElementTree as E; [E.fromstring(s) for s in json.load(sys.stdin)]",
    ],
    { input: JSON.stringify(documents), encoding: "utf8", timeout: 30000 },
  );
  assert.equal(xml.status, 0, xml.error?.message ?? xml.stderr);
  for (const title of ["bad\u0001title", "bad\ufffetitle", "bad\ud800title"]) {
    assert.throws(
      () => illustrateMesh({ ...fixtures[0].input, svg: { title } }),
      /invalid XML character/u,
    );
  }
  console.log(
    `Native illustration: ${fixtures.length} exact A0/SVG parity and determinism cases passed.`,
  );
  rmSync(temporary, { recursive: true });
} catch (error) {
  console.error(`Parity fixtures and results retained at ${temporary}`);
  throw error;
}
