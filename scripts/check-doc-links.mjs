import { existsSync, readdirSync, readFileSync } from "node:fs";
import { dirname, extname, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const failures = [];
let checked = 0;
function visit(directory) {
  for (const entry of readdirSync(directory, { withFileTypes: true })) {
    if (entry.name.startsWith(".") || entry.name === "node_modules") continue;
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
visit(resolve(root, "examples"));
const inventory = JSON.parse(
  readFileSync(resolve(root, "docs/developer/documentation-map.json"), "utf8"),
);
for (const document of inventory.documents) {
  if (!existsSync(resolve(root, document.destination)))
    failures.push(`Missing disposition target: ${document.destination}`);
}
// Govern entrypoint conventions, not every browser worker or support module.
const demoAudit = readFileSync(resolve(root, "docs/developer/demo-status.md"), "utf8");
let demos = 0;
for (const [directory, suffixes] of [
  ["examples/wasm", [".html"]],
  ["examples/python", [".py"]],
  ["examples/cpp", [".cpp"]],
  ["examples/node", [".ts", ".mjs"]],
]) {
  for (const entry of readdirSync(resolve(root, directory), { withFileTypes: true })) {
    if (!entry.isFile() || !suffixes.includes(extname(entry.name))) continue;
    const path = `${directory}/${entry.name}`;
    demos += 1;
    if (!demoAudit.includes(`](../../${path})`)) failures.push(`Demo lacks audit entry: ${path}`);
  }
}
if (failures.length) throw new Error(`Broken documentation file links:\n${failures.join("\n")}`);
console.log(
  `Documentation current: ${checked} file links; disposition targets present; ${demos} demo entrypoints registered.`,
);
