export function renderNativeGeometrySvg(geometry, title) {
    const namespace = "http://www.w3.org/2000/svg";
    const root = document.createElementNS(namespace, "svg");
    root.setAttribute("xmlns", namespace);
    const width = Math.max(geometry.bounds.max[0] - geometry.bounds.min[0], 1e-9);
    const height = Math.max(geometry.bounds.max[1] - geometry.bounds.min[1], 1e-9);
    const pad = geometry.presentation.padding;
    root.setAttribute("viewBox", `${geometry.bounds.min[0] - pad} ${-geometry.bounds.max[1] - pad} ${width + 2 * pad} ${height + 2 * pad}`);
    const titleElement = document.createElementNS(namespace, "title");
    titleElement.textContent = title;
    root.append(titleElement);
    if (!geometry.presentation.transparent_background) {
        const background = document.createElementNS(namespace, "rect");
        background.setAttribute("x", String(geometry.bounds.min[0] - pad));
        background.setAttribute("y", String(-geometry.bounds.max[1] - pad));
        background.setAttribute("width", String(width + 2 * pad));
        background.setAttribute("height", String(height + 2 * pad));
        background.setAttribute("fill", geometry.presentation.background);
        root.append(background);
    }
    for (const surface of geometry.surfaces)
        for (const layer of surface.layers) {
            const path = document.createElementNS(namespace, "path");
            path.setAttribute("d", layer.rings
                .filter((ring) => ring.points.length > 0)
                .map((ring) => `${ring.points
                .map((point, index) => `${index === 0 ? "M" : "L"}${point[0]} ${-point[1]}`)
                .join("")}Z`)
                .join(""));
            path.setAttribute("fill", layer.fill);
            path.setAttribute("fill-rule", geometry.presentation.fill_rule);
            path.setAttribute("stroke", layer.fill);
            path.setAttribute("stroke-width", String(geometry.presentation.seam_width));
            path.setAttribute("stroke-linejoin", geometry.presentation.line_join);
            if (layer.opacity < 0.999)
                path.setAttribute("opacity", String(layer.opacity));
            root.append(path);
        }
    for (const line of geometry.lines) {
        const element = document.createElementNS(namespace, "line");
        element.setAttribute("x1", String(line.start[0]));
        element.setAttribute("y1", String(-line.start[1]));
        element.setAttribute("x2", String(line.end[0]));
        element.setAttribute("y2", String(-line.end[1]));
        element.setAttribute("stroke", line.color);
        element.setAttribute("stroke-width", String(line.width));
        element.setAttribute("stroke-linecap", geometry.presentation.line_cap);
        element.setAttribute("stroke-linejoin", geometry.presentation.line_join);
        root.append(element);
    }
    return `<?xml version="1.0" encoding="UTF-8"?>\n${new XMLSerializer().serializeToString(root)}`;
}
export function renderNativeGeometryCanvas(context, geometry) {
    const canvas = context.canvas;
    context.clearRect(0, 0, canvas.width, canvas.height);
    const width = Math.max(geometry.bounds.max[0] - geometry.bounds.min[0], 1e-9);
    const height = Math.max(geometry.bounds.max[1] - geometry.bounds.min[1], 1e-9);
    const pad = geometry.presentation.padding;
    const scale = Math.min(canvas.width / (width + 2 * pad), canvas.height / (height + 2 * pad));
    const offsetX = (canvas.width - width * scale) / 2 - geometry.bounds.min[0] * scale;
    const offsetY = (canvas.height - height * scale) / 2 + geometry.bounds.max[1] * scale;
    const point = (value) => [
        offsetX + value[0] * scale,
        offsetY - value[1] * scale,
    ];
    if (!geometry.presentation.transparent_background) {
        context.fillStyle = geometry.presentation.background;
        context.fillRect(0, 0, canvas.width, canvas.height);
    }
    context.lineCap = geometry.presentation.line_cap;
    context.lineJoin = geometry.presentation.line_join;
    for (const surface of geometry.surfaces)
        for (const layer of surface.layers) {
            context.beginPath();
            for (const ring of layer.rings) {
                if (ring.points.length === 0)
                    continue;
                context.moveTo(...point(ring.points[0]));
                for (const value of ring.points.slice(1))
                    context.lineTo(...point(value));
                context.closePath();
            }
            context.globalAlpha = layer.opacity;
            context.fillStyle = context.strokeStyle = layer.fill;
            context.fill(geometry.presentation.fill_rule);
            context.lineWidth = Math.max(0.7, geometry.presentation.seam_width * scale);
            context.stroke();
        }
    context.globalAlpha = 1;
    for (const line of geometry.lines) {
        context.beginPath();
        context.moveTo(...point(line.start));
        context.lineTo(...point(line.end));
        context.strokeStyle = line.color;
        context.lineWidth = Math.max(0.6, line.width * scale);
        context.stroke();
    }
    return geometry.stats.commands;
}
