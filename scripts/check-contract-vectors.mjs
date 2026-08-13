// @ts-check

import { existsSync } from "node:fs";
import { readdir, readFile } from "node:fs/promises";
import { dirname, join, relative, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";

import Ajv2020 from "ajv/dist/2020.js";

const repositoryRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const vectorRoot = join(repositoryRoot, "tests", "contracts", "vectors");
const manifestPath = join(vectorRoot, "manifest.json");
const catalogPath = join(
  repositoryRoot,
  "contracts",
  "geometer",
  "generated",
  "wn_geometer_contract_catalog.a0.json",
);

async function main() {
  const manifest = JSON.parse(await readFile(manifestPath, "utf8"));
  const catalog = JSON.parse(await readFile(catalogPath, "utf8"));
  validateManifest(manifest);

  const schemas = new Map();
  const ajv = new Ajv2020({ allErrors: true, strict: true });
  ajv.addKeyword({ keyword: "x-wn-default-intent", schemaType: "string" });
  for (const root of catalog.roots) {
    const filename = `${root.name.slice(root.name.lastIndexOf(".") + 1)}.json`;
    const schemaPath = join(
      repositoryRoot,
      "contracts",
      "geometer",
      "generated",
      "schema",
      filename,
    );
    schemas.set(
      root.contract_identity,
      ajv.compile(JSON.parse(await readFile(schemaPath, "utf8"))),
    );
  }

  const referencedFiles = new Set();
  for (const vector of manifest.vectors) {
    const vectorPath = join(vectorRoot, vector.file);
    if (!existsSync(vectorPath)) {
      throw new Error(`${vector.id}: missing ${vector.file}`);
    }
    referencedFiles.add(vector.file);
    const storedBytes = await readFile(vectorPath);
    const bytes =
      vector.oracle === "strict_parser_hex" ? decodeHexVector(storedBytes, vector.id) : storedBytes;
    let value;
    let parseError = null;
    try {
      value = parseStrictJson(bytes);
    } catch (error) {
      parseError = error;
    }

    if (vector.lane === "strict_json") {
      assertOutcome(vector, parseError === null, parseError);
      continue;
    }
    if (parseError) {
      throw new Error(`${vector.id}: non-strict lane has invalid JSON: ${formatError(parseError)}`);
    }
    if (vector.lane === "schema") {
      const validate = schemas.get(vector.contract_identity);
      if (!validate) {
        throw new Error(`${vector.id}: unknown contract ${vector.contract_identity}`);
      }
      const valid = validate(value);
      assertOutcome(vector, valid, valid ? null : ajv.errorsText(validate.errors));
      continue;
    }
    if (vector.lane === "semantic" && vector.oracle === "presence_projection") {
      const actual = {};
      for (const field of vector.fields) {
        actual[field] = Object.hasOwn(value, field) ? "present" : "absent";
      }
      assertEqual(actual, vector.expected_value, `${vector.id} presence projection`);
      continue;
    }
    throw new Error(`${vector.id}: unsupported lane/oracle ${vector.lane}/${vector.oracle}`);
  }

  const caseFiles = (await listFiles(join(vectorRoot, "cases"))).map((path) => `cases/${path}`);
  assertEqual([...referencedFiles].sort(), caseFiles.sort(), "vector case inventory");
  process.stdout.write(`Validated ${manifest.vectors.length} governed contract vectors.\n`);
}

function decodeHexVector(storedBytes, id) {
  const text = storedBytes.toString("ascii").trim();
  if (!/^(?:[0-9a-f]{2})+$/u.test(text)) {
    throw new Error(`${id}: invalid lowercase even-length hex fixture`);
  }
  return Buffer.from(text, "hex");
}

function validateManifest(value) {
  if (value.manifest_identity !== "wn.geometer.contract_vectors" || value.generation !== "a0") {
    throw new Error("Unexpected contract-vector manifest identity or generation.");
  }
  if (!Array.isArray(value.vectors) || value.vectors.length === 0) {
    throw new Error("Contract-vector manifest must contain vectors.");
  }
  const ids = new Set();
  for (const vector of value.vectors) {
    if (!vector.id || ids.has(vector.id)) {
      throw new Error(`Missing or duplicate vector id ${String(vector.id)}.`);
    }
    ids.add(vector.id);
    if (!["strict_json", "schema", "semantic"].includes(vector.lane)) {
      throw new Error(`${vector.id}: unsupported initial assertion lane ${vector.lane}.`);
    }
    if (!["accept", "reject"].includes(vector.expected)) {
      throw new Error(`${vector.id}: expected must be accept or reject.`);
    }
    if (vector.comparison !== "exact" || !Array.isArray(vector.excluded_fields)) {
      throw new Error(`${vector.id}: comparison and excluded_fields must be explicit.`);
    }
    if (vector.tolerance !== null) {
      throw new Error(`${vector.id}: initial exact vectors must declare null tolerance.`);
    }
    if (!vector.file.startsWith("cases/") || vector.file.includes("..")) {
      throw new Error(`${vector.id}: invalid repository-relative case path.`);
    }
  }
}

function assertOutcome(vector, valid, detail) {
  const expectedValid = vector.expected === "accept";
  if (valid !== expectedValid) {
    throw new Error(
      `${vector.id}: expected ${vector.expected}, got ${valid ? "accept" : "reject"}${detail ? `: ${formatError(detail)}` : ""}`,
    );
  }
}

function parseStrictJson(bytes) {
  const text = new TextDecoder("utf-8", { fatal: true }).decode(bytes);
  return new StrictJsonParser(text).parse();
}

class StrictJsonParser {
  constructor(text) {
    this.text = text;
    this.offset = 0;
  }

  parse() {
    this.skipWhitespace();
    const value = this.parseValue();
    this.skipWhitespace();
    if (this.offset !== this.text.length) this.fail("trailing JSON data");
    return value;
  }

  parseValue() {
    const character = this.text[this.offset];
    if (character === "{") return this.parseObject();
    if (character === "[") return this.parseArray();
    if (character === '"') return this.parseString();
    if (character === "t") return this.parseLiteral("true", true);
    if (character === "f") return this.parseLiteral("false", false);
    if (character === "n") return this.parseLiteral("null", null);
    if (character === "-" || (character >= "0" && character <= "9")) return this.parseNumber();
    this.fail("expected a JSON value");
  }

  parseObject() {
    this.offset += 1;
    this.skipWhitespace();
    const value = {};
    const keys = new Set();
    if (this.consume("}")) return value;
    while (true) {
      if (this.text[this.offset] !== '"') this.fail("expected an object key");
      const key = this.parseString();
      if (keys.has(key)) this.fail(`duplicate object key ${JSON.stringify(key)}`);
      keys.add(key);
      this.skipWhitespace();
      if (!this.consume(":")) this.fail("expected ':' after object key");
      this.skipWhitespace();
      value[key] = this.parseValue();
      this.skipWhitespace();
      if (this.consume("}")) return value;
      if (!this.consume(",")) this.fail("expected ',' or '}' in object");
      this.skipWhitespace();
    }
  }

  parseArray() {
    this.offset += 1;
    this.skipWhitespace();
    const value = [];
    if (this.consume("]")) return value;
    while (true) {
      value.push(this.parseValue());
      this.skipWhitespace();
      if (this.consume("]")) return value;
      if (!this.consume(",")) this.fail("expected ',' or ']' in array");
      this.skipWhitespace();
    }
  }

  parseString() {
    const start = this.offset;
    this.offset += 1;
    let escaped = false;
    while (this.offset < this.text.length) {
      const code = this.text.charCodeAt(this.offset);
      const character = this.text[this.offset];
      if (!escaped && character === '"') {
        this.offset += 1;
        return JSON.parse(this.text.slice(start, this.offset));
      }
      if (!escaped && code < 0x20) this.fail("unescaped control character in string");
      if (!escaped && character === "\\") {
        escaped = true;
      } else {
        escaped = false;
      }
      this.offset += 1;
    }
    this.fail("unterminated string");
  }

  parseNumber() {
    const match = /^-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?/u.exec(
      this.text.slice(this.offset),
    );
    if (!match) this.fail("invalid JSON number");
    this.offset += match[0].length;
    const value = Number(match[0]);
    if (!Number.isFinite(value)) this.fail("non-finite JSON number");
    return value;
  }

  parseLiteral(source, value) {
    if (!this.text.startsWith(source, this.offset)) this.fail(`invalid literal ${source}`);
    this.offset += source.length;
    return value;
  }

  skipWhitespace() {
    while ([" ", "\t", "\r", "\n"].includes(this.text[this.offset])) this.offset += 1;
  }

  consume(character) {
    if (this.text[this.offset] !== character) return false;
    this.offset += 1;
    return true;
  }

  fail(message) {
    throw new Error(`${message} at byte-compatible character offset ${this.offset}`);
  }
}

async function listFiles(root) {
  const output = [];
  async function visit(directory) {
    const entries = await readdir(directory, { withFileTypes: true });
    entries.sort((left, right) => left.name.localeCompare(right.name));
    for (const entry of entries) {
      const absolute = join(directory, entry.name);
      if (entry.isDirectory()) await visit(absolute);
      else if (entry.isFile()) output.push(relative(root, absolute).split(sep).join("/"));
      else throw new Error(`Unsupported vector entry ${absolute}.`);
    }
  }
  await visit(root);
  return output;
}

function assertEqual(actual, expected, label) {
  if (JSON.stringify(actual) !== JSON.stringify(expected)) {
    throw new Error(
      `${label}: expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`,
    );
  }
}

function formatError(value) {
  return value instanceof Error ? value.message : String(value);
}

await main();
