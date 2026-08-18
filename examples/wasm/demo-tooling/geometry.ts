export interface Vec2 {
  readonly x: number;
  readonly y: number;
}

export interface Bounds2 {
  readonly min: Vec2;
  readonly max: Vec2;
}

export interface ViewportSize {
  readonly width: number;
  readonly height: number;
}

export function addVec2(left: Vec2, right: Vec2): Vec2 {
  return { x: left.x + right.x, y: left.y + right.y };
}

export function subtractVec2(left: Vec2, right: Vec2): Vec2 {
  return { x: left.x - right.x, y: left.y - right.y };
}

export function scaleVec2(value: Vec2, scale: number): Vec2 {
  return { x: value.x * scale, y: value.y * scale };
}

export function interpolateVec2(from: Vec2, to: Vec2, amount: number): Vec2 {
  return {
    x: from.x + (to.x - from.x) * amount,
    y: from.y + (to.y - from.y) * amount,
  };
}
