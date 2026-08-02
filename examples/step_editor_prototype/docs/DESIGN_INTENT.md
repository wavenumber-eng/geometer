# Design intent for the STEP editor prototype

The GUI should have the `wn3d_logo.png` on the upper left.

This is a GUI program that opens any STEP file AP214, AP203, AP242 and then when the user is done doing actions on the STEP file, an AP242 file is generated next to the one loaded in with a new name:

`wheel4506.step` -> `wheel4506_AP242_conditioned.step`

Definition of tools, a tool in this GUI is a MODE that the user is indicated of by a rectangle in the upper middle of the screen. The tool can be changed by clicking on this rectangle or pressing T. A tool has specific actions listed on the right side of the screen in a split window. Users can select tool actions that might be more nuanced than the tool's default use.

Tools avaliable in the editor:

`Redefine Z-Sit plane`, USERS click 3 points to define a Z plane and then a 4th point to define the direction of the Z axis. This will reposition the whole model (even if it is an assembly) so that the coordinate system matches the users intent. The three points define a planar rectangle at which the XYZ origin is placed. The 4th point defines the direction of the Z vector. Essentially this will turn any model Z up.

`Redefine Front` (replaces `Redefine Pin 1 Quadrant`), the Step-2 in-plane rotation tool. Z-Sit (Step 1) already stood the part Z-up, centred, and seated on Z=0; this tool fixes the remaining freedom — the rotation about +Z — so the part faces a canonical "front". The user supplies two constraints: (1) click a POINT that must end up in the −Y half-plane (the FRONT of the part), and (2) define a LINE that must become parallel to the X axis. With those two, the model rotates about its centre so the line lies along X and the front point drops into −Y (the line fixes the angle to within 180°; the −Y point picks which of the two flips). This deliberately decouples rotation from Z-Sit so the seat-orientation model never has to also get rotation right.

Canonical front conventions (the ground truth the user defines with this tool, later learned by a per-package-type model):
 - SISO / single-row packages: pins run L→R from −X to +X (pin 1 at −X, ascending toward +X).
 - SOT packages: pins read L→R, with the pin rows on the −X and +X sides.
 - QFN packages: the pin-1 indicator sits in the +X +Y quadrant (upper-right).
 - Connectors, board-PARALLEL mating (the connector mates horizontally, parallel to the board — e.g. a card-edge or right-angle header): the CONNECTOR/mating pins are the front face (they point toward −Y).
 - Connectors, board-PERPENDICULAR mating (the connector mates vertically, perpendicular to the board — e.g. a vertical socket): the SMT pins define the front face.

`Detect Pins`, is where a USER drags a rectangle in orthographic mode over what they think are the pins of the chip. Then a system does edge flow until there is a discontinuity and then labels that enclosed shape as a PIN. Pin ordering is done via those pins geometric centers (based on their 3D geometry) relative to the origin. For instance pins 1 - 5 may range from X = -5 X = 5 and Y = -3, pins 6 - 10 may range from X = 5 X = -5 and Y = 3. Pins can be reordered in this tool mode. Caveats to be aware of, some 3D models already have Pins as separate bodies, if this is the case, then detecting pins will be super easy, if the model is a unibody, the system must be smart enough to detect pin shapes and distinguish them from the bodies of the chips. If Pin 1 Quadrant has been defined, then the user can easily define where pin 2 is and then the system should be able to propogate these numbers.

`Assign Pin Hitboxes`, is a mode where pins on the chip can be bounded by a user clicking on 3 points to place a rectangular prism around each pin. This is stored as metadata and not an actual 3D object. It defines where that pin intersects with geometry. Any program that wants to know if that PIN is physically connected to something can check if there is intersections with this Pin Hitbox. Male pins can have a box around them, female pins can have a box inside the region of which a male pin may sit. BGA pins would have a small cube. Large ground pads can have a flat rectangular prisim. Curved pins can be bounded by a convex hull. These Hitboxes for SMT components would essentially be the IPC standard footprint for that 3D object if cross sectioned on the XY plane.

`Assign Pin Functions/Names`, shows a mini schematic of the chip on the right in the actions splitscreen. Each pin can have different labels on them for instance PWM, GPIO, GND, PWR, AGND, CLK, SDA, RX, TX, etc. Since each pin is physically defined on the model, the pin functions are geometric and not just informational.

`Redefine Colors`, Possibly once the chip has been dissected, maybe some new colors would be of interest to the user that has not been defined in the original 3D model. This mode allows selecting individual bodies and assigning a color to them.

`Apply LOGO`, There is a `wn3d_logo.png` file inside the `step_editor_prototype` folder and a corresponding DXF that the user can apply to their model as a watermark. It just embosses the DXF onto any surface after the user has picked a surface, resized the DXF in 2D orthographic view, and then chose an emboss depth.


What does this type of conditioning allow:

Multiboard systems that track NETS across actual 3D space between board models.
Simulation of chips using real geometric pin outputs across multiboard stackups.
Full definition of a FOOTPRINT as well as SCHEMATIC within the 3D file metadata.
Makes DUPONT connectors on PCBs have actual utility instead of for 3D crosschecking.
Complex board connections could now be done using the hitboxes.
Makes all 3D models useful and coherent.


Additive Design Intent

 - The project should depend on Altium monkey and Kicad monkey systems to allow it to open standard KICAD and ALTIUM files which have embedded STEP files within them.

 The flow:
 - Open an Altium or Kicad file and extract the STEP data within.
 - User applies the process we have honed in here to add metadata and fix up the STEP file.
 - STEP AP242 is generated with metadata within.
 - Bake the STEP file into the Altium/Kicad file and open it in the respective program.
 - Ensure no load problems with this new baked in format.
 - Re-extract the baked STEP file into AP242 and see if the metadata has survived.

 What has to work (kicad and altium must both work):
 - [1] kicad_mod -> [2] STEP AP242 -> [3] kicad_mod -> [4] STEP AP242 -> [5] kicad_mod
 - system loads in [1] and user generates [2] with the editor
 - [3] must work in kicad
 - [2] and [4] must be completely identical line for line.
 - [3] and [5] must be completely identical line for line.
 - [4] must contain metadata
