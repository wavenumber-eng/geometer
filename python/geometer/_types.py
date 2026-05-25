from __future__ import annotations

import json
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any, Mapping, Sequence


Matrix4 = Sequence[Sequence[float]] | Sequence[float]
StepInput = bytes | bytearray | memoryview | str | Path
ModelInput = StepInput


@dataclass(frozen=True)
class Version:
    major: int
    minor: int
    patch: int
    abi: int
    string: str


@dataclass(frozen=True)
class ProjectionView:
    id: str
    direction: tuple[float, float, float]
    up: tuple[float, float, float]

    @staticmethod
    def top() -> "ProjectionView":
        return ProjectionView("top", (0.0, 0.0, 1.0), (0.0, 1.0, 0.0))

    @staticmethod
    def bottom() -> "ProjectionView":
        return ProjectionView("bottom", (0.0, 0.0, -1.0), (0.0, 1.0, 0.0))

    @staticmethod
    def front() -> "ProjectionView":
        return ProjectionView("front", (0.0, -1.0, 0.0), (0.0, 0.0, 1.0))

    @staticmethod
    def back() -> "ProjectionView":
        return ProjectionView("back", (0.0, 1.0, 0.0), (0.0, 0.0, 1.0))

    @staticmethod
    def right() -> "ProjectionView":
        return ProjectionView("right", (1.0, 0.0, 0.0), (0.0, 0.0, 1.0))

    @staticmethod
    def left() -> "ProjectionView":
        return ProjectionView("left", (-1.0, 0.0, 0.0), (0.0, 0.0, 1.0))

    @staticmethod
    def camera(direction: Sequence[float], up: Sequence[float], id: str = "camera") -> "ProjectionView":
        return ProjectionView(id, _vec3(direction, "direction"), _vec3(up, "up"))

    def to_json_value(self) -> dict[str, Any]:
        return {"id": self.id, "direction": list(self.direction), "up": list(self.up)}


@dataclass
class HlrOptions:
    projection_algorithm: str = "poly"
    curve_mode: str = "polyline"
    samples_per_curve: int = 24
    round_digits: int = 3
    union_simple_polygons: bool = True
    mesh_linear_deflection: float = 0.01
    mesh_angular_deflection: float = 0.5
    mesh_relative: bool = False
    hlr_angle_tolerance: float = 0.0174533
    edge_v_sharp: bool = True
    edge_v_outline: bool = True
    edge_v_smooth: bool = False
    edge_v_sewn: bool = False
    edge_v_iso: bool = False
    edge_h_sharp: bool = False
    edge_h_outline: bool = False
    edge_h_smooth: bool = False
    edge_h_sewn: bool = False
    edge_h_iso: bool = False

    @classmethod
    def assembly_outline(cls) -> "HlrOptions":
        return cls()

    @classmethod
    def visible_detail(cls) -> "HlrOptions":
        return cls(edge_v_smooth=True, edge_v_sewn=True)

    @classmethod
    def visible_and_hidden(cls) -> "HlrOptions":
        return cls(edge_h_sharp=True, edge_h_outline=True)

    @classmethod
    def exact_arcs(cls) -> "HlrOptions":
        return cls(projection_algorithm="exact", curve_mode="native_arcs")

    @classmethod
    def debug_all_edges(cls) -> "HlrOptions":
        return cls(
            projection_algorithm="exact",
            edge_v_smooth=True,
            edge_v_sewn=True,
            edge_v_iso=True,
            edge_h_sharp=True,
            edge_h_outline=True,
            edge_h_smooth=True,
            edge_h_sewn=True,
            edge_h_iso=True,
        )

    def to_json_value(self) -> dict[str, Any]:
        return asdict(self)


@dataclass(frozen=True)
class HlrProjectionResult:
    data: dict[str, Any]

    @property
    def schema(self) -> str | None:
        return self.data.get("schema")

    @property
    def units(self) -> str | None:
        return self.data.get("units")

    @property
    def source_hash(self) -> str | None:
        source = self.data.get("source")
        if isinstance(source, Mapping):
            value = source.get("hash")
            return value if isinstance(value, str) else None
        return None

    @property
    def timings(self) -> Mapping[str, Any]:
        timings = self.data.get("timings")
        return timings if isinstance(timings, Mapping) else {}

    @property
    def views(self) -> Sequence[Mapping[str, Any]]:
        views = self.data.get("views")
        return views if isinstance(views, Sequence) and not isinstance(views, (str, bytes)) else []

    def view(self, view_id: str) -> Mapping[str, Any]:
        for view in self.views:
            if isinstance(view, Mapping) and view.get("id") == view_id:
                return view
        raise KeyError(view_id)

    def geometry(self, view_id: str, mode: str = "detail") -> Mapping[str, Any]:
        view = self.view(view_id)
        modes = view.get("modes")
        if not isinstance(modes, Mapping) or mode not in modes:
            raise KeyError(f"{view_id}.{mode}")
        geometry = modes[mode]
        if not isinstance(geometry, Mapping):
            raise KeyError(f"{view_id}.{mode}")
        return geometry

    def to_json(self) -> str:
        return json.dumps(self.data, separators=(",", ":"))


