// Test-only oracle: native runtime consumers never load JavaScript.
import { readFileSync } from "node:fs";
import { illustrateMesh, createIllustrator } from "../../dist/wasm/npm/geometer/mesh-illustration.js";

const value = JSON.parse(readFileSync(0, "utf8"));
if (value.input && value.hlr) {
  const mirror = value.input.view.mirror_x ? -1 : 1;
  const segments = (layer) => layer.segments.map(([x1, y1, x2, y2]) => ({
    points: [[mirror * x1, y1], [mirror * x2, y2]],
  }));
  const view = value.hlr.views[0];
  const illustrator = createIllustrator(value.input, {
    outlineSegments: segments(view.modes.outline),
    detailSegments: segments(view.modes.detail),
  });
  try { process.stdout.write(JSON.stringify(illustrator.renderSvg())); }
  finally { illustrator.dispose(); }
} else {
  process.stdout.write(JSON.stringify(illustrateMesh(value)));
}
