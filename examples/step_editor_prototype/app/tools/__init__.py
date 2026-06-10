"""Tool registry. Order here is the T-key cycling order; new tools register
by appending their class. One tool lands per milestone."""

from __future__ import annotations

from .base import ToolMode
from .inspect_tool import InspectTool
from .zsit import ZSitTool

TOOL_CLASSES: list[type[ToolMode]] = [
    InspectTool,
    ZSitTool,
]
