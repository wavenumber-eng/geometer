# Demo editor tooling

This directory contains strict TypeScript, runtime-dependency-free interaction
building blocks for Geometer browser examples. The modules are generic: they do
not contain PCB, schematic, CAD-product, or solver policy.

The implementation is a clean-room behavioral design written for Geometer. Viz,
the floorplan prototype, and the early PNS spike informed desired behaviors only;
no source, comments, assets, names, or implementation structure were copied.
The modules have no runtime dependency on those applications and do not reuse
their global namespaces, assets, CSS names, state managers, or product policy.

`PanelManager` provides dependency-free activity rails and resizable left,
right, and bottom docks for browser demos. Panels implement a small typed
`mount` lifecycle and can be hidden, collapsed, or open. The companion
`panels.css` owns the generic presentation; applications provide their own panel
contents and may override its `--gdm-panel-*` variables.

`CommandHistory` snapshots state with `structuredClone` at every public and
command boundary, rejects reducers that return their input object, and captures
before/after state snapshots instead of replaying command closures during
undo/redo. Callers with non-cloneable state can provide an equivalent cloning
function explicitly.
