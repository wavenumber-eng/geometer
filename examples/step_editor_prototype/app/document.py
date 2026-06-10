"""EditorDocument: every OpenCascade (OCP) call in the editor lives here.

The public API accepts and returns only plain Python / numpy data so the
document could later move behind a subprocess boundary (the wn3d_browser
pattern) without touching tool or UI code. All calls are expected to run on
the Qt main thread — never on a worker thread while VTK is rendering.
"""

from __future__ import annotations

import math
import re
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np

from OCP.Bnd import Bnd_Box
from OCP.BRep import BRep_Tool
from OCP.BRepBndLib import BRepBndLib
from OCP.BRepBuilderAPI import BRepBuilderAPI_Transform
from OCP.gp import gp_Trsf
from OCP.BRepGProp import BRepGProp
from OCP.BRepMesh import BRepMesh_IncrementalMesh
from OCP.GProp import GProp_GProps
from OCP.IFSelect import IFSelect_RetDone
from OCP.Quantity import Quantity_Color
from OCP.STEPCAFControl import STEPCAFControl_Reader
from OCP.TCollection import TCollection_AsciiString, TCollection_ExtendedString
from OCP.TDataStd import TDataStd_Name
from OCP.TDF import TDF_LabelSequence
from OCP.TDocStd import TDocStd_Document
from OCP.TopAbs import (
    TopAbs_EDGE,
    TopAbs_FACE,
    TopAbs_REVERSED,
    TopAbs_SHELL,
    TopAbs_SOLID,
    TopAbs_VERTEX,
)
from OCP.TopExp import TopExp, TopExp_Explorer
from OCP.TopLoc import TopLoc_Location
from OCP.TopTools import TopTools_IndexedMapOfShape
from OCP.TopoDS import TopoDS, TopoDS_Shape
from OCP.XCAFApp import XCAFApp_Application
from OCP.XCAFDoc import XCAFDoc_ColorType, XCAFDoc_DocumentTool, XCAFDoc_ShapeTool


NEUTRAL_RGB = (0.62, 0.64, 0.66)


@dataclass
class BodyMesh:
    points: np.ndarray          # (N, 3) float64, world coordinates
    tris: np.ndarray            # (M, 3) int64 indices into points
    tri_face_ids: np.ndarray    # (M,) int32, 1-based face index within the body
    face_count: int
    face_colors: dict[int, tuple[float, float, float]] = field(default_factory=dict)


@dataclass
class BodyRecord:
    solid: TopoDS_Shape
    name: str
    color: tuple[float, float, float] | None
    role: str = "body"
    mesh: BodyMesh | None = None
    original_color: tuple[float, float, float] | None = None


@dataclass(frozen=True)
class DocumentInfo:
    path: Path
    schema: str
    body_count: int
    face_count: int
    edge_count: int
    vertex_count: int
    bounds: tuple[float, float, float, float, float, float]  # xmin,xmax,ymin,ymax,zmin,zmax


def _label_name(label) -> str | None:
    attr = TDataStd_Name()
    try:
        if label.FindAttribute(TDataStd_Name.GetID_s(), attr):
            ext = attr.Get()
            try:
                return TCollection_AsciiString(ext).ToCString()
            except Exception:
                return TCollection_AsciiString(ext, "?").ToCString()
    except Exception:
        pass
    return None


def _shape_rgb(color_tool, shape) -> tuple[float, float, float] | None:
    """Surface colour first, then generic colour. Guarded: GetColor raises on
    unlabelled shapes in some OCP builds."""
    if color_tool is None:
        return None
    col = Quantity_Color()
    for ctype in (XCAFDoc_ColorType.XCAFDoc_ColorSurf, XCAFDoc_ColorType.XCAFDoc_ColorGen):
        try:
            if color_tool.GetColor(shape, ctype, col):
                return (col.Red(), col.Green(), col.Blue())
        except Exception:
            continue
    return None


def _solid_color(color_tool, solid) -> tuple[float, float, float] | None:
    rgb = _shape_rgb(color_tool, solid)
    if rgb is not None:
        return rgb
    explorer = TopExp_Explorer(solid, TopAbs_FACE)
    if explorer.More():
        return _shape_rgb(color_tool, TopoDS.Face_s(explorer.Current()))
    return None


def _iter_solids(shape):
    explorer = TopExp_Explorer(shape, TopAbs_SOLID)
    while explorer.More():
        yield TopoDS.Solid_s(explorer.Current())
        explorer.Next()


def _count(shape, kind) -> int:
    found = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(shape, kind, found)
    return found.Extent()


