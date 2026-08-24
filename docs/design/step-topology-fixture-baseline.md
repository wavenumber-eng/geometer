# STEP Topology Fixture Baseline

Status: measured research evidence

Date: 2026-08-22

OCCT: 8.0.1 (`V8_0_1`)

## Purpose

This baseline records what OCCT/XCAF exposes before Geometer defines a topology
inspection contract. It distinguishes product definitions, component
occurrences, and topology owned by simple-shape definitions. It does not assign
durable topology identity and does not define an annotation vocabulary.

The machine-readable evidence is
[`docs/reports/step-topology-fixture-baseline.json`](../reports/step-topology-fixture-baseline.json).
Its native generator and freshness test are implemented by
`geometer_step_topology_fixture_inventory`.

## Fixture Findings

| Case | Definitions | Assemblies | Component labels | Expanded paths | Simple definitions | Solids | Faces |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| miniature test point, AP203 | 1 | 0 | 0 | 0 | 1 | 1 | 50 |
| SOT-23, AP214 | 1 | 0 | 0 | 0 | 1 | 1 | 53 |
| RESC1608X06L, AP214 | 5 | 2 | 5 | 6 | 3 | 3 | 22 |
| SOIC-20-300, AP214 | 3 | 1 | 21 | 21 | 2 | 2 | 46 |
| ABM3B, AP214 | 6 | 1 | 7 | 7 | 5 | 5 | 171 |
| generated nested repeated assembly, AP242 | 3 | 2 | 4 | 6 | 1 | 1 | 6 |
| generated fused slab, AP242 | 1 | 0 | 0 | 0 | 1 | 1 | 14 |
| generated flat multi-solid, AP242 | 1 | 0 | 0 | 0 | 1 | 2 | 12 |

Solid, shell, and face counts are taken only from simple-shape definitions.
Assembly compound shapes are not counted again, so repeated occurrences do not
inflate definition topology. Component labels count the unique XCAF definition
graph. Expanded paths recursively instantiate reused subassemblies from every
free root. Each expanded path retains its full label path and accumulated 3x4
transform. The report hashes that canonical path/transform evidence separately
from a semantic digest covering definition names, topology counts, bounds,
volume, and occurrences. `located_occurrences` compares the accumulated 3x4
matrix to numerical identity with a `1e-9` tolerance; it does not use OCCT's
location-chain `IsIdentity()` predicate.

STEP byte counts and SHA-256 values canonicalize CRLF and LF to LF so the
fixture evidence is stable across Git checkouts. This normalization is limited
to the baseline report; runtime source identity must hash the exact submitted
bytes.

The three generated fixtures are redistribution-safe outputs of code in the test
helper:

- `generated_repeated_occurrences.step` contains one box definition used twice
  beneath a subassembly that is itself used twice beneath a root assembly. Its
  four component labels expand to six occurrence paths at depth two. The test
  proves that the second subassembly's 90-degree rotation and translation are
  composed into its child paths rather than discarded. It also assigns a test
  material to the box definition and proves that OCCT's AP242 writer/reader
  preserves both the material definition and the shape-to-material link.
- `generated_fused_slab.step` is the Boolean union of two overlapping boxes and
  reloads as one simple definition and one solid. Its free root carries a
  180-degree Y rotation plus translation, providing a regression case for
  separating root placement from definition-local geometry. It is also the
  negative test for pretending an arbitrary face group is a product hierarchy.
- `generated_flat_multi_solid.step` reloads as one simple definition containing
  two disconnected solids and no component occurrences. It is the positive
  test for a flat STEP body collection that has topology but no assembly
  hierarchy.

Fixture freshness is semantic, not a raw Part 21 text comparison. The test
regenerates all three files in a unique temporary directory, reloads both the
committed and regenerated copies, and compares topology counts, bounds, volume,
names, expanded occurrence paths, and accumulated transforms. Temporary state
is removed by an RAII guard even when an assertion fails.

