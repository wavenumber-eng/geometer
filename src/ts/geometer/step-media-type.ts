/** Canonical STEP input attachment type; legacy application/step remains accepted. */
export const STEP_MEDIA_TYPE = "model/step";
const specials = new Set('()<>@,;:\\"/[]?=');
/** Input syntax failure, distinct from transport or runtime failures. */
export class StepMediaTypeError extends Error {}

class Field {
  position = 0;
  constructor(readonly value: string) {}
  take(expected: string): void {
    this.space();
    if (this.value[this.position] !== expected)
      throw new StepMediaTypeError("Invalid STEP MIME syntax");
    this.position++;
  }
  token(): string {
    this.space();
    const start = this.position;
    while (this.position < this.value.length) {
      const character = this.value.charAt(this.position);
      if (specials.has(character) || character === " " || character === "\t") break;
      this.position++;
    }
    if (start === this.position) throw new StepMediaTypeError("Invalid STEP MIME token");
    return this.value.slice(start, this.position);
  }
  space(): void {
    while (this.position < this.value.length) {
      const character = this.value[this.position];
      if (character === " " || character === "\t") this.position++;
      else if (character === "(") this.comment();
      else return;
    }
  }
  comment(): void {
    let depth = 1;
    this.position++;
    while (this.position < this.value.length) {
      const character = this.value[this.position++];
      if (character === "\\") {
        if (this.position >= this.value.length)
          throw new StepMediaTypeError("Invalid STEP MIME escape");
        this.position++;
      } else if (character === "(") depth++;
      else if (character === ")" && --depth === 0) return;
    }
    throw new StepMediaTypeError("Unclosed STEP MIME comment");
  }
  parameterValue(): void {
    this.space();
    if (this.value[this.position] !== '"') {
      this.token();
      return;
    }
    this.position++;
    while (this.position < this.value.length) {
      const character = this.value[this.position++];
      if (character === '"') return;
      if (character === "\\") {
        if (this.position >= this.value.length)
          throw new StepMediaTypeError("Invalid STEP MIME escape");
        this.position++;
      }
    }
    throw new StepMediaTypeError("Unclosed STEP MIME quote");
  }
}

/** Validate the complete bounded RFC 2045 field, without interpreting auxiliary parameters. */
export function canonicalStepMediaType(value: string): string | undefined {
  if (
    value.length === 0 ||
    value.length > 128 ||
    [...value].some((character) => {
      const code = character.charCodeAt(0);
      return code > 126 || (code < 32 && character !== "\t");
    })
  )
    return undefined;
  const field = new Field(value);
  try {
    const major = field.token().toLowerCase();
    field.take("/");
    if (!["model", "application"].includes(major) || field.token().toLowerCase() !== "step")
      return undefined;
    field.space();
    while (field.position < value.length) {
      field.take(";");
      field.token();
      field.take("=");
      field.parameterValue();
      field.space();
    }
    return STEP_MEDIA_TYPE;
  } catch (error) {
    if (error instanceof StepMediaTypeError) return undefined;
    throw error;
  }
}

/** Normalize only a negotiated STEP input slot; non-STEP attachment identities remain exact. */
export function stepInputMediaType(value: string, declared: readonly string[]): string {
  if (!declared.some((type) => type === "model/step" || type === "application/step")) return value;
  const canonical = canonicalStepMediaType(value);
  if (canonical !== undefined) return canonical;
  if (declared.includes(value)) return value;
  throw new StepMediaTypeError("Invalid STEP input media type");
}

/** Match STEP request aliases against either declared STEP token; all other types stay exact. */
export function stepInputMatchesDeclaration(value: string, declared: readonly string[]): boolean {
  return (
    declared.includes(value) ||
    (declared.some((type) => type === "model/step" || type === "application/step") &&
      canonicalStepMediaType(value) !== undefined)
  );
}
