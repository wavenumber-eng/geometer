import type { MeshIllustrationInputA0, MeshIllustrationRenderStats as MeshIllustrationRenderStatsA0, MeshIllustrationResultA0, MeshIllustrationStyleA0, MeshIllustrationSvgOptions as MeshIllustrationSvgOptionsA0 } from "./generated/contracts.js";
export type Vec2 = readonly [number, number];
export type Vec3 = readonly [number, number, number];
export type Rgb = readonly [number, number, number];
export interface MeshIllustrationMaterial {
    /** Source color expressed as sRGB channel values in the inclusive range [0, 1]. */
    color: Rgb;
    opacity?: number;
    name?: string;
}
export interface MeshIllustrationMesh {
    id: string;
    positions: ArrayLike<number>;
    normals?: ArrayLike<number>;
    indices?: ArrayLike<number>;
    matrix?: ArrayLike<number>;
    materials: readonly MeshIllustrationMaterial[];
    triangleMaterialIndices?: ArrayLike<number>;
    doubleSided?: boolean;
}
export interface MeshIllustrationInput {
    meshes: readonly MeshIllustrationMesh[];
}
export interface MeshIllustrationView {
    direction: Vec3;
    up: Vec3;
    mirrorX?: boolean;
}
export interface MeshIllustrationPrepareOptions {
    maxTriangles?: number;
    weldTolerance?: number;
}
export interface MeshIllustrationSvgOptions {
    /** Integer units across the larger unpadded artwork axis; default 1,000,000. */
    coordinateSpan?: number;
}
export type MeshIllustrationShading = "unlit" | "flat" | "lambert" | "banded" | "toon";
export interface MeshIllustrationStyle {
    shading: MeshIllustrationShading;
    ambient: number;
    keyIntensity: number;
    lightDirection: Vec3;
    bands: number;
    sourceColors: boolean;
    fallbackColor: Rgb;
    background: string;
    transparentBackground: boolean;
    /** Merge safe, adjacent, identically styled projected triangles into filled paths. */
    fuseSurfaces: boolean;
    /** Underpaint safe opaque coplanar material partitions before drawing their markings. */
    layerCoplanarMaterials: boolean;
    showHlrOutline: boolean;
    showHlrDetail: boolean;
    showOutlines: boolean;
    showCreases: boolean;
    creaseAngleDegrees: number;
    outlineColor: string;
    creaseColor: string;
    outlineWidth: number;
    creaseWidth: number;
    doubleSided: boolean;
    rimAmount: number;
}
export interface IllustrationTriangle {
    points: readonly [Vec2, Vec2, Vec2];
    depths: readonly [number, number, number];
    depth: number;
    normal: Vec3;
    geometricNormal: Vec3;
    baseColor: Rgb;
    opacity: number;
    frontFacing: boolean;
    doubleSided: boolean;
    meshId: string;
    triangleIndex: number;
}
export interface IllustrationEdge {
    points: readonly [Vec2, Vec2];
    depth: number;
    frontA: boolean;
    frontB: boolean | null;
    normalA: Vec3;
    normalB: Vec3 | null;
    meshId: string;
}
export interface IllustrationLineSegment {
    points: readonly [Vec2, Vec2];
}
export interface MeshIllustrationScene {
    view: {
        direction: Vec3;
        up: Vec3;
        right: Vec3;
        mirrorX: boolean;
    };
    bounds: {
        minX: number;
        minY: number;
        maxX: number;
        maxY: number;
    };
    triangles: readonly IllustrationTriangle[];
    edges: readonly IllustrationEdge[];
    /** Optional CAD-derived linework, such as Geometer's HLR mesh-shadow outline. */
    outlineSegments?: readonly IllustrationLineSegment[];
    /** Optional visible-edge detail produced by Geometer HLR. */
    detailSegments?: readonly IllustrationLineSegment[];
    stats: {
        sourceMeshes: number;
        sourceTriangles: number;
        projectedTriangles: number;
        edges: number;
    };
    warnings: readonly string[];
}
export interface IllustrationRenderStats {
    /** Front-facing triangles submitted to painting; some may be fully occluded. */
    triangles: number;
    surfaceDraws: number;
    layeredSurfaces: number;
    outlines: number;
    details: number;
    creases: number;
    commands: number;
}
export declare function prepareMeshIllustration(input: MeshIllustrationInput, view: MeshIllustrationView, options?: MeshIllustrationPrepareOptions): MeshIllustrationScene;
export declare function renderMeshIllustrationSvg(scene: MeshIllustrationScene, style: MeshIllustrationStyle, title?: string, options?: MeshIllustrationSvgOptions): {
    svg: string;
    stats: IllustrationRenderStats;
};
export declare function renderMeshIllustrationCanvas(context: CanvasRenderingContext2D, scene: MeshIllustrationScene, style: MeshIllustrationStyle): IllustrationRenderStats;
/** Resolve a governed presence-preserving style patch to package defaults. */
export declare function resolveMeshIllustrationStyle(style?: MeshIllustrationStyleA0): MeshIllustrationStyle;
export interface MeshIllustratorA0 {
    readonly disposed: boolean;
    renderSvg(style?: MeshIllustrationStyleA0, svg?: MeshIllustrationSvgOptionsA0): MeshIllustrationResultA0;
    renderCanvas(context: CanvasRenderingContext2D, style?: MeshIllustrationStyleA0): MeshIllustrationRenderStatsA0;
    dispose(): void;
}
/** Prepare one governed illustration input once, then render multiple styles or targets. */
export declare function createIllustrator(input: MeshIllustrationInputA0): MeshIllustratorA0;
/** Prepare and render one governed mesh-illustration A0 input to SVG. */
export declare function illustrateMesh(input: MeshIllustrationInputA0): MeshIllustrationResultA0;
