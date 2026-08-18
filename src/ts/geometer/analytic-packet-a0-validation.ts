import {
  compareNumberArrays,
  compareSource,
  diagnosticCodes,
  encodeTables,
  fail,
  flags,
  MAX_JOBS,
  MAX_LENGTH_NM,
  MAX_QUERIES,
  type MutableTable,
  mapped,
  oneBased,
  outcomeKinds,
  partition,
  putU64,
  RESULT_MAGIC,
  RESULT_TABLES,
  record,
  recordView,
  remapHandle,
  required,
  sliceRange,
  sparseMap,
  strictlyIncreasing,
  U32_NONE,
} from "./analytic-packet-a0-common.js";
import type { DirectoryTable, ResultRecords } from "./analytic-packet-a0-result.js";

export function validateResultRecords(
  records: ResultRecords,
  tables: readonly DirectoryTable[],
): Selection[] {
  if (records.jobs.length > MAX_JOBS || records.relationships.length > MAX_QUERIES)
    fail("Analytic result exceeds its job or relationship-result limit.");
  const reserved = (kind: number, index: number, ranges: readonly [number, number][]): void => {
    const view = recordView(required(tables[kind - 101], `result table ${kind}`), index);
    for (const [offset, length] of ranges)
      for (let at = 0; at < length; at += 1)
        if (view.getUint8(offset + at) !== 0)
          fail(`Nonzero reserved byte in result table ${kind}.`);
  };
  records.jobs.forEach((job, index) => {
    reserved(101, index, [
      [9, 7],
      [40, 8],
    ]);
    if (job.jobId === 0n || job.status > 1) fail("Invalid job-result identity or status.");
    sliceRange(records.diagnostics, job.diagnosticBegin, job.diagnosticCount, "job diagnostics");
    sliceRange(records.regions, job.regionBegin, job.regionCount, "job regions");
    sliceRange(records.events, job.eventBegin, job.eventCount, "job events");
    if (job.status === 1 && (job.regionCount !== 0 || job.diagnosticCount === 0))
      fail("Failed job owns invalid result ranges.");
    const ownedDiagnostics = sliceRange(
      records.diagnostics,
      job.diagnosticBegin,
      job.diagnosticCount,
      "job diagnostics",
    );
    if (ownedDiagnostics.some((value) => value.jobId !== job.jobId))
      fail("Job diagnostic identity does not match its owner.");
    const hasError = ownedDiagnostics.some((value) => value.severity === 1);
    if ((job.status === 1) !== hasError)
      fail("Job status does not match its diagnostic severity closure.");
  });
  strictlyIncreasing(
    records.jobs.map((value) => value.jobId),
    "job-result ids",
  );
  partition(
    records.jobs.map((value) => [value.diagnosticBegin, value.diagnosticCount]),
    records.diagnostics.length,
    "job diagnostic",
  );
  partition(
    records.jobs.map((value) => [value.regionBegin, value.regionCount]),
    records.regions.length,
    "job region",
  );
  partition(
    records.jobs.map((value) => [value.eventBegin, value.eventCount]),
    records.events.length,
    "job event",
  );
  let previousDiagnostic: readonly Comparable[] | undefined;
  records.diagnostics.forEach((value, index) => {
    reserved(102, index, [[44, 12]]);
    const presenceMatches =
      ((value.presence & 1) !== 0) === (value.jobId !== 0n) &&
      ((value.presence & 2) !== 0) === (value.stageId !== 0n) &&
      ((value.presence & 4) !== 0) === (value.operandId !== 0n) &&
      ((value.presence & 8) !== 0) === (value.geometryId !== 0n);
    if (
      recordView(required(tables[1], "diagnostic table"), index).getUint8(5) !== 1 ||
      diagnosticCodes[value.code] === undefined ||
      value.severity < 1 ||
      value.severity > 2 ||
      value.presence > 15 ||
      (value.presence & 1) === 0 ||
      !presenceMatches ||
      value.pathToken > 26
    )
      fail("Invalid diagnostic presence flags.");
    const key: readonly Comparable[] = [
      value.jobId,
      value.severity,
      value.code,
      value.presence,
      value.stageId,
      value.operandId,
      value.geometryId,
      value.pathToken,
    ];
    if (previousDiagnostic !== undefined && compareComparable(previousDiagnostic, key) >= 0)
      fail("Diagnostic records are not strictly canonical.");
    previousDiagnostic = key;
  });
  records.vertices.forEach((value) => {
    if (
      value.id === 0n ||
      value.sourceSet > records.sourceSets.length ||
      value.flags > 1 ||
      value.flags !== (value.sourceSet === 0 ? 0 : 1)
    )
      fail("Invalid result vertex.");
  });
  oneBased(
    records.vertices.map((value) => value.id),
    "result vertex",
  );
  records.fragments.forEach((value, index) => {
    reserved(104, index, [
      [19, 5],
      [40, 8],
    ]);
    if (
      value.id === 0n ||
      value.start >= records.vertices.length ||
      value.end >= records.vertices.length ||
      value.start === value.end ||
      recordView(required(tables[3], "fragment table"), index).getUint8(18) > 1 ||
      value.kind < 1 ||
      value.kind > 2 ||
      value.direction > 2 ||
      (value.kind === 1 && (value.direction !== 0 || value.major || value.radius !== 0n)) ||
      (value.kind === 2 &&
        (value.direction < 1 || value.radius === 0n || value.radius > MAX_LENGTH_NM)) ||
      value.positiveSet > records.sourceSets.length ||
      value.subtractionSet > records.sourceSets.length
    )
      fail("Invalid directed fragment.");
    if (value.kind === 2) {
      const start = required(records.vertices[value.start], "arc start");
      const end = required(records.vertices[value.end], "arc end");
      const dx = end.x - start.x;
      const dy = end.y - start.y;
      const chordSquared = dx * dx + dy * dy;
      const diameterSquared = 4n * value.radius * value.radius;
      if (chordSquared > diameterSquared || (chordSquared === diameterSquared && value.major))
        fail("Circular-arc radius and branch are incoherent with its endpoints.");
    }
  });
  oneBased(
    records.fragments.map((value) => value.id),
    "result fragment",
  );
  const usedFragmentReferences = new Set<number>();
  records.rings.forEach((value, index) => {
    reserved(105, index, [[28, 4]]);
    sliceRange(
      records.fragmentReferences,
      value.referenceBegin,
      value.referenceCount,
      "ring fragments",
    );
    if (
      value.id === 0n ||
      (value.parent !== U32_NONE &&
        (value.parent >= index ||
          required(records.rings[value.parent], "parent ring").depth + 1 !== value.depth)) ||
      (value.parent === U32_NONE && value.depth !== 0) ||
      (value.flags & ~1) !== 0 ||
      ((value.flags & 1) !== 0) !== (value.depth % 2 === 1)
    )
      fail("Invalid result ring.");
    const fragments = sliceRange(
      records.fragmentReferences,
      value.referenceBegin,
      value.referenceCount,
      "ring fragments",
    ).map((fragment) => required(records.fragments[fragment], "ring fragment"));
    if (fragments.length < 2) fail("Result ring has too few fragments.");
    for (const reference of sliceRange(
      records.fragmentReferences,
      value.referenceBegin,
      value.referenceCount,
      "ring fragments",
    )) {
      if (usedFragmentReferences.has(reference))
        fail("A directed fragment is referenced more than once.");
      usedFragmentReferences.add(reference);
    }
    fragments.forEach((fragment, fragmentIndex) => {
      const next = required(
        fragments[(fragmentIndex + 1) % fragments.length],
        "next ring fragment",
      );
      if (fragment.end !== next.start) fail("Result ring fragment topology is disconnected.");
    });
  });
  oneBased(
    records.rings.map((value) => value.id),
    "result ring",
  );
  partition(
    records.rings.map((value) => [value.referenceBegin, value.referenceCount]),
    records.fragmentReferences.length,
    "ring fragment-reference",
  );
  for (const reference of records.fragmentReferences)
    if (reference >= records.fragments.length) fail("Invalid fragment reference.");
  if (usedFragmentReferences.size !== records.fragments.length)
    fail("A directed fragment is unreferenced.");
  records.regions.forEach((value, index) => {
    reserved(107, index, [[16, 8]]);
    if (
      value.id === 0n ||
      value.outer >= records.rings.length ||
      required(records.rings[value.outer], "region outer ring").depth % 2 !== 0 ||
      value.positiveSet === 0 ||
      value.positiveSet > records.sourceSets.length
    )
      fail("Invalid result region.");
  });
  oneBased(
    records.regions.map((value) => value.id),
    "result region",
  );
  const regionOuters = records.regions.map((value) => value.outer);
  if (new Set(regionOuters).size !== regionOuters.length)
    fail("A result ring is owned by more than one region.");
  const regionOuterSet = new Set(regionOuters);
  records.rings.forEach((ring, index) => {
    if ((ring.depth % 2 === 0) !== regionOuterSet.has(index))
      fail("Even-depth result rings and result regions are not one-to-one.");
  });
  records.sourceSets.forEach((value) => {
    sliceRange(records.sourceIndices, value.begin, value.count, "source indices");
    if (value.count === 0) fail("Empty source-set record.");
  });
  partition(
    records.sourceSets.map((value) => [value.begin, value.count]),
    records.sourceIndices.length,
    "source-set index",
  );
  for (const index of records.sourceIndices)
    if (index >= records.sources.length) fail("Invalid source index.");
  records.sources.forEach((value, index) => {
    reserved(110, index, [[4, 4]]);
    const high = Number(value.secondaryId >> 32n);
    const low = Number(value.secondaryId & 0xffff_ffffn);
    let allowedRole = false;
    if (value.kind === 1) {
      allowedRole = value.secondaryId !== 0n && (value.role === 1 || value.role === 2);
    } else if (value.kind === 2) {
      if ([16, 17, 32, 33, 34, 35].includes(value.role)) allowedRole = value.secondaryId === 0n;
      else if ([48, 49, 50, 51, 54].includes(value.role)) allowedRole = high !== 0 && low === 0;
      else if (value.role === 52) allowedRole = high !== 0 && low !== 0;
      else if (value.role === 53) allowedRole = high === 1 && low === 0;
    } else if (value.kind === 3) {
      allowedRole = value.role === 0 && value.secondaryId === 0n;
    }
    if (value.operandId === 0n || value.primaryId === 0n || !allowedRole)
      fail("Invalid source reference.");
  });
  for (let index = 1; index < records.sources.length; index += 1)
    if (
      compareSource(
        required(records.sources[index - 1], "source"),
        required(records.sources[index], "source"),
      ) >= 0
    )
      fail("Source-reference table is not strictly canonical.");
  records.sourceSets.forEach((set) => {
    const members = sliceRange(records.sourceIndices, set.begin, set.count, "source set");
    for (let index = 1; index < members.length; index += 1)
      if (
        required(members[index - 1], "source member") >= required(members[index], "source member")
      )
        fail("Source-set members are not strictly ordered.");
  });
  for (let index = 1; index < records.sourceSets.length; index += 1) {
    const left = required(records.sourceSets[index - 1], "source set");
    const right = required(records.sourceSets[index], "source set");
    const leftMembers = sliceRange(records.sourceIndices, left.begin, left.count, "source set");
    const rightMembers = sliceRange(records.sourceIndices, right.begin, right.count, "source set");
    if (compareNumberArrays(leftMembers, rightMembers) >= 0)
      fail("Source-set table is not strictly canonical.");
  }
  records.events.forEach((value, index) => {
    reserved(111, index, [
      [10, 2],
      [24, 24],
    ]);
    sliceRange(
      records.ringRegionReferences,
      value.referenceBegin,
      value.referenceCount,
      "event references",
    );
    if (
      value.operandId === 0n ||
      outcomeKinds[value.kind] === undefined ||
      value.sourceSet > records.sourceSets.length
    )
      fail("Invalid operand event.");
    const references = sliceRange(
      records.ringRegionReferences,
      value.referenceBegin,
      value.referenceCount,
      "event references",
    );
    for (let offset = 0; offset < references.length; offset += 1) {
      const reference = required(references[offset], "event reference");
      if (offset > 0 && required(references[offset - 1], "event reference") >= reference)
        fail("Operand-event references are not strictly ordered.");
      const kind = Number(reference >> 32n);
      const target = Number(reference & 0xffff_ffffn);
      if (
        (kind === 1 && target >= records.rings.length) ||
        (kind === 2 && target >= records.regions.length) ||
        (kind !== 1 && kind !== 2)
      )
        fail("Invalid operand-event result reference.");
    }
  });
  partition(
    records.events.map((value) => [value.referenceBegin, value.referenceCount]),
    records.ringRegionReferences.length,
    "operand-event result-reference",
  );
  records.relationships.forEach((value, index) => {
    reserved(112, index, [
      [10, 2],
      [20, 12],
    ]);
    sliceRange(records.pairs, value.pairBegin, value.pairCount, "relationship pairs");
    if (
      value.queryId === 0n ||
      value.status > 1 ||
      value.dimension > 3 ||
      (value.status === 1 &&
        (value.dimension !== 0 || value.pairBegin !== 0 || value.pairCount !== 0))
    )
      fail("Invalid relationship result.");
    let aggregate = 0;
    let previousPair: readonly Comparable[] | undefined;
    for (const pair of sliceRange(
      records.pairs,
      value.pairBegin,
      value.pairCount,
      "relationship pairs",
    )) {
      const key: readonly Comparable[] = [
        pair.left,
        pair.right,
        pair.dimension,
        pair.equality ? 1 : 0,
        pair.leftContains ? 1 : 0,
        pair.rightContains ? 1 : 0,
      ];
      if (previousPair !== undefined && compareComparable(previousPair, key) >= 0)
        fail("Relationship pairs are not strictly canonical.");
      previousPair = key;
      aggregate = Math.max(aggregate, pair.dimension);
    }
    if (value.status === 0 && value.dimension !== aggregate)
      fail("Relationship aggregate dimension does not match its pairs.");
  });
  strictlyIncreasing(
    records.relationships.map((value) => value.queryId),
    "relationship query ids",
  );
  partition(
    records.relationships.map((value) => [value.pairBegin, value.pairCount]),
    records.pairs.length,
    "relationship pair",
  );
  records.pairs.forEach((value, index) => {
    reserved(113, index, [[20, 12]]);
    const pair = recordView(required(tables[12], "relationship pair table"), index);
    if (
      value.dimension > 3 ||
      value.left < 1n ||
      value.left > BigInt(records.regions.length) ||
      value.right < 1n ||
      value.right > BigInt(records.regions.length) ||
      pair.getUint8(17) > 1 ||
      pair.getUint8(18) > 1 ||
      pair.getUint8(19) > 1
    )
      fail("Invalid relationship pair.");
    if (
      ((value.equality || value.leftContains || value.rightContains) && value.dimension !== 3) ||
      (value.equality && (!value.leftContains || !value.rightContains))
    )
      fail("Relationship pair flags are inconsistent with its dimension.");
  });
  const usedSets = flags(records.sourceSets.length);
  const markHandle = (handle: number): void => {
    if (handle !== 0) usedSets[handle - 1] = true;
  };
  records.vertices.forEach((value) => {
    markHandle(value.sourceSet);
  });
  records.fragments.forEach((value) => {
    markHandle(value.positiveSet);
    markHandle(value.subtractionSet);
  });
  records.regions.forEach((value) => {
    markHandle(value.positiveSet);
  });
  records.events.forEach((value) => {
    markHandle(value.sourceSet);
  });
  if (usedSets.some((value) => !value)) fail("Result packet contains an unused source set.");
  const usedSources = flags(records.sources.length);
  for (const index of records.sourceIndices) usedSources[index] = true;
  if (usedSources.some((value) => !value))
    fail("Result packet contains an unused source reference.");

  const mutableOwners = {
    vertices: Array.from({ length: records.vertices.length }, () => -1),
    fragments: Array.from({ length: records.fragments.length }, () => -1),
    rings: Array.from({ length: records.rings.length }, () => -1),
    regions: Array.from({ length: records.regions.length }, () => -1),
    events: Array.from({ length: records.events.length }, () => -1),
  };
  records.jobs.forEach((job, jobIndex) => {
    for (let offset = 0; offset < job.regionCount; offset += 1)
      mutableOwners.regions[job.regionBegin + offset] = jobIndex;
    for (let offset = 0; offset < job.eventCount; offset += 1)
      mutableOwners.events[job.eventBegin + offset] = jobIndex;
  });
  const outerRegions = Array.from({ length: records.rings.length }, () => -1);
  records.regions.forEach((region, index) => {
    outerRegions[region.outer] = index;
  });
  records.rings.forEach((ring, index) => {
    const owner =
      ring.parent === U32_NONE
        ? mutableOwners.regions[required(outerRegions[index], "root-ring region")]
        : mutableOwners.rings[ring.parent];
    if (owner === undefined || owner < 0) fail("Result ring has no owning job.");
    mutableOwners.rings[index] = owner;
    for (const fragment of sliceRange(
      records.fragmentReferences,
      ring.referenceBegin,
      ring.referenceCount,
      "ring fragments",
    )) {
      const previous = mutableOwners.fragments[fragment];
      if (previous === undefined || (previous !== -1 && previous !== owner))
        fail("A mutable result record is shared by jobs.");
      mutableOwners.fragments[fragment] = owner;
    }
  });
  records.regions.forEach((region, index) => {
    if (
      required(mutableOwners.rings[region.outer], "outer-ring owner") !==
      required(mutableOwners.regions[index], "region owner")
    )
      fail("Result region and its outer ring belong to different jobs.");
  });
  records.fragments.forEach((fragment, index) => {
    const owner = required(mutableOwners.fragments[index], "fragment owner");
    for (const vertex of [fragment.start, fragment.end]) {
      const previous = mutableOwners.vertices[vertex];
      if (previous === undefined || (previous !== -1 && previous !== owner))
        fail("A mutable result record is shared by jobs.");
      mutableOwners.vertices[vertex] = owner;
    }
  });
  for (const owners of Object.values(mutableOwners))
    if (owners.some((owner) => owner === -1))
      fail("Result packet contains an unowned result record.");

  const jobBounds = records.jobs.map(() => ({
    hasVertex: false,
    minX: 0n,
    maxX: 0n,
    minY: 0n,
    maxY: 0n,
  }));
  records.vertices.forEach((vertex, index) => {
    const owner = required(mutableOwners.vertices[index], "vertex owner");
    const bounds = required(jobBounds[owner], "job bounds");
    if (!bounds.hasVertex) {
      bounds.hasVertex = true;
      bounds.minX = bounds.maxX = vertex.x;
      bounds.minY = bounds.maxY = vertex.y;
    } else {
      bounds.minX = vertex.x < bounds.minX ? vertex.x : bounds.minX;
      bounds.maxX = vertex.x > bounds.maxX ? vertex.x : bounds.maxX;
      bounds.minY = vertex.y < bounds.minY ? vertex.y : bounds.minY;
      bounds.maxY = vertex.y > bounds.maxY ? vertex.y : bounds.maxY;
    }
  });
  if (
    jobBounds.some(
      (bounds) =>
        bounds.hasVertex &&
        (bounds.maxX - bounds.minX > MAX_LENGTH_NM || bounds.maxY - bounds.minY > MAX_LENGTH_NM),
    )
  )
    fail("A job result exceeds the governed coordinate span.");

  records.events.forEach((event, index) => {
    const owner = required(mutableOwners.events[index], "event owner");
    for (const reference of sliceRange(
      records.ringRegionReferences,
      event.referenceBegin,
      event.referenceCount,
      "event references",
    )) {
      const kind = Number(reference >> 32n);
      const target = Number(reference & 0xffff_ffffn);
      const referencedOwner =
        kind === 1
          ? required(mutableOwners.rings[target], "ring owner")
          : required(mutableOwners.regions[target], "region owner");
      if (referencedOwner !== owner) fail("Operand event references a different job closure.");
    }
  });

  validateCanonicalResultOrder(records, mutableOwners);
  return buildJobSelections(records, mutableOwners);
}

