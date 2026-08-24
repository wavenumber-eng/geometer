# AP242 Product, Body, Logical Face Group, And Hierarchy Evidence

Status: experimental OCCT self-round-trip evidence; no AP242 conformance or
third-party survival claim

Date: 2026-08-23

## Result

OCCT 8.0.1 can construct, write, reload, and reconstruct the expanded Geometer
research carrier entirely through its XCAF, work-session, and STEP model APIs.
The focused test proves these connected layers:

- a product-level namespaced string written from `TDataStd_NamedData` through
  OCCT's exactly-one general-property association and represented-property
  path, and restored by normal
  `STEPCAFControl_Reader` metadata handling;
- one body `SHAPE_ASPECT` plus `GEOMETRIC_ITEM_SPECIFIC_USAGE` whose identified
  item is the product representation's only `MANIFOLD_SOLID_BREP`; and
- two face `SHAPE_ASPECT` plus `GEOMETRIC_ITEM_SPECIFIC_USAGE` relationships,
  each identifying a distinct `ADVANCED_FACE` and carrying the same authored
  logical-group id with a distinct member index; and
- a root assembly containing the box directly and through a nested assembly,
  so both occurrences refer to the same product definition.

The topology-link descriptions contain compact namespaced research JSON. The
shared group id is a Geometer research convention over individual AP242
relationships; the test does not claim a native AP242 logical-group semantic.
This is carrier-mechanics evidence, not the future Appz annotation vocabulary.
It does not reuse the legacy Appz glTF enrichment contract.

## Exact Relationship Graph

For each topology target, the test verifies this graph before write and after
reload:

```text
GEOMETRIC_ITEM_SPECIFIC_USAGE
  definition -> SHAPE_ASPECT
                  of_shape -> PRODUCT_DEFINITION_SHAPE
                                  definition -> PRODUCT_DEFINITION
  used_representation -> SHAPE_REPRESENTATION
  identified_item -> MANIFOLD_SOLID_BREP | ADVANCED_FACE

SHAPE_DEFINITION_REPRESENTATION
  definition -> same PRODUCT_DEFINITION_SHAPE
  used_representation -> same SHAPE_REPRESENTATION
```

Every referenced entity must be present in the same `StepData_StepModel`. The
body and both face aspects' product definition is also the product definition
characterized by the namespaced general property. The body representation
contains exactly one manifold solid and it is the GISU identified item. The
face identified item has the exact `ADVANCED_FACE` type. Each GISU used
representation is dynamically a STEP `SHAPE_REPRESENTATION`, and each
identified item is reachable through that representation's transitive entity
content.

The product payload is checked as one connected and cardinality-exact graph:

```text
GENERAL_PROPERTY_ASSOCIATION
  general_property -> GENERAL_PROPERTY named with the WN key
  property_definition -> PROPERTY_DEFINITION named with the WN key
                           definition -> same PRODUCT_DEFINITION

PROPERTY_DEFINITION_REPRESENTATION
  definition -> same PROPERTY_DEFINITION
  used_representation -> REPRESENTATION with exactly one
                           DESCRIPTIVE_REPRESENTATION_ITEM
```

The implementation subclasses `STEPCAFControl_Writer` only inside the test to
expose OCCT's protected `writeShapeAspect` helper. The helper uses the existing
finder process and model graph; the test then assigns the research id and
payload and writes normally. There is no ISO 10303-21 text injection.

## Reload Resolution

Normal root transfer reconstructs the XCAF assembly, component reference, and
definition geometry, but it does not automatically bind these generic GISU
identified items in `XSControl_TransferReader`. Geometer must walk the STEP
entity graph and explicitly call `XSControl_WorkSession::TransferReadOne()` on
the identified representation item.

