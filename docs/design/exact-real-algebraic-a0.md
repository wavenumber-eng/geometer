# Exact Real-Algebraic Backend A0

## Status

Proposed for focused review before implementation. This document freezes the
portable value identity, construction DAG, conformance encoding, and resource
boundary required by ADR-012. It does not promote the analytic planar Boolean
operation or define bytes inside its public request/result packet.

## Purpose And Authority

The analytic solver needs exact equality, order, sign, root selection, and
half-nanometer comparison for values produced by line/circle arrangements.
Binary floating point and OCCT tolerances are not decision oracles. The same
C++17 implementation and governed work counters run natively and under
Emscripten.

There are two deliberately separate identities:

- a **value key** identifies one exact real number independently of the
  expression used to construct it; and
- a **construction key** identifies one normalized expression node and its
  normalized children for structural interning and diagnostic replay.

Exact equality uses value keys. Construction keys cannot make two unequal
values equal and are never used as a substitute for algebraic comparison.

## Dependency

The backend uses header-only `boost::multiprecision::cpp_int` from Boost
1.92.0. The governed source is:

- URL: `https://archives.boost.io/release/1.92.0/source/boost_1_92_0.tar.gz`
- upstream commit: `afdfa32505af73e3d208144b3f623f0096cb62b6`
- archive SHA-256:
  `c4a3b310ddd2472416e091067166b0713be97c63f38c212c484ada022fd296ce`

A restore helper verifies the archive before extracting it beneath `.deps/`.
Boost source and archives remain generated local state and are never committed.
No compiled Boost library or platform-specific arithmetic backend is used.

## Canonical Integer And Rational Values

An integer is encoded as sign plus minimal unsigned magnitude:

- sign `0` is zero and requires zero magnitude bytes;
- sign `1` is positive and sign `2` is negative;
- a nonzero magnitude is unsigned big-endian base-256 with no leading zero;
- negative zero and all other sign values are invalid.

A rational is `(numerator, denominator)` with a positive denominator,
`gcd(abs(numerator), denominator) == 1`, and zero represented only as `0/1`.
Normalization occurs before charging or interning the value.

## Canonical Polynomial And Root Identity

Coefficients are integers in ascending degree order. A canonical irrational
algebraic value contains:

1. its primitive irreducible minimal polynomial over the integers;
2. a positive leading coefficient;
3. a zero-based ordinal among the polynomial's distinct real roots in strictly
   increasing order;
4. the canonical dyadic isolating interval below; and
5. the sign of every derivative from order one through the degree at that root
   (the Thom encoding).

There are no trailing zero coefficients. Content is one. The polynomial is
square-free. Degree-one values are encoded as rationals, never as algebraic
records. Resultant output is primitive-normalized, square-free factored, then
factored over the integers; exact common-root isolation selects the unique
irreducible factor and root ordinal. This makes equal values reached through
different expressions share one value key.

For an irrational root, the canonical interval is the unique adjacent dyadic
pair `k / 2^p` and `(k + 1) / 2^p` at the smallest nonnegative precision `p`
for which the open interval contains the selected root and no other root of the
minimal polynomial. Irrational roots cannot equal an endpoint. `k` is a signed
canonical integer and `p` is a `u32`. The stored Thom signs are each exactly
`-1`, `0`, or `1` and must agree with exact polynomial evaluation over the
isolating interval.

The value key is the canonical scalar encoding described below. Interval and
Thom data are redundant validation evidence, not alternative identities.

## Construction DAG

The closed A0 node catalog is:

| Kind | Identity | Children | Rules |
| ---: | --- | ---: | --- |
| 1 | `rational` | 0 | value is the reduced rational payload |
| 2 | `sum` | 2 or more | associative flattening; rational terms folded; zero removed; children sorted |
| 3 | `product` | 2 or more | associative flattening; rational factors folded; zero short-circuits; one removed; children sorted |
| 4 | `reciprocal` | 1 | child must be nonzero |
| 5 | `nonnegative_square_root` | 1 | child must be nonnegative; selects the nonnegative root |

Subtraction lowers to a sum with a rational `-1` product. Division lowers to a
product with a reciprocal. Negation lowers to multiplication by rational `-1`.
There are no approximate constants or generic power nodes in A0.

Each node carries its evaluated canonical value key. Sum and product children
sort by `(value key, construction key)` after flattening and folding. A node's
construction key is `(kind, ordered child construction keys, value key)`.
Identical complete keys intern to one node only. Reaching the same normalized
expression through different allocation or traversal orders therefore produces
the same DAG, while algebraically equal but structurally different expressions
still converge on the same value key after exact evaluation.

Serialization includes only nodes reachable from the ordered root list. Nodes
sort first by dependency depth and then by their complete construction key;
indices are assigned after sorting, so every child index is less than its
parent. The caller-provided root order is semantic and is preserved.

## Conformance Encoding

