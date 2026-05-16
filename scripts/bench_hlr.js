// Benchmark the geometer HLR projection CLI against the embedded_models
// corpus. Runs the same STEP files through two builds (1.1 dist/ and v0.1.0
// dist/baseline/) for a default-vs-default comparison. The CLI in each build
// uses library defaults: 1.1 picks HLRBRep_PolyAlgo with mesh deflection
// 0.01 mm; baseline picks HLRBRep_Algo (exact).
//
// Usage:
//   node scripts/bench_hlr.js                     # all models, top view
//   node scripts/bench_hlr.js --view front        # different view
//   node scripts/bench_hlr.js --limit 5           # first 5 models
//   node scripts/bench_hlr.js --backends current  # skip baseline
//   node scripts/bench_hlr.js --out results.md    # custom output file

"use strict";

const fs = require("node:fs");
const path = require("node:path");
const { spawnSync } = require("node:child_process");

const ROOT = path.resolve(__dirname, "..");

function parseArgs(argv) {
  const opts = {
    view: "top",
    limit: 0,
    backends: ["baseline", "current"],
    out: path.join(ROOT, "docs", "plans", "004_poly_hlr_perf_results.md"),
  };
  for (let i = 2; i < argv.length; i += 1) {
    const a = argv[i];
    if (a === "--view") opts.view = argv[++i];
    else if (a === "--limit") opts.limit = Number.parseInt(argv[++i], 10);
    else if (a === "--backends") opts.backends = argv[++i].split(",");
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

function runProjection(cliPath, stepPath, jsonOutPath, view) {
  const start = Date.now();
  const result = spawnSync("node", [cliPath, "step-project-hlr", stepPath, jsonOutPath, "--view", view], {
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
  } catch (e) {
    return { ok: false, wallMs, error: `parse: ${e.message}` };
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

function pct(num, denom) {
  if (!Number.isFinite(num) || !Number.isFinite(denom) || denom <= 0) return "-";
  return `${(num / denom).toFixed(2)}x`;
}

function backendCli(backend) {
  if (backend === "baseline") return path.join(ROOT, "dist", "baseline", "geometer.js");
  if (backend === "current") return path.join(ROOT, "dist", "geometer.js");
  throw new Error(`Unknown backend: ${backend}`);
}

function main() {
  const opts = parseArgs(process.argv);
  const manifest = loadManifest();
  const models = opts.limit > 0 ? manifest.slice(0, opts.limit) : manifest;

  // Sanity: ensure baseline/geometer.js actually has CLI flavor (NODERAWFS).
  // The previous baseline snapshot might have been the browser flavor.
  for (const backend of opts.backends) {
    const cli = backendCli(backend);
    if (!fs.existsSync(cli)) throw new Error(`Missing CLI for backend "${backend}": ${cli}`);
  }

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
    const result = { name: model.name, stepBytes: model.stepBytes };
    for (const backend of opts.backends) {
      const cli = backendCli(backend);
      const out = path.join(tmpDir, `${backend}_${i}.json`);
      process.stdout.write(`[${i}/${models.length}] ${backend.padEnd(8)} ${model.name} ... `);
      const r = runProjection(cli, stepPath, out, opts.view);
      result[backend] = r;
      if (r.ok) {
        const phases = [
          r.timings.step_read_ms != null ? `read ${formatMs(r.timings.step_read_ms)}` : null,
          r.timings.mesh_ms != null ? `mesh ${formatMs(r.timings.mesh_ms)}` : null,
          r.timings.hlr_ms != null ? `hlr ${formatMs(r.timings.hlr_ms)}` : null,
          r.timings.extract_ms != null ? `ext ${formatMs(r.timings.extract_ms)}` : null,
        ].filter(Boolean).join(" / ");
        console.log(`wall ${formatMs(r.wallMs)} [${phases}] detail=${r.detail} simple=${r.simple}`);
      } else {
        console.log(`FAIL ${r.error}`);
      }
    }
    rows.push(result);
  }

  // Build a markdown report. The 1.1 build emits per-phase timings; baseline
  // does not, so its "hlr" cell shows the wall time only.
  const lines = [];
  lines.push("# Poly HLR perf — results");
  lines.push("");
  lines.push(`- View: \`${opts.view}\``);
  lines.push(`- Models: ${rows.length}`);
  lines.push(`- Generated: ${new Date().toISOString()}`);
  lines.push("");
  lines.push("Per-model wall-clock time (Node startup + module init + HLR + JSON write).");
  lines.push("Each backend's CLI uses its own library defaults: baseline = HLRBRep_Algo (exact); 1.1 = HLRBRep_PolyAlgo with mesh@0.01 mm.");
  lines.push("");
  lines.push("| Model | STEP | Baseline wall | 1.1 wall | Speedup | 1.1 mesh | 1.1 hlr | 1.1 extract | det B / 1.1 | sim B / 1.1 |");
  lines.push("|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|");

  let baseTotal = 0;
  let currentTotal = 0;
  let baseCount = 0;
  let currentCount = 0;

  for (const r of rows) {
    const b = r.baseline;
    const c = r.current;
    const speedup = (b && b.ok && c && c.ok) ? `${(b.wallMs / Math.max(c.wallMs, 1)).toFixed(2)}x` : "-";
    if (b && b.ok) { baseTotal += b.wallMs; baseCount += 1; }
    if (c && c.ok) { currentTotal += c.wallMs; currentCount += 1; }
    lines.push([
      r.name,
      `${(r.stepBytes / 1024).toFixed(0)} KB`,
      b && b.ok ? formatMs(b.wallMs) : (b ? `FAIL` : "-"),
      c && c.ok ? formatMs(c.wallMs) : (c ? `FAIL` : "-"),
      speedup,
      c && c.ok ? formatMs(c.timings.mesh_ms) : "-",
      c && c.ok ? formatMs(c.timings.hlr_ms) : "-",
      c && c.ok ? formatMs(c.timings.extract_ms) : "-",
      `${b && b.ok ? b.detail : "-"} / ${c && c.ok ? c.detail : "-"}`,
      `${b && b.ok ? b.simple : "-"} / ${c && c.ok ? c.simple : "-"}`,
    ].map(String).join(" | ").replace(/^/, "| ").replace(/$/, " |"));
  }

  lines.push("");
  lines.push("## Aggregates");
  lines.push("");
  lines.push(`- Baseline total: ${formatMs(baseTotal)} across ${baseCount} models`);
  lines.push(`- 1.1 total: ${formatMs(currentTotal)} across ${currentCount} models`);
  if (baseCount > 0 && currentCount > 0) {
    lines.push(`- Mean wall baseline: ${formatMs(baseTotal / baseCount)}`);
    lines.push(`- Mean wall 1.1: ${formatMs(currentTotal / currentCount)}`);
    lines.push(`- Overall speedup: ${(baseTotal / Math.max(currentTotal, 1)).toFixed(2)}x`);
  }

  fs.writeFileSync(opts.out, lines.join("\n") + "\n", "utf8");
  console.log(`\nWrote ${opts.out}`);
}

main();
