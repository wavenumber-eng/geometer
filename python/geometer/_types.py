from __future__ import annotations

import json
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any, Mapping, Sequence


Matrix4 = Sequence[Sequence[float]] | Sequence[float]
StepInput = bytes | bytearray | memoryview | str | Path
ModelInput = StepInput
PlanarBatchInput = bytes | bytearray | memoryview | str | Path
PlanarPoint = tuple[float, float]
PlanarRing = list[PlanarPoint]


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
    fast: dict[str, Any] = field(default_factory=dict)
    output_outline: bool = True
    output_detail: bool = True
    output_bbox: bool = True
    curve_mode: str = "polyline"
    samples_per_curve: int = 24
    round_digits: int = 3
    union_outline_polygons: bool = True
    mesh_linear_deflection: float = 0.01
    mesh_angular_deflection: float = 0.5
    mesh_relative: bool = False
    mesh_deflection_mode: str = "bbox-relative"
    mesh_deflection_coefficient: float = 0.004
    outline_algorithm: str = "hlr-close"
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
        return cls(outline_algorithm="mesh-shadow")

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
class PlanarRegion:
    outer: PlanarRing
    holes: list[PlanarRing]

    def to_json_value(self) -> dict[str, Any]:
        return {
            "outer": _ring_json_value(self.outer),
            "holes": [_ring_json_value(hole) for hole in self.holes],
        }


@dataclass(frozen=True)
class PlanarBatchSolveJob:
    regions: list[PlanarRegion]
    area_mm2: float
    source_subject_ring_count: int = 0
    raw_subject_ring_count: int = 0
    stroke_path_count: int = 0
    stroke_region_count: int = 0
    local_subtract_ring_count: int = 0
    common_subtract_ring_count: int = 0


@dataclass(frozen=True)
class PlanarBatchSolveResult:
    data: dict[str, Any]
    jobs: list[PlanarBatchSolveJob]

    @classmethod
    def from_json_value(cls, data: dict[str, Any]) -> "PlanarBatchSolveResult":
        jobs_value = data.get("jobs", [])
        if not isinstance(jobs_value, list):
            raise ValueError("planar solve JSON field 'jobs' must be a list")
        return cls(data=data, jobs=[_planar_job(job, index) for index, job in enumerate(jobs_value)])

    @property
    def schema(self) -> str | None:
        schema = self.data.get("schema")
        return schema if isinstance(schema, str) else None

    @property
    def units(self) -> str | None:
        units = self.data.get("units")
        return units if isinstance(units, str) else None

    def regions(self, job_index: int = 0) -> list[PlanarRegion]:
        return self.jobs[job_index].regions

    def to_json(self) -> str:
        return json.dumps(self.data, separators=(",", ":"))

    def to_region_json_value(self) -> dict[str, Any]:
        return {
            "schema": self.schema,
            "units": self.units,
            "jobs": [
                {
                    "area_mm2": job.area_mm2,
                    "regions": [region.to_json_value() for region in job.regions],
                }
                for job in self.jobs
            ],
        }


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


def read_planar_batch_input(request: PlanarBatchInput) -> bytes:
    if isinstance(request, bytes):
        return request
    if isinstance(request, bytearray):
        return bytes(request)
    if isinstance(request, memoryview):
        return request.tobytes()
    if isinstance(request, (str, Path)):
        return Path(request).read_bytes()
    raise TypeError("request must be bytes, bytearray, memoryview, str, or pathlib.Path")


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

    alias_format = payload.pop("model_format", None)
    selected_format = payload.get("format", alias_format if alias_format is not None else format)
    payload["format"] = normalize_model_format(str(selected_format))
    alias_transform = payload.pop("modelTransform", None)
    if "model_transform" not in payload and alias_transform is not None:
        payload["model_transform"] = _flat_matrix4_json_value(alias_transform)
    if model_transform is not None:
        payload["model_transform"] = _flat_matrix4_json_value(model_transform)
    elif "model_transform" in payload:
        payload["model_transform"] = _flat_matrix4_json_value(payload["model_transform"])
    return {key: payload[key] for key in ("format", "model_transform") if key in payload}


def encode_json_options(payload: Mapping[str, Any] | None) -> bytes | None:
    if not payload:
        return None
    return json.dumps(payload, separators=(",", ":")).encode("utf-8")


