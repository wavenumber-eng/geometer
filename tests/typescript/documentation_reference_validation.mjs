import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { maturityOf, runtimeOf } from "../../scripts/contract-docs-coverage.mjs";

assert.equal(
  runtimeOf({ runtime_available: true, native_runtime_available: false }),
  "Portable and native IPC",
);
assert.equal(
  runtimeOf({ runtime_available: false, native_runtime_available: true }),
  "Native IPC only",
);
assert.equal(
  runtimeOf({ runtime_available: false, native_runtime_available: false }),
  "Structural only",
);
assert.match(runtimeOf(undefined), /no generic IPC adapter/);
assert.equal(
  maturityOf({ id: "geometry.analytic_planar_boolean_batch.a0", status: "promoted" }),
  "experimental_not_production_ready",
);
assert.equal(
  maturityOf({ id: "future.operation", maturity: "explicit_maturity" }),
  "explicit_maturity",
);
const reference = (path) =>
  readFileSync(new URL(`../../docs/generated/contracts/${path}`, import.meta.url), "utf8");
const bounds = reference("operations/geometry-model-bounds-a0.html");
assert.match(bounds, /Native executable runtime<\/th><td><code>true<\/code>/);
const analytic = reference("operations/geometry-analytic-planar-boolean-batch-a0.html");
assert.match(analytic, /experimental_not_production_ready/);
const coverage = reference("coverage.html");
assert.match(coverage, /geometry\.clipper2_boolean\.a0/);
assert.match(coverage, /No generated operation \/ no generic IPC adapter/);
assert.match(coverage, /GMIMSH01/);
assert.match(coverage, /GMC2BQ01/);
assert.match(coverage, /maximum_vertices: 2000000/);
const guide = reference("guides/docs/design/executable-ipc.html");
assert.match(guide, /data-authority="authored-markdown"/);
assert.match(guide, /<header><h1 id="calling-the-geometer-executable">/);
assert.match(guide, /id="runnable-model-bounds-example"/);
assert.match(guide, /Documentation index/);
assert.match(guide, /Edit Markdown source <code>docs\/design\/executable-ipc\.md<\/code>/);
const adrGuide = reference(
  "guides/docs/geometer/adr/geometer-adr-001-cmake_fetchcontent_for_dependencies.html",
);
assert.doesNotMatch(adrGuide, /href="[^"]*\.md"/);
assert.match(reference("guides.html"), /guides\/docs\/developer\/demo-status.html/);
console.log("Documentation availability, maturity and handwritten-gap regressions passed.");
