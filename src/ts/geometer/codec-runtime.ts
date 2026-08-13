export interface ContractConstraints {
  readonly max_items?: number;
  readonly max_length?: number;
  readonly max_value?: number;
  readonly min_items?: number;
  readonly min_length?: number;
  readonly min_value?: number;
}

export type ContractTypeDescriptor =
  | { readonly kind: "array"; readonly element: ContractTypeDescriptor }
  | { readonly kind: "literal"; readonly value: unknown; readonly value_type: string }
  | { readonly kind: "primitive"; readonly name: string }
  | { readonly kind: "reference"; readonly target: string };

interface ContractPropertyDescriptor {
  readonly constraints: ContractConstraints;
  readonly optional: boolean;
  readonly type: ContractTypeDescriptor;
}

export type ContractDescriptor =
  | {
      readonly constraints: ContractConstraints;
      readonly element: ContractTypeDescriptor;
      readonly kind: "array";
    }
  | { readonly kind: "enum"; readonly values: readonly unknown[] }
  | {
      readonly kind: "object";
      readonly properties: Readonly<Record<string, ContractPropertyDescriptor>>;
    }
  | { readonly kind: "union"; readonly variants: readonly ContractTypeDescriptor[] };

export type ContractDescriptorMap = Readonly<Record<string, ContractDescriptor>>;

export class ContractCodecError extends Error {
  readonly code: string;
  readonly path: string;

  constructor(code: string, path: string, message: string) {
    super(`${message}${path ? ` at ${path}` : ""}`);
    this.name = "ContractCodecError";
    this.code = code;
    this.path = path;
  }
}

export function decodeContractJson(
  data: string | Uint8Array,
  type: ContractTypeDescriptor,
  declarations: ContractDescriptorMap,
): unknown {
  let text: string;
  try {
    text = typeof data === "string" ? data : new TextDecoder("utf-8", { fatal: true }).decode(data);
  } catch {
    throw new ContractCodecError("geometer.contract.invalid_utf8", "", "Input is not valid UTF-8.");
  }
  const value = new StrictJsonParser(text).parse();
  return canonicalize(value, type, declarations, "", {});
}

export function encodeContractJson(
  value: unknown,
  type: ContractTypeDescriptor,
  declarations: ContractDescriptorMap,
): string {
  return JSON.stringify(canonicalize(value, type, declarations, "", {}));
}

function canonicalize(
  value: unknown,
  type: ContractTypeDescriptor,
  declarations: ContractDescriptorMap,
  path: string,
  constraints: ContractConstraints,
): unknown {
  if (type.kind === "reference") {
    const declaration = declarations[type.target];
    if (!declaration) fail("geometer.contract.unknown_reference", path, `Unknown ${type.target}.`);
    return canonicalizeDeclaration(value, declaration, declarations, path, constraints);
  }
  if (type.kind === "literal") {
    if (value !== type.value)
      fail("geometer.contract.literal_mismatch", path, "Unexpected literal.");
    return value;
  }
  if (type.kind === "array") {
    return canonicalizeArray(value, type.element, declarations, path, constraints);
  }
  if (type.name === "string") {
    if (typeof value !== "string")
      fail("geometer.contract.type_mismatch", path, "Expected string.");
    validateString(value, path, constraints);
    return value;
  }
  if (type.name === "boolean") {
    if (typeof value !== "boolean")
      fail("geometer.contract.type_mismatch", path, "Expected boolean.");
    return value;
  }
  if (type.name === "float64") {
    if (typeof value !== "number" || !Number.isFinite(value)) {
      fail("geometer.contract.type_mismatch", path, "Expected finite number.");
    }
    if (constraints.min_value !== undefined && value < constraints.min_value) {
      fail("geometer.contract.number_range", path, "Number is below its minimum.");
    }
    if (constraints.max_value !== undefined && value > constraints.max_value) {
      fail("geometer.contract.number_range", path, "Number is above its maximum.");
    }
    return value;
  }
  fail("geometer.contract.unsupported_type", path, `Unsupported primitive ${type.name}.`);
}

