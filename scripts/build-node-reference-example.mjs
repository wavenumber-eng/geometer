// @ts-check

import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { mkdir, readFile, rename, rm } from "node:fs/promises";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import { build } from "esbuild";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const output = join(root, "dist", "native", "examples", "step-topology-annotation-reference.mjs");
const stagingRoot = join(root, "dist", "native", `.node-reference-stage-${process.pid}`);
const stagedOutput = join(stagingRoot, "step-topology-annotation-reference.mjs");
const checkOnly = process.argv.slice(2).includes("--check");

if (process.argv.slice(2).some((argument) => argument !== "--check")) {
  throw new Error("Usage: node scripts/build-node-reference-example.mjs [--check]");
}

await rm(stagingRoot, { recursive: true, force: true });
await mkdir(stagingRoot, { recursive: true });
try {
  const compiler = join(root, "node_modules", "typescript", "bin", "tsc");
  const typecheck = spawnSync(
    process.execPath,
    [compiler, "--project", join(root, "examples", "node", "tsconfig.json")],
    { cwd: root, encoding: "utf8" },
  );
  if (typecheck.error) throw typecheck.error;
  if (typecheck.status !== 0) {
    throw new Error(
      `Node reference TypeScript compilation failed.\n${typecheck.stdout}${typecheck.stderr}`.trimEnd(),
    );
  }

  await build({
    bundle: true,
    charset: "utf8",
    entryPoints: [join(root, "examples", "node", "step_topology_annotation_reference.ts")],
    format: "esm",
    legalComments: "none",
    logLevel: "silent",
    outfile: stagedOutput,
    platform: "node",
    target: "node24",
  });
  if ((await readFile(stagedOutput, "utf8")).includes("\r")) {
    throw new Error("Bundled Node reference example is not LF-normalized.");
  }

  if (checkOnly) {
    if (
      !existsSync(output) ||
      (await readFile(stagedOutput)).compare(await readFile(output)) !== 0
    ) {
      throw new Error("Node reference example is stale. Run npm run generate:contracts.");
    }
    process.stdout.write("Node reference example is current.\n");
  } else {
    await mkdir(dirname(output), { recursive: true });
    await rm(output, { force: true });
    await rename(stagedOutput, output);
    process.stdout.write("Built Node STEP topology annotation reference.\n");
  }
} finally {
  await rm(stagingRoot, { recursive: true, force: true });
}
