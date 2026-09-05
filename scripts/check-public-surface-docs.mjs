import { createHash } from "node:crypto";
import { readdir, readFile, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const lockPath = resolve(root, "docs/contracts/public-surface-review.json");
const args = process.argv.slice(2);
if (args.some((arg) => arg !== "--refresh"))
  throw new Error("Use --refresh only after reviewing public interface documentation.");
const paths = [
  "src/cpp/lib/geometer.h",
  "src/cpp/lib/CMakeLists.txt",
  "src/cpp/lib/operation_registry.cpp",
  "src/cpp/lib/native_topology_operation_registry.cpp",
  "src/cpp/lib/model_bounds_options_json.cpp",
  "src/cpp/lib/projection_options_json.cpp",
  "src/cpp/lib/step_to_glb_options_json.cpp",
  "src/cpp/cli/CMakeLists.txt",
  "src/ts/geometer/package.json",
];
async function collect(directory, suffix) {
  for (const item of await readdir(resolve(root, directory), { withFileTypes: true })) {
    if (item.name === "generated" || item.name === "_generated" || item.name.startsWith("."))
      continue;
    const path = `${directory}/${item.name}`;
    if (item.isDirectory() && item.name !== "__pycache__") await collect(path, suffix);
    else if (item.isFile() && item.name.endsWith(suffix)) paths.push(path);
  }
}
await collect("src/cpp/lib/geometer", ".h");
await collect("src/cpp/cli", ".h");
await collect("src/cpp/cli", ".cpp");
await collect("src/ts/geometer", ".ts");
await collect("src/rust/geometer-client/src", ".rs");
await collect("python/geometer", ".py");
paths.sort();
const files = [];
for (const path of paths) {
  const text = (await readFile(resolve(root, path), "utf8")).replaceAll("\r\n", "\n");
  files.push({ path, sha256: createHash("sha256").update(text).digest("hex") });
}
const inventory = {
  schema: "wn.geometer.public_surface_documentation_review.a0",
  reference: "docs/contracts/public-entrypoints.md",
  files,
};
const rendered = `${JSON.stringify(inventory, null, 2)}\n`;
if (args.includes("--refresh")) {
  await writeFile(lockPath, rendered);
  console.log(
    "Refreshed public-source review lock; review and commit it with interface documentation.",
  );
} else {
  const recorded = await readFile(lockPath, "utf8");
  if (recorded.replaceAll("\r\n", "\n") !== rendered)
    throw new Error(
      "Public source inventory changed. Review docs/contracts/public-entrypoints.md and normative references, then explicitly refresh the review lock.",
    );
  console.log(`Public-source documentation review current: ${files.length} files.`);
}
