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
  const numericValues = [
    0, -0, 1e23, -1e23, 1e21, 1e20, 1e-6, -1e-6, 1e-7,
    Number.MIN_VALUE, -Number.MIN_VALUE, Number.MAX_VALUE, -Number.MAX_VALUE,
    0.5001220703125, -0.5001220703125, 1000000000000000100, -0.5,
  ];
  let numericSeed = 12345;
  for (let i = 0; i < 300; ++i) {
    numericSeed = (Math.imul(numericSeed, 1664525) + 1013904223) >>> 0;
    numericValues.push((numericSeed / 2 ** 32 - 0.5) * 10 ** (((i * 37) % 601) - 300));
  }
  const numericPath = join(temporary, "numbers.json");
  const bits = new DataView(new ArrayBuffer(8));
  for (let exponent = -1074; exponent <= 1023; ++exponent) {
    bits.setFloat64(0, 2 ** exponent);
    const center = bits.getBigUint64(0);
    for (const offset of [-1n, 0n, 1n]) {
      bits.setBigUint64(0, center + offset);
      const value = bits.getFloat64(0);
      if (Number.isFinite(value)) numericValues.push(value);
    }
  }
  writeFileSync(numericPath, JSON.stringify(numericValues));
  const formatted = native(["--numbers", numericPath]);
  numericValues.forEach((value, index) => {
    const rounded = Number(value.toPrecision(12));
    assert.deepEqual(formatted[index], [
      Number.isFinite(rounded) ? String(rounded) : "overflow",
      String(Math.round(value)),
    ], `numeric format ${value}`);
  });
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
