import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { createHash } from "node:crypto";
import { mkdtemp, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join, resolve } from "node:path";

import { encodeAnalyticPlanarBooleanBatchRequestA0Packet } from "../../dist/wasm/npm/geometer/analytic-packet-a0.js";
import {
  TRACE_ENVELOPE_WIDTH_NM,
  TRACE_WIDTH_NM,
  CLEARANCE_NM,
  classifyPcbLayerJob,
  addTrace,
  addVia,
  initialPcbDemoState,
  makePcbPolygonPourRequest,
  moveBoardVertex,
  moveVia,
  snapPointTo45,
} from "../../dist/wasm/demos/pcb_polygon_pour_model.js";

const root = resolve(import.meta.dirname, "../..");
const state = initialPcbDemoState();
const request = makePcbPolygonPourRequest(state);
assert.equal(TRACE_WIDTH_NM, 150_000);
assert.equal(CLEARANCE_NM, 200_000);
assert.equal(TRACE_ENVELOPE_WIDTH_NM, 550_000);
assert.equal(classifyPcbLayerJob(31n), "base");
assert.equal(classifyPcbLayerJob(40_001n), "clearance");
assert.equal(classifyPcbLayerJob(50_001n), "thermal");
assert.throws(() => classifyPcbLayerJob(32n), /Unknown PCB layer job/);
assert.deepEqual(snapPointTo45({ x: 1, y: 1 }, { x: 3.1, y: 2.7 }), { x: 3.1, y: 3.1 });

const pour = request.jobs.find((job) => job.job_id === 31n);
assert.ok(pour, "PCB request must contain the primary pour job.");
assert.deepEqual(pour.stages.map((stage) => stage.operation), ["union", "difference"]);
const difference = pour.stages[1];
assert.ok(difference);
assert.equal(difference.operands.filter((operand) => operand.kind === "disk").length, 2);
const clearanceJobs = request.jobs.filter((job) => job.job_id >= 40_000n && job.job_id < 50_000n);
const capsules = clearanceJobs.flatMap((job) => job.stages[0].operands).filter((operand) => operand.kind === "capsule");
assert.equal(capsules.length, 1);
assert.equal(capsules[0].width_nm, 550_000n);
assert.equal(clearanceJobs.flatMap((job) => job.stages[0].operands).filter((operand) => operand.kind === "disk").length, 1);
assert.ok(clearanceJobs.every((job) => job.stages.length === 1 && job.stages[0].operands.length === 1));
const thermalJobs = request.jobs.filter((job) => job.job_id >= 50_000n);
assert.equal(thermalJobs.length, 3);
assert.ok(thermalJobs.every((job) => job.stages.length === 1 && job.stages[0].operands.length === 1));

let native;
if (process.argv.includes("--native")) {
  const executable = join(root, "dist", "native", "windows-x64", "geometer.exe");
  const temporary = await mkdtemp(join(tmpdir(), "geometer-pcb-native-"));
  try {
    const moved = moveVia(state, 1, { x: 8.25, y: 5.25 });
    const added = addVia(moved, { x: 13, y: 9 });
    const routed = addTrace(added, [{ x: 9, y: 3 }, { x: 11, y: 5 }]);
    const edited = moveBoardVertex(routed, 0, { x: 0.75, y: 0.5 });
    const replayStates = [
      ["initial", state],
      ["moved", moved],
      ["added", added],
      ["routed_45", routed],
      ["vertex_edited", edited],
    ];
    native = [];
    for (const [name, replayState] of replayStates) {
      const input = join(temporary, `${name}.bin`);
      await writeFile(input, encodeAnalyticPlanarBooleanBatchRequestA0Packet(makePcbPolygonPourRequest(replayState)));
      const completed = spawnSync("uv", ["run", "python", "scripts/validate_pcb_demo_native.py", input, "--executable", executable], {
        cwd: root,
        encoding: "utf8",
        timeout: 60_000,
      });
      assert.equal(completed.status, 0, `${name}: ${completed.stdout}${completed.stderr}`);
      const jobs = JSON.parse(completed.stdout);
      const closureText = jobs
        .slice()
        .sort((left, right) => left.job_id - right.job_id)
        .map((job) => `${job.job_id}\0${job.digest}\n`)
        .join("");
      native.push({
        closure: createHash("sha256").update(closureText).digest("hex"),
        jobs,
        name,
      });
    }
    assert.equal(new Set(native.map((item) => item.closure)).size, native.length, "every Chrome edit must change the native composite closure");
  } finally {
    await rm(temporary, { recursive: true, force: true });
  }
}

console.log(JSON.stringify({ capsuleWidthNm: TRACE_ENVELOPE_WIDTH_NM, jobs: request.jobs.length, native }));
