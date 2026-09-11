# GT-USB-7010C partial-meshing regression

Extracted with Altium Cruncher's existing embedded-model catalog from the
user-supplied TDM Monkey PCB, component J2. The user authorized using this model
as a Geometer test fixture. It contains connector geometry only, not the PCB.
Line endings are normalized; geometry and STEP entities are unchanged.

The STEP header identifies SolidWorks 2026 / AP214. With OCCT 8.0.1 and 0.01 mm
linear / 0.5 rad angular deflection, the model imports and passes BRep validity
checks. A microscopic cylindrical sliver fails during face discretization:
internal face 16 has area approximately 2.18466782315e-08 mm² and no triangles.
The native mesher reports done=true and IMeshData_Failure (4). The other 973
faces produce 8,579 triangles on the Windows reference build.

Tests assert usable partial output with warnings, explicit strict rejection,
limit enforcement and process recovery. Counts are bounded rather than exact
across platforms; face indices are not persistent model identifiers.
