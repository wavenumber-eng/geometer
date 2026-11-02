"""Undo / redo for every tool.

Every journaled operation is one step. After each one (and when a model
loads) the editor keeps a snapshot of the document state: the bodies (solids
are immutable OpenCascade handles, so they are shared; names, roles, colors
and meshes are copied), the pins, the journal and the step marks.
Undo restores the previous snapshot, redo the next. Very large models
(over MAX_POINTS mesh points) keep only the step marks and pins, so undo stays
cheap rather than copying huge meshes."""

from __future__ import annotations

import copy
from dataclasses import dataclass

MAX_STEPS = 30
MAX_POINTS = 1_500_000


@dataclass
class Snapshot:
    bodies: list | None
    pins: list
    ops: list
    step_status: dict
    label: str


def _copy_body(body):
    clone = copy.copy(body)
    if body.mesh is not None:
        clone.mesh = copy.copy(body.mesh)
        clone.mesh.points = body.mesh.points.copy()
        clone.mesh.face_colors = dict(body.mesh.face_colors)
    clone.original_face_colors = dict(body.original_face_colors)
    return clone


class History:
    def __init__(self) -> None:
        self.states: list[Snapshot] = []
        self.index = -1

    def reset(self, window, label: str = "opened") -> None:
        self.states = []
        self.index = -1
        self.capture(window, label)

    def capture(self, window, label: str) -> None:
        document = window.document
        if document is None:
            return
        points = sum(len(b.mesh.points) for b in document.bodies if b.mesh is not None)
        bodies = [_copy_body(b) for b in document.bodies] if points <= MAX_POINTS else None
        snap = Snapshot(bodies, copy.deepcopy(window.pins.pins), list(window.journal.operations),
                        dict(getattr(document, "step_status", None) or {}), label)
        del self.states[self.index + 1:]  # a new action drops the redo branch
        self.states.append(snap)
        if len(self.states) > MAX_STEPS:
            self.states.pop(0)
        self.index = len(self.states) - 1

    def can_undo(self) -> bool:
        return self.index > 0

    def can_redo(self) -> bool:
        return self.index < len(self.states) - 1

    def undo(self, window) -> str | None:
        if not self.can_undo():
            return None
        label = self.states[self.index].label
        self.index -= 1
        self._restore(window, self.states[self.index])
        return label

    def redo(self, window) -> str | None:
        if not self.can_redo():
            return None
        self.index += 1
        snap = self.states[self.index]
        self._restore(window, snap)
        return snap.label

    @staticmethod
    def _restore(window, snap: Snapshot) -> None:
        document = window.document
        if snap.bodies is not None:
            document.bodies = [_copy_body(b) for b in snap.bodies]
        window.pins.pins = copy.deepcopy(snap.pins)
        # the journal keeps the record of what was done; undone operations are
        # dropped so an export only replays what is really in the model
        listener = window.journal.listener
        window.journal.listener = None
        window.journal.operations[:] = list(snap.ops)
        window.journal.listener = listener
        document.step_status = dict(snap.step_status)
