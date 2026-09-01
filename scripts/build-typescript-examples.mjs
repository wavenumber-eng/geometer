// @ts-check

import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { mkdir, readFile, rename, rm } from "node:fs/promises";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const outputRoot = join(root, "dist", "wasm", "demos");
const stagingRoot = join(root, "dist", "wasm", `.typescript-demo-stage-${process.pid}`);
const outputs = [
  "analytic_canvas_arc.js",
  "analytic_polygon_pour_bootstrap_guard.js",
  "analytic_polygon_pour_demo.js",
  "analytic_polygon_pour_fixture.js",
  "analytic_polygon_pour_worker.js",
  "demo-tooling/animation.js",
  "demo-tooling/camera2d.js",
  "demo-tooling/commands.js",
  "demo-tooling/geometry.js",
  "demo-tooling/history.js",
  "demo-tooling/index.js",
  "demo-tooling/input.js",
  "demo-tooling/panels.js",
  "demo-tooling/tool-controller.js",
  "illustration_demo.js",
  "mesh_illustration.js",
  "model_bounds_demo.js",
  "model_bounds_worker.js",
  "pcb_polygon_pour_demo.js",
  "pcb_polygon_pour_model.js",
  "pcb_polygon_pour_worker.js",
];
const checkOnly = process.argv.slice(2).includes("--check");

if (process.argv.slice(2).some((argument) => argument !== "--check")) {
  throw new Error("Usage: node scripts/build-typescript-examples.mjs [--check]");
}

await rm(stagingRoot, { recursive: true, force: true });
await mkdir(stagingRoot, { recursive: true });
try {
  const compiler = join(root, "node_modules", "typescript", "bin", "tsc");
  const result = spawnSync(
    process.execPath,
    [
      compiler,
      "--project",
      join(root, "examples", "wasm", "tsconfig.json"),
      "--outDir",
      stagingRoot,
    ],
    { cwd: root, encoding: "utf8" },
  );
  if (result.error) throw result.error;
  if (result.status !== 0) {
    throw new Error(
      `TypeScript example compilation failed.\n${result.stdout}${result.stderr}`.trimEnd(),
    );
  }
  for (const filename of outputs) {
    const stagedOutput = join(stagingRoot, filename);
    if (!existsSync(stagedOutput)) throw new Error(`TypeScript example did not emit ${filename}.`);
    if ((await readFile(stagedOutput, "utf8")).includes("\r"))
      throw new Error(`TypeScript example ${filename} is not LF-normalized.`);
  }
  if (checkOnly) {
    for (const filename of outputs) {
      const output = join(outputRoot, filename);
      const stagedOutput = join(stagingRoot, filename);
      if (
        !existsSync(output) ||
        (await readFile(stagedOutput)).compare(await readFile(output)) !== 0
      ) {
        throw new Error(`TypeScript example ${filename} is stale. Run npm run generate:contracts.`);
      }
    }
    process.stdout.write("TypeScript examples are current.\n");
  } else {
    await mkdir(outputRoot, { recursive: true });
    for (const filename of outputs) {
      const output = join(outputRoot, filename);
      await mkdir(dirname(output), { recursive: true });
      await rm(output, { force: true });
      await rename(join(stagingRoot, filename), output);
    }
    process.stdout.write("Built TypeScript browser examples.\n");
  }
} finally {
  await rm(stagingRoot, { recursive: true, force: true });
}
