function isReference(value) {
    return (typeof value === "object" && value !== null) || typeof value === "function";
}
export class CommandHistory {
    stateValue;
    snapshotState;
    undoEntries = [];
    redoEntries = [];
    transaction;
    constructor(initialState, options = {}) {
        this.snapshotState = options.snapshot ?? ((state) => structuredClone(state));
        this.stateValue = this.snapshot(initialState, "initial state");
    }
    get state() {
        return this.snapshot(this.stateValue, "published state");
    }
    get canUndo() {
        return this.undoEntries.length > 0;
    }
    get canRedo() {
        return this.redoEntries.length > 0;
    }
    get inTransaction() {
        return this.transaction !== undefined;
    }
    execute(command) {
        const before = this.snapshot(this.stateValue, `${command.label}.before`);
        const nextState = this.applyReducer(this.stateValue, (state) => command.apply(state), `${command.label}.apply`);
        const after = this.snapshot(nextState, `${command.label}.after`);
        this.stateValue = nextState;
        if (this.transaction !== undefined) {
            this.transaction.commandCount += 1;
            return;
        }
        this.undoEntries.push({ label: command.label, before, after });
        this.redoEntries.length = 0;
    }
    beginTransaction(label) {
        if (this.transaction !== undefined)
            throw new Error("A command transaction is already active.");
        if (label.length === 0)
            throw new Error("Transaction label cannot be empty.");
        this.transaction = {
            label,
            before: this.snapshot(this.stateValue, `${label}.before`),
            commandCount: 0,
        };
    }
    commitTransaction() {
        const transaction = this.requireTransaction();
        if (transaction.commandCount === 0) {
            this.transaction = undefined;
            return false;
        }
        const entry = {
            label: transaction.label,
            before: this.snapshot(transaction.before, `${transaction.label}.committed before`),
            after: this.snapshot(this.stateValue, `${transaction.label}.committed after`),
        };
        this.undoEntries.push(entry);
        this.redoEntries.length = 0;
        this.transaction = undefined;
        return true;
    }
    rollbackTransaction() {
        const transaction = this.requireTransaction();
        if (transaction.commandCount === 0) {
            this.transaction = undefined;
            return false;
        }
        const nextState = this.snapshot(transaction.before, `${transaction.label}.rollback`);
        this.stateValue = nextState;
        this.transaction = undefined;
        return true;
    }
    undo() {
        this.requireNoTransaction("undo");
        const entry = this.undoEntries.at(-1);
        if (entry === undefined)
            return false;
        const nextState = this.snapshot(entry.before, `${entry.label}.undo`);
        this.stateValue = nextState;
        this.undoEntries.pop();
        this.redoEntries.push(entry);
        return true;
    }
    redo() {
        this.requireNoTransaction("redo");
        const entry = this.redoEntries.at(-1);
        if (entry === undefined)
            return false;
        const nextState = this.snapshot(entry.after, `${entry.label}.redo`);
        this.stateValue = nextState;
        this.redoEntries.pop();
        this.undoEntries.push(entry);
        return true;
    }
    applyReducer(state, reducer, label) {
        const input = this.snapshot(state, `${label} input`);
        const output = reducer(input);
        if (isReference(input) && output === input)
            throw new Error(`${label} mutated or returned its input; commands must return replacement state.`);
        return this.snapshot(output, `${label} output`);
    }
    snapshot(state, label) {
        const copy = this.snapshotState(state);
        if (isReference(state) && copy === state)
            throw new Error(`The history snapshot policy returned the original ${label} reference.`);
        return copy;
    }
    requireTransaction() {
        if (this.transaction === undefined)
            throw new Error("No command transaction is active.");
        return this.transaction;
    }
    requireNoTransaction(operation) {
        if (this.transaction !== undefined)
            throw new Error(`Cannot ${operation} while a command transaction is active.`);
    }
}
