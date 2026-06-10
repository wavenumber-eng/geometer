"""ToolMode: a tool is a MODE (per the design intent). The GUI routes picks to
the active tool; each tool contributes an actions widget to the right-hand
split panel. Tool math must live in pure functions so the journal can replay
executions headlessly."""

from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING

from PySide6.QtWidgets import QWidget

if TYPE_CHECKING:
    from ..document import EditorDocument
    from ..journal import Journal
    from ..scene import PickResult, SceneManager


@dataclass
class ToolContext:
    document: "EditorDocument | None"
    scene: "SceneManager"
    journal: "Journal"
    window: object  # MainWindow; tools use status() / refresh hooks on it


class ToolMode:
    id: str = "tool"
    title: str = "Tool"
    accent: str = "#4477aa"

    def __init__(self, ctx: ToolContext) -> None:
        self.ctx = ctx
        self._actions_widget: QWidget | None = None

    # Lifecycle -----------------------------------------------------------
    def enter(self) -> None:
        """Called when the tool becomes the active mode."""

    def exit(self) -> None:
        """Called when the user switches away."""

    def on_pick(self, pick: "PickResult") -> None:
        """A click in the 3D view resolved to (body, face, world point)."""

    def on_document_changed(self) -> None:
        """The document was reloaded or mutated by another tool."""

    # Actions panel ---------------------------------------------------------
    def actions_widget(self) -> QWidget:
        if self._actions_widget is None:
            self._actions_widget = self.build_actions_widget()
        return self._actions_widget

    def build_actions_widget(self) -> QWidget:
        return QWidget()

    # Convenience -----------------------------------------------------------
    def status(self, message: str) -> None:
        self.ctx.window.show_status(message)