function canonicalizeDeclaration(
  value: unknown,
  declaration: ContractDescriptor,
  declarations: ContractDescriptorMap,
  path: string,
  constraints: ContractConstraints,
): unknown {
  if (declaration.kind === "array") {
    return canonicalizeArray(
      value,
      declaration.element,
      declarations,
      path,
      mergeConstraints(declaration.constraints, constraints),
    );
  }
  if (declaration.kind === "enum") {
    if (!declaration.values.includes(value)) {
      fail("geometer.contract.enum_mismatch", path, "Unexpected enum value.");
    }
    return value;
  }
  if (declaration.kind === "union") {
    const failures: ContractCodecError[] = [];
    for (const variant of declaration.variants) {
      try {
        return canonicalize(value, variant, declarations, path, constraints);
      } catch (error) {
        if (error instanceof ContractCodecError) failures.push(error);
        else throw error;
      }
    }
    const best = failures.find((error) => error.path !== path) ?? failures[0];
    fail(
      "geometer.contract.union_mismatch",
      best?.path ?? path,
      best?.message ?? "Value does not match a union variant.",
    );
  }
  if (!isObject(value)) fail("geometer.contract.type_mismatch", path, "Expected object.");
  const known = new Set(Object.keys(declaration.properties));
  for (const key of Object.keys(value)) {
    if (!known.has(key)) {
      fail("geometer.contract.unknown_field", childPath(path, key), "Unknown field.");
    }
  }
  const result: Record<string, unknown> = {};
  for (const [name, property] of Object.entries(declaration.properties)) {
    if (!Object.hasOwn(value, name)) {
      if (!property.optional) {
        fail(
          "geometer.contract.missing_field",
          childPath(path, name),
          "Required field is missing.",
        );
      }
      continue;
    }
    result[name] = canonicalize(
      value[name],
      property.type,
      declarations,
      childPath(path, name),
      property.constraints,
    );
  }
  return result;
}

function canonicalizeArray(
  value: unknown,
  element: ContractTypeDescriptor,
  declarations: ContractDescriptorMap,
  path: string,
  constraints: ContractConstraints,
): readonly unknown[] {
  if (!Array.isArray(value)) fail("geometer.contract.type_mismatch", path, "Expected array.");
  if (constraints.min_items !== undefined && value.length < constraints.min_items) {
    fail("geometer.contract.array_size", path, "Array is shorter than its minimum.");
  }
  if (constraints.max_items !== undefined && value.length > constraints.max_items) {
    fail("geometer.contract.array_size", path, "Array is longer than its maximum.");
  }
  return Array.from({ length: value.length }, (_, index) =>
    canonicalize(value[index], element, declarations, childPath(path, String(index)), {}),
  );
}

function validateString(value: string, path: string, constraints: ContractConstraints): void {
  for (let index = 0; index < value.length; index += 1) {
    const code = value.charCodeAt(index);
    if (code >= 0xd800 && code <= 0xdbff) {
      const next = value.charCodeAt(index + 1);
      if (next < 0xdc00 || next > 0xdfff) {
        fail("geometer.contract.invalid_unicode", path, "String contains an unpaired surrogate.");
      }
      index += 1;
    } else if (code >= 0xdc00 && code <= 0xdfff) {
      fail("geometer.contract.invalid_unicode", path, "String contains an unpaired surrogate.");
    }
  }
  const bytes = new TextEncoder().encode(value).byteLength;
  if (constraints.min_length !== undefined && bytes < constraints.min_length) {
    fail("geometer.contract.string_size", path, "String is shorter than its minimum.");
  }
  if (constraints.max_length !== undefined && bytes > constraints.max_length) {
    fail("geometer.contract.string_size", path, "String is longer than its maximum.");
  }
}

