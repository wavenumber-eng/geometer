// Benchmark the current Geometer HLR projection CLI against the embedded_models
// corpus. This is a current-build timing tool; the old checked-in baseline
// artifacts were removed after the poly HLR performance work was completed.
//
// Usage:
//   node scripts/bench_hlr.js                  # all models, top view
//   node scripts/bench_hlr.js --view front     # different view
//   node scripts/bench_hlr.js --limit 5        # first 5 models
//   node scripts/bench_hlr.js --out result.md  # custom output file

"use strict";

const fs = require("node:fs");
const path = require("node:path");
const { spawnSync } = require("node:child_process");

const ROOT = path.resolve(__dirname, "..");
const CURRENT_CLI = resolveCurrentCli();

function resolveCurrentCli() {
  const candidates = [
    path.join(ROOT, "dist", "wasm", "node-test", "geometer-node-test.js"),
    path.join(ROOT, "dist", "geometer-node-test.js"),
  ];
  const found = candidates.find((candidate) => fs.existsSync(candidate));
  if (!found) {
    throw new Error(`Missing geometer-node-test.js. Looked in: ${candidates.join(", ")}`);
  }
  return found;
}

function parseArgs(argv) {
  const opts = {
    view: "top",
    limit: 0,
    out: path.join(ROOT, "docs", "plans", "004_poly_hlr_current_results.md"),
  };
  for (let i = 2; i < argv.length; i += 1) {
    const a = argv[i];
    if (a === "--view") opts.view = argv[++i];
    else if (a === "--limit") opts.limit = Number.parseInt(argv[++i], 10);
    else if (a === "--out") opts.out = argv[++i];
    else throw new Error(`Unknown arg: ${a}`);
  }
  return opts;
}

function loadManifest() {
  let text = fs.readFileSync(path.join(ROOT, "tests", "fixtures", "embedded_models_manifest.json"), "utf8");
  if (text.charCodeAt(0) === 0xFEFF) text = text.slice(1);
  return JSON.parse(text);
}

function runProjection(stepPath, jsonOutPath, view) {
  const start = Date.now();
  const result = spawnSync("node", [CURRENT_CLI, "step-project-hlr", stepPath, jsonOutPath, "--view", view], {
    cwd: ROOT,
    encoding: "utf8",
    timeout: 120_000,
  });
  const wallMs = Date.now() - start;
  if (result.status !== 0) {
    return { ok: false, wallMs, error: (result.stderr || result.stdout || "").slice(0, 400) };
  }

  let payload = null;
  try {
    payload = JSON.parse(fs.readFileSync(jsonOutPath, "utf8"));
  } catch (error) {
    return { ok: false, wallMs, error: `parse: ${error.message}` };
  }

  const v = (payload.views || []).find((view0) => view0.id === view) || (payload.views || [])[0];
  const detail = v && v.modes && v.modes.detail ? v.modes.detail : { segments: [], arcs: [] };
  const simple = v && v.modes && v.modes.simple ? v.modes.simple : { segments: [], arcs: [] };
  return {
    ok: true,
    wallMs,
    timings: payload.timings || {},
    detail: (detail.segments || []).length + (detail.arcs || []).length,
    simple: (simple.segments || []).length + (simple.arcs || []).length,
  };
}

function formatMs(ms) {
  if (!Number.isFinite(ms)) return "-";
  if (ms >= 1000) return `${(ms / 1000).toFixed(2)} s`;
  return `${Math.round(ms)} ms`;
}

function main() {
  const opts = parseArgs(process.argv);
  if (!fs.existsSync(CURRENT_CLI)) throw new Error(`Missing current CLI: ${CURRENT_CLI}`);

  const manifest = loadManifest();
  const models = opts.limit > 0 ? manifest.slice(0, opts.limit) : manifest;
  const tmpDir = path.join(ROOT, ".bench-tmp");
  fs.mkdirSync(tmpDir, { recursive: true });

  const rows = [];
  let i = 0;
  for (const model of models) {
    i += 1;
    const stepPath = path.join(ROOT, model.step);
    if (!fs.existsSync(stepPath)) {
      console.warn(`SKIP missing STEP: ${model.name}`);
      continue;
    }

    const out = path.join(tmpDir, `current_${i}.json`);
    process.stdout.write(`[${i}/${models.length}] ${model.name} ... `);
    const current = runProjection(stepPath, out, opts.view);
    rows.push({ name: model.name, stepBytes: model.stepBytes, current });

    if (current.ok) {
      const phases = [
        current.timings.step_read_ms != null ? `read ${formatMs(current.timings.step_read_ms)}` : null,
        current.timings.mesh_ms != null ? `mesh ${formatMs(current.timings.mesh_ms)}` : null,
        current.timings.hlr_ms != null ? `hlr ${formatMs(current.timings.hlr_ms)}` : null,
        current.timings.extract_ms != null ? `ext ${formatMs(current.timings.extract_ms)}` : null,
      ].filter(Boolean).join(" / ");
      console.log(`wall ${formatMs(current.wallMs)} [${phases}] detail=${current.detail} simple=${current.simple}`);
    } else {
      console.log(`FAIL ${current.error}`);
    }
  }

  const lines = [];
  lines.push("# Current HLR perf results");
  lines.push("");
  lines.push(`- View: \`${opts.view}\``);
  lines.push(`- Models: ${rows.length}`);
  lines.push(`- Generated: ${new Date().toISOString()}`);
  lines.push("");
  lines.push("Per-model wall-clock time includes Node startup, module init, HLR, and JSON write.");
  lines.push("");
  lines.push("| Model | STEP | Wall | Mesh | HLR | Extract | Detail | Simple |");
  lines.push("|---|---:|---:|---:|---:|---:|---:|---:|");

  let total = 0;
  let count = 0;
  for (const row of rows) {
    const current = row.current;
    if (current.ok) {
      total += current.wallMs;
      count += 1;
    }
    lines.push([
      row.name,
      `${(row.stepBytes / 1024).toFixed(0)} KB`,
      current.ok ? formatMs(current.wallMs) : "FAIL",
      current.ok ? formatMs(current.timings.mesh_ms) : "-",
      current.ok ? formatMs(current.timings.hlr_ms) : "-",
      current.ok ? formatMs(current.timings.extract_ms) : "-",
      current.ok ? current.detail : "-",
      current.ok ? current.simple : "-",
    ].map(String).join(" | ").replace(/^/, "| ").replace(/$/, " |"));
  }

  lines.push("");
  lines.push("## Aggregates");
  lines.push("");
  lines.push(`- Current total: ${formatMs(total)} across ${count} models`);
  if (count > 0) lines.push(`- Mean wall current: ${formatMs(total / count)}`);

  fs.mkdirSync(path.dirname(opts.out), { recursive: true });
  fs.writeFileSync(opts.out, lines.join("\n") + "\n", "utf8");
  console.log(`\nWrote ${opts.out}`);
}

main();