## Corpus Provenance Boundary

This report is a structure/topology inventory, not a complete corpus provenance
manifest. It records content hashes and generated-fixture construction, but it
does not yet record source, license-or-usage basis, or redistribution permission
for every pre-existing embedded model. The three generated fixtures are
reproducible project outputs; additional Appz or private fixtures still require
the explicit provenance fields described in the Annotation Lab handoff.

The freshness test is also not a full-corpus unmodified read/write/reload
baseline. It regenerates the three synthetic fixtures and compares semantic
reload results. Appz issue #140 still needs recorded before/after evidence for
unmodified write/reload across its selected corpus.

The current corpus demonstrates that a large occurrence count does not imply a
large definition count. It also demonstrates that existing files vary sharply
in labeled subshape coverage: one AP203 fixture has none, while the AP214
SOT-23 has 342. XCAF subshape labels therefore cannot be required as the sole
face-enumeration or identity mechanism.

`named_data_labels` is an observation of all imported `TDataStd_NamedData`,
including ordinary validation/product metadata. It is not evidence of a
Geometer annotation.

## Explicit Import Posture

The baseline calls the `STEPCAFControl_Reader` overload that accepts
`DESTEP_Parameters` and does not depend on reader defaults. It enables:

- products, all product contexts, all shape representations, tessellation, and
  every assembly level;
- representation relationships, shape aspects, subshape names, and root
  transformations;
- names, colors, layers, validation properties, metadata, product metadata,
  SHUO, GDT, materials, and views.

Constructive geometry, non-manifold mode, and indiscriminate loading of every
top-level shape remain disabled for this baseline. Later experiments must
report deviations rather than silently changing these modes.

## Payload Sizing Result

The report measures two synthetic normalized-table encodings from the observed
cardinality. Compact tables contain numeric references. Verbose tables add
diagnostic carrier fields and empty diagnostic arrays. These are sizing probes,
not candidate wire contracts.

It also measures the direct native render proof at 0.1 mm absolute linear
deflection, 0.5-radian angular deflection, serial meshing, and an identity
source-to-render transform. Render counts distinguish unique definition
geometry from effective instanced triangles. Logical binding-table bytes count
four 64-bit fields plus the UTF-8 opaque-handle bytes per span; they are an
encoding-neutral comparison, not `sizeof` or a proposed JSON representation.

The largest current verbose probe is about 11 KiB, but this small package corpus
cannot establish a safe upper bound for arbitrary CAD. Slice A therefore starts
with these containment rules:

- summary JSON remains below 1 MiB;
- repeated topology records are paged in at most 1,024-record pages;
- verbose carrier diagnostics are opt-in; and
- a compact binary/table attachment is required before the framed transport's
  8 MiB JSON limit can be approached.

The limits are subject to revision from measured larger models, but an
unbounded all-faces JSON response is not an acceptable starting point.

The current largest direct binding table is the repeated SOIC assembly at 312
spans and 73,632 logical bytes. Its 636 unique triangles expand to 2,992
instanced triangles. See
[direct render-binding research](step-topology-render-binding.md) for the
selection and reverse-resolution evidence.

The A2 report also records the actual deterministic binding-GLB JSON, binary,
and total byte counts, plus one-primitive-per-face draw-call cardinality and a
one-merged-primitive-per-occurrence projection. See
[GLB work-packet research](step-topology-glb-binding.md) for the route and
layout comparison and the real Three.js raycast result.

## Runtime, Fixture, And Matrix Impact

The completion audit on Windows x64 with OCCT 8.0.1 recorded the following
local evidence on 2026-08-23:

- all 12 registered `step_topology` CTests passed in 8.88 seconds within the
  existing native CTest/Rack stratum;
- the three committed generated STEP fixtures total 83,398 bytes;
- the machine-readable baseline report is 15,135 bytes; and
- the bundled Node/TypeScript student reference is 1,202,603 bytes.

