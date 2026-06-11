"""Tool registry. Order here is the T-key cycling order; new tools register
by appending their class. One tool lands per milestone."""

from __future__ import annotations

from .base import ToolMode
from .colors import ColorsTool
from .detect_pins import DetectPinsTool
from .hitboxes import HitboxTool
from .inspect_tool import InspectTool
from .logo import LogoTool
from .pin1_quadrant import Pin1QuadrantTool
from .pin_functions import PinFunctionsTool
from .separate import SeparateUnibodyTool
from .zsit import ZSitTool

TOOL_CLASSES: list[type[ToolMode]] = [
    InspectTool,
    ZSitTool,
    Pin1QuadrantTool,
    DetectPinsTool,
    SeparateUnibodyTool,
    HitboxTool,
    PinFunctionsTool,
    ColorsTool,
    LogoTool,
]