After that targeted transfer, OCCT resolves the body item to a `TopoDS_SOLID`
and both face items to distinct `TopoDS_FACE` values. The body also resolves
uniquely through the same product representation and the restored definition's
single solid. Each resolved face has exactly one correspondent among the
restored body's faces when surface type, area, centroid, and bounds are
compared at the test's declared tolerance. The test asserts that all three
research items are initially unbound and that exactly three explicit transfers
complete; this is measured
OCCT 8.0.1 behavior, not a promise for every reader or future version.
This measured behavior is important for the eventual importer: the absence of
a mapping immediately after root transfer is not evidence that an AP242 link
was stripped.

OCCT's built-in metadata reader restores the product property to
`TDataStd_NamedData`. It does not turn the generic GISU records into a Geometer
annotation object. Direct STEP model inspection therefore remains required.

## Writer, Reader, And Geometry Evidence

`geometer_step_topology_ap242_persistence_test` forces
`DESTEP_Parameters::WriteMode_StepSchema_AP242DIS` and asserts the resulting
singular `FILE_SCHEMA` identifier contains
`AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF`. The test checks:

- writer transfer check list;
- `StepData_StepModel::VerifyCheck` for STEP header integrity;
- `Interface_CheckTool::VerifyCheckList` for OCCT's implemented model integrity
  checks on the writer model;
- a separate `StepData_StepWriter::SendModel` serialization preflight and its
  check list;
- reader analysis and verification check lists;
- reader transfer check list; and
- direct required-field and relationship assertions described above.

These OCCT checks are not complete EXPRESS `WHERE`-rule validation. In
particular, the OCCT 8.0.1 GISU read/write module does not add a GISU-specific
entity check. The direct graph assertions cover the exact subset claimed here;
independent schema validation remains unperformed.

The generated fixture is a 10 by 20 by 30 box used twice from one definition.
The root contains an identity occurrence plus a nested assembly translated by
`(10, 0, 0)`; that assembly contains the second definition occurrence at local
translation `(30, 0, 0)`, producing accumulated translation `(40, 0, 0)`.
Reload preserves the two-level component graph, repeated-definition identity,
all local and accumulated transforms, B-rep validity, face count, volume,
surface area, center of mass, and SHA-256 over the quantized per-body property
signature. The complete root assembly geometry/property fingerprint also
matches before and after round trip.

## Specification And Conformance Posture

The local licensed ISO 10303-242:2025 package is the authority for the entity
availability and relationship interpretations. Exact relative locators are
recorded in
[step-topology-fixture-baseline.md](step-topology-fixture-baseline.md); neither
the package nor its absolute workstation path is committed.

The four outcomes are deliberately separate:

| Evidence class | Result |
| --- | --- |
| OCCT 8.0.1 self-round-trip | pass for one generated product/body/two-face group/nested-hierarchy fixture |
| OCCT header/model integrity and serialization checks | pass for OCCT's implemented checks listed above |
| Tested ISO relationship assertions | pass for the explicit subset above |
| Independent external validator or CAD survival | not run |

OCCT's `AP242DIS` writer emits its supported schema identifier with object
identifier `{1 0 10303 442 1 1 4}`. The 2025 package confirms that the tested
entity families remain in the AP242 MIM, but that does not upgrade OCCT's
emitted implementation edition or establish CC1 support. This work is not a
PICS, not generic PMI support, and not an AP242 conformance claim.

## Deferred Evidence

- AP203 and AP214 input re-exported through this AP242 carrier;
- a pre-existing AP242 input rather than generated geometry;
- multiple independent body definitions;
- occurrence-specific authored metadata (the hierarchy itself and placements
  survive, but the current authored payload targets the shared definition and
  its faces);
- negative malformed relationships and independent EXPRESS-constraint fixtures;
- explicit property payloads attached directly to shape aspects rather than
  the current product summary plus GISU descriptions;
- changed-topology recovery and ambiguous/split/merged results;
- independent STEP validators and exact third-party CAD versions; and
- carrier size, cancellation, and worker-containment measurements.
