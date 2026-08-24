import type { ChildProcessWithoutNullStreams } from "node:child_process";

export interface ProcessExitA0 {
  readonly code: number | null;
  readonly signal: NodeJS.Signals | null;
}

/** @internal Process observation kept outside the package export map for focused lifecycle tests. */
export class ChildProcessObservationA0 {
  readonly exit: Promise<ProcessExitA0>;
  readonly firstError: Promise<Error>;
  lastError: Error | undefined;

  constructor(child: ChildProcessWithoutNullStreams) {
    this.firstError = new Promise((resolve) => {
      child.on("error", (error) => {
        this.lastError = error;
        resolve(error);
      });
    });
    this.exit = new Promise((resolve) => {
      child.once("exit", (code, signal) => resolve({ code, signal }));
    });
  }
}

/** @internal Direct-child termination with bounded SIGTERM-to-SIGKILL escalation. */
export class ChildTerminationA0 {
  private escalation: ReturnType<typeof setTimeout> | undefined;
  private requested = false;

  constructor(
    private readonly child: Pick<
      ChildProcessWithoutNullStreams,
      "exitCode" | "kill" | "signalCode"
    >,
    private readonly exit: Promise<ProcessExitA0>,
    private readonly graceMs: number,
  ) {
    void exit.then(() => this.clearEscalation());
  }

  request(): void {
    if (this.requested) return;
    this.requested = true;
    if (this.child.exitCode !== null || this.child.signalCode !== null) return;
    try {
      this.child.kill("SIGTERM");
    } catch {
      // Only an exit event proves completion; escalation remains armed.
    }
    this.escalation = setTimeout(() => {
      if (this.child.exitCode === null && this.child.signalCode === null) {
        try {
          this.child.kill("SIGKILL");
        } catch {
          // terminateAndWait reports a bounded failure if the process remains alive.
        }
      }
    }, this.graceMs);
  }

  async terminateAndWait(): Promise<ProcessExitA0> {
    this.request();
    return withTimeout(
      this.exit,
      this.graceMs * 2 + 1_000,
      "Geometer child process did not exit after forced termination.",
    );
  }

  private clearEscalation(): void {
    if (this.escalation !== undefined) clearTimeout(this.escalation);
    this.escalation = undefined;
  }
}

async function withTimeout<T>(promise: Promise<T>, timeoutMs: number, message: string): Promise<T> {
  let timer: ReturnType<typeof setTimeout> | undefined;
  try {
    return await Promise.race([
      promise,
      new Promise<T>((_resolve, reject) => {
        timer = setTimeout(() => reject(new Error(message)), timeoutMs);
      }),
    ]);
  } finally {
    if (timer !== undefined) clearTimeout(timer);
  }
}