type Comparable = bigint | number | boolean | readonly Comparable[];

function validateCanonicalResultOrder(
  records: ResultRecords,
  owners: {
    readonly vertices: readonly number[];
    readonly fragments: readonly number[];
    readonly rings: readonly number[];
    readonly regions: readonly number[];
    readonly events: readonly number[];
  },
): void {
  const jobId = (owner: number): bigint => required(records.jobs[owner], "owner job").jobId;
  const incidents: Comparable[][][] = records.vertices.map(() => []);
  records.fragments.forEach((fragment) => {
    const start = required(records.vertices[fragment.start], "fragment start vertex");
    const end = required(records.vertices[fragment.end], "fragment end vertex");
    required(incidents[fragment.start], "start incidents").push([
      0,
      end.x,
      end.y,
      fragment.kind,
      fragment.direction,
      fragment.major,
      fragment.radius,
    ]);
    required(incidents[fragment.end], "end incidents").push([
      1,
      start.x,
      start.y,
      fragment.kind,
      fragment.direction,
      fragment.major,
      fragment.radius,
    ]);
  });
  const vertexKeys: Comparable[][] = records.vertices.map((vertex, vertexIndex) => {
    const vertexIncidents = required(incidents[vertexIndex], "vertex incidents");
    vertexIncidents.sort(compareComparable);
    return [
      jobId(required(owners.vertices[vertexIndex], "vertex owner")),
      vertex.x,
      vertex.y,
      vertexIncidents,
      vertex.sourceSet,
    ];
  });
  strictlyCanonicalKeys(vertexKeys, "result vertices");

  const fragmentKeys: Comparable[][] = records.fragments.map((fragment, index) => [
    jobId(required(owners.fragments[index], "fragment owner")),
    fragment.start,
    fragment.end,
    fragment.kind,
    fragment.direction,
    fragment.major,
    fragment.radius,
    fragment.positiveSet,
    fragment.subtractionSet,
  ]);
  strictlyCanonicalKeys(fragmentKeys, "directed fragments");

  const ringKeys: Comparable[][] = records.rings.map((ring, index) => {
    const references = [
      ...sliceRange(
        records.fragmentReferences,
        ring.referenceBegin,
        ring.referenceCount,
        "ring references",
      ),
    ];
    const rotation = leastRotation(references);
    if (rotation !== 0) fail("Result ring does not use its least canonical fragment rotation.");
    return [
      jobId(required(owners.rings[index], "ring owner")),
      ring.depth,
      references,
      ring.parent,
    ];
  });
  strictlyCanonicalKeys(ringKeys, "result rings");

  const regionKeys: Comparable[][] = records.regions.map((region, index) => [
    jobId(required(owners.regions[index], "region owner")),
    region.outer,
    region.positiveSet,
  ]);
  strictlyCanonicalKeys(regionKeys, "result regions");

  for (const job of records.jobs) {
    const keys = sliceRange(records.events, job.eventBegin, job.eventCount, "job events").map(
      (event): Comparable[] => [
        event.operandId,
        event.kind,
        [
          ...sliceRange(
            records.ringRegionReferences,
            event.referenceBegin,
            event.referenceCount,
            "event references",
          ),
        ],
        event.sourceSet,
      ],
    );
    strictlyCanonicalKeys(keys, "operand events");
  }
}