function isObject(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

function mergeConstraints(
  declaration: ContractConstraints,
  useSite: ContractConstraints,
): ContractConstraints {
  return { ...declaration, ...useSite };
}

function childPath(path: string, token: string): string {
  return `${path}/${token.replaceAll("~", "~0").replaceAll("/", "~1")}`;
}

function fail(code: string, path: string, message: string): never {
  throw new ContractCodecError(code, path, message);
}

class StrictJsonParser {
  readonly text: string;
  offset = 0;

  constructor(text: string) {
    this.text = text;
  }

  parse(): unknown {
    this.skipWhitespace();
    const value = this.parseValue();
    this.skipWhitespace();
    if (this.offset !== this.text.length) this.parseFailure("Trailing JSON data");
    return value;
  }

  private parseValue(): unknown {
    const character = this.text[this.offset];
    if (character === "{") return this.parseObject();
    if (character === "[") return this.parseArray();
    if (character === '"') return this.parseString();
    if (character === "t") return this.parseLiteral("true", true);
    if (character === "f") return this.parseLiteral("false", false);
    if (character === "n") return this.parseLiteral("null", null);
    if (character === "-" || (character !== undefined && character >= "0" && character <= "9")) {
      return this.parseNumber();
    }
    return this.parseFailure("Expected a JSON value");
  }

  private parseObject(): Record<string, unknown> {
    this.offset += 1;
    this.skipWhitespace();
    const value: Record<string, unknown> = Object.create(null);
    const keys = new Set<string>();
    if (this.consume("}")) return value;
    while (true) {
      if (this.text[this.offset] !== '"') this.parseFailure("Expected an object key");
      const key = this.parseString();
      if (keys.has(key)) {
        throw new ContractCodecError(
          "geometer.contract.duplicate_field",
          childPath("", key),
          "Object contains a duplicate field.",
        );
      }
      keys.add(key);
      this.skipWhitespace();
      if (!this.consume(":")) this.parseFailure("Expected ':' after object key");
      this.skipWhitespace();
      value[key] = this.parseValue();
      this.skipWhitespace();
      if (this.consume("}")) return value;
      if (!this.consume(",")) this.parseFailure("Expected ',' or '}' in object");
      this.skipWhitespace();
    }
  }

  private parseArray(): unknown[] {
    this.offset += 1;
    this.skipWhitespace();
    const value: unknown[] = [];
    if (this.consume("]")) return value;
    while (true) {
      value.push(this.parseValue());
      this.skipWhitespace();
      if (this.consume("]")) return value;
      if (!this.consume(",")) this.parseFailure("Expected ',' or ']' in array");
      this.skipWhitespace();
    }
  }

  private parseString(): string {
    const start = this.offset;
    this.offset += 1;
    let escaped = false;
    while (this.offset < this.text.length) {
      const code = this.text.charCodeAt(this.offset);
      const character = this.text[this.offset];
      if (!escaped && character === '"') {
        this.offset += 1;
        const value = JSON.parse(this.text.slice(start, this.offset));
        validateString(value, "", {});
        return value;
      }
      if (!escaped && code < 0x20) this.parseFailure("Unescaped control character in string");
      if (!escaped && character === "\\") escaped = true;
      else escaped = false;
      this.offset += 1;
    }
    return this.parseFailure("Unterminated string");
  }

  private parseNumber(): number {
    const match = /^-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?/u.exec(
      this.text.slice(this.offset),
    );
    if (!match) return this.parseFailure("Invalid JSON number");
    this.offset += match[0].length;
    const value = Number(match[0]);
    if (!Number.isFinite(value)) return this.parseFailure("Non-finite JSON number");
    return value;
  }

  private parseLiteral(source: string, value: unknown): unknown {
    if (!this.text.startsWith(source, this.offset)) this.parseFailure(`Invalid literal ${source}`);
    this.offset += source.length;
    return value;
  }

  private skipWhitespace(): void {
    while ([" ", "\t", "\r", "\n"].includes(this.text[this.offset] ?? "")) this.offset += 1;
  }

  private consume(character: string): boolean {
    if (this.text[this.offset] !== character) return false;
    this.offset += 1;
    return true;
  }

  private parseFailure(message: string): never {
    throw new ContractCodecError(
      "geometer.contract.invalid_json",
      "",
      `${message} at character offset ${this.offset}.`,
    );
  }
}
