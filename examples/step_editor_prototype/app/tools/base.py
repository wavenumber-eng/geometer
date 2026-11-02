"""ToolMode: a tool is a MODE (per the design intent). The GUI routes picks to
the active tool; each tool contributes an actions widget to the right-hand
split panel. Tool math must live in pure functions so the journal can replay
executions headlessly."""

from __future__ import annotations

from dataclasses import dataclass
from typing import TYPE_CHECKING

from PySide6.QtWidgets import QPushButton, QWidget


class OperationCancelled(Exception):
    """Raised from the window's progress() when the user presses Cancel; long
    operations leave the document untouched when it propagates."""


def make_apply_button(text: str = "Apply") -> QPushButton:
    """The green commit button every tool uses to apply its staged action."""
    button = QPushButton(text)
    # visible in the tool (apply and stay, like Enter); CONFIRM presses it and moves on
    button.setProperty("wn3d_apply", True)
    button.setStyleSheet(
        "QPushButton {background-color: #1f9d3a; color: #ffffff;"
        " font-weight: 700; padding: 6px;}"
        "QPushButton:hover {background-color: #23b243;}"
        "QPushButton:disabled {background-color: #95ab9b; color: #dfe6e0;}"
    )
    return button

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

    # Shared frame: AUTO / CONFIRM / DENY + the staged-changes list ---------
    auto_method: str = "run_auto"  # the handler AUTO calls (None: no AUTO)

    def run_step_auto(self) -> bool:
        """AUTO: the step's automatic action (its ML model once trained).
        Returns False when the tool has none."""
        handler = getattr(self, self.auto_method, None) if self.auto_method else None
        if handler is None:
            return False
        handler()
        return True

    def staged_rows(self) -> list[tuple[str, str, str]]:
        """Pending changes, three columns each, for the staged list. The
        default reads the tool's `pending` (dict or list) and `clicks` /
        `picks`; tools override for richer rows."""
        rows: list[tuple[str, str, str]] = []
        pending = getattr(self, "pending", None)
        if isinstance(pending, dict):
            for key, value in pending.items():
                detail = value[1] if isinstance(value, tuple) and len(value) > 1 else ""
                kind = value[0].get("type", "") if isinstance(value, tuple) and isinstance(value[0], dict) else ""
                rows.append((f"pin {key}", str(detail), str(kind)))
        elif isinstance(pending, list):
            for item in pending:
                name = getattr(item, "name", "") or getattr(item, "number", "")
                centroid = getattr(item, "centroid", None)
                where = "" if centroid is None else "(" + ", ".join(f"{c:.2f}" for c in centroid) + ")"
                rows.append((f"pin {name}", getattr(item, "role", ""), where))
        for attr in ("clicks", "picks", "points"):
            value = getattr(self, attr, None)
            if isinstance(value, list):
                for k, point in enumerate(value):
                    try:
                        xyz = "(" + ", ".join(f"{float(c):.3f}" for c in point) + ")"
                    except (TypeError, ValueError):
                        xyz = str(point)
                    rows.append((f"point {k + 1}", attr, xyz))
        return rows

    def is_active(self) -> bool:
        """This tool is the one on screen (overlays draw only then: tools
        also refresh while another tool is active)."""
        window = self.ctx.window
        tools = getattr(window, "tools", None)
        return bool(tools) and tools[window.current_tool_index] is self

    def has_unsaved(self) -> bool:
        """Work done in this tool but not applied yet: Tab applies it before
        moving on (as CONFIRM does). Default: anything in the staged list."""
        return bool(self.staged_rows())

    def apply_staged(self) -> None:
        """Enter: apply what is staged in this tool and stay in it. The
        default presses the tool's apply button (or apply()); tools whose
        apply also finishes the step override this."""
        page = getattr(self, "_actions_widget", None)
        buttons = [b for b in page.findChildren(QPushButton) if b.property("wn3d_apply") and b.isEnabled()]             if page is not None else []
        if buttons:
            buttons[0].click()
        elif hasattr(self, "apply"):
            self.apply()

    def deny(self) -> None:
        """DENY: drop everything staged and redraw. Tools with more state
        override this."""
        for attr in ("pending", "clicks", "picks", "points"):
            value = getattr(self, attr, None)
            if isinstance(value, (list, dict)):
                setattr(self, attr, type(value)())
        refresh = getattr(self, "_refresh_views", None) or getattr(self, "_refresh", None)
        if callable(refresh):
            try:
                refresh()
            except Exception:  # noqa: BLE001 - a redraw must never break DENY
                pass
        self.ctx.scene.set_markers([])

    # Convenience -----------------------------------------------------------
    def status(self, message: str) -> None:
        self.ctx.window.show_status(message)


def shrink_to_grid(button, minimum: int = 48) -> None:
    """Let a packed button take its grid cell's width instead of forcing the
    panel as wide as its label (the full name stays in the tooltip)."""
    from PySide6.QtWidgets import QSizePolicy

    if not button.toolTip():
        button.setToolTip(button.text())
    button.setSizePolicy(QSizePolicy.Policy.Ignored, QSizePolicy.Policy.Fixed)
    button.setMinimumWidth(minimum)