def shape_bounds(shapes) -> tuple[float, float, float, float, float, float]:
    box = Bnd_Box()
    for shape in shapes:
        BRepBndLib.Add_s(shape, box)
    if box.IsVoid():
        return (-0.5, 0.5, -0.5, 0.5, -0.5, 0.5)
    xmin, ymin, zmin, xmax, ymax, zmax = box.Get()
    return (xmin, xmax, ymin, ymax, zmin, zmax)


def preview_deflection(bounds: tuple[float, float, float, float, float, float]) -> float:
    xmin, xmax, ymin, ymax, zmin, zmax = bounds
    diag = math.sqrt((xmax - xmin) ** 2 + (ymax - ymin) ** 2 + (zmax - zmin) ** 2)
    if diag <= 0.0:
        return 0.1
    return min(max(diag / 1000.0, 0.002), 10.0)


def shape_volume(shape) -> float | None:
    try:
        props = GProp_GProps()
        BRepGProp.VolumeProperties_s(shape, props)
        return float(props.Mass())
    except Exception:
        return None


def tessellate_body(
    solid: TopoDS_Shape,
    deflection: float,
    color_tool=None,
) -> BodyMesh:
    """Mesh one solid and keep the triangle → B-rep-face mapping.

    Face ids are 1-based indices into the body's TopTools face map, which is
    deterministic for an unmodified solid, so picks can be resolved back to
    the exact TopoDS_Face later.
    """
    BRepMesh_IncrementalMesh(solid, deflection, False, 0.5, True)
    face_map = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(solid, TopAbs_FACE, face_map)

    point_blocks: list[np.ndarray] = []
    tri_blocks: list[np.ndarray] = []
    fid_blocks: list[np.ndarray] = []
    face_colors: dict[int, tuple[float, float, float]] = {}
    offset = 0

    for face_index in range(1, face_map.Extent() + 1):
        face = TopoDS.Face_s(face_map.FindKey(face_index))
        loc = TopLoc_Location()
        triangulation = BRep_Tool.Triangulation_s(face, loc)
        if triangulation is None:
            continue
        trsf = loc.Transformation()

        node_count = triangulation.NbNodes()
        points = np.empty((node_count, 3), dtype=np.float64)
        for i in range(1, node_count + 1):
            p = triangulation.Node(i).Transformed(trsf)
            points[i - 1] = (p.X(), p.Y(), p.Z())

        tri_count = triangulation.NbTriangles()
        reversed_face = face.Orientation() == TopAbs_REVERSED
        tris = np.empty((tri_count, 3), dtype=np.int64)
        for i in range(1, tri_count + 1):
            tri = triangulation.Triangle(i)
            i1, i2, i3 = tri.Value(1), tri.Value(2), tri.Value(3)
            if reversed_face:
                i2, i3 = i3, i2
            tris[i - 1] = (i1 - 1 + offset, i2 - 1 + offset, i3 - 1 + offset)

        point_blocks.append(points)
        tri_blocks.append(tris)
        fid_blocks.append(np.full(tri_count, face_index, dtype=np.int32))
        offset += node_count

        if color_tool is not None:
            rgb = _shape_rgb(color_tool, face)
            if rgb is not None:
                face_colors[face_index] = rgb

    if not point_blocks:
        return BodyMesh(
            points=np.empty((0, 3), dtype=np.float64),
            tris=np.empty((0, 3), dtype=np.int64),
            tri_face_ids=np.empty(0, dtype=np.int32),
            face_count=face_map.Extent(),
        )
    return BodyMesh(
        points=np.concatenate(point_blocks),
        tris=np.concatenate(tri_blocks),
        tri_face_ids=np.concatenate(fid_blocks),
        face_count=face_map.Extent(),
        face_colors=face_colors,
    )


def _carry_face_colors(previous: BodyMesh | None, current: BodyMesh) -> None:
    """Re-tessellation drops XCAF colour lookups (the source document is
    gone), but face ids are stable across rigid transforms, so reuse the
    colours harvested at load time."""
    if previous is None or current.face_colors:
        return
    if previous.face_count == current.face_count:
        current.face_colors = dict(previous.face_colors)


def _read_file_schema(path: Path) -> str:
    try:
        head = path.read_text(encoding="utf-8", errors="replace")[:4096]
        match = re.search(r"FILE_SCHEMA\s*\(\s*\(\s*'([^']+)'", head)
        if match:
            return match.group(1)
    except OSError:
        pass
    return "?"


