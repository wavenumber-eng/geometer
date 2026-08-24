import { spawnSync } from "node:child_process";
import { mkdtemp, readFile, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(fileURLToPath(new URL("../..", import.meta.url)));
const workspace = await mkdtemp(join(tmpdir(), "geometer-ts-consumer-"));
const npmCommand =
  process.platform === "win32"
    ? { command: process.env.ComSpec ?? "cmd.exe", prefix: ["/d", "/s", "/c", "npm"] }
    : { command: "npm", prefix: [] };

try {
  run(
    npmCommand.command,
    [
      ...npmCommand.prefix,
      "pack",
      join(root, "dist", "wasm", "npm", "geometer"),
      "--pack-destination",
      workspace,
    ],
    root,
  );
  const archive = (await import("node:fs/promises")).readdir(workspace).then(async (items) => {
    const filename = items.find((item) => item.endsWith(".tgz"));
    if (!filename) throw new Error("npm pack did not produce an archive.");
    return join(workspace, filename);
  });
  await writeFile(
    join(workspace, "package.json"),
    `${JSON.stringify({ name: "geometer-clean-consumer", private: true, type: "module" }, null, 2)}\n`,
  );
  await writeFile(
    join(workspace, "tsconfig.json"),
    `${JSON.stringify({ compilerOptions: { module: "NodeNext", moduleResolution: "NodeNext", strict: true, target: "ES2022", noEmit: true }, include: ["consumer.ts"] }, null, 2)}\n`,
  );
  await writeFile(
    join(workspace, "consumer.ts"),
    `import { encodeModelBoundsOptionsA0Json } from "@wavenumber/geometer/contracts";\nimport { encodeAnalyticPlanarBooleanBatchRequestA0Packet } from "@wavenumber/geometer/analytic-packet-a0";\nimport { encodeGeometerIpcFrame } from "@wavenumber/geometer/ipc-a0";\nimport { GeometerIpcClientA0 } from "@wavenumber/geometer/ipc-client-a0";\nimport { GeometerNodeProcessA0 } from "@wavenumber/geometer/node-process-a0";\nimport { createGeometerWasmClient } from "@wavenumber/geometer/wasm";\nimport { createGeometerWorkerClient } from "@wavenumber/geometer/worker";\nimport { startGeometerWorkerHost } from "@wavenumber/geometer/worker-host";\nimport type { AnalyticPlanarBooleanBatchRequestA0, AnalyticPlanarBooleanBatchResultA0, EmscriptenGeometerFactory, ModelBoundsResultA0 } from "@wavenumber/geometer";\nexport const encoded: string = encodeModelBoundsOptionsA0Json({ format: "step" });\nexport const analyticRequest: AnalyticPlanarBooleanBatchRequestA0 = { jobs: [{ job_id: 1n, stages: [] }], relationship_queries: [] };\nexport const analyticPacket: Uint8Array = encodeAnalyticPlanarBooleanBatchRequestA0Packet(analyticRequest);\nexport const ipcFrame: Uint8Array = encodeGeometerIpcFrame({ attachments: [], json: "{}", kind: 1, requestId: 0n });\nexport const ipcClientType = GeometerIpcClientA0;\nexport const nodeProcessType = GeometerNodeProcessA0;\n// @ts-expect-error Analytic uint64 identities intentionally reject JavaScript number.\nexport const unsafeAnalyticRequest: AnalyticPlanarBooleanBatchRequestA0 = { jobs: [{ job_id: 1, stages: [] }], relationship_queries: [] };\nexport async function run(factory: EmscriptenGeometerFactory, bytes: Uint8Array): Promise<ModelBoundsResultA0> {\n  return (await createGeometerWasmClient(factory)).modelBounds({ model: bytes });\n}\nexport async function runAnalytic(factory: EmscriptenGeometerFactory): Promise<AnalyticPlanarBooleanBatchResultA0> {\n  return (await createGeometerWasmClient(factory)).analyticPlanarBooleanBatch(analyticRequest);\n}\nexport async function runWorker(worker: Worker, wasmBinary: ArrayBuffer, bytes: Uint8Array): Promise<ModelBoundsResultA0> {\n  return (await createGeometerWorkerClient(worker, { wasmBinary })).modelBounds({ model: bytes });\n}\nexport const host = startGeometerWorkerHost;\n`,
  );
  run(
    npmCommand.command,
    [
      ...npmCommand.prefix,
      "install",
      await archive,
      "--ignore-scripts",
      "--no-audit",
      "--no-fund",
      "--package-lock=false",
    ],
    workspace,
  );
  const compiler = join(root, "node_modules", "typescript", "bin", "tsc");
  run(process.execPath, [compiler, "--project", join(workspace, "tsconfig.json")], workspace);
  const packageJson = JSON.parse(
    await readFile(
      join(workspace, "node_modules", "@wavenumber", "geometer", "package.json"),
      "utf8",
    ),
  );
  if (packageJson.name !== "@wavenumber/geometer" || packageJson.type !== "module") {
    throw new Error("Installed package identity is incorrect.");
  }
  console.log(JSON.stringify({ package: packageJson.name, format: packageJson.type }));
} finally {
  await rm(workspace, { recursive: true, force: true });
}

function run(command, args, cwd) {
  const result = spawnSync(command, args, {
    cwd,
    encoding: "utf8",
  });
  if (result.error) throw result.error;
  if (result.status !== 0) {
    throw new Error(`${command} ${args.join(" ")} failed.\n${result.stdout}${result.stderr}`);
  }
}
