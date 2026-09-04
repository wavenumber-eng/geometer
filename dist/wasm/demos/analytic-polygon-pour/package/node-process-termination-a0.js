/** @internal Process observation kept outside the package export map for focused lifecycle tests. */
export class ChildProcessObservationA0 {
    exit;
    firstError;
    lastError;
    constructor(child) {
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
    child;
    exit;
    graceMs;
    escalation;
    requested = false;
    constructor(child, exit, graceMs) {
        this.child = child;
        this.exit = exit;
        this.graceMs = graceMs;
        void exit.then(() => this.clearEscalation());
    }
    request() {
        if (this.requested)
            return;
        this.requested = true;
        if (this.child.exitCode !== null || this.child.signalCode !== null)
            return;
        try {
            this.child.kill("SIGTERM");
        }
        catch {
            // Only an exit event proves completion; escalation remains armed.
        }
        this.escalation = setTimeout(() => {
            if (this.child.exitCode === null && this.child.signalCode === null) {
                try {
                    this.child.kill("SIGKILL");
                }
                catch {
                    // terminateAndWait reports a bounded failure if the process remains alive.
                }
            }
        }, this.graceMs);
    }
    async terminateAndWait() {
        this.request();
        return withTimeout(this.exit, this.graceMs * 2 + 1_000, "Geometer child process did not exit after forced termination.");
    }
    clearEscalation() {
        if (this.escalation !== undefined)
            clearTimeout(this.escalation);
        this.escalation = undefined;
    }
}
async function withTimeout(promise, timeoutMs, message) {
    let timer;
    try {
        return await Promise.race([
            promise,
            new Promise((_resolve, reject) => {
                timer = setTimeout(() => reject(new Error(message)), timeoutMs);
            }),
        ]);
    }
    finally {
        if (timer !== undefined)
            clearTimeout(timer);
    }
}
