import type { MeshIllustrationInput, MeshIllustrationScene } from "./mesh-illustration.js";
export interface ExperimentalAmbientOcclusionOptions {
    samples: number;
    radiusFraction: number;
    maxTriangles?: number;
    signal?: AbortSignal;
}
export interface ExperimentalAmbientOcclusionStats {
    triangles: number;
    samples: number;
    radius: number;
    milliseconds: number;
    minimumAccessibility: number;
    meanAccessibility: number;
}
export interface ExperimentalAmbientOcclusionResult {
    readonly accessibilityByMesh: ReadonlyMap<string, Float32Array>;
    readonly stats: ExperimentalAmbientOcclusionStats;
}
export declare function prepareExperimentalAmbientOcclusion(input: MeshIllustrationInput, options: ExperimentalAmbientOcclusionOptions): Promise<ExperimentalAmbientOcclusionResult>;
export declare function applyExperimentalAmbientOcclusion(scene: MeshIllustrationScene, result: ExperimentalAmbientOcclusionResult): void;
