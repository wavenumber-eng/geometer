function normalizedKey(key) {
    return key.length === 1 ? key.toLowerCase() : key;
}
function hasModifier(value) {
    return value ?? false;
}
function matchesShortcut(input, shortcut) {
    return (normalizedKey(input.key) === normalizedKey(shortcut.key) &&
        input.altKey === hasModifier(shortcut.alt) &&
        input.ctrlKey === hasModifier(shortcut.control) &&
        input.metaKey === hasModifier(shortcut.meta) &&
        input.shiftKey === hasModifier(shortcut.shift));
}
export function isEditableTarget(target) {
    if (target === null || typeof target !== "object")
        return false;
    const candidate = target;
    if (candidate.isContentEditable === true)
        return true;
    const tagName = candidate.tagName?.toLowerCase();
    if (tagName === "input" || tagName === "textarea" || tagName === "select")
        return true;
    return candidate.getAttribute?.("role") === "textbox";
}
export function formatShortcut(shortcut) {
    const parts = [];
    if (shortcut.control)
        parts.push("Ctrl");
    if (shortcut.meta)
        parts.push("Meta");
    if (shortcut.alt)
        parts.push("Alt");
    if (shortcut.shift)
        parts.push("Shift");
    parts.push(shortcut.key.length === 1 ? shortcut.key.toUpperCase() : shortcut.key);
    return parts.join("+");
}
export class CommandRegistry {
    commands = [];
    commandIds = new Set();
    register(command) {
        const registered = CommandRegistry.snapshotCommand(command);
        if (this.commandIds.has(registered.id))
            throw new Error(`Command '${registered.id}' is already registered.`);
        this.commands.push(registered);
        this.commandIds.add(registered.id);
        return () => this.unregister(registered);
    }
    execute(id, context) {
        const command = this.commands.find((candidate) => candidate.id === id);
        if (command === undefined || command.enabled?.(context) === false)
            return false;
        command.execute(context);
        return true;
    }
    dispatchShortcut(input, context, activeScopes = new Set()) {
        const editable = isEditableTarget(input.target) || input.composedPath?.().some(isEditableTarget) === true;
        const candidates = this.commands.filter((command) => {
            return (command.shortcut !== undefined &&
                (command.scope === "global" || activeScopes.has(command.scope)) &&
                (!editable || command.allowInEditable) &&
                matchesShortcut(input, command.shortcut) &&
                command.enabled?.(context) !== false);
        });
        if (candidates.length === 0)
            return false;
        const scoped = candidates.find((command) => command.scope !== "global");
        (scoped ?? candidates[0])?.execute(context);
        input.preventDefault();
        return true;
    }
    help(activeScopes) {
        return this.commands
            .filter((command) => {
            return (activeScopes === undefined ||
                command.scope === "global" ||
                activeScopes.has(command.scope));
        })
            .map((command) => ({
            id: command.id,
            title: command.title,
            help: command.help,
            shortcut: command.shortcut === undefined ? undefined : formatShortcut(command.shortcut),
            scope: command.scope,
        }));
    }
    unregister(registered) {
        const index = this.commands.indexOf(registered);
        if (index < 0)
            return;
        this.commands.splice(index, 1);
        this.commandIds.delete(registered.id);
    }
    static snapshotCommand(command) {
        if (typeof command.id !== "string" || command.id.length === 0)
            throw new Error("Command id cannot be empty.");
        if (typeof command.title !== "string" || typeof command.help !== "string")
            throw new TypeError("Command title and help must be strings.");
        if (typeof command.execute !== "function")
            throw new TypeError("Command execute must be a function.");
        if (command.enabled !== undefined && typeof command.enabled !== "function")
            throw new TypeError("Command enabled must be a function when provided.");
        let shortcut;
        if (command.shortcut !== undefined) {
            if (typeof command.shortcut.key !== "string" || command.shortcut.key.length === 0)
                throw new Error("Shortcut key cannot be empty.");
            shortcut = Object.freeze({ ...command.shortcut });
        }
        const receiver = Object.freeze({ ...command, shortcut });
        const executeMethod = command.execute;
        const enabledMethod = command.enabled;
        return Object.freeze({
            id: command.id,
            title: command.title,
            help: command.help,
            shortcut,
            scope: command.scope ?? "global",
            allowInEditable: command.allowInEditable ?? false,
            enabled: enabledMethod === undefined
                ? undefined
                : (context) => enabledMethod.call(receiver, context),
            execute: (context) => executeMethod.call(receiver, context),
        });
    }
}
