# Geom A0 Contract Alignment

## Status

**Historical compatibility record.** The MATZ integration direction described
here is no longer an active production plan. The analytic solver is retained as
experimental and not production-ready under
[ADR-017](../../geometer/adr/geometer-adr-017-retain_analytic_planar_boolean_as_experimental.md).

Accepted for the `geom-contract-alignment` gate at commit `433bad5` by reviewer
packet `reviewer-019ffce8-ac66-76c0-877d-3fcb5c1aa6c5`. This report compares
the digest-locked `appz/data_models` Geom A0 consumer vocabulary with the
Geometer-owned logical model proposed for
`geometry.analytic_planar_boolean_batch.a0`. It is a compatibility and adapter
report, not shared runtime authority and not a promotion decision. The
operation, packet, solver, and release remain unfrozen.

The frozen inputs, source revision, and SHA-256 digests are recorded in
[`data-models-geom-a0-2026-08-12.toml`](../../contracts/compatibility/data-models-geom-a0-2026-08-12.toml).
Geometer generation, builds, and tests do not read the sibling checkout.

## Authority Boundary

Geom A0 and the MATZ requirements packet are consumer inputs. Geometer owns
the final TypeSpec models, normalized catalog, packed attachment, validation,
normalization, provenance, diagnostics, and implementation. Once the analytic
operation is promoted, those Geometer artifacts are its only structural
authority. `data_models` may consume a released generated projection or keep
an explicit adapter; its JSON Schema does not become a second schema inside
Geometer.

The proposed reusable Geometer subset is intentionally smaller than Geom A0:

- signed `PointNm` coordinates and positive lengths with fixed 64-bit bounds;
- topology-indexed lines and circular arcs;
- open paths, closed rings, and planar regions;
- disks, annuli, endpoint-defined capsules, and round swept paths; and
- result regions, ring hierarchy, directed fragments, and geometry
  provenance needed by generic analytic operations.

Ellipses, elliptical arcs, Beziers, splines, infinite lines, rectangles,
rounded/chamfered rectangles, trapezoids, arbitrary affine transforms, and
open extension metadata are not part of this operation's A0 subset. Fixture
rectangle shorthand lowers to a line ring before encoding. This exclusion is
an operation capability boundary, not a claim that those shapes are invalid in
the broader Geom domain.

## Mapping

| Frozen Geom A0 concept | Proposed Geometer concept | Mapping and deliberate differences |
| --- | --- | --- |
| `coordinate2d` and `coordinate2d_pair` | `PointNm { x: int64, y: int64 }` | Object and tuple inputs both map to one logical point. Geom integers inherit a unit from their owner and have no JSON-level 64-bit bound; this operation requires integer nanometers in the signed 64-bit range. |
| `geom.path_segment2d(kind=line)` | authored line segment | Endpoints remain topology-owned. Geometer additionally requires nonzero packet-local segment and curve IDs. Geom metadata is not carried. |
| center-form or radius-form `geom.path_segment2d(kind=arc)` | `CircularArcDescriptor { center, direction, majorArc }` | The center form maps `clockwise` to direction and derives the major branch exactly from the two radius vectors. The radius form is accepted only under the frozen Geom minor-or-semicircle policy described below. Geometer validates exact equal nonzero squared radii. Ambiguous, incoherent, or full-circle authored arcs fail; compact disk/annulus operands represent full circles. |
| `geom.path2d` (`points`, `segs`) | `PlanarPath` (`pathId`, vertices, segments) | Segment/point indexing is compatible and the adapter supplies packet-local IDs. Geometer A0 accepts only line/circular-arc segments, requires at least one nonzero segment, and applies the swept-path restrictions when used as a centerline. |
| `geom.ring2d` (`points`, `segs`) | `PlanarRing` (`ringId`, vertices, segments) | Closure by cyclic topology is compatible. Geometer permits the two-half-arc decomposition required for canonical full-circle results, while the frozen Geom schema has `minItems: 3`. A downstream Geom adapter must either support the released Geometer form or split each half arc analytically; it must not polygonize it. Input winding may be either direction; Geometer canonicalizes result outer rings counterclockwise and holes clockwise. |
| ADR name `geom.planar_region`; frozen schema `geom.region` with UUIDv7 `id` | authored planar-region operand plus canonical result-region/ring hierarchy | Outer-plus-holes input is compatible. The frozen naming discrepancy is retained as consumer history, not copied. Geometer IDs are nonzero packet-local uint64 lifting tokens, not stable application UUIDs. Nested positive islands are separate regions; result containment is owned solely by the result ring hierarchy. |
| `geom.disk2d` | disk operand | Center/radius maps directly after unit/range validation. Radius must be positive. |
| `geom.annulus2d` | annulus operand | Center and edge radii map directly, but Geometer requires `0 < innerRadius < outerRadius`; a zero inner radius is a disk. |
| `geom.capsule2d` (center, local-X total length, width, optional owning transform) | capsule operand (start, end, width) | Frozen Geom length is tip-to-tip, so the untransformed centerline distance is exactly `length - width`, not `length`. The deterministic transform and integer-representability rules are specified below. Geometer requires distinct endpoints and positive width but does not adopt scene-graph transform ownership. |
| ADR name `geom.swept_path2d`; frozen schema `geom.sweep2d` | swept-path operand (centerline, width) | The path and width map after ID lifting. Operation A0 supports round caps and joins only; `butt`, `square`, `miter`, and `bevel` are rejected as unsupported rather than silently lowered. The frozen naming discrepancy is not copied. |
| no equivalent | ordered jobs/stages, operand outcomes, relationship queries, result fragments, source sets, diagnostics, standalone result digest | These are Geometer operation semantics. They do not belong in the reusable source-shape vocabulary and are not projected back into Geom metadata. |

