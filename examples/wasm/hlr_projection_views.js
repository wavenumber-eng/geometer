// A preset frame is explicit: Top and Front are viewer directions in model
// coordinates. Right is derived as Top x Front to keep the frame right-handed.
// Defaults use the demo convention requested for STEP models: Top +Y, Front +Z.
export const AXIS_VECTORS = {
  "+x": [1, 0, 0],
  "-x": [-1, 0, 0],
  "+y": [0, 1, 0],
  "-y": [0, -1, 0],
  "+z": [0, 0, 1],
  "-z": [0, 0, -1],
};

function negateVector(value) {
  return value.map((component) => -component);
}

function crossVector(a, b) {
  return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
}

function normalizedSum(...vectors) {
  const result = [0, 0, 0];
  for (const vector of vectors)
    for (let index = 0; index < 3; index += 1) result[index] += vector[index];
  const length = Math.hypot(...result) || 1;
  return result.map((component) => component / length);
}

export function buildProjectionViews(topAxisId, frontAxisId) {
  const top = AXIS_VECTORS[topAxisId];
  const front = AXIS_VECTORS[frontAxisId];
  const right = crossVector(top, front);
  return [
    { id: "top", label: "Top", direction: top, up: negateVector(front) },
    { id: "bottom", label: "Bottom", direction: negateVector(top), up: negateVector(front) },
    { id: "front", label: "Front", direction: front, up: top },
    { id: "back", label: "Back", direction: negateVector(front), up: top, mirrorX: true },
    { id: "left", label: "Left", direction: negateVector(right), up: top, mirrorX: true },
    { id: "right", label: "Right", direction: right, up: top },
    {
      id: "isoTop",
      label: "ISO Top",
      direction: normalizedSum(right, negateVector(front), top),
      up: negateVector(front),
    },
    {
      id: "isoBottom",
      label: "ISO Bot",
      direction: normalizedSum(right, front, negateVector(top)),
      up: front,
    },
    { id: "isoFront", label: "ISO Front", direction: normalizedSum(right, front, top), up: top },
    {
      id: "isoBack",
      label: "ISO Back",
      direction: normalizedSum(right, negateVector(front), negateVector(top)),
      up: negateVector(top),
    },
  ];
}
