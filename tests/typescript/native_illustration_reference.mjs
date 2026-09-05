// Test-only oracle: native runtime consumers never load JavaScript.
import { readFileSync } from "node:fs";
import { illustrateMesh } from "../../dist/wasm/npm/geometer/mesh-illustration.js";

process.stdout.write(JSON.stringify(illustrateMesh(JSON.parse(readFileSync(0, "utf8")))));
