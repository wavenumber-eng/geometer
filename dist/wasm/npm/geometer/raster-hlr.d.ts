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
/**
 * Raster-only hidden-line scene. Faces populate the ordinary GPU
 * depth buffer; candidate edges render afterwards with depth testing enabled.
 * The polygon offset is an explicit, configurable self-occlusion bias.
 */
export declare class RasterHlrModel {
    readonly root: THREE.Object3D;
    readonly stats: RasterHlrBuildStats;
    private readonly surfaceMaterial;
    private readonly lineMaterial;
    private readonly edgeGeometries;
    constructor(source: THREE.Object3D, requested?: Partial<RasterHlrStyle>);
    setStyle(requested: Partial<RasterHlrStyle>): void;
    dispose(): void;
}
/** Owns a WebGL framebuffer renderer; output is pixels, never vector linework. */
export declare class RasterHlrViewport {
    readonly renderer: THREE.WebGLRenderer;
    private readonly scene;
    private model;
    private style;
    private lastFrameAt;
    private meanCpuMs;
    private meanGpuMs;
    private meanFrameMs;
    private samples;
    private gpuSamples;
    private readonly gl;
    private readonly timerQuery;
    private readonly pendingTimerQueries;
    constructor(canvas: HTMLCanvasElement);
    setSource(source: THREE.Object3D | null): RasterHlrBuildStats | null;
    setStyle(requested: Partial<RasterHlrStyle>): void;
    setSize(width: number, height: number): void;
    render(camera: THREE.Camera): RasterHlrFrameStats | null;
    dispose(): void;
    private beginTimerQuery;
    private endTimerQuery;
    private pollTimerQueries;
}
