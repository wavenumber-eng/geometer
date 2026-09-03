import { existsSync, readFileSync } from "node:fs";
import { resolve } from "node:path";

const root = resolve(import.meta.dirname, "../..");
const themePath = resolve(root, "examples/wasm/geometer_demo.css");
const theme = readFileSync(themePath, "utf8");
const designTheme = readFileSync(resolve(root, "docs/design/styles.css"), "utf8");
const pages = [
  "analytic_polygon_pour_demo.html",
  "illustration_demo.html",
  "model_bounds_demo.html",
  "pcb_polygon_pour_demo.html",
];

if (!theme.includes('font-family: "JetBrains Mono"')) {
  throw new Error("Shared demo theme must use the governed JetBrains Mono webfont.");
}
if (!theme.includes("border-radius: 0 !important")) {
  throw new Error("Shared demo theme must enforce flat component geometry.");
}
for (const stylesheet of [theme, designTheme]) {
  if (
    !stylesheet.includes("--wn-accent: #b45309;") ||
    !stylesheet.includes("--wn-accent-bg: #fff1e6;")
  ) {
    throw new Error("Shared visual-system highlights must use the governed orange palette.");
  }
}
for (const rootSelector of ["#pour-shell", ".illustration-app", ".bounds-app", "#pcb-shell"]) {
  if (!theme.includes(rootSelector)) {
    throw new Error(`Shared demo theme is missing the ${rootSelector} layout namespace.`);
  }
}
if (!theme.includes("min-height: max(620px, 100vh)")) {
  throw new Error("Model-bounds desktop layout must fill viewports taller than its minimum.");
}
if (!/\.bounds-app \.primary-action:hover,[\s\S]*?color: var\(--wn-paper\);/u.test(theme)) {
  throw new Error("Model-bounds primary actions must retain readable text on interaction.");
}

for (const page of pages) {
  const path = resolve(root, "examples/wasm", page);
  const html = readFileSync(path, "utf8");
  const stylesheetLinks = [...html.matchAll(/<link\b[^>]*rel="stylesheet"[^>]*>/gu)];
  if (stylesheetLinks.length !== 1 || !stylesheetLinks[0]?.[0].includes("geometer_demo.css")) {
    throw new Error(`${page} must use geometer_demo.css as its one stylesheet.`);
  }
  if (/<style\b/iu.test(html) || /\sstyle\s*=/iu.test(html)) {
    throw new Error(`${page} must not contain inline visual styles.`);
  }
  if (!html.includes('data-wn-watermark="true"')) {
    throw new Error(`${page} must opt into the shared Wavenumber watermark.`);
  }
}

if (existsSync(resolve(root, "examples/wasm/model_bounds_demo.css"))) {
  throw new Error("The retired model-bounds-only stylesheet must not exist.");
}
const modelBoundsSource = readFileSync(resolve(root, "examples/wasm/model_bounds_demo.ts"), "utf8");
if (/0x[\da-f]{6}|#[\da-f]{3,8}/iu.test(modelBoundsSource)) {
  throw new Error("Model-bounds scene colors must resolve from shared CSS variables.");
}

process.stdout.write(
  "Shared analytic, illustration, PCB, and model-bounds demo theme validated.\n",
);
