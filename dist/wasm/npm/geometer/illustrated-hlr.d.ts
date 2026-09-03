import type { HlrProjectionOptionsA0, HlrProjectionResultA0, MeshIllustrationInputA0, MeshIllustrationResultA0 } from "./generated/contracts.js";
import type { IndexedTriangleMeshA0 } from "./indexed-mesh-packet-a0.js";
import { type MeshIllustratorA0 } from "./mesh-illustration.js";
export interface MeshHlrProjector {
    meshHlrProjection(request: {
        readonly mesh: IndexedTriangleMeshA0 | Uint8Array;
        readonly options?: HlrProjectionOptionsA0;
    }): Promise<HlrProjectionResultA0>;
}
export interface FastHlrIllustrationRequest {
    /** Illustration positions and transforms are interpreted as millimeters. */
    readonly illustration: MeshIllustrationInputA0;
    readonly hlr?: HlrProjectionOptionsA0;
}
export interface PreparedFastHlrIllustration {
    readonly hlr: HlrProjectionResultA0;
    readonly illustrator: MeshIllustratorA0;
}
export interface FastHlrIllustrationResult {
    readonly hlr: HlrProjectionResultA0;
    readonly illustration: MeshIllustrationResultA0;
}
/** Prepare Fast vector linework and the colorized scene once for repeated rendering. */
export declare function createFastHlrIllustrator(projector: MeshHlrProjector, request: FastHlrIllustrationRequest): Promise<PreparedFastHlrIllustration>;
/** Project Fast vector linework, colorize the mesh, and return one SVG result. */
export declare function illustrateMeshWithFastHlr(projector: MeshHlrProjector, request: FastHlrIllustrationRequest): Promise<FastHlrIllustrationResult>;
/** Flatten governed illustration meshes and column-major transforms into one HLR mesh. */
export declare function indexedMeshFromIllustrationInput(input: MeshIllustrationInputA0): IndexedTriangleMeshA0;