function strictlyCanonicalKeys(keys: readonly (readonly Comparable[])[], label: string): void {
  for (let index = 1; index < keys.length; index += 1)
    if (
      compareComparable(
        required(keys[index - 1], `${label} key`),
        required(keys[index], `${label} key`),
      ) >= 0
    )
      fail(`${label} are not strictly canonical.`);
}

function compareComparable(left: Comparable, right: Comparable): number {
  if (Array.isArray(left) && Array.isArray(right)) {
    const count = Math.min(left.length, right.length);
    for (let index = 0; index < count; index += 1) {
      const comparison = compareComparable(
        required(left[index], "comparison member"),
        required(right[index], "comparison member"),
      );
      if (comparison !== 0) return comparison;
    }
    return left.length - right.length;
  }
  if (Array.isArray(left) || Array.isArray(right)) fail("Canonical comparison shape mismatch.");
  const a = typeof left === "boolean" ? (left ? 1 : 0) : left;
  const b = typeof right === "boolean" ? (right ? 1 : 0) : right;
  return a < b ? -1 : a > b ? 1 : 0;
}

function leastRotation(values: readonly number[]): number {
  if (values.length === 0) return 0;
  let left = 0;
  let right = 1;
  let offset = 0;
  while (left < values.length && right < values.length && offset < values.length) {
    const a = required(values[(left + offset) % values.length], "rotation member");
    const b = required(values[(right + offset) % values.length], "rotation member");
    if (a === b) {
      offset += 1;
      continue;
    }
    if (a > b) {
      left += offset + 1;
      if (left === right) left += 1;
    } else {
      right += offset + 1;
      if (left === right) right += 1;
    }
    offset = 0;
  }
  return Math.min(left, right);
}

