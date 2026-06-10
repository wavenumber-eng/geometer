# Design intent for the STEP editor prototype

The GUI should have the `wn3d_logo.png` on the upper left.

This is a GUI program that opens any STEP file AP214, AP203, AP242 and then when the user is done doing actions on the STEP file, an AP242 file is generated next to the one loaded in with a new name:

`wheel4506.step` -> `wheel4506_AP242_conditioned.step`

Definition of tools, a tool in this GUI is a MODE that the user is indicated of by a rectangle in the upper middle of the screen. The tool can be changed by clicking on this rectangle or pressing T. A tool has specific actions listed on the right side of the screen in a split window. Users can select tool actions that might be more nuanced than the tool's default use.

Tools avaliable in the editor:

`Redefine Z-Sit plane`, USERS click 3 points to define a Z plane and then a 4th point to define the direction of the Z axis. This will reposition the whole model (even if it is an assembly) so that the coordinate system matches the users intent. The three points define a planar rectangle at which the XYZ origin is placed. The 4th point defines the direction of the Z vector. Essentially this will turn any model Z up.

`Redefine Pin 1 Quadrant`, Users will click Pin 1 on the chip and this will swing around the whole model to the X Positive Y Positive directions.

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

