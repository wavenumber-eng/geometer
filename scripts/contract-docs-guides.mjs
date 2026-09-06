import { readdir, readFile } from "node:fs/promises";
import { dirname, relative, resolve, sep } from "node:path";
import MarkdownIt from "markdown-it";

const slash = (value) => value.split(sep).join("/");
const escapeMarkup = (value) =>
  String(value)
    .replaceAll("&", "&amp;")
    .replaceAll('"', "&quot;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;");

export async function generateGuides(root, output) {
  const sources = ["README.md", "examples/README.md"];
  async function discover(directory) {
    for (const entry of await readdir(resolve(root, directory), { withFileTypes: true })) {
      if (entry.name.startsWith(".")) continue;
      const path = `${directory}/${entry.name}`;
      if (entry.isDirectory()) await discover(path);
      else if (entry.name.endsWith(".md")) sources.push(path);
    }
  }
  for (const directory of ["docs/design", "docs/contracts", "docs/developer", "docs/geometer"])
    await discover(directory);
  sources.sort();
  const destinations = new Map(
    sources.map((source) => [resolve(root, source), `guides/${source.replace(/\.md$/, ".html")}`]),
  );
  const pages = [];
  for (const source of sources) {
    const destination = destinations.get(resolve(root, source));
    const relativeLink = (target) => slash(relative(dirname(resolve(output, destination)), target));
    const sourceText = (await readFile(resolve(root, source), "utf8")).replace(/\r\n/g, "\n");
    const markdown = sourceText.replace(/^\+\+\+\n[\s\S]*?\n\+\+\+\n/, "");
    const title = /^# (.+)$/m.exec(markdown)?.[1] ?? source;
    const md = new MarkdownIt({ html: true });
    const headingIds = new Map();
    md.renderer.rules.heading_open = (tokens, index, options, _env, renderer) => {
      const label = tokens[index + 1].content.replace(/\[([^\]]+)\]\([^)]+\)/g, "$1");
      const base = label
        .toLowerCase()
        .replace(/[^\p{L}\p{N} _-]/gu, "")
        .replaceAll(" ", "-");
      const count = headingIds.get(base) ?? 0;
      headingIds.set(base, count + 1);
      tokens[index].attrSet("id", count ? `${base}-${count}` : base);
      return renderer.renderToken(tokens, index, options);
    };
    // Shared ALX presentation places white H1 text on a dark header.
    let rendered = md.render(markdown).replace(/<h1\b[^>]*>[\s\S]*?<\/h1>/g, "<header>$&</header>");
    rendered = rendered.replace(/\b(href|src)="([^"]+)"/g, (match, attribute, target) => {
      if (/^(?:[a-z]+:|\/|#)/i.test(target)) return match;
      const [file, fragment] = target.split("#");
      const absolute = resolve(dirname(resolve(root, source)), file);
      const mapped = destinations.get(absolute);
      return `${attribute}="${escapeMarkup(relativeLink(mapped ? resolve(output, mapped) : absolute) + (fragment === undefined ? "" : `#${fragment}`))}"`;
    });
    pages.push({
      path: destination,
      title: `${title} — Geometer`,
      depth: destination.split("/").length - 1,
      bodyAttributes: 'data-page-kind="guide" data-authority="authored-markdown"',
      content: `<nav class="nav"><a href="${escapeMarkup(relativeLink(resolve(output, "guides.html")))}">Documentation index</a></nav><aside class="callout">Generated presentation of authored documentation. Edit Markdown source <code>${escapeMarkup(source)}</code>, not this HTML. Contract structure remains governed separately by TypeSpec and its promotion manifest.</aside>${rendered}`,
    });
  }
  pages.push({
    path: "guides.html",
    title: "Geometer Documentation",
    depth: 0,
    bodyAttributes: 'data-page-kind="guide-index"',
    content: `<header><p class="page-type">Generated documentation navigation</p><h1>Geometer Documentation</h1></header><nav class="nav"><a href="index.html">Generated contracts</a><a href="coverage.html">Operation coverage</a></nav><p>Authored interface, contract, developer, decision and requirement pages share one local stylesheet. Historical research remains in its source directory.</p><ul>${pages.map((page) => `<li><a href="${escapeMarkup(page.path)}">${escapeMarkup(page.title)}</a></li>`).join("\n")}</ul>`,
  });
  return pages;
}