export interface Selection {
  vertices: number[];
  fragments: number[];
  rings: number[];
  regions: number[];
  events: number[];
  sets: number[];
  sources: number[];
}

function buildJobSelections(
  records: ResultRecords,
  owners: {
    readonly vertices: readonly number[];
    readonly fragments: readonly number[];
    readonly rings: readonly number[];
    readonly regions: readonly number[];
    readonly events: readonly number[];
  },
): Selection[] {
  const selections: Selection[] = records.jobs.map(() => ({
    vertices: [],
    fragments: [],
    rings: [],
    regions: [],
    events: [],
    sets: [],
    sources: [],
  }));
  const setsByJob = records.jobs.map(() => new Set<number>());
  const appendOwned = (values: readonly number[], key: keyof Selection): void => {
    values.forEach((owner, index) => {
      required(selections[owner], "job selection")[key].push(index);
    });
  };
  appendOwned(owners.vertices, "vertices");
  appendOwned(owners.fragments, "fragments");
  appendOwned(owners.rings, "rings");
  appendOwned(owners.regions, "regions");
  appendOwned(owners.events, "events");

  const markSet = (owner: number, handle: number): void => {
    if (handle !== 0) required(setsByJob[owner], "job source sets").add(handle - 1);
  };
  records.vertices.forEach((value, index) => {
    markSet(required(owners.vertices[index], "vertex owner"), value.sourceSet);
  });
  records.fragments.forEach((value, index) => {
    const owner = required(owners.fragments[index], "fragment owner");
    markSet(owner, value.positiveSet);
    markSet(owner, value.subtractionSet);
  });
  records.regions.forEach((value, index) => {
    markSet(required(owners.regions[index], "region owner"), value.positiveSet);
  });
  records.events.forEach((value, index) => {
    markSet(required(owners.events[index], "event owner"), value.sourceSet);
  });

  selections.forEach((selection, jobIndex) => {
    selection.sets = [...required(setsByJob[jobIndex], "job source sets")].sort((a, b) => a - b);
    const sources = new Set<number>();
    for (const setIndex of selection.sets) {
      const set = required(records.sourceSets[setIndex], "source set");
      for (const source of sliceRange(records.sourceIndices, set.begin, set.count, "source set"))
        sources.add(source);
    }
    selection.sources = [...sources].sort((a, b) => a - b);
  });
  return selections;
}

