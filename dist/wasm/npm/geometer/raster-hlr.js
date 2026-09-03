import * as THREE from "three";
const DEFAULT_STYLE = {
    surfaceColor: 0xf4f3ed,
    lineColor: 0x17252c,
    background: 0xffffff,
    creaseAngleDegrees: 25,
    depthBiasFactor: 1,
    depthBiasUnits: 1,
    doubleSided: false,
};
function triangleCount(geometry) {
    const indices = geometry.getIndex();
    const positions = geometry.getAttribute("position");
    return Math.floor((indices?.count ?? positions?.count ?? 0) / 3);
}
/**
 * Raster-only hidden-line scene. Faces populate the ordinary GPU
 * depth buffer; candidate edges render afterwards with depth testing enabled.
 * The polygon offset is an explicit, configurable self-occlusion bias.
 */
export class RasterHlrModel {
    root;
    stats;
    surfaceMaterial;
    lineMaterial;
    edgeGeometries = [];
    constructor(source, requested = {}) {
        const started = performance.now();
        const style = { ...DEFAULT_STYLE, ...requested };
        this.surfaceMaterial = new THREE.MeshBasicMaterial({
            color: style.surfaceColor,
            side: style.doubleSided ? THREE.DoubleSide : THREE.FrontSide,
            polygonOffset: true,
            polygonOffsetFactor: style.depthBiasFactor,
            polygonOffsetUnits: style.depthBiasUnits,
        });
        this.lineMaterial = new THREE.LineBasicMaterial({
            color: style.lineColor,
            depthTest: true,
            depthWrite: false,
            transparent: false,
        });
        // Clone only the scene graph. BufferGeometry remains shared with the GLB
        // preview, while every generated EdgesGeometry is owned by this model.
        this.root = source.clone(true);
        const meshes = [];
        this.root.traverse((node) => {
            if (node instanceof THREE.Mesh && node.visible)
                meshes.push(node);
        });
        let triangles = 0;
        let candidateEdges = 0;
        for (const mesh of meshes) {
            const geometry = mesh.geometry;
            if (!geometry.getAttribute("position"))
                continue;
            triangles += triangleCount(geometry);
            mesh.material = this.surfaceMaterial;
            mesh.renderOrder = 0;
            // Three's edge builder hashes positions, so it bridges the face-local
            // duplicate indices in Geometer's current GLB within each mesh. This is
            // deliberately renderer policy, not a vector topology contract.
            const edgeGeometry = new THREE.EdgesGeometry(geometry, style.creaseAngleDegrees);
            this.edgeGeometries.push(edgeGeometry);
            const edgePositions = edgeGeometry.getAttribute("position");
            candidateEdges += Math.floor((edgePositions?.count ?? 0) / 2);
            if ((edgePositions?.count ?? 0) === 0)
                continue;
            const edges = new THREE.LineSegments(edgeGeometry, this.lineMaterial);
            edges.name = "geometer-fast-hlr-edges";
            edges.renderOrder = 1;
            edges.frustumCulled = mesh.frustumCulled;
            mesh.add(edges);
        }
        this.stats = {
            buildMs: performance.now() - started,
            meshes: meshes.length,
            triangles,
            candidateEdges,
        };
    }
    setStyle(requested) {
        if (requested.surfaceColor !== undefined)
            this.surfaceMaterial.color.set(requested.surfaceColor);
        if (requested.lineColor !== undefined)
            this.lineMaterial.color.set(requested.lineColor);
        if (requested.depthBiasFactor !== undefined)
            this.surfaceMaterial.polygonOffsetFactor = requested.depthBiasFactor;
        if (requested.depthBiasUnits !== undefined)
            this.surfaceMaterial.polygonOffsetUnits = requested.depthBiasUnits;
        if (requested.doubleSided !== undefined)
            this.surfaceMaterial.side = requested.doubleSided ? THREE.DoubleSide : THREE.FrontSide;
        this.surfaceMaterial.needsUpdate = true;
        this.lineMaterial.needsUpdate = true;
    }
    dispose() {
        for (const geometry of this.edgeGeometries)
            geometry.dispose();
        this.edgeGeometries.length = 0;
        this.surfaceMaterial.dispose();
        this.lineMaterial.dispose();
        this.root.removeFromParent();
    }
}
/** Owns a WebGL framebuffer renderer; output is pixels, never vector linework. */
export class RasterHlrViewport {
    renderer;
    scene = new THREE.Scene();
    model = null;
    style = { ...DEFAULT_STYLE };
    lastFrameAt = 0;
    meanCpuMs = 0;
    meanGpuMs = 0;
    meanFrameMs = 0;
    samples = 0;
    gpuSamples = 0;
    gl;
    timerQuery;
    pendingTimerQueries = [];
    constructor(canvas) {
        this.renderer = new THREE.WebGLRenderer({
            canvas,
            antialias: true,
            preserveDrawingBuffer: true,
        });
        this.renderer.outputColorSpace = THREE.SRGBColorSpace;
        this.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
        this.scene.background = new THREE.Color(this.style.background);
        const context = this.renderer.getContext();
        this.gl = context instanceof WebGL2RenderingContext ? context : null;
        this.timerQuery = this.gl
            ? this.gl.getExtension("EXT_disjoint_timer_query_webgl2")
            : null;
    }
    setSource(source) {
        this.model?.dispose();
        this.model = null;
        this.samples = 0;
        this.meanCpuMs = 0;
        this.meanGpuMs = 0;
        this.meanFrameMs = 0;
        this.gpuSamples = 0;
        this.lastFrameAt = 0;
        if (!source)
            return null;
        this.model = new RasterHlrModel(source, this.style);
        this.scene.add(this.model.root);
        return this.model.stats;
    }
    setStyle(requested) {
        this.style = { ...this.style, ...requested };
        this.model?.setStyle(requested);
        if (requested.background !== undefined)
            this.scene.background = new THREE.Color(requested.background);
    }
    setSize(width, height) {
        this.renderer.setSize(Math.max(1, Math.round(width)), Math.max(1, Math.round(height)), false);
    }
    render(camera) {
        if (!this.model)
            return null;
        this.pollTimerQueries();
        const now = performance.now();
        camera.updateMatrixWorld();
        const query = this.beginTimerQuery();
        const started = performance.now();
        this.renderer.render(this.scene, camera);
        const cpuMs = performance.now() - started;
        this.endTimerQuery(query);
        const frameMs = this.lastFrameAt > 0 ? now - this.lastFrameAt : 0;
        this.lastFrameAt = now;
        const weight = this.samples < 60 ? 1 / (this.samples + 1) : 1 / 60;
        this.meanCpuMs += (cpuMs - this.meanCpuMs) * weight;
        if (frameMs > 0)
            this.meanFrameMs += (frameMs - this.meanFrameMs) * weight;
        this.samples += 1;
        return {
            ...this.model.stats,
            cpuMs: this.meanCpuMs,
            gpuMs: this.gpuSamples > 0 ? this.meanGpuMs : null,
            frameMs: this.meanFrameMs,
            fps: this.meanFrameMs > 0 ? 1000 / this.meanFrameMs : 0,
            drawCalls: this.renderer.info.render.calls,
        };
    }
    dispose() {
        this.model?.dispose();
        this.model = null;
        if (this.gl)
            for (const query of this.pendingTimerQueries)
                this.gl.deleteQuery(query);
        this.pendingTimerQueries.length = 0;
        this.renderer.dispose();
    }
    beginTimerQuery() {
        if (!this.gl || !this.timerQuery || this.pendingTimerQueries.length >= 4)
            return null;
        const query = this.gl.createQuery();
        if (!query)
            return null;
        this.gl.beginQuery(this.timerQuery.TIME_ELAPSED_EXT, query);
        return query;
    }
    endTimerQuery(query) {
        if (!query || !this.gl || !this.timerQuery)
            return;
        this.gl.endQuery(this.timerQuery.TIME_ELAPSED_EXT);
        this.pendingTimerQueries.push(query);
    }
    pollTimerQueries() {
        if (!this.gl || !this.timerQuery)
            return;
        while (this.pendingTimerQueries.length > 0) {
            const query = this.pendingTimerQueries[0];
            if (!query || !this.gl.getQueryParameter(query, this.gl.QUERY_RESULT_AVAILABLE))
                return;
            this.pendingTimerQueries.shift();
            const disjoint = Boolean(this.gl.getParameter(this.timerQuery.GPU_DISJOINT_EXT));
            if (!disjoint) {
                const nanoseconds = Number(this.gl.getQueryParameter(query, this.gl.QUERY_RESULT));
                const gpuMs = nanoseconds / 1_000_000;
                const weight = this.gpuSamples < 60 ? 1 / (this.gpuSamples + 1) : 1 / 60;
                this.meanGpuMs += (gpuMs - this.meanGpuMs) * weight;
                this.gpuSamples += 1;
            }
            this.gl.deleteQuery(query);
        }
    }
}
