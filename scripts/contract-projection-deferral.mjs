// @ts-check

import { readFile } from "node:fs/promises";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const repositoryRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const manifestPath = join(repositoryRoot, "docs", "contracts", "promotion-manifest.toml");

/**
 * Remove frozen candidate operations whose language projection is deferred by
 * the promotion manifest, together with their contract roots and any
 * declarations no longer reachable from the remaining roots. The normalized
 * catalog itself stays complete; only the requested projection is narrowed.
 */
export async function applyProjectionDeferrals(catalog, language, options = {}) {
  const deferred = (await readCandidateOperations()).filter(
    (candidate) =>
      candidate.status === "contract_frozen" &&
      (candidate.deferred_projections ?? []).includes(language),
  );
  if (deferred.length === 0) return catalog;
  const deferredOperations = new Set(deferred.map((candidate) => candidate.id));
  const deferredContracts = new Set(
    deferred.flatMap((candidate) => [candidate.request_contract, candidate.result_contract]),
  );
  const roots = catalog.roots.filter((root) => !deferredContracts.has(root.contract_identity));
  const operations = catalog.operations.filter(
    (operation) =>
      !deferredOperations.has(operation.identity) ||
      (options.retainRuntimeDispatch === true &&
        operation.runtime_dispatch === "packed_attachment"),
  );
  const keep = reachableNames(catalog, roots);
  return {
    ...catalog,
    roots,
    operations,
    declarations: catalog.declarations.filter((declaration) => keep.has(declaration.name)),
  };
}

function reachableNames(catalog, roots) {
  const declarationByName = new Map(
    catalog.declarations.map((declaration) => [declaration.name, declaration]),
  );
  const found = new Set();
  function visitName(name) {
    if (found.has(name)) return;
    const declaration = declarationByName.get(name);
    if (!declaration) throw new Error(`Catalog reference does not resolve: ${name}.`);
    found.add(name);
    if (declaration.kind === "model") {
      visitType(declaration.base);
      visitType(declaration.index_value);
      for (const property of declaration.properties) visitType(property.type);
    } else if (declaration.kind === "scalar") {
      visitType(declaration.base);
    } else if (declaration.kind === "union") {
      for (const variant of declaration.variants) visitType(variant.type);
    }
  }
  function visitType(type) {
    if (!type) return;
    if (type.kind === "reference") visitName(type.target);
    else if (type.kind === "array") visitType(type.element);
    else if (type.kind === "record") visitType(type.value);
  }
  for (const root of roots) visitName(root.name);
  return found;
}

async function readCandidateOperations() {
  const text = await readFile(manifestPath, "utf8");
  const candidates = [];
  let target = null;
  for (const rawLine of text.split(/\r?\n/u)) {
    const line = rawLine.trim();
    if (!line || line.startsWith("#")) continue;
    if (line === "[[candidate_operations]]") {
      target = {};
      candidates.push(target);
      continue;
    }
    if (line.startsWith("[") && line.endsWith("]")) {
      target = null;
      continue;
    }
    if (!target) continue;
    const match = /^([A-Za-z0-9_]+)\s*=\s*(.+)$/u.exec(line);
    if (match) target[match[1]] = parseTomlValue(match[2]);
  }
  return candidates;
}

function parseTomlValue(source) {
  if (source === "true") return true;
  if (source === "false") return false;
  if (/^-?[0-9]+$/u.test(source)) return Number(source);
  if (source.startsWith('"') && source.endsWith('"')) return JSON.parse(source);
  if (source.startsWith("[") && source.endsWith("]")) return JSON.parse(source);
  throw new Error(`Unsupported promotion-manifest value: ${source}`);
}
