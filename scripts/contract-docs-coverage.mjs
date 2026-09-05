// Presentation only: identities and authority come from the catalog/manifest.
export function maturityOf(record) {
  if (record.maturity) return record.maturity;
  if (/analytic|step_topology/.test(record.id ?? "")) return "experimental_not_production_ready";
  return "See interface support policy; promotion is not maturity";
}

export function runtimeOf(operation) {
  if (!operation) return "No generated operation / no generic IPC adapter";
  if (operation.runtime_available) return "Portable and native IPC";
  if (operation.native_runtime_available) return "Native IPC only";
  return "Structural only";
}

export function renderCoverage(catalog, manifest, escapeHtml) {
  const e = (value) => escapeHtml(String(value ?? ""));
  const generated = new Map(catalog.operations.map((item) => [item.identity, item]));
  const entries = [...manifest.operations, ...manifest.candidateOperations];
  const ids = new Set(entries.map((item) => item.id));
  if (ids.size !== entries.length) throw new Error("Duplicate governed operation identity.");
  for (const operation of catalog.operations) {
    if (!ids.has(operation.identity))
      throw new Error(`Missing operation inventory: ${operation.identity}`);
  }
  const rows = entries
    .map((item) => {
      const operation = generated.get(item.id);
      const aliases = [...(item.cli_names ?? []), ...(item.compatibility_cli_names ?? [])];
      return `<tr><td><code>${e(item.id)}</code></td><td>${e(item.status)}</td><td>${e(maturityOf(item))}</td><td>${e(runtimeOf(operation))}</td><td><code>${e(item.request_contract)}</code><br><code>${e(item.result_contract)}</code></td><td>${aliases.map(e).join("<br>") || "none inventoried"}</td></tr>`;
    })
    .join("\n");
  const roots = new Set(catalog.roots.map((item) => item.contract_identity));
  const packets = manifest.binaryFormats
    .map(
      (item) =>
        `<tr><td><code>${e(item.id)}</code></td><td>${e(item.status)}; handwritten codec</td><td><code>${e(item.request_magic)}</code> / <code>${e(item.response_magic ?? "input only")}</code></td><td>${e(item.version)}</td><td><code>${e(item.source)}</code></td><td>${
          Object.entries(item)
            .filter(([key]) => key.startsWith("maximum_"))
            .map(([key, value]) => `${e(key)}: ${e(value)}`)
            .join("<br>") || "See authored specification"
        }</td></tr>`,
    )
    .join("\n");
  const contracts = manifest.contracts
    .map(
      (item) =>
        `<tr><td><code>${e(item.id)}</code></td><td>${e(item.status)}</td><td>${e(item.current_authority)}</td><td>${roots.has(item.id) ? "C++, TS, Rust, Python, JSON Schema, HTML" : "Not a generated root"}</td><td><code>${e(item.source ?? item.sources ?? "See promotion manifest")}</code></td></tr>`,
    )
    .join("\n");
  return `<header><p class="page-type">Generated coverage audit</p><h1>Operation And Contract Coverage</h1></header>
<nav class="nav"><a href="index.html">Contract reference</a><a href="../../contracts/current-interface-inventory.md">Interface and packet authority</a><a href="../../contracts/typespec-coverage-assessment.md">Migration waves</a><a href="../../design/executable-ipc.md">IPC quick start</a></nav>
<aside class="callout">Generated from the complete promotion inventory, including handwritten gaps. This is not a list of universally callable operations. Solver maturity, structural promotion and runtime exposure are independent.</aside>
<section><h2>Operations (${entries.length}; ${catalog.operations.length} TypeSpec declarations)</h2><table><thead><tr><th>Identity</th><th>Lifecycle</th><th>Maturity</th><th>Generic runtime</th><th>Logical request / result</th><th>CLI batch aliases</th></tr></thead><tbody>${rows}</tbody></table></section>
<section><h2>Structural authority</h2><table><thead><tr><th>Identity</th><th>Lifecycle</th><th>Authority</th><th>Generated projections</th><th>Source</th></tr></thead><tbody>${contracts}</tbody></table></section>
<section><h2>Governed packet inventory</h2><table><thead><tr><th>Identity</th><th>Lifecycle / codec</th><th>Request / result magic</th><th>Version</th><th>Source</th><th>Governed limits</th></tr></thead><tbody>${packets}</tbody></table></section>
<section><h2>Coverage boundaries</h2><p>Browser illustration has generated roots but executes in TypeScript, not executable IPC. Generated HLR/analytic envelopes do not make indexed-mesh or analytic packed codecs generated. Framing GMIPCA01 and planar/Clipper2 packets remain separately governed. Direct C++ helpers, version/free functions and CLI aliases are not automatically new semantic operations.</p><p>Binary offsets are not generated here because no complete machine-readable layout authority exists yet. Use the <a href="../../design/binary-formats.md">authored packet reference</a> and <a href="../../design/analytic-planar-boolean-packet-a0.md">experimental analytic packet specification</a>.</p></section>`;
}
