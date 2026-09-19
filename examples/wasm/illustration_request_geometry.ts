import type { MeshIllustrationView } from "@wavenumber/geometer/mesh-illustration";
import * as THREE from "three";
import {
  type ClipMode,
  type ClipSide,
  type IllustrationClipping,
  normalizeTuple,
} from "./illustration_demo_support.js";

interface ClippingElements {
  clipMode: HTMLSelectElement;
  clipSide: HTMLSelectElement;
  clipPosition: HTMLInputElement;
  clipPositionValue: HTMLOutputElement;
  ambientOcclusion: HTMLInputElement;
}

export function currentClipping(
  root: THREE.Object3D,
  view: MeshIllustrationView,
  elements: ClippingElements,
): IllustrationClipping | undefined {
  const mode = elements.clipMode.value as ClipMode;
  const active = mode !== "off";
  elements.clipSide.disabled = !active;
  elements.clipPosition.disabled = !active;
  elements.ambientOcclusion.disabled = active;
  if (!active) {
    elements.clipPositionValue.value = "Off";
    return undefined;
  }
  const basis: readonly [number, number, number] =
    mode === "x"
      ? [1, 0, 0]
      : mode === "y"
        ? [0, 1, 0]
        : mode === "z"
          ? [0, 0, 1]
          : normalizeTuple(view.direction);
  const sign = (elements.clipSide.value as ClipSide) === "positive" ? 1 : -1;
  const normal: [number, number, number] = [basis[0] * sign, basis[1] * sign, basis[2] * sign];
  root.updateMatrixWorld(true);
  const bounds = new THREE.Box3().setFromObject(root);
  const distances: number[] = [];
  for (const x of [bounds.min.x, bounds.max.x])
    for (const y of [bounds.min.y, bounds.max.y])
      for (const z of [bounds.min.z, bounds.max.z])
        distances.push((normal[0] * x + normal[1] * y + normal[2] * z) * 1000);
  const minimum = Math.min(...distances);
  const maximum = Math.max(...distances);
  const fraction = Number.parseFloat(elements.clipPosition.value) / 100;
  const distanceMm = minimum + (maximum - minimum) * fraction;
  elements.clipPositionValue.value = `${distanceMm.toFixed(2)} mm`;
  return {
    planes: [{ normal, distance_mm: distanceMm, tolerance_mm: 1e-6 }],
    cap_policy: "none",
  };
}

export function wireClippingControls(
  elements: ClippingElements,
  render: () => void,
  cancelAmbientOcclusion: () => void,
): void {
  let timer = 0;
  elements.clipMode.addEventListener("change", () => {
    if (elements.clipMode.value !== "off" && elements.ambientOcclusion.checked) {
      elements.ambientOcclusion.checked = false;
      cancelAmbientOcclusion();
    }
    render();
  });
  elements.clipSide.addEventListener("change", render);
  elements.clipPosition.addEventListener("input", () => {
    window.clearTimeout(timer);
    timer = window.setTimeout(render, 120);
  });
}

export function currentModelTransform(root: THREE.Object3D): number[] {
  root.updateMatrixWorld(true);
  const value = root.matrixWorld.elements;
  const at = (index: number): number => value[index] ?? (index % 5 === 0 ? 1 : 0);
  // Three stores column-major matrices; Geometer's HLR option is row-major.
  return [
    at(0),
    at(4),
    at(8),
    at(12) * 1000,
    at(1),
    at(5),
    at(9),
    at(13) * 1000,
    at(2),
    at(6),
    at(10),
    at(14) * 1000,
    at(3),
    at(7),
    at(11),
    at(15),
  ];
}

export function currentModelIllustrationTransform(root: THREE.Object3D): number[] {
  root.updateMatrixWorld(true);
  const value = root.matrixWorld.elements.slice();
  value[12] = (value[12] ?? 0) * 1000;
  value[13] = (value[13] ?? 0) * 1000;
  value[14] = (value[14] ?? 0) * 1000;
  return value;
}
