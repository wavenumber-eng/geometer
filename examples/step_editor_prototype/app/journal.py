"""Operation journal: every applied tool execution recorded as declarative
JSON. This is the seed of the future fully-automatic conditioning system —
the GUI only *collects* inputs; each operation is replayable headlessly."""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class Operation:
    tool: str                 # tool id, e.g. "zsit"
    params: dict[str, Any]    # user-chosen execution parameters
    inputs: dict[str, Any]    # picked points / selections that drove the op
    result: dict[str, Any]    # computed outcome (e.g. the 4x4 applied)


@dataclass
class Journal:
    operations: list[Operation] = field(default_factory=list)

    def record(self, tool: str, params: dict, inputs: dict, result: dict) -> None:
        self.operations.append(Operation(tool, params, inputs, result))

    def to_jsonable(self) -> list[dict[str, Any]]:
        return [
            {"tool": op.tool, "params": op.params, "inputs": op.inputs, "result": op.result}
            for op in self.operations
        ]

    def save(self, path: Path) -> None:
        path.write_text(json.dumps(self.to_jsonable(), indent=2), encoding="utf-8")

    @classmethod
    def load(cls, path: Path) -> "Journal":
        data = json.loads(Path(path).read_text(encoding="utf-8"))
        journal = cls()
        for item in data:
            journal.operations.append(
                Operation(item["tool"], item.get("params", {}), item.get("inputs", {}), item.get("result", {}))
            )
        return journal
