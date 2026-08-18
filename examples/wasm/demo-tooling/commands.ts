export interface KeyboardInput {
  readonly key: string;
  readonly altKey: boolean;
  readonly ctrlKey: boolean;
  readonly metaKey: boolean;
  readonly shiftKey: boolean;
  readonly target?: unknown;
  composedPath?(): readonly unknown[];
  preventDefault(): void;
}

export interface Shortcut {
  readonly key: string;
  readonly alt?: boolean;
  readonly control?: boolean;
  readonly meta?: boolean;
  readonly shift?: boolean;
}

export interface Command<Context> {
  readonly id: string;
  readonly title: string;
  readonly help: string;
  readonly shortcut?: Shortcut;
  readonly scope?: string;
  readonly allowInEditable?: boolean;
  readonly enabled?: (context: Context) => boolean;
  execute(context: Context): void;
}

export interface HelpEntry {
  readonly id: string;
  readonly title: string;
  readonly help: string;
  readonly shortcut: string | undefined;
  readonly scope: string;
}

interface EditableTargetLike {
  readonly tagName?: string;
  readonly isContentEditable?: boolean;
  getAttribute?(name: string): string | null;
}

interface RegisteredCommand<Context> {
  readonly id: string;
  readonly title: string;
  readonly help: string;
  readonly shortcut: Readonly<Shortcut> | undefined;
  readonly scope: string;
  readonly allowInEditable: boolean;
  readonly enabled: ((context: Context) => boolean) | undefined;
  readonly execute: (context: Context) => void;
}

function normalizedKey(key: string): string {
  return key.length === 1 ? key.toLowerCase() : key;
}

function hasModifier(value: boolean | undefined): boolean {
  return value ?? false;
}

function matchesShortcut(input: KeyboardInput, shortcut: Shortcut): boolean {
  return (
    normalizedKey(input.key) === normalizedKey(shortcut.key) &&
    input.altKey === hasModifier(shortcut.alt) &&
    input.ctrlKey === hasModifier(shortcut.control) &&
    input.metaKey === hasModifier(shortcut.meta) &&
    input.shiftKey === hasModifier(shortcut.shift)
  );
}

export function isEditableTarget(target: unknown): boolean {
  if (target === null || typeof target !== "object") return false;
  const candidate = target as EditableTargetLike;
  if (candidate.isContentEditable === true) return true;
  const tagName = candidate.tagName?.toLowerCase();
  if (tagName === "input" || tagName === "textarea" || tagName === "select") return true;
  return candidate.getAttribute?.("role") === "textbox";
}

export function formatShortcut(shortcut: Shortcut): string {
  const parts: string[] = [];
  if (shortcut.control) parts.push("Ctrl");
  if (shortcut.meta) parts.push("Meta");
  if (shortcut.alt) parts.push("Alt");
  if (shortcut.shift) parts.push("Shift");
  parts.push(shortcut.key.length === 1 ? shortcut.key.toUpperCase() : shortcut.key);
  return parts.join("+");
}

export class CommandRegistry<Context> {
  private readonly commands: RegisteredCommand<Context>[] = [];
  private readonly commandIds = new Set<string>();

  register(command: Command<Context>): () => void {
    const registered = CommandRegistry.snapshotCommand(command);
    if (this.commandIds.has(registered.id))
      throw new Error(`Command '${registered.id}' is already registered.`);
    this.commands.push(registered);
    this.commandIds.add(registered.id);
    return () => this.unregister(registered);
  }

  execute(id: string, context: Context): boolean {
    const command = this.commands.find((candidate) => candidate.id === id);
    if (command === undefined || command.enabled?.(context) === false) return false;
    command.execute(context);
    return true;
  }

  dispatchShortcut(
    input: KeyboardInput,
    context: Context,
    activeScopes: ReadonlySet<string> = new Set<string>(),
  ): boolean {
    const editable =
      isEditableTarget(input.target) || input.composedPath?.().some(isEditableTarget) === true;
    const candidates = this.commands.filter((command) => {
      return (
        command.shortcut !== undefined &&
        (command.scope === "global" || activeScopes.has(command.scope)) &&
        (!editable || command.allowInEditable) &&
        matchesShortcut(input, command.shortcut) &&
        command.enabled?.(context) !== false
      );
    });
    if (candidates.length === 0) return false;
    const scoped = candidates.find((command) => command.scope !== "global");
    (scoped ?? candidates[0])?.execute(context);
    input.preventDefault();
    return true;
  }

  help(activeScopes?: ReadonlySet<string>): readonly HelpEntry[] {
    return this.commands
      .filter((command) => {
        return (
          activeScopes === undefined ||
          command.scope === "global" ||
          activeScopes.has(command.scope)
        );
      })
      .map((command) => ({
        id: command.id,
        title: command.title,
        help: command.help,
        shortcut: command.shortcut === undefined ? undefined : formatShortcut(command.shortcut),
        scope: command.scope,
      }));
  }

  private unregister(registered: RegisteredCommand<Context>): void {
    const index = this.commands.indexOf(registered);
    if (index < 0) return;
    this.commands.splice(index, 1);
    this.commandIds.delete(registered.id);
  }

  private static snapshotCommand<Context>(command: Command<Context>): RegisteredCommand<Context> {
    if (typeof command.id !== "string" || command.id.length === 0)
      throw new Error("Command id cannot be empty.");
    if (typeof command.title !== "string" || typeof command.help !== "string")
      throw new TypeError("Command title and help must be strings.");
    if (typeof command.execute !== "function")
      throw new TypeError("Command execute must be a function.");
    if (command.enabled !== undefined && typeof command.enabled !== "function")
      throw new TypeError("Command enabled must be a function when provided.");
    let shortcut: Readonly<Shortcut> | undefined;
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
      enabled:
        enabledMethod === undefined
          ? undefined
          : (context: Context) => enabledMethod.call(receiver, context),
      execute: (context: Context) => executeMethod.call(receiver, context),
    });
  }
}
