import { spawn as spawnChild } from "node:child_process";
import { Readable, Writable } from "node:stream";
import { GeometerIpcClientA0, GeometerIpcClientError, } from "./ipc-client-a0.js";
import { ChildProcessObservationA0, ChildTerminationA0, } from "./node-process-termination-a0.js";
const defaultConnectTimeoutMs = 10_000;
const defaultShutdownTimeoutMs = 35_000;
const defaultStderrBytes = 1024 * 1024;
const defaultTerminationGraceMs = 2_000;
export class GeometerNodeProcessA0 {
    termination;
    stderrSnapshot;
    shutdownTimeoutMs;
    client;
    processId;
    exit;
    constructor(client, processId, exit, termination, stderrSnapshot, shutdownTimeoutMs) {
        this.termination = termination;
        this.stderrSnapshot = stderrSnapshot;
        this.shutdownTimeoutMs = shutdownTimeoutMs;
        this.client = client;
        this.processId = processId;
        this.exit = exit;
    }
    static async spawn(executable, options) {
        const connectTimeoutMs = positiveTimeout(options.connectTimeoutMs, defaultConnectTimeoutMs);
        const shutdownTimeoutMs = positiveTimeout(options.shutdownTimeoutMs, defaultShutdownTimeoutMs);
        const maxStderrBytes = positiveInteger(options.maxStderrBytes, defaultStderrBytes);
        const terminationGraceMs = positiveTimeout(options.terminationGraceMs, defaultTerminationGraceMs);
        const child = spawnChild(executable, ["serve", "--stdio"], {
            ...(options.workingDirectory === undefined ? {} : { cwd: options.workingDirectory }),
            ...(options.environment === undefined
                ? {}
                : { env: { ...process.env, ...options.environment } }),
            stdio: ["pipe", "pipe", "pipe"],
            windowsHide: true,
        });
        const observation = new ChildProcessObservationA0(child);
        const termination = new ChildTerminationA0(child, observation.exit, terminationGraceMs);
        const stderr = new BoundedByteLog(maxStderrBytes);
        child.stderr.on("data", (chunk) => stderr.append(chunk));
        const processId = child.pid;
        if (processId === undefined) {
            const spawnError = await withTimeout(observation.firstError, connectTimeoutMs, "Geometer process did not report its spawn failure.");
            throw processError("before receiving a process id", spawnError, stderr.text());
        }
        try {
            const client = await withTimeout(Promise.race([
                GeometerIpcClientA0.connect({
                    readable: Readable.toWeb(child.stdout),
                    terminate: () => termination.request(),
                    writable: Writable.toWeb(child.stdin),
                }, { ...options, runtimeTarget: "native" }),
                observation.exit.then((result) => {
                    throw processExitError("before completing its IPC handshake", result, stderr.text());
                }),
            ]), connectTimeoutMs, "Geometer IPC handshake timed out.");
            return new GeometerNodeProcessA0(client, processId, observation.exit, termination, () => stderr.text(), shutdownTimeoutMs);
        }
        catch (error) {
            const result = await termination.terminateAndWait();
            if (result.signal === null) {
                throw processExitError("before completing its IPC handshake", result, stderr.text());
            }
            throw error;
        }
    }
    stderrText() {
        return this.stderrSnapshot();
    }
    async close(reason) {
        try {
            const acknowledgment = await withTimeout(this.client.close(reason), this.shutdownTimeoutMs, "Geometer IPC shutdown acknowledgment timed out.");
            const exit = await withTimeout(this.exit, this.shutdownTimeoutMs, "Geometer process did not exit after shutdown acknowledgment.");
            if (exit.code !== 0) {
                throw processExitError("after shutdown acknowledgment", exit, this.stderrText());
            }
            return acknowledgment;
        }
        catch (error) {
            await this.terminate(error instanceof Error ? error : new GeometerIpcClientError(String(error)));
            throw error;
        }
    }
    async terminate(reason = new GeometerIpcClientError("Geometer process was terminated.")) {
        this.client.terminate(reason);
        await this.termination.terminateAndWait();
    }
}
function processExitError(phase, result, stderr) {
    const detail = `code ${String(result.code)} and signal ${String(result.signal)}`;
    const suffix = stderr.length === 0 ? "" : `\nCaptured stderr:\n${stderr}`;
    return new GeometerIpcClientError(`Geometer process exited ${phase} with ${detail}.${suffix}`);
}
function processError(phase, error, stderr) {
    const suffix = stderr.length === 0 ? "" : `\nCaptured stderr:\n${stderr}`;
    return new GeometerIpcClientError(`Geometer process failed ${phase}: ${error.message}.${suffix}`);
}
class BoundedByteLog {
    maximumBytes;
    chunks = [];
    totalBytes = 0;
    constructor(maximumBytes) {
        this.maximumBytes = maximumBytes;
    }
    append(value) {
        let chunk = value;
        if (chunk.byteLength >= this.maximumBytes) {
            this.chunks.length = 0;
            chunk = chunk.subarray(chunk.byteLength - this.maximumBytes);
            this.totalBytes = 0;
        }
        this.chunks.push(chunk);
        this.totalBytes += chunk.byteLength;
        while (this.totalBytes > this.maximumBytes) {
            const first = this.chunks[0];
            if (first === undefined)
                break;
            const excess = this.totalBytes - this.maximumBytes;
            if (first.byteLength <= excess) {
                this.chunks.shift();
                this.totalBytes -= first.byteLength;
            }
            else {
                this.chunks[0] = first.subarray(excess);
                this.totalBytes -= excess;
            }
        }
    }
    text() {
        return Buffer.concat(this.chunks, this.totalBytes).toString("utf8");
    }
}
function positiveTimeout(value, fallback) {
    return positiveInteger(value, fallback);
}
function positiveInteger(value, fallback) {
    const selected = value ?? fallback;
    if (!Number.isSafeInteger(selected) || selected <= 0) {
        throw new GeometerIpcClientError("Geometer process limits must be positive safe integers.");
    }
    return selected;
}
async function withTimeout(promise, timeoutMs, message) {
    let timer;
    try {
        return await Promise.race([
            promise,
            new Promise((_resolve, reject) => {
                timer = setTimeout(() => reject(new GeometerIpcClientError(message)), timeoutMs);
            }),
        ]);
    }
    finally {
        if (timer !== undefined)
            clearTimeout(timer);
    }
}
