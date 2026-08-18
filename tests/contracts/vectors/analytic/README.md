# Analytic Packet A0 Vectors

These lowercase-hex files are the governed, language-neutral exact-byte corpus
for the separately governed `GMABRQ01` request and `GMABRS01` result formats.
They are emitted by the native C++ codec tests and replayed without rewriting by
the C++, TypeScript, Rust, and Python strict packet codecs.

Regenerate after an intentional packet-contract change with:

```powershell
python scripts/write_analytic_packet_vectors.py
```

`manifest.json` pins byte counts, SHA-256 digests, normative per-job standalone
digests, and the strict mutation families every projection must reject. These
are binary packet vectors, not logical JSON contract vectors.

The `cross_transport_parity` entry is a production-boundary corpus rather than
a codec-producer vector. It adapts MATZ `analytic_primitive_family` case 3 and
adds a self-relationship query. The validator sends its one canonical request
unchanged through native executable IPC and the distributed full-browser WASM
generic-operation ABI, then requires both result attachments to equal the
governed result bytes exactly. Strict result decoding independently recomputes
the same standalone job digest in both transports.

The `real-board/` subtree contains separately authorized external qualification
requests. RT `PWR4` is retained once as raw `.gmabrq01` bytes with its exporter
manifest, corpus, and clearly local/nonpromotional report. It is not generated
by `write_analytic_packet_vectors.py`, is not duplicated as hex, and contains no
source CAD or full result packet.
