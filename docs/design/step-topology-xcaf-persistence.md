# Standard XCAF Binary And XML Persistence Baseline

Status: experimental standard-attribute authored-state baseline and
cross-version custom-driver probe

Date: 2026-08-23

## Question And Boundary

Can standard OCCT storage preserve neutral metadata at the XCAF locations that
matter to the annotation research without defining a custom attribute or
changing B-rep geometry? The focused native baseline answers yes for same-build
OCCT 8.0.1 using both standard binary XCAF (`BinXCAF`) and XML XCAF
(`XmlXCAF`).

This is an OCCT working-document/cache result. XBF and XML XCAF are not STEP,
AP242, vendor-neutral interchange, browser state, or evidence of third-party
CAD survival. The tested values remain explicitly namespaced Geometer research
probes, not an annotation contract.

## Exact Driver Posture

Native Geometer now links `TKBinXCAF` and `TKXmlXCAF` in addition to its
existing XCAF toolkits. The test obtains the one valid
`XCAFApp_Application`, explicitly calls `BinXCAFDrivers::DefineFormat()` and
`XmlXCAFDrivers::DefineFormat()`, and verifies that both formats appear in the
application's reading and writing inventories before creating a document.

The current matrix row is:

| Writer | Reader | Formats | OCAF storage version | Result |
| --- | --- | --- | ---: | --- |
| OCCT 8.0.1, MSVC v143 x64 static `/MD` | same build | `BinXCAF`, `XmlXCAF` | serialized header 12 (`TDocStd_FormatVersion_CURRENT`) | pass |

The custom-driver probe adds these exact rows:

| Writer | Reader | Formats | OCAF storage version | Result |
| --- | --- | --- | ---: | --- |
| OCCT 7.9.3 | OCCT 7.9.3 | binary and XML research formats | 12 | exact payload pass |
| OCCT 7.9.3 | OCCT 8.0.1 | binary and XML research formats | 12 | exact payload pass |
| OCCT 8.0.1 | OCCT 8.0.1 | binary and XML research formats | 12 | exact payload pass |
| OCCT 8.0.1 | OCCT 7.9.3 | binary and XML research formats | 12 | exact payload pass |
| either tested version with custom writer | same-version stock XCAF reader, probe driver absent | binary and XML research formats | 12 | document reports readable; probe attribute omitted |

The selected compatibility pair is OCCT 7.9.3
(`a016080bf6738d6aeae020badee4e888ad1540a5`) versus OCCT 8.0.1
(`b8f597c677811d1f9f4d8a97f5ae2825c0353a42`), using the same MSVC v143 x64
static `/MD` posture and compiler 19.44.35221.0. Both custom-driver directions
were executed. They do not establish compatibility for arbitrary OCAF
attributes, geometry, larger payloads, or authored group/hierarchy state.
OCCT's format-version warning still applies whenever a writer actually uses a
storage version newer than the reader supports.

## Minimal Custom-Driver Probe

`geometer_step_topology_xcaf_custom_driver_test` defines one test-only
`TDF_Attribute`, rather than treating a standard attribute with a user GUID as
a custom-driver result. Its Wavenumber research GUID is
`eacfa1c8-42b2-4b9e-9d69-e3f050eaf8a1`; the XML driver uses prefix `wn` and
namespace `https://wavenumber.com/ns/geometer/research/ocaf/a0`. Both binary and
XML storage/retrieval driver tables are derived from the normal XCAF tables and
then extended with only this attribute driver. The test registers isolated
research format names programmatically and verifies the serialized format
version from each artifact. `scripts/run_xcaf_custom_driver_matrix.py` builds
the identical source against both installed versions, runs same-version and
both cross-version directions, exercises the missing-reader loss mode, and
writes an uncommitted JSON evidence report with source/artifact digests,
compiler identity, commands, and timing. The report measures the host,
compiler, and runtime OCCT version. It labels the selected source tag/commit as
expected rather than pretending the harness validated source provenance, and
does not infer static/shared linkage or C runtime posture from an install path.

For this minimal payload, both writers produced byte-identical artifacts:
the 1,328-byte XBF SHA-256 is
`fbeffa3be14b37aaeb7a3810ad07be04860ecb34ed602b04e5ac382fa7252b32`, and
the 2,248-byte XML SHA-256 is
`6468a1b173679d7b5c4596d4ee370623d817d41a3c8172af08eacac03e8fc188`.
This is an observed property of the tiny probe, not a general deterministic
serialization guarantee.

The missing-reader experiment deliberately writes with the custom driver and
loads through an otherwise normal XCAF retrieval driver that does not know the
attribute. Both tested versions return `PCDM_RS_OK` and preserve the readable
document while dropping the attribute. The binary reader also warns that
`GeometerResearchProbe` has no driver; XML omission was not accompanied by the
same console warning.
Consumers therefore must verify required authored records after reload; open
success alone is insufficient.

## Persisted Evidence

`geometer_step_topology_xcaf_persistence_test` constructs a redistribution-safe
box definition, two explicit face subshape labels, and a nested assembly graph
that references the same definition twice. One occurrence is directly below
the root with identity placement; the other is below a subassembly with a
30-unit local translation and 40-unit accumulated translation. It assigns both
`TDataStd_Name` and `TDataStd_NamedData` evidence across:

- document main label;
- shape definition;
- root and nested assembly definitions;
- all three component occurrences; and
- both face subshapes.

Namespaced standard NamedData records model representative authored state: one
logical group with two face members, a hierarchy summary, authored ids for the
product and assemblies, and distinct authored ids for each occurrence. These
are deliberately research keys and compact exact JSON/string payloads, not the
future carrier-neutral annotation contract.

For each format it saves to an isolated temporary directory, closes the live
document, reopens the artifact through the registered retrieval driver, and
resolves the original label entries without creating missing labels. All
standard names, authored keys, and values survive exactly. The restored XCAF
graph retains both assembly roles, the two root components and one nested
component, all occurrence-to-definition references, both explicit
face-subshape roles, and both definition-to-face associations. Local
placements and the accumulated nested placement also survive. The test reads
version 12 from the serialized binary info section and XML `DocVersion`
attribute rather than relying on a new document's default property.

The reloaded definition and root assembly are non-null and B-rep valid. The
definition retains six faces and 6,000 cubic model units; the root retains the
pre-save 12 faces and 12,000 cubic model units within the asserted `1e-9`
absolute volume tolerance. This proves the tested group and hierarchy records
round trip without editing or duplicating the definition B-rep.

## Consequence

Standard NamedData is viable for a same-version XBF/XML autosave or research
cache. It can preserve representative group and synthetic-hierarchy state at
document, definition, assembly, occurrence, and face locations that are
difficult to represent uniformly in STEP. It does not by itself preserve the
native edit journal's atomic transaction history, source-bound recovery proof,
or future semantic meaning. The exact-source binary edit journal therefore
remains the current restart authority.

## Deferred Evidence

- unknown transient type/GUID variants beyond the measured missing-reader
  driver loss mode;
- a production mapping of the complete logical-group and edit journal into
  XCAF attributes;
- storage-version downgrade behavior;
- cancellation, size, and corrupted-file behavior; and
- comparison with AP242 and the portable sidecar path.
