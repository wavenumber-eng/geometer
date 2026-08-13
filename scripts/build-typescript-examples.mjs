// @ts-check

import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { mkdir, readFile, rename, rm } from "node:fs/promises";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const output = join(root, "dist", "wasm", "demos", "model_bounds_demo.js");
const stagingRoot = join(root, "dist", "wasm", `.typescript-demo-stage-${process.pid}`);
const stagedOutput = join(stagingRoot, "model_bounds_demo.js");
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
  if (!existsSync(stagedOutput))
    throw new Error("TypeScript example did not emit model_bounds_demo.js.");
  const staged = await readFile(stagedOutput);
  if (staged.toString("utf8").includes("\r"))
    throw new Error("TypeScript example is not LF-normalized.");
  if (checkOnly) {
    if (!existsSync(output) || staged.compare(await readFile(output)) !== 0) {
      throw new Error("TypeScript model-bounds example is stale. Run npm run generate:contracts.");
    }
    process.stdout.write("TypeScript model-bounds example is current.\n");
  } else {
    await mkdir(dirname(output), { recursive: true });
    await rm(output, { force: true });
    await rename(stagedOutput, output);
    process.stdout.write("Built TypeScript model-bounds browser example.\n");
  }
} finally {
  await rm(stagingRoot, { recursive: true, force: true });
}
