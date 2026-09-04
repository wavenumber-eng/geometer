import type { IpcShutdownAckA0 } from "./generated/contracts.js";
import { GeometerIpcClientA0, GeometerIpcClientError, type GeometerIpcConnectOptionsA0 } from "./ipc-client-a0.js";
export interface GeometerNodeProcessOptionsA0 extends GeometerIpcConnectOptionsA0 {
    readonly connectTimeoutMs?: number;
    readonly maxStderrBytes?: number;
    readonly shutdownTimeoutMs?: number;
    readonly terminationGraceMs?: number;
    readonly workingDirectory?: string;
    readonly environment?: Readonly<Record<string, string>>;
}
export declare class GeometerNodeProcessA0 {
    private readonly termination;
    private readonly stderrSnapshot;
    private readonly shutdownTimeoutMs;
    readonly client: GeometerIpcClientA0;
    readonly processId: number;
    private readonly exit;
    private constructor();
    static spawn(executable: string, options: GeometerNodeProcessOptionsA0): Promise<GeometerNodeProcessA0>;
    stderrText(): string;
    close(reason?: string): Promise<IpcShutdownAckA0>;
    terminate(reason?: GeometerIpcClientError): Promise<void>;
}
