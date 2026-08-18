import { resolveReflectedCanvasArc } from "../../dist/wasm/demos/analytic_canvas_arc.js";

function close(actual, expected, label) {
  if (Math.abs(actual - expected) > 1e-9) throw new Error(`${label}: ${actual} != ${expected}`);
}

function check(direction, majorArc, centerX, centerY, counterclockwise) {
  const arc = resolveReflectedCanvasArc(1, 0, 0, 1, 1, direction, majorArc);
  if (arc === undefined) throw new Error(`${direction}/${majorArc}: arc was not resolved.`);
  close(arc.centerX, centerX, `${direction}/${majorArc} center x`);
  close(arc.centerY, centerY, `${direction}/${majorArc} center y`);
  if (arc.counterclockwise !== counterclockwise)
    throw new Error(`${direction}/${majorArc}: reflected Canvas direction drifted.`);
}

check("ccw", false, 0, 0, false);
check("cw", false, 1, 1, true);
check("ccw", true, 1, 1, false);
check("cw", true, 0, 0, true);

const semicircle = resolveReflectedCanvasArc(-1, 0, 1, 0, 1, "ccw", false);
if (semicircle === undefined || semicircle.counterclockwise)
  throw new Error("L-to-R CCW semicircle must use increasing Canvas angles after reflection.");

const roundedDiameter = resolveReflectedCanvasArc(0, 0, 2 + Number.EPSILON * 2, 0, 1, "ccw", false);
if (roundedDiameter === undefined)
  throw new Error("A diameter arc within floating-point slack must remain renderable.");
if (resolveReflectedCanvasArc(0, 0, 2.001, 0, 1, "ccw", false) !== undefined)
  throw new Error("An arc beyond its diameter tolerance must be rejected.");

console.log(JSON.stringify({ branches: 7, reflected: true }));
