// @ts-check

import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { mkdir, readdir, readFile, rename, rm, writeFile } from "node:fs/promises";
import { dirname, join, relative, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const source = join(root, "src", "ts", "geometer");
const output = join(root, "dist", "npm", "geometer");
const staging = join(root, "dist", "npm", `.geometer-stage-${process.pid}`);
const backup = join(root, "dist", "npm", `.geometer-backup-${process.pid}`);
const checkOnly = process.argv.slice(2).includes("--check");

if (process.argv.slice(2).some((argument) => argument !== "--check")) {
  throw new Error("Usage: node scripts/build-typescript-package.mjs [--check]");
}

await rm(staging, { recursive: true, force: true });
await rm(backup, { recursive: true, force: true });
await mkdir(staging, { recursive: true });

try {
  compilePackage();
  await copyText("package.json");
  await copyText("README.md");
  await validatePackage();
  if (checkOnly) {
    const differences = await compareDirectories(staging, output);
    if (differences.length) {
      throw new Error(
        `TypeScript package artifact is stale:\n${differences.map((item) => `  - ${item}`).join("\n")}\nRun npm run generate:contracts.`,
      );
    }
    process.stdout.write("TypeScript package artifact is current.\n");
  } else {
    await installPackage();
    process.stdout.write("Built dist/npm/geometer.\n");
  }
} finally {
  await rm(staging, { recursive: true, force: true });
  await rm(backup, { recursive: true, force: true });
}

function compilePackage() {
  const compiler = join(root, "node_modules", "typescript", "bin", "tsc");
  const result = spawnSync(
    process.execPath,
    [compiler, "--project", join(source, "tsconfig.json"), "--outDir", staging],
    { cwd: root, encoding: "utf8" },
  );
  if (result.error) throw result.error;
  if (result.status !== 0) {
    throw new Error(
      `TypeScript package compilation failed.\n${result.stdout}${result.stderr}`.trimEnd(),
    );
  }
}

async function copyText(filename) {
  const content = await readFile(join(source, filename), "utf8");
  await writeFile(join(staging, filename), content.replaceAll("\r\n", "\n"), "utf8");
}

async function validatePackage() {
  const packageJson = JSON.parse(await readFile(join(staging, "package.json"), "utf8"));
  if (packageJson.name !== "@wavenumber/geometer" || packageJson.type !== "module") {
    throw new Error("Unexpected TypeScript package identity or module format.");
  }
  for (const required of [
    "index.js",
    "index.d.ts",
    "wasm.js",
    "wasm.d.ts",
    "generated/index.js",
    "generated/index.d.ts",
    "package.json",
    "README.md",
  ]) {
    if (!existsSync(join(staging, required))) throw new Error(`Package is missing ${required}.`);
  }
  for (const path of await listFiles(staging)) {
    const bytes = await readFile(join(staging, path));
    const text = bytes.toString("utf8");
    if (Buffer.from(text, "utf8").compare(bytes) !== 0 || text.includes("\r")) {
      throw new Error(`Package file ${path} is not normalized UTF-8/LF text.`);
    }
    if (!text.endsWith("\n") || text.endsWith("\n\n")) {
      throw new Error(`Package file ${path} must end with exactly one newline.`);
    }
    if (text.includes(root) || text.includes(root.replaceAll("\\", "/"))) {
      throw new Error(`Package file ${path} contains a host path.`);
    }
  }
}

async function installPackage() {
  let movedExisting = false;
  try {
    await mkdir(dirname(output), { recursive: true });
    if (existsSync(output)) {
      await rename(output, backup);
      movedExisting = true;
    }
    await rename(staging, output);
    await rm(backup, { recursive: true, force: true });
  } catch (error) {
    if (!existsSync(output) && movedExisting && existsSync(backup)) await rename(backup, output);
    throw error;
  }
}

async function compareDirectories(actualRoot, expectedRoot) {
  if (!existsSync(expectedRoot)) return [`missing ${repositoryRelative(expectedRoot)}`];
  const actualFiles = await listFiles(actualRoot);
  const expectedFiles = await listFiles(expectedRoot);
  const actualSet = new Set(actualFiles);
  const expectedSet = new Set(expectedFiles);
  const differences = [];
  for (const path of actualFiles) {
    if (!expectedSet.has(path))
      differences.push(`missing ${repositoryRelative(join(expectedRoot, path))}`);
    else if (
      (await readFile(join(actualRoot, path))).compare(await readFile(join(expectedRoot, path))) !==
      0
    )
      differences.push(`stale ${repositoryRelative(join(expectedRoot, path))}`);
  }
  for (const path of expectedFiles) {
    if (!actualSet.has(path))
      differences.push(`unexpected ${repositoryRelative(join(expectedRoot, path))}`);
  }
  return differences.sort();
}

async function listFiles(directory) {
  const outputFiles = [];
  async function visit(current) {
    const entries = await readdir(current, { withFileTypes: true });
    entries.sort((left, right) => left.name.localeCompare(right.name));
    for (const entry of entries) {
      const absolute = join(current, entry.name);
      if (entry.isDirectory()) await visit(absolute);
      else if (entry.isFile()) outputFiles.push(relative(directory, absolute).split(sep).join("/"));
      else throw new Error(`Unsupported package entry ${absolute}.`);
    }
  }
  await visit(directory);
  return outputFiles;
}

function repositoryRelative(path) {
  return relative(root, path).split(sep).join("/");
}
