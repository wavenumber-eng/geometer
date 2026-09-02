import type { ChildProcessWithoutNullStreams } from "node:child_process";
export interface ProcessExitA0 {
    readonly code: number | null;
    readonly signal: NodeJS.Signals | null;
}
/** @internal Process observation kept outside the package export map for focused lifecycle tests. */
export declare class ChildProcessObservationA0 {
    readonly exit: Promise<ProcessExitA0>;
    readonly firstError: Promise<Error>;
    lastError: Error | undefined;
    constructor(child: ChildProcessWithoutNullStreams);
}
/** @internal Direct-child termination with bounded SIGTERM-to-SIGKILL escalation. */
export declare class ChildTerminationA0 {
    private readonly child;
    private readonly exit;
    private readonly graceMs;
    private escalation;
    private requested;
    constructor(child: Pick<ChildProcessWithoutNullStreams, "exitCode" | "kill" | "signalCode">, exit: Promise<ProcessExitA0>, graceMs: number);
    request(): void;
    terminateAndWait(): Promise<ProcessExitA0>;
    private clearEscalation;
}
