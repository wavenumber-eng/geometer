# Demo editor tooling

This directory contains strict TypeScript, runtime-dependency-free interaction
building blocks for Geometer browser examples. The modules are generic: they do
not contain PCB, schematic, CAD-product, or solver policy.

The implementation is a clean-room behavioral design written for Geometer. Viz,
the floorplan prototype, and the early PNS spike informed desired behaviors only;
no source, comments, assets, names, or implementation structure were copied.
Direct ports remain prohibited until their licensing and provenance are resolved.

`CommandHistory` snapshots state with `structuredClone` at every public and
command boundary, rejects reducers that return their input object, and captures
before/after state snapshots instead of replaying command closures during
undo/redo. Callers with non-cloneable state can provide an equivalent cloning
function explicitly.