These are repository-impact observations, not performance commitments. The
topology tests remain in the existing native lane; no new mandatory CI matrix
lane was added. Same-version XBF/XML and AP242 evidence runs in ordinary CTest.
The older/newer OCCT custom-driver compatibility matrix remains an explicit
manual qualification because it builds separate dependency trees and is not an
appropriate per-change CI cost. Future fixture additions must update this size,
runtime, provenance, and lane review rather than silently expanding the corpus.

## ISO 10303-242:2025 Traceability

`GEOMETER_AP242_SPEC_ROOT` is a manual shell convention for a reviewer opening
the licensed local ISO package; no Geometer test currently reads that variable.
Its path and contents are not committed. The following table records
schema/entity locations and short interpretations, not copied specification
text.

All paths below are relative to the licensed package root. Each candidate is
present in AP242 MIM Table 3 at
`data/application_protocols/managed_model_based_3d_engineering/sys/6_ccs_mim_table.htm`
and in the long-form MIM at
`data/modules/ap242_managed_model_based_3d_engineering/sys/e_exp_mim_lf.htm#ap242_managed_model_based_3d_engineering_mim_lf.<entity>`.

| Candidate | Normative resource locator | Constraint relevant to the experiment | Research use |
| --- | --- | --- | --- |
| `shape_aspect` | `data/resources/product_property_definition_schema/product_property_definition_schema.htm#product_property_definition_schema.shape_aspect` | Names an aspect of one `product_definition_shape`; the id/owner pair is unique and at most one id attribute may be attached | Candidate product/body/face target description |
| `shape_aspect_relationship` | `data/resources/product_property_definition_schema/product_property_definition_schema.htm#product_property_definition_schema.shape_aspect_relationship` | Relates two shape aspects; at most one id attribute may be attached | Component-to-group relationship candidate |
| `composite_shape_aspect` | `data/resources/shape_aspect_definition_schema/shape_aspect_definition_schema.htm#shape_aspect_definition_schema.composite_shape_aspect` | Inverse component relationship set has lower bound two | Only appropriate for a group with at least two members |
| `composite_group_shape_aspect` | `data/resources/shape_aspect_definition_schema/shape_aspect_definition_schema.htm#shape_aspect_definition_schema.composite_group_shape_aspect` | Specializes `composite_shape_aspect` | Candidate logical multi-member shape group |
| `geometric_item_specific_usage` | `data/resources/shape_tolerance_schema/shape_tolerance_schema.htm#shape_tolerance_schema.geometric_item_specific_usage` | Definition is a shape aspect or relationship; used representation is a shape model; identified item is a geometric model item | Candidate topology link from a shape aspect/group to represented geometry |
| `property_definition` | `data/resources/product_property_definition_schema/product_property_definition_schema.htm#product_property_definition_schema.property_definition` | Characterizes a supported definition and permits at most one id attribute | Candidate namespaced metadata property owner |
| `property_definition_representation` | `data/resources/product_property_representation_schema/product_property_representation_schema.htm#product_property_representation_schema.property_definition_representation` | Joins a represented definition to a representation; name and description are each at most singular | Candidate property payload representation path |
| `descriptive_representation_item` | `data/resources/qualified_measure_schema/qualified_measure_schema.htm#qualified_measure_schema.descriptive_representation_item` | A representation item with a textual description | Candidate compact research payload item, not a final annotation model |

The first focused [AP242 persistence baseline](step-topology-ap242-persistence.md)
now proves an OCCT 8.0.1 model-API round trip using an exactly connected
product-level general-property graph plus separate body and face
`geometric_item_specific_usage` links to that same product. The reloaded face
item resolves uniquely to one restored-body face under the recorded geometric
comparison.
The table remains the specification traceability for the tested subset; it
does not imply that OCCT writes every AP242 relationship or promotes generic
topology links into high-level XCAF attributes.

### Conformance posture

