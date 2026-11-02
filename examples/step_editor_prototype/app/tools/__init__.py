"""Tool registry. Order here is the T-key cycling order: exactly the 10
conditioning steps of WN3D_STEP_Editor_Pipeline.md, so T-cycling walks a part
from raw vendor file to finished model. (Inspect lives in the MODEL INFO
panel; the logo emboss is not a conditioning step and stays replay-only.)"""

from __future__ import annotations

from .base import ToolMode
from .colors import ColorsTool
from .detect_pins import DetectPinsTool
from .footprint_embed import FootprintEmbedTool
from .front_face import FrontFaceTool
from .hitboxes import HitboxTool
from .identity import IdentityTool
from .pin_functions import PinFunctionsTool
from .repair import RepairTool
from .separate import SeparateUnibodyTool
from .zsit import ZSitTool

TOOL_CLASSES: list[type[ToolMode]] = [
    RepairTool,           # 1 Model Health + Repair
    IdentityTool,         # 2 Identity + Provenance
    ZSitTool,             # 3 Z-Sit
    DetectPinsTool,       # 4 Detect + Name Pins (pin 1)
    FrontFaceTool,        # 5 Front (IPC orientation)
    SeparateUnibodyTool,  # 6 Separate Unibody
    HitboxTool,           # 7 Pin Hitboxes
    PinFunctionsTool,     # 8 Embedded Schematic (pin functions)
    FootprintEmbedTool,   # 9 Embedded Footprint
    ColorsTool,           # 10 Colors
]

# House conventions, shown on each tool's panel. The automation (and the ML
# models) must follow exactly these; keep them short.
CONVENTIONS: dict[str, str] = {
    "repair": "Goal: closed solids with positive volume. A healthy model passes untouched.",
    "identity": "MPN exactly as the manufacturer writes it. Source = where the file came from.",
    "zsit": "The PCB top surface is Z=0, +Z points up out of the board. "
            "Default: 4-point pick on the seating faces (most stable).",
    "detect_pins": "Numbers follow your click order. Predict fills only unnamed pins. "
                   "ANCHOR = soldered casing/mechanical pin in a plated hole (on a net). "
                   "GUIDE = plastic locating peg in a non-plated hole (no net). "
                   "Hotkeys with a pin selected: A = ANCHOR, G = GUIDE.",
    "front": "Front ends up toward -Y. For 2-in / 2-out parts the OUTPUT side is the front.",
    "separate": "Each electrical pin becomes its own solid. ANCHOR and GUIDE pins stay "
                "with the body. Painting pin faces first gives the cleanest split.",
    "hitboxes": "One hitbox per electrical pin, tip to the Z-Sit plane. "
                "ANCHOR pins get one; GUIDE pins never do.",
    "pin_functions": "Pins sharing a designator are one net. Functions come from the datasheet.",
    "footprint_embed": "Pads come from where contacts meet the Z-Sit plane; ANCHOR/GUIDE holes "
                       "from the outline they cut through it. Tolerances: datasheet first, "
                       "else IPC nominal.",
    "colors": "Keeping the vendor's colors is a valid choice. Eyedropper reuses an "
              "existing face color.",
}

# One-sentence panel headers; the full how-to stays behind "How ▸".
SHORT_HELP: dict[str, str] = {
    "repair": "Check the model; Repair only if problems are listed.",
    "identity": "Record what this part is, then Save Identity.",
    "zsit": "Pick points on the seating faces (or Auto), then Apply.",
    "detect_pins": "Seed a tip + Detect Exact, or Context Plane / Auto, then Apply.",
    "front": "Click the front, then a line that should run along X; Apply.",
    "separate": "Preview growth or Split by color, check it, then Apply.",
    "hitboxes": "Auto ALL (or pick a pin and a style), then Apply.",
    "pin_functions": "Click a pin, then set its name and function.",
    "footprint_embed": "Pins from geometry, or load the part's .kicad_mod.",
    "colors": "Pick a color, click faces or bodies, then Apply.",
}
