# STEP Topology Multidimensional Recovery

Status: experimental native policy model; not yet an importer or wire contract

Date: 2026-08-23

## Result

`analyze_step_topology_recovery` is a bounded C++ value API that evaluates
already-collected carrier and geometry evidence without exposing OCCT objects.
It proves the recovery decision model before TypeSpec Slice C or an Appz
Annotation Lab contract adopts it.

The result keeps these dimensions independent:

- resolution state: resolved, ambiguous, unresolved, or unsupported;
- resolution method: authored id with a verified topology link, validated
  carrier locator, unique geometry/adjacency fingerprint, or none;
- topology comparison: unchanged, relocated, split, merged, otherwise changed,
  not compared, or unavailable;
- confidence: high, medium, low, or none;
- evidence: candidate and match counts, compared fields, explicit tolerances,
  carrier records, and rejected alternatives; and
- group completeness: fully recovered, partially recovered, unrecovered, or
  unsupported, with separate member-state counts and one result per input
  member.

This separation matters. A face with a durable authored id and a verified
topology link may resolve with high confidence while its geometry is reported
as changed. Conversely, a geometrically identical face with no durable carrier
resolves only by the lower-confidence fingerprint method. Two symmetric
geometry matches remain ambiguous; input order is never a tie-breaker.

`compared_fields` covers both the winning resolution tier and the independent
topology comparison. Thus an authored-id resolution that reports changed
geometry still lists the area/volume, centroid, bounds, surface/topology kind,
frame, occurrence, and adjacency fields used for that comparison. Split/merge
reports list lineage evidence; unavailable comparisons list fingerprint
availability.

## Evidence Precedence

For one body or face member, the analyzer evaluates candidates in this order:

1. matching nonempty authored target id plus a verified topology link;
2. matching nonempty carrier locator whose validation flag is true; and
3. a unique complete geometry/adjacency fingerprint match.

Once a tier has candidates, lower tiers cannot override it. Multiple matches
at the winning tier produce `ambiguous`, no resolved handle, no confidence, and
`not_compared` topology. No match produces `unresolved`. The native value API
uses `unsupported` for definition, occurrence, and shell inputs. The current
TypeSpec recovery request admits only body and face members, so no governed
analyzer vector claims that its `unsupported` enum value is currently
reachable; future carrier extraction may use it only after separate evidence.

A verified topology-link or carrier-locator flag requires a nonempty carrier
evidence record. A lineage assertion can report split or merged only when the
candidate also has such a verified topology link or validated carrier locator.
Geometry alone does not infer lineage.

The current native result resolves one target handle per source member. A
`split` comparison therefore says that the selected durable successor carries
verified split lineage; it is not yet a complete bounded successor-set model.
TypeSpec Slice C must decide whether to add an explicit complete descendant set
before representing one-to-many repair actions.

## Fingerprint Context

An available fingerprint must record:

- normalized millimeter units;
- a named coordinate frame and occurrence context;
- body topology kind or face surface kind;
- area for faces or volume for bodies;
- centroid and bounds;
- a lowercase SHA-256 adjacency signature; and
- finite values and ordered bounds.

Geometry selection compares all of those fields, including occurrence context,
under explicit positive absolute length, area, and volume tolerances. Topology
comparison reuses the local-geometry comparison without occurrence context so
an otherwise identical target in a different occurrence can be reported as
`relocated`.

The adjacency digest is evidence supplied by an upstream OCCT extractor. This
slice does not yet define its canonical graph encoding; TypeSpec Slice C must
not freeze that encoding until the fixture and version matrices establish it.

## Provenance And Limits

Every report copies required source/candidate artifact digests, exact source
and candidate OCCT versions, drivers, writer-setting records, command
provenance, and caller-measured wall time. Unknown external writer settings
must be stated explicitly rather than omitted.

The analyzer rejects malformed or non-finite evidence, duplicate member ids,
duplicate candidate handles within a member, unverified lineage assertions,
invalid digests, nonpositive tolerances, oversized strings, excessive aggregate
string bytes, excessive members, and excessive candidates. Failure clears the
output before returning.

## Evidence

`geometer_step_topology_recovery_test` proves:

- a verified authored face id resolves even when its geometry changed, while
  the change remains visible in the topology-comparison dimension;
- a validated carrier locator outranks a geometry-only candidate and can
  independently report relocation;
- a body geometry candidate is selected by volume rather than the face-area
  branch, with its topology comparison and evidence fields asserted;
- two symmetric geometry candidates fail closed as ambiguous;
- one recovered and one missing member produce an explicitly partial group;
- split, merged, and unsupported outcomes remain distinct, while a foreign
  lineage record selected only by geometry cannot assert split/merge;
- duplicate member ids and candidate-budget exhaustion fail without partial
  publication; and
- 20,000 identical geometry candidates produce ambiguity through a linear
  membership pass rather than quadratic rejection scanning.

## Boundary And Next Work

This API does not read STEP, XBF/XML, GLB extras, or edit-journal bytes. It does
not discover AP242 carriers, calculate adjacency fingerprints, alter session
state, or apply repairs. Those producers must supply evidence without treating
STEP entity numbers, XCAF label paths, traversal indexes, GLB ids, or runtime
handles as durable identity.

The next recovery work joins this policy model to measured unchanged reload,
XBF/XML, AP242, and GLB-regeneration candidate extraction. TypeSpec Slice C
will be registered only after those result shapes and compatibility matrices
stabilize. Appz remains a later consumer; the legacy Appz glTF enrichment model
was not reused.