The internal conformance artifact identity is
`geometry.exact_real_algebraic.feasibility.a0`; magic is the eight ASCII bytes
`GEXPA001`. All integers are little-endian except canonical big-integer
magnitudes, which are byte strings in big-endian numeric order. All reserved
fields and alignment bytes are zero. Every offset, length, addition,
multiplication, alignment, and native-size conversion is checked before access
or allocation.

### Header, 32 bytes

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | 8 bytes | magic `GEXPA001` |
| 8 | `u16` | generation `1` |
| 10 | `u16` | flags, zero |
| 12 | `u32` | node count |
| 16 | `u32` | root count |
| 20 | `u32` | reserved, zero |
| 24 | `u64` | total bytes, exactly the buffer length |

Node records follow the header. The root table follows the final aligned node
record and contains one `u32` node index per root.

### Node Record

Every node begins with a 24-byte header:

| Offset | Type | Field |
| ---: | --- | --- |
| 0 | `u32` | complete aligned record bytes |
| 4 | `u8` | node kind |
| 5 | `u8` | flags, zero |
| 6 | `u16` | reserved, zero |
| 8 | `u32` | child count |
| 12 | `u32` | scalar-value bytes |
| 16 | `u32` | reserved, zero |
| 20 | `u32` | reserved, zero |

The header is followed by child indices, the canonical scalar value, the node
and minimum zero padding to eight-byte alignment. Node-specific numeric content
is represented only by the scalar value; there is no second competing payload.

### Scalar Value

A scalar begins with `kind u8`, three reserved zero bytes, and `record_bytes
u32`. Kind `1` is rational and then encodes numerator and denominator integers.
Kind `2` is irrational algebraic and encodes, in order:

- coefficient count `u32` followed by that many canonical integers;
- real-root ordinal `u32`;
- interval precision `u32` and canonical integer `k`;
- Thom-sign count `u32` followed by one signed byte per derivative; and
- minimum zero padding to eight-byte alignment.

Each embedded integer is `sign u8`, three reserved zero bytes, `magnitude_bytes
u32`, then its minimal magnitude and minimum zero padding to four-byte
alignment. The scalar record length is self-inclusive and minimally aligned.

Decoders reject unknown kinds, nonminimal encodings, inconsistent lengths,
invalid child topology, duplicate construction keys, noncanonical order,
incorrect evaluated value keys, reducible/nonprimitive polynomials, invalid
root ordinals or isolation, and inconsistent Thom signs.

## Exact Algorithms And Total Outcomes

The feasibility implementation must exercise the same algorithms intended for
production:

- integer and polynomial content/GCD;
- square-free decomposition and deterministic integer factorization through
  degree 64;
- Sturm or signed-subresultant real-root counts and isolation;
- resultant construction for sum, product, reciprocal, and square root;
- unique factor/root selection from operand intervals and exact signs;
- exact equality, order, and sign;
- outward-rounded dyadic interval acceleration at 256, 512, 1024, 2048, and
  4096 bits; and
- nearest-nanometer ties-away-from-zero using interval certification followed
  by exact comparison at a half-grid boundary.

No iteration cap may return an approximate answer. Before work that would
exceed a governed counter, the operation returns
`resource_limit_exceeded`. Invalid domain input such as reciprocal of zero or
square root of a negative value is a deterministic contract error in the
internal API and never enters the analytic arrangement.

## Resource Accounting

The backend receives an explicit budget object. It charges before allocation or
work for live scalar count, coefficient bytes, polynomial degree, coefficient
bit length, predicate calls, interval-refinement steps, and total owned bytes.
The maxima come from the frozen analytic numeric catalog. Tests also run with
smaller injected budgets to prove that each boundary fails before mutation and
with the same identity natively and under Emscripten.

## Feasibility Vectors

The first governed vector set must include:

- integers beyond 128 bits and signed zero/minimal-magnitude rejection;
- rational sign/GCD normalization and equivalent rational constructions;
- commuted and reassociated sum/product construction orders;
- `sqrt(2)`, `-sqrt(2)`, `sqrt(8) / 2`, and equal values reached by distinct
  resultant paths;
- line/line, line/circle, and circle/circle intersections with rational and
  quadratic-irrational coordinates;
- equal, less-than, greater-than, sign, and exact-zero decisions;
- values immediately below, at, and above positive and negative half-nm ties;
- every interval precision transition and exact fallback after 4096 bits;
- each governed resource-limit failure; and
- malformed byte cases for every length, reserved, canonicalization, index,
  polynomial, interval, and Thom invariant.

Native and Emscripten emit byte-identical canonical artifacts and SHA-256
digests. The pull-request feasibility corpus remains small and deterministic;
larger polynomial/factorization and degeneracy seeds belong to the nightly
synthetic-correctness stratum.

## Gate

Focused independent review must accept this node/value/byte design before the
dependency restore helper or backend implementation lands. Production solver
work remains blocked until the implemented native/Emscripten feasibility corpus
passes and the separate OCCT 8.0.0/8.0.1 qualification step is accepted.