export function encodeStandalone(
  input: ResultRecords,
  jobIndex: number,
  selection: Selection,
): Uint8Array {
  const vertexMap = sparseMap(selection.vertices);
  const fragmentMap = sparseMap(selection.fragments);
  const ringMap = sparseMap(selection.rings);
  const regionMap = sparseMap(selection.regions);
  const setMap = sparseMap(selection.sets);
  const sourceMap = sparseMap(selection.sources);
  const job = required(input.jobs[jobIndex], "standalone job");
  const output: ResultRecords = {
    jobs: [],
    diagnostics: [
      ...sliceRange(input.diagnostics, job.diagnosticBegin, job.diagnosticCount, "diagnostics"),
    ],
    vertices: [],
    fragments: [],
    rings: [],
    fragmentReferences: [],
    regions: [],
    ringRegionReferences: [],
    sourceSets: [],
    sources: selection.sources.map((index) => required(input.sources[index], "source")),
    events: [],
    relationships: [],
    pairs: [],
    sourceIndices: [],
  };
  for (const index of selection.vertices) {
    const value = required(input.vertices[index], "vertex");
    output.vertices.push({
      ...value,
      id: BigInt(output.vertices.length + 1),
      sourceSet: remapHandle(value.sourceSet, setMap),
    });
  }
  for (const index of selection.fragments) {
    const value = required(input.fragments[index], "fragment");
    output.fragments.push({
      ...value,
      id: BigInt(output.fragments.length + 1),
      start: mapped(vertexMap, value.start),
      end: mapped(vertexMap, value.end),
      positiveSet: remapHandle(value.positiveSet, setMap),
      subtractionSet: remapHandle(value.subtractionSet, setMap),
    });
  }
  for (const index of selection.rings) {
    const value = required(input.rings[index], "ring");
    const begin = output.fragmentReferences.length;
    for (const reference of sliceRange(
      input.fragmentReferences,
      value.referenceBegin,
      value.referenceCount,
      "ring references",
    ))
      output.fragmentReferences.push(mapped(fragmentMap, reference));
    output.rings.push({
      ...value,
      id: BigInt(output.rings.length + 1),
      referenceBegin: begin,
      parent: value.parent === U32_NONE ? U32_NONE : mapped(ringMap, value.parent),
    });
  }
  for (const index of selection.regions) {
    const value = required(input.regions[index], "region");
    output.regions.push({
      ...value,
      id: BigInt(output.regions.length + 1),
      outer: mapped(ringMap, value.outer),
      positiveSet: remapHandle(value.positiveSet, setMap),
    });
  }
  for (const index of selection.sets) {
    const value = required(input.sourceSets[index], "source set");
    const begin = output.sourceIndices.length;
    for (const source of sliceRange(
      input.sourceIndices,
      value.begin,
      value.count,
      "source indices",
    ))
      output.sourceIndices.push(mapped(sourceMap, source));
    output.sourceSets.push({ begin, count: value.count });
  }
  for (const index of selection.events) {
    const value = required(input.events[index], "event");
    const begin = value.referenceCount === 0 ? 0 : output.ringRegionReferences.length;
    for (const reference of sliceRange(
      input.ringRegionReferences,
      value.referenceBegin,
      value.referenceCount,
      "event references",
    )) {
      const kind = Number(reference >> 32n);
      const target = Number(reference & 0xffff_ffffn);
      output.ringRegionReferences.push(
        (BigInt(kind) << 32n) |
          BigInt(kind === 1 ? mapped(ringMap, target) : mapped(regionMap, target)),
      );
    }
    output.events.push({
      ...value,
      referenceBegin: begin,
      sourceSet: remapHandle(value.sourceSet, setMap),
    });
  }
  output.jobs.push({ ...job, diagnosticBegin: 0, regionBegin: 0, eventBegin: 0 });
  return encodeResultRecords(output);
}