Clause 6.1 and 6.2 in
`data/application_protocols/managed_model_based_3d_engineering/sys/6_ccs.htm`
define conformance as support for this application protocol, its normative
references, and at least one implementation method. AP242:2025 defines one
conformance class, `CC1 managed_model_based_3d_engineering_cc1`, whose mandatory
elements must be supported.

Normative Annex C in
`data/application_protocols/managed_model_based_3d_engineering/sys/annex_imp_meth.htm`
defines the implementation methods and mandatory-value/null handling. The
mechanical geometry/annotated-3D experiment will use ISO 10303-21 with the MIM,
including the required `FILE_SCHEMA` schema name and object identifier. The
generated OCCT 8.0.1 fixture currently reports schema object identifier
`{1 0 10303 442 1 1 4}`. That observed implementation edition is recorded; it
must not be silently treated as proof of the 2025 edition.

Normative Annex D in
`data/application_protocols/managed_model_based_3d_engineering/sys/annex_pics.htm`
requires a protocol implementation conformance statement to identify the
implementation method, preprocessor/postprocessor support, and CC1 support.
This baseline is not a PICS and makes no AP242 conformance claim. Entity
membership establishes generic EXPRESS legality and AP242 MIM availability,
not OCCT read/write support, the correct ARM use-case mapping, or complete CC1
support. Each of those must be proven separately in later work packages.

## Native Capability Baseline

The current generated OCCT install contains the required public headers and
static libraries for XCAF inspection, `TKBinXCAF`, `TKXmlXCAF`, and BRepGraph.
Geometer now links `TKBinXCAF` and `TKXmlXCAF`; the same-version standard
driver evidence is recorded in `step-topology-xcaf-persistence.md`.
The toolkits are native-only and are not added to the browser WASM target.

The follow-on [native inspection research](step-topology-native-inspection.md)
now proves BRepGraph population, reconstruction, UID behavior, repeated product
occurrences, layer registration, and retained face mesh in an isolated native
test. XCAF remains the document/session source of truth; the BRepGraph probe is
not yet an integration decision.

`RWGltf_CafWriter` 8.0.1 exposes face-merging control and protected
`writePrimArray` and `writeExtrasAttributes` hooks. The completed GLB
comparison found that its flat node extras are insufficient for an exact
nested occurrence/primitive binding without private traversal coupling. The
selected research packet instead encodes the sealed OCCT tessellation artifact
directly and proves the result with pinned Three.js.

## Selected XBF Compatibility Matrix

The first cross-version XBF/XML comparison uses:

| Role | Tag | Peeled commit | Toolchain/runtime | Drivers |
| --- | --- | --- | --- | --- |
| Older | `V7_9_3` | `a016080bf6738d6aeae020badee4e888ad1540a5` | MSVC v143 19.44, x64, Release, static OCCT, `/MD` | `BinXCAF`, `XmlXCAF` |
| Current | `V8_0_1` | `b8f597c677811d1f9f4d8a97f5ae2825c0353a42` | MSVC v143 19.44, x64, Release, static OCCT, `/MD` | `BinXCAF`, `XmlXCAF` |

Each version is built and run in its own executable/install tree. No OCCT 7.9
and 8.0 binary objects are loaded into one process. Required directions are
7.9-write/8.0-read, 8.0-write/7.9-read, and same-version controls for both
binary and XML storage. The later matrix records storage versions, complete
driver registration, standard attributes, the mandatory custom attribute/GUID
probe, unknown-driver behavior, and exact losses.

## Reproduction

From a configured native build in a Visual Studio developer shell:

```powershell
cmake --build build --config Release --target geometer_step_topology_fixture_inventory
ctest --test-dir build -C Release -R geometer_step_topology --output-on-failure
```

Regenerate the three committed fixtures only when intentionally rotating the
baseline, then review the STEP and report diffs:

```powershell
.\build\tests\cpp\geometer_step_topology_fixture_inventory.exe `
  --generate tests\fixtures\step\generated_topology
```

The CTest freshness check re-runs all eight observations and rejects a stale
JSON report.
