// Governed A0 cases shared by the production TS renderer and direct native API.
const cube = {
  id: "cube",
  positions: [-1, -1, -1, 1, -1, -1, 1, 1, -1, -1, 1, -1, -1, -1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1],
  indices: [
    4, 5, 6, 4, 6, 7, 1, 0, 3, 1, 3, 2, 1, 2, 6, 1, 6, 5, 0, 4, 7, 0, 7, 3, 3, 7, 6, 3, 6, 2, 0, 1,
    5, 0, 5, 4,
  ],
  materials: [{ color: [0.2, 0.7, 0.62] }, { color: [0.8, 0.2, 0.1] }],
  triangle_material_indices: [0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0],
};

export function nativeIllustrationFixtures() {
  const cases = [];
  const input = (meshes, style = {}, view = { direction: [0, 0, 1], up: [0, 1, 0] }) => ({
    schema: "geometry.mesh_illustration.input.a0",
    meshes,
    view,
    style,
  });
  for (const shading of ["unlit", "flat", "lambert", "banded", "toon"]) {
    for (const direction of [
      [0, 0, 1],
      [0.4, 0.7, 1],
      [-1, -0.4, -0.8],
    ]) {
      cases.push({
        name: `${shading}-${direction.join("_")}`,
        input: input([cube], { shading }, { direction, up: [0, 1, 0] }),
      });
    }
  }
  for (const [name, style] of Object.entries({
    no_fusion: { fuse_surfaces: false },
    no_layers: { layer_coplanar_materials: false },
    no_lines: { show_outlines: false, show_creases: false },
    double_sided: { double_sided: true },
    fallback: { source_colors: false, fallback_color: [0.9, 0.1, 0.5] },
    light: {
      ambient: 0.05,
      key_intensity: 2,
      bands: 1,
      light_direction: [-1, 0, 0.3],
      rim_amount: 0.9,
    },
    transparent: { transparent_background: true },
    unicode_css: {
      background: "\u00a0red\u00a0",
      outline_color: "rgb(1,\u00a02,3)",
      crease_color: "h\u017fla(3,4%,5%,.6)",
    },
    unsafe_css: { background: "red;}</style><script>x</script>", outline_color: "url(file:bad)" },
  }))
    cases.push({ name, input: input([cube], style, { direction: [0.4, 0.7, 1], up: [0, 1, 0] }) });
  for (const [name, matrix] of Object.entries({
    reflect: [-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1],
    affine: [2, 0, 0.2, 0, 0.3, 1, 0, 0, 0, 0.1, 0.7, 0, 3, -2, 4, 1],
    projective: [1, 0, 0, 0.05, 0, 1, 0, 0.02, 0, 0, 1, 0, 0, 0, 0, 1],
  }))
    cases.push({
      name,
      input: input(
        [{ ...cube, matrix, normals: cube.positions.map((v) => v * 0.5) }],
        {},
        { direction: [0.4, 0.7, 1], up: [0, 1, 0], mirror_x: true },
      ),
    });
  const triangle = (id, positions, color = [0.5, 0.5, 0.5]) => ({
    id,
    positions,
    materials: [{ color }],
  });
  cases.push({
    name: "overlap-depth",
    input: input(
      [
        triangle("slope", [-2, -2, -3, 2, -2, 3, 0, 2, 0], [0.8, 0.1, 0.1]),
        triangle("front", [0.4, -1, 2, 1.4, -1, 2, 0.9, 0.2, 2], [0.1, 0.8, 0.1]),
        triangle("back", [-1.5, -1, -2, -0.5, -1, -2, -1, 0.2, -2], [0.1, 0.1, 0.8]),
      ],
      { shading: "unlit" },
    ),
  });
  const tiled = {
    id: "tiles",
    positions: [0, 0, 0, 3, 0, 0, 3, 3, 0, 0, 3, 0, 1, 1, 0, 2, 1, 0, 2, 2, 0, 1, 2, 0],
    indices: [
      0, 1, 5, 0, 5, 4, 1, 2, 6, 1, 6, 5, 2, 3, 7, 2, 7, 6, 3, 0, 4, 3, 4, 7, 4, 5, 6, 4, 6, 7,
    ],
    materials: [{ color: [0.2, 0.2, 0.2] }, { color: [0.9, 0.8, 0.7] }],
    triangle_material_indices: [0, 0, 0, 0, 0, 0, 0, 0, 1, 1],
  };
  cases.push({ name: "coplanar-inlay", input: input([tiled], { shading: "unlit" }) });
  cases.push({
    name: "ring-hole",
    input: input([
      {
        ...tiled,
        indices: tiled.indices.slice(0, 24),
        triangle_material_indices: tiled.triangle_material_indices.slice(0, 8),
      },
    ]),
  });
  cases.push({
    name: "opacity",
    input: input(
      [{ ...cube, materials: cube.materials.map((m) => ({ ...m, opacity: 0.375 })) }],
      { double_sided: true },
      { direction: [0.4, 0.7, 1], up: [0, 1, 0] },
    ),
  });
  cases.push({
    name: "decimal-halfway",
    input: input(
      [{ ...cube, materials: cube.materials.map((m) => ({ ...m, opacity: 0.5001220703125 })) }],
      { outline_width: 100000000.0005 },
      { direction: [0.4, 0.7, 1], up: [0, 1, 0] },
    ),
  });
  cases.push({ name: "warnings", input: input([{ ...cube, indices: [99, 0, 1, 0, 0, 0] }]) });
  cases.push({
    name: "warning-cap",
    input: input([{ ...cube, indices: Array.from({ length: 780 }, () => 99) }]),
  });
  cases.push({
    name: "svg-options",
    input: { ...input([cube]), svg: { title: 'A < B & "quoted"', coordinate_span: 1234567 } },
  });
  cases.push({
    name: "xml-text",
    input: { ...input([cube]), svg: { title: "a]]>b — µ 🔬", coordinate_span: 10000 } },
  });
  // Deterministic triangle soup exercises non-fusible interpenetration and SCC ordering.
  let state = 12345;
  const random = () => {
    state = (Math.imul(state, 1664525) + 1013904223) >>> 0;
    return state / 4294967296;
  };
  const soup = Array.from({ length: 60 }, (_, i) =>
    triangle(
      `soup-${i}`,
      Array.from({ length: 9 }, () => random() * 4 - 2),
      [random(), random(), random()],
    ),
  );
  cases.push({ name: "soup", input: input(soup, { double_sided: true }) });
  return cases;
}