def _planar_job(value: Any, index: int) -> PlanarBatchSolveJob:
    if not isinstance(value, Mapping):
        raise ValueError(f"planar solve job {index} must be an object")
    regions_value = value.get("regions", [])
    if not isinstance(regions_value, list):
        raise ValueError(f"planar solve job {index} field 'regions' must be a list")
    return PlanarBatchSolveJob(
        regions=[_planar_region(region, region_index) for region_index, region in enumerate(regions_value)],
        area_mm2=_float_field(value, "area_mm2"),
        source_subject_ring_count=_int_field(value, "source_subject_ring_count"),
        raw_subject_ring_count=_int_field(value, "raw_subject_ring_count"),
        stroke_path_count=_int_field(value, "stroke_path_count"),
        stroke_region_count=_int_field(value, "stroke_region_count"),
        local_subtract_ring_count=_int_field(value, "local_subtract_ring_count"),
        common_subtract_ring_count=_int_field(value, "common_subtract_ring_count"),
    )


def _planar_region(value: Any, index: int) -> PlanarRegion:
    if not isinstance(value, Mapping):
        raise ValueError(f"planar solve region {index} must be an object")
    outer_value = value.get("outer", value.get("outline"))
    if outer_value is None:
        raise ValueError(f"planar solve region {index} must contain 'outer' or 'outline'")
    holes_value = value.get("holes", [])
    if not isinstance(holes_value, list):
        raise ValueError(f"planar solve region {index} field 'holes' must be a list")
    return PlanarRegion(
        outer=_planar_ring(outer_value, f"region {index} outer"),
        holes=[_planar_ring(hole, f"region {index} hole {hole_index}") for hole_index, hole in enumerate(holes_value)],
    )


def _planar_ring(value: Any, label: str) -> PlanarRing:
    if not isinstance(value, list):
        raise ValueError(f"planar solve {label} must be a list of [x, y] points")
    return [_planar_point(point, f"{label} point {point_index}") for point_index, point in enumerate(value)]


def _planar_point(value: Any, label: str) -> PlanarPoint:
    if not isinstance(value, (list, tuple)) or len(value) != 2:
        raise ValueError(f"planar solve {label} must be a 2D point")
    return (float(value[0]), float(value[1]))


def _int_field(value: Mapping[str, Any], key: str) -> int:
    return int(value.get(key, 0) or 0)


def _float_field(value: Mapping[str, Any], key: str) -> float:
    return float(value.get(key, 0.0) or 0.0)


def _ring_json_value(ring: PlanarRing) -> list[list[float]]:
    return [[point[0], point[1]] for point in ring]


def _view_json_value(view: ProjectionView | Mapping[str, Any]) -> dict[str, Any]:
    if isinstance(view, ProjectionView):
        return view.to_json_value()
    if isinstance(view, Mapping):
        return dict(view)
    raise TypeError("views must contain ProjectionView objects or mappings")


def _matrix4_json_value(matrix: Matrix4) -> list[Any]:
    values = list(matrix)
    if len(values) == 16:
        flat_values: list[float] = []
        for item in values:
            if isinstance(item, Sequence) and not isinstance(item, str):
                break
            flat_values.append(float(item))
        else:
            return flat_values
    if len(values) == 4:
        rows: list[list[float]] = []
        for row in values:
            if not isinstance(row, Sequence) or isinstance(row, str):
                raise ValueError("model_transform rows must be sequences")
            row_values = list(row)
            if len(row_values) != 4:
                raise ValueError("model_transform rows must contain 4 values")
            rows.append([float(item) for item in row_values])
        return rows
    raise ValueError("model_transform must be a flat 16-value sequence or a 4x4 sequence")


def _flat_matrix4_json_value(matrix: Matrix4) -> list[float]:
    value = _matrix4_json_value(matrix)
    if len(value) == 16:
        return [float(item) for item in value]
    return [float(item) for row in value for item in row]


def _vec3(values: Sequence[float], label: str) -> tuple[float, float, float]:
    items = tuple(float(value) for value in values)
    if len(items) != 3:
        raise ValueError(f"{label} must contain exactly 3 values")
    return items