### Radius-form arc adapter

For adjacent, distinct topology endpoints `P0` and `P1`, positive radius `r`,
and Geom sweep `cw` or `ccw`, the adapter applies the frozen Geom recommendation
as an authoritative compatibility rule: radius-form arcs map only to a directed
minor arc or semicircle, never a major arc. It performs these steps with exact
integer/rational predicates:

1. require `|P1 - P0|^2 <= 4 r^2`;
2. select the unique circle center for which the requested directed sweep lies
   in `(0, 180 degrees]` (the diameter case has one center);
3. require that center to have exact integer-nanometer coordinates in the
   signed 64-bit range; and
4. encode that center and direction with `majorArc = false`.

If there is no unique center, if the selected center is non-integral or out of
range, or if the endpoints coincide, the adapter rejects the arc. It does not
round a center or choose between two solutions by iteration order. Callers
needing a major arc must use the Geom center form, whose center and direction
determine the branch exactly; a directed sweep greater than 180 degrees maps
to `majorArc = true`, and a semicircle maps to `false` by convention.

### Capsule adapter

For a frozen Geom capsule with center `C = (cx, cy)`, total tip-to-tip length
`L`, and width `W`, first require `L > W > 0`. Its local-X centerline endpoints
are exactly:

```text
P0 = (cx - (L - W) / 2, cy)
P1 = (cx + (L - W) / 2, cy)
```

An absent owning transform is the identity. A present affine transform is
accepted only when its finite, nonsingular linear part is provably a
similarity: `A^T A = s^2 I` for one positive uniform scale magnitude `s`.
Translation, rotation, and reflection are allowed. Shear, anisotropic scale,
singular transforms, and any transform whose similarity cannot be proved in
the adapter's governed numeric representation are rejected because they do not
preserve a circular capsule.

The adapter applies the transform to `P0` and `P1` and scales the width to
`W' = s W`. It accepts the result only if both transformed endpoints are exact
integer-nanometer points in the signed 64-bit range and `W'` is an exact
positive integer nanometer in the unsigned 64-bit range. Thus odd `L - W`, an
arbitrary rotation, a fractional uniform scale, or translation is allowed only
when the final endpoint and width values remain exact integers; no intermediate
or final value is rounded. The encoded Geometer capsule is `(P0', P1', W')`.
`L == W` maps to a disk rather than a zero-centerline capsule. `L < W` and all
unrepresentable or overflowing cases fail closed.

## Units And Numeric Ownership

Geom A0 fields are unit-neutral and rely on an owning coordinate contract.
The analytic Boolean A0 boundary is deliberately not unit-neutral: every
coordinate is an integer nanometer, every positive length is an unsigned
integer nanometer, and numeric bounds are fixed by the separately governed
packet catalog. A source adapter owns conversion into nanometers before packet
encoding and must reject non-integral or out-of-range values rather than round
them implicitly.

Boolean evaluation retains exact line/circle identities internally. Only the
final result is normalized to the one-nanometer grid, once, using the certified
rule in the analytic Boolean design. Native, WASM, IPC, and generated clients
must not implement independent snapping policies.

## Validation Ownership

Validation is layered rather than duplicated:

1. Generated codecs reject malformed envelopes, unknown fields, invalid
   scalars, invalid uint64 values, and malformed attachment tables.
2. The packet decoder rejects duplicate/zero IDs, invalid references, table
   bounds, unsupported flags or kinds, and resource-limit violations before
   geometry execution.
3. Each isolated job validates arc coherence, ring/path topology, containment,
   compact primitive constraints, and swept-path restrictions. These failures
   are stable job-local diagnostics.
4. The solver and certified normalizer either produce canonical analytic
   topology or fail closed. They never substitute sampled chords.

A structurally invalid batch is rejected as a whole. A structurally valid job
with invalid geometry or an unrepresentable normalized result fails only that
job. PCB source semantics, copper history, layers, tool-specific repair, and
MATZ publication policy are neither validation inputs nor diagnostics in
Geometer.

## Result And Provenance Boundary

The Geom source vocabulary describes shapes; it does not close the result
requirements of a deterministic analytic Boolean. Geometer therefore owns a
standalone result model with explicit connected regions, a single ring
containment hierarchy, directed line/arc fragments, source sets,
many-to-many operand/result associations, ordered-stage outcome events, and a
canonical digest.

Packet-local source IDs are returned for geometric traceability only. They do
not assign PCB nets, layers, material, source-tool objects, or stable MATZ
keys. A downstream materializer derives those domain meanings after validating
the Geometer result.

## Migration And Approval Evidence

The frozen MATZ packet supplies ten portable cases and two real-board cases.
Its joint consumer/provider review records no known representability blocker,
and MATZ explicitly accepts adapting to the final released Geometer contract.
The cases exercise ordered union/difference, disks, annuli, capsules, swept
paths, analytic arcs, holes/islands, normalization success/failure, and batch
independence. They remain design inputs until imported and governed by
Geometer; they are not production proof.

The original migration sequence was additive:

1. freeze this mapping and the Geometer logical proposal;
2. define the proposal in TypeSpec and the normalized catalog;
3. freeze the packed projection and import the portable fixtures;
4. prove native/WASM/IPC and generated-client conformance;
5. publish a tagged Geometer release; and
6. let MATZ pin that release, adapt its Geom-domain values, and pass its
   real-board integration gates before switching production.

Steps 5 and 6 no longer describe an active production direction; the retained
mapping remains useful as compatibility history.

Existing sampled planar and Clipper2 operations remain unchanged throughout
this migration.