class EditorDocument:
    """Holds the live B-rep state: a flat table of solids (the source of
    truth for all edits) plus the meshes used for rendering and picking."""

    def __init__(self, path: Path, bodies: list[BodyRecord], schema: str) -> None:
        self.path = path
        self.bodies = bodies
        self.schema = schema
        self._xcaf_doc = None  # kept alive: the color tool dangles if the doc is GC'd

    # ------------------------------------------------------------------ load

    @classmethod
    def load(cls, path: str | Path) -> "EditorDocument":
        path = Path(path)
        doc = TDocStd_Document(TCollection_ExtendedString("MDTV-XCAF"))
        XCAFApp_Application.GetApplication_s().InitDocument(doc)

        reader = STEPCAFControl_Reader()
        reader.SetColorMode(True)
        reader.SetNameMode(True)
        status = reader.ReadFile(str(path))
        if status != IFSelect_RetDone:
            raise RuntimeError(f"STEP read failed for {path}")
        if not reader.Transfer(doc):
            raise RuntimeError(f"STEP transfer failed for {path}")

        main = doc.Main()
        shape_tool = XCAFDoc_DocumentTool.ShapeTool_s(main)
        color_tool = XCAFDoc_DocumentTool.ColorTool_s(main)
        labels = TDF_LabelSequence()
        shape_tool.GetFreeShapes(labels)

        bodies: list[BodyRecord] = []
        for li in range(1, labels.Length() + 1):
            label = labels.Value(li)
            shape = XCAFDoc_ShapeTool.GetShape_s(label)
            if shape is None or shape.IsNull():
                continue
            base_name = _label_name(label) or path.stem
            solids = list(_iter_solids(shape))
            if not solids:
                # Open shells / loose faces: keep the shape as one body so it
                # still renders and exports.
                solids = [shape]
            for index, solid in enumerate(solids):
                name = base_name if len(solids) == 1 else f"{base_name}.{index + 1}"
                color = _solid_color(color_tool, solid)
                bodies.append(
                    BodyRecord(solid=solid, name=name, color=color, original_color=color)
                )

        if not bodies:
            raise RuntimeError(f"No shapes found in {path}")

        document = cls(path=path, bodies=bodies, schema=_read_file_schema(path))
        document._xcaf_doc = doc
        document.remesh_all(color_tool=color_tool)
        return document

    # ------------------------------------------------------------ tessellate

    def remesh_all(self, color_tool=None) -> None:
        deflection = preview_deflection(self.bounds())
        for body in self.bodies:
            previous = body.mesh
            body.mesh = tessellate_body(body.solid, deflection, color_tool)
            _carry_face_colors(previous, body.mesh)

    def remesh_body(self, body_index: int) -> None:
        deflection = preview_deflection(self.bounds())
        body = self.bodies[body_index]
        previous = body.mesh
        body.mesh = tessellate_body(body.solid, deflection)
        _carry_face_colors(previous, body.mesh)

    # ------------------------------------------------------------- mutation

    def apply_trsf(self, matrix) -> None:
        """Apply a rigid 4x4 transform (row-major, numpy or nested lists) to
        every body. Topology is preserved, so face ids — and therefore pin
        face references and face colours — stay valid."""
        m = np.asarray(matrix, dtype=np.float64)
        if m.shape != (4, 4):
            raise ValueError("apply_trsf expects a 4x4 matrix")
        trsf = gp_Trsf()
        trsf.SetValues(
            m[0, 0], m[0, 1], m[0, 2], m[0, 3],
            m[1, 0], m[1, 1], m[1, 2], m[1, 3],
            m[2, 0], m[2, 1], m[2, 2], m[2, 3],
        )
        for body in self.bodies:
            builder = BRepBuilderAPI_Transform(body.solid, trsf, True)
            body.solid = builder.Shape()
        self.remesh_all()

    # ----------------------------------------------------------------- query

    def bounds(self) -> tuple[float, float, float, float, float, float]:
        return shape_bounds(body.solid for body in self.bodies)

    def face_of(self, body_index: int, face_index: int) -> TopoDS_Shape:
        face_map = TopTools_IndexedMapOfShape()
        TopExp.MapShapes_s(self.bodies[body_index].solid, TopAbs_FACE, face_map)
        return TopoDS.Face_s(face_map.FindKey(face_index))

    def info(self) -> DocumentInfo:
        faces = sum(_count(body.solid, TopAbs_FACE) for body in self.bodies)
        edges = sum(_count(body.solid, TopAbs_EDGE) for body in self.bodies)
        vertices = sum(_count(body.solid, TopAbs_VERTEX) for body in self.bodies)
        return DocumentInfo(
            path=self.path,
            schema=self.schema,
            body_count=len(self.bodies),
            face_count=faces,
            edge_count=edges,
            vertex_count=vertices,
            bounds=self.bounds(),
        )