export function encodeResultRecords(records: ResultRecords): Uint8Array {
  const tables = RESULT_TABLES.map<MutableTable>((recordBytes, index) => ({
    kind: 101 + index,
    recordBytes,
    records: [],
  }));
  const add = (index: number, bytes: Uint8Array): void => {
    required(tables[index], `result table ${index}`).records.push(bytes);
  };
  for (const v of records.jobs)
    add(
      0,
      record(48, (d) => {
        putU64(d, 0, v.jobId, "job id", true);
        d.setUint8(8, v.status);
        d.setUint32(16, v.diagnosticBegin, true);
        d.setUint32(20, v.diagnosticCount, true);
        d.setUint32(24, v.regionBegin, true);
        d.setUint32(28, v.regionCount, true);
        d.setUint32(32, v.eventBegin, true);
        d.setUint32(36, v.eventCount, true);
      }),
    );
  for (const v of records.diagnostics)
    add(
      1,
      record(56, (d) => {
        d.setUint32(0, v.code, true);
        d.setUint8(4, v.severity);
        d.setUint8(5, 1);
        d.setUint16(6, v.presence, true);
        d.setBigUint64(8, v.jobId, true);
        d.setBigUint64(16, v.stageId, true);
        d.setBigUint64(24, v.operandId, true);
        d.setBigUint64(32, v.geometryId, true);
        d.setUint32(40, v.pathToken, true);
      }),
    );
  for (const v of records.vertices)
    add(
      2,
      record(32, (d) => {
        d.setBigUint64(0, v.id, true);
        d.setBigInt64(8, v.x, true);
        d.setBigInt64(16, v.y, true);
        d.setUint32(24, v.sourceSet, true);
        d.setUint32(28, v.flags, true);
      }),
    );
  for (const v of records.fragments)
    add(
      3,
      record(48, (d) => {
        d.setBigUint64(0, v.id, true);
        d.setUint32(8, v.start, true);
        d.setUint32(12, v.end, true);
        d.setUint8(16, v.kind);
        d.setUint8(17, v.direction);
        d.setUint8(18, v.major ? 1 : 0);
        d.setBigUint64(24, v.radius, true);
        d.setUint32(32, v.positiveSet, true);
        d.setUint32(36, v.subtractionSet, true);
      }),
    );
  for (const v of records.rings)
    add(
      4,
      record(32, (d) => {
        d.setBigUint64(0, v.id, true);
        d.setUint32(8, v.referenceBegin, true);
        d.setUint32(12, v.referenceCount, true);
        d.setUint32(16, v.parent, true);
        d.setUint32(20, v.depth, true);
        d.setUint32(24, v.flags, true);
      }),
    );
  for (const v of records.fragmentReferences)
    add(
      5,
      record(4, (d) => d.setUint32(0, v, true)),
    );
  for (const v of records.regions)
    add(
      6,
      record(24, (d) => {
        d.setBigUint64(0, v.id, true);
        d.setUint32(8, v.outer, true);
        d.setUint32(12, v.positiveSet, true);
      }),
    );
  for (const v of records.ringRegionReferences)
    add(
      7,
      record(8, (d) => d.setBigUint64(0, v, true)),
    );
  for (const v of records.sourceSets)
    add(
      8,
      record(8, (d) => {
        d.setUint32(0, v.begin, true);
        d.setUint32(4, v.count, true);
      }),
    );
  for (const v of records.sources)
    add(
      9,
      record(32, (d) => {
        d.setUint16(0, v.kind, true);
        d.setUint16(2, v.role, true);
        d.setBigUint64(8, v.operandId, true);
        d.setBigUint64(16, v.primaryId, true);
        d.setBigUint64(24, v.secondaryId, true);
      }),
    );
  for (const v of records.events)
    add(
      10,
      record(48, (d) => {
        d.setBigUint64(0, v.operandId, true);
        d.setUint16(8, v.kind, true);
        d.setUint32(12, v.referenceBegin, true);
        d.setUint32(16, v.referenceCount, true);
        d.setUint32(20, v.sourceSet, true);
      }),
    );
  for (const v of records.relationships)
    add(
      11,
      record(32, (d) => {
        d.setBigUint64(0, v.queryId, true);
        d.setUint8(8, v.status);
        d.setUint8(9, v.dimension);
        d.setUint32(12, v.pairBegin, true);
        d.setUint32(16, v.pairCount, true);
      }),
    );
  for (const v of records.pairs)
    add(
      12,
      record(32, (d) => {
        d.setBigUint64(0, v.left, true);
        d.setBigUint64(8, v.right, true);
        d.setUint8(16, v.dimension);
        d.setUint8(17, v.equality ? 1 : 0);
        d.setUint8(18, v.leftContains ? 1 : 0);
        d.setUint8(19, v.rightContains ? 1 : 0);
      }),
    );
  for (const v of records.sourceIndices)
    add(
      13,
      record(4, (d) => d.setUint32(0, v, true)),
    );
  return encodeTables(RESULT_MAGIC, tables, records.jobs.length, records.relationships.length);
}