@dataclass(frozen=True)
class ModelBoundsResult:
    data: dict[str, Any]

    @property
    def schema(self) -> str | None:
        return self.data.get("schema")

    @property
    def units(self) -> str | None:
        return self.data.get("units")

    @property
    def source_format(self) -> str | None:
        source = self.data.get("source")
        if isinstance(source, Mapping):
            value = source.get("format")
            return value if isinstance(value, str) else None
        return None

    @property
    def source_hash(self) -> str | None:
        source = self.data.get("source")
        if isinstance(source, Mapping):
            value = source.get("hash")
            return value if isinstance(value, str) else None
        return None

    @property
    def bounds(self) -> Mapping[str, Any]:
        bounds = self.data.get("bounds")
        return bounds if isinstance(bounds, Mapping) else {}

    @property
    def timings(self) -> Mapping[str, Any]:
        timings = self.data.get("timings")
        return timings if isinstance(timings, Mapping) else {}

    def to_json(self) -> str:
        return json.dumps(self.data, separators=(",", ":"))


def read_step_input(step: StepInput) -> bytes:
    if isinstance(step, bytes):
        return step
    if isinstance(step, bytearray):
        return bytes(step)
    if isinstance(step, memoryview):
        return step.tobytes()
    if isinstance(step, (str, Path)):
        return Path(step).read_bytes()
    raise TypeError("step must be bytes, bytearray, memoryview, str, or pathlib.Path")


def normalize_model_format(format: str) -> str:
    normalized = str(format).strip().lower()
    if normalized != "step":
        raise ValueError('model operations currently support only format="step"')
    return normalized


def build_hlr_options_payload(
    *,
    views: Sequence[ProjectionView | Mapping[str, Any]] | None = None,
    model_transform: Matrix4 | None = None,
    options: HlrOptions | Mapping[str, Any] | None = None,
) -> dict[str, Any]:
    if options is None:
        payload: dict[str, Any] = {}
    elif isinstance(options, HlrOptions):
        payload = options.to_json_value()
    elif isinstance(options, Mapping):
        payload = dict(options)
    else:
        raise TypeError("options must be HlrOptions, a mapping, or None")

    if views is not None:
        payload["views"] = [_view_json_value(view) for view in views]
    if model_transform is not None:
        payload["model_transform"] = _matrix4_json_value(model_transform)
    return payload


def build_model_options_payload(
    *,
    format: str = "step",
    model_transform: Matrix4 | None = None,
    options: Mapping[str, Any] | None = None,
) -> dict[str, Any]:
    if options is None:
        payload: dict[str, Any] = {}
    elif isinstance(options, Mapping):
        payload = dict(options)
    else:
        raise TypeError("options must be a mapping or None")

    payload["format"] = normalize_model_format(str(payload.get("format", format)))
    if model_transform is not None:
        payload["model_transform"] = _matrix4_json_value(model_transform)
    return payload


def encode_json_options(payload: Mapping[str, Any] | None) -> bytes | None:
    if not payload:
        return None
    return json.dumps(payload, separators=(",", ":")).encode("utf-8")


def _view_json_value(view: ProjectionView | Mapping[str, Any]) -> dict[str, Any]:
    if isinstance(view, ProjectionView):
        return view.to_json_value()
    if isinstance(view, Mapping):
        return dict(view)
    raise TypeError("views must contain ProjectionView objects or mappings")


def _matrix4_json_value(matrix: Matrix4) -> list[Any]:
    values = list(matrix)
    if len(values) == 16 and not any(isinstance(item, Sequence) and not isinstance(item, str) for item in values):
        return [float(item) for item in values]
    if len(values) == 4:
        rows = []
        for row in values:
            row_values = list(row)  # type: ignore[arg-type]
            if len(row_values) != 4:
                raise ValueError("model_transform rows must contain 4 values")
            rows.append([float(item) for item in row_values])
        return rows
    raise ValueError("model_transform must be a flat 16-value sequence or a 4x4 sequence")


def _vec3(values: Sequence[float], label: str) -> tuple[float, float, float]:
    items = tuple(float(value) for value in values)
    if len(items) != 3:
        raise ValueError(f"{label} must contain exactly 3 values")
    return items
