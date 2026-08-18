import type { EmscriptenGeometerFactory } from "./wasm.js";
import type { GeometerWorkerResponseMessage } from "./worker.js";
export interface GeometerWorkerScope {
    addEventListener(type: "message", listener: (event: MessageEvent<unknown>) => void): void;
    close?(): void;
    postMessage(message: GeometerWorkerResponseMessage, transfer?: readonly Transferable[]): void;
}
export interface GeometerWorkerHostOptions {
    /** Worker-local Emscripten options merged before structured-cloned initialization options. */
    readonly moduleOptions?: Readonly<Record<string, unknown>>;
}
/** Installs the generic Geometer A0 protocol into a dedicated Worker scope. */
export declare function startGeometerWorkerHost(factory: EmscriptenGeometerFactory, scope: GeometerWorkerScope, options?: GeometerWorkerHostOptions): void;
