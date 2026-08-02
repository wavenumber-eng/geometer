"""Tool registry. Order here is the T-key cycling order; new tools register
by appending their class. One tool lands per milestone."""

from __future__ import annotations

from .base import ToolMode
from .colors import ColorsTool
from .detect_pins import DetectPinsTool
from .hitboxes import HitboxTool
from .inspect_tool import InspectTool
from .front_face import FrontFaceTool
from .logo import LogoTool
from .separate import SeparateUnibodyTool
from .zsit import ZSitTool

# Assign Pin Functions/Names was removed from the UI: pin KIND (SMT/THR/CON/HEAD)
# is autodetected, and a pin's MEANING is PCB-context-dependent (connectors,
# programmable SOTs, FPGAs reassign pinouts), so the editor only needs to make
# each pin UNIQUE and link nets on connectors — not bake in functions. The
# pin_functions MODULE stays for parse_function_assignments (journal replay +
# selftest m5).
TOOL_CLASSES: list[type[ToolMode]] = [
    InspectTool,
    ZSitTool,
    FrontFaceTool,
    DetectPinsTool,
    SeparateUnibodyTool,
    HitboxTool,
    ColorsTool,
    LogoTool,
]
