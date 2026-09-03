import * as THREE from "three";

/** Browser/WebGL-only raster HLR presentation controls. */
export interface RasterHlrStyle {
  surfaceColor: THREE.ColorRepresentation;
  lineColor: THREE.ColorRepresentation;
  background: THREE.ColorRepresentation;
  creaseAngleDegrees: number;
  depthBiasFactor: number;
  depthBiasUnits: number;
  doubleSided: boolean;
}

export interface RasterHlrBuildStats {
  buildMs: number;
  meshes: number;
  triangles: number;
  candidateEdges: number;
}

export interface RasterHlrFrameStats extends RasterHlrBuildStats {
  cpuMs: number;
  gpuMs: number | null;
  frameMs: number;
  fps: number;
  drawCalls: number;
}

interface DisjointTimerQueryWebGl2 {
  readonly TIME_ELAPSED_EXT: number;
  readonly GPU_DISJOINT_EXT: number;
}

const DEFAULT_STYLE: RasterHlrStyle = {
  surfaceColor: 0xf4f3ed,
  lineColor: 0x17252c,
  background: 0xffffff,
  creaseAngleDegrees: 25,
  depthBiasFactor: 1,
  depthBiasUnits: 1,
  doubleSided: false,
};

function triangleCount(geometry: THREE.BufferGeometry): number {
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
  readonly root: THREE.Object3D;
  readonly stats: RasterHlrBuildStats;

  private readonly surfaceMaterial: THREE.MeshBasicMaterial;
  private readonly lineMaterial: THREE.LineBasicMaterial;
  private readonly edgeGeometries: THREE.EdgesGeometry[] = [];

  constructor(source: THREE.Object3D, requested: Partial<RasterHlrStyle> = {}) {
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
    const meshes: THREE.Mesh[] = [];
    this.root.traverse((node) => {
      if (node instanceof THREE.Mesh && node.visible) meshes.push(node);
    });

    let triangles = 0;
    let candidateEdges = 0;
    for (const mesh of meshes) {
      const geometry = mesh.geometry as THREE.BufferGeometry;
      if (!geometry.getAttribute("position")) continue;
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
      if ((edgePositions?.count ?? 0) === 0) continue;
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

  setStyle(requested: Partial<RasterHlrStyle>): void {
    if (requested.surfaceColor !== undefined)
      this.surfaceMaterial.color.set(requested.surfaceColor);
    if (requested.lineColor !== undefined) this.lineMaterial.color.set(requested.lineColor);
    if (requested.depthBiasFactor !== undefined)
      this.surfaceMaterial.polygonOffsetFactor = requested.depthBiasFactor;
    if (requested.depthBiasUnits !== undefined)
      this.surfaceMaterial.polygonOffsetUnits = requested.depthBiasUnits;
    if (requested.doubleSided !== undefined)
      this.surfaceMaterial.side = requested.doubleSided ? THREE.DoubleSide : THREE.FrontSide;
    this.surfaceMaterial.needsUpdate = true;
    this.lineMaterial.needsUpdate = true;
  }

  dispose(): void {
    for (const geometry of this.edgeGeometries) geometry.dispose();
    this.edgeGeometries.length = 0;
    this.surfaceMaterial.dispose();
    this.lineMaterial.dispose();
    this.root.removeFromParent();
  }
}

/** Owns a WebGL framebuffer renderer; output is pixels, never vector linework. */
export class RasterHlrViewport {
  readonly renderer: THREE.WebGLRenderer;

  private readonly scene = new THREE.Scene();
  private model: RasterHlrModel | null = null;
  private style: RasterHlrStyle = { ...DEFAULT_STYLE };
  private lastFrameAt = 0;
  private meanCpuMs = 0;
  private meanGpuMs = 0;
  private meanFrameMs = 0;
  private samples = 0;
  private gpuSamples = 0;
  private readonly gl: WebGL2RenderingContext | null;
  private readonly timerQuery: DisjointTimerQueryWebGl2 | null;
  private readonly pendingTimerQueries: WebGLQuery[] = [];

  constructor(canvas: HTMLCanvasElement) {
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
      ? (this.gl.getExtension("EXT_disjoint_timer_query_webgl2") as DisjointTimerQueryWebGl2 | null)
      : null;
  }

  setSource(source: THREE.Object3D | null): RasterHlrBuildStats | null {
    this.model?.dispose();
    this.model = null;
    this.samples = 0;
    this.meanCpuMs = 0;
    this.meanGpuMs = 0;
    this.meanFrameMs = 0;
    this.gpuSamples = 0;
    this.lastFrameAt = 0;
    if (!source) return null;
    this.model = new RasterHlrModel(source, this.style);
    this.scene.add(this.model.root);
    return this.model.stats;
  }

  setStyle(requested: Partial<RasterHlrStyle>): void {
    this.style = { ...this.style, ...requested };
    this.model?.setStyle(requested);
    if (requested.background !== undefined)
      this.scene.background = new THREE.Color(requested.background);
  }

  setSize(width: number, height: number): void {
    this.renderer.setSize(Math.max(1, Math.round(width)), Math.max(1, Math.round(height)), false);
  }

  render(camera: THREE.Camera): RasterHlrFrameStats | null {
    if (!this.model) return null;
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
    if (frameMs > 0) this.meanFrameMs += (frameMs - this.meanFrameMs) * weight;
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

  dispose(): void {
    this.model?.dispose();
    this.model = null;
    if (this.gl) for (const query of this.pendingTimerQueries) this.gl.deleteQuery(query);
    this.pendingTimerQueries.length = 0;
    this.renderer.dispose();
  }

  private beginTimerQuery(): WebGLQuery | null {
    if (!this.gl || !this.timerQuery || this.pendingTimerQueries.length >= 4) return null;
    const query = this.gl.createQuery();
    if (!query) return null;
    this.gl.beginQuery(this.timerQuery.TIME_ELAPSED_EXT, query);
    return query;
  }

  private endTimerQuery(query: WebGLQuery | null): void {
    if (!query || !this.gl || !this.timerQuery) return;
    this.gl.endQuery(this.timerQuery.TIME_ELAPSED_EXT);
    this.pendingTimerQueries.push(query);
  }

  private pollTimerQueries(): void {
    if (!this.gl || !this.timerQuery) return;
    while (this.pendingTimerQueries.length > 0) {
      const query = this.pendingTimerQueries[0];
      if (!query || !this.gl.getQueryParameter(query, this.gl.QUERY_RESULT_AVAILABLE)) return;
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
