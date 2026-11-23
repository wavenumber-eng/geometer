import { existsSync, readdirSync, readFileSync } from "node:fs";
import { dirname, extname, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const failures = [];
let checked = 0;
function visit(directory) {
  for (const entry of readdirSync(directory, { withFileTypes: true })) {
    if (entry.name.startsWith(".") || ["node_modules", "target"].includes(entry.name)) continue;
    const path = resolve(directory, entry.name);
    if (entry.isDirectory()) visit(path);
    else if (extname(path) === ".md") check(path);
  }
}
function check(path) {
  const text = readFileSync(path, "utf8").replace(/^```[^\n]*\n[\s\S]*?^```[^\n]*$/gm, "");
  for (const match of text.matchAll(/\]\(([^\s)]+)(?:\s+"[^"]*")?\)/g)) {
    const target = match[1];
    if (/^(?:[a-z]+:|\/|#)/i.test(target)) continue;
    const file = decodeURIComponent(target.split("#")[0]);
    checked += 1;
    if (!existsSync(resolve(dirname(path), file)))
      failures.push(`${relative(root, path)}: ${target}`);
  }
}
check(resolve(root, "README.md"));
visit(resolve(root, "docs"));
const examplesRoot = resolve(root, "examples");
visit(examplesRoot);
const examplesIndex = readFileSync(resolve(examplesRoot, "README.md"), "utf8");
let demos = 0;
for (const [directory, suffixes] of [
  ["wasm", [".html"]],
  ["python", [".py"]],
  ["cpp", [".cpp"]],
  ["node", [".ts", ".mjs"]],
]) {
  for (const entry of readdirSync(resolve(examplesRoot, directory), { withFileTypes: true })) {
    if (!entry.isFile() || !suffixes.includes(extname(entry.name))) continue;
    const path = `${directory}/${entry.name}`;
    demos += 1;
    if (!examplesIndex.includes(`](${path})`))
      failures.push(`Demo lacks examples index entry: ${path}`);
  }
}
for (const entry of readdirSync(resolve(examplesRoot, "rust"), { withFileTypes: true })) {
  if (!entry.isDirectory()) continue;
  const path = `rust/${entry.name}/src/main.rs`;
  if (!existsSync(resolve(examplesRoot, path))) continue;
  demos += 1;
  if (!examplesIndex.includes(`](${path})`))
    failures.push(`Demo lacks examples index entry: ${path}`);
}
if (failures.length) throw new Error(`Broken documentation file links:\n${failures.join("\n")}`);
console.log(`Documentation current: ${checked} file links; ${demos} demo entrypoints registered.`);
