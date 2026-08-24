import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join, resolve } from "node:path";

const root = resolve(import.meta.dirname, "..", "..");
const output = mkdtempSync(join(tmpdir(), "geometer-emitter-validation-"));
try {
  const result = spawnSync(
    process.execPath,
    [
      join(root, "node_modules", "@typespec", "compiler", "cmd", "tsp.js"),
      "compile",
      join(root, "tests", "typescript", "native_experimental_fail_closed.tsp"),
      "--emit",
      "@wavenumber/wn-geometer-contract-emitter",
      "--output-dir",
      output,
    ],
    { cwd: root, encoding: "utf8" },
  );
  assert.notEqual(result.status, 0, "unpaired native-only marker must fail compilation");
  assert.match(
    `${result.stdout}${result.stderr}`,
    /uses @nativeExperimentalOperation without @experimentalOperation/,
  );
  process.stdout.write(JSON.stringify({ nativeExperimentalFailClosed: true }));
} finally {
  rmSync(output, { force: true, recursive: true });
}
