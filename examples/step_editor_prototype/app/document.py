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
from OCP.BRepAdaptor import BRepAdaptor_Curve
from OCP.BRepAlgoAPI import BRepAlgoAPI_Splitter
from OCP.BRepBndLib import BRepBndLib
from OCP.BRepBuilderAPI import BRepBuilderAPI_MakeFace, BRepBuilderAPI_Transform
from OCP.GeomAPI import GeomAPI_ProjectPointOnSurf
from OCP.GeomLProp import GeomLProp_SLProps
from OCP.gp import gp_Dir, gp_Pln, gp_Pnt, gp_Trsf
from OCP.TopTools import TopTools_ListOfShape
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
from OCP.TopTools import (
    TopTools_IndexedDataMapOfShapeListOfShape,
    TopTools_IndexedMapOfShape,
)
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
    original_face_colors: dict[int, tuple[float, float, float]] = field(default_factory=dict)


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


def _harvest_face_colors(color_tool, solid) -> dict[int, tuple[float, float, float]]:
    """Per-face colours keyed by face-map index. Must run BEFORE the solid's
    location is baked — the XCAF colour tool keys by exact shape identity."""
    result: dict[int, tuple[float, float, float]] = {}
    if color_tool is None:
        return result
    face_map = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(solid, TopAbs_FACE, face_map)
    for index in range(1, face_map.Extent() + 1):
        rgb = _shape_rgb(color_tool, TopoDS.Face_s(face_map.FindKey(index)))
        if rgb is not None:
            result[index] = rgb
    return result


def _bake_location(solid):
    """Assembly instances carry a TopLoc_Location; bake it into the geometry
    so every body is a plain located-at-identity solid. Required for XCAF
    sub-shape (face colour) export — located shapes register as references,
    not simple shapes. (An identity-trsf transform is a no-op in OCCT, so
    strip the location and re-apply it as a real copying transform.)"""
    location = solid.Location()
    if location.IsIdentity():
        return solid
    unplaced = solid.Located(TopLoc_Location())
    return BRepBuilderAPI_Transform(unplaced, location.Transformation(), True).Shape()


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


def _edge_dihedral_deg(edge, face1, face2) -> float | None:
    """Angle between the two faces' outward normals at the edge midpoint.
    None means the normal could not be evaluated (treated as sharp)."""
    try:
        curve = BRepAdaptor_Curve(edge)
        t = 0.5 * (curve.FirstParameter() + curve.LastParameter())
        point = curve.Value(t)
        normals = []
        for face in (face1, face2):
            surface = BRep_Tool.Surface_s(face)
            projector = GeomAPI_ProjectPointOnSurf(point, surface)
            if projector.NbPoints() < 1:
                return None
            u, v = projector.LowerDistanceParameters()
            props = GeomLProp_SLProps(surface, u, v, 1, 1.0e-6)
            if not props.IsNormalDefined():
                return None
            direction = props.Normal()
            normal = np.array([direction.X(), direction.Y(), direction.Z()])
            if face.Orientation() == TopAbs_REVERSED:
                normal = -normal
            normals.append(normal)
        cosine = float(np.clip(np.dot(normals[0], normals[1]), -1.0, 1.0))
        return math.degrees(math.acos(cosine))
    except Exception:
        return None


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
                # Colours first (keyed by the original shape identity), then
                # bake the instance location into the geometry.
                color = _solid_color(color_tool, solid)
                face_colors = _harvest_face_colors(color_tool, solid)
                solid = _bake_location(solid)
                bodies.append(
                    BodyRecord(
                        solid=solid,
                        name=name,
                        color=color,
                        original_color=color,
                        original_face_colors=face_colors,
                    )
                )

        if not bodies:
            raise RuntimeError(f"No shapes found in {path}")

        document = cls(path=path, bodies=bodies, schema=_read_file_schema(path))
        document._xcaf_doc = doc
        document.remesh_all()
        for body in document.bodies:
            if body.mesh is not None and body.original_face_colors:
                if body.mesh.face_count >= max(body.original_face_colors):
                    body.mesh.face_colors = dict(body.original_face_colors)
        return document

    # ------------------------------------------------------------ tessellate

    def remesh_all(self, color_tool=None, progress=None) -> None:
        deflection = preview_deflection(self.bounds())
        total = len(self.bodies)
        for index, body in enumerate(self.bodies):
            if progress is not None:
                progress("Meshing bodies", index, total)
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

    def split_by_plane(self, point, normal) -> list[int]:
        """Split every body that crosses the plane into separate solids
        (BRepAlgoAPI_Splitter with a large planar face). Returns the indices
        of the NEW bodies on the positive-normal side of the plane (the pin
        side). Bodies that don't cross are kept as-is."""
        point = np.asarray(point, dtype=np.float64)
        normal = np.asarray(normal, dtype=np.float64)
        normal = normal / max(float(np.linalg.norm(normal)), 1.0e-12)
        xmin, xmax, ymin, ymax, zmin, zmax = self.bounds()
        size = max(
            float(np.linalg.norm([xmax - xmin, ymax - ymin, zmax - zmin])), 1.0
        ) * 2.0
        eps = size * 1.0e-9

        plane = gp_Pln(gp_Pnt(*point), gp_Dir(*normal))
        tool_face = BRepBuilderAPI_MakeFace(plane, -size, size, -size, size).Face()

        new_bodies: list[BodyRecord] = []
        pin_side_indices: list[int] = []
        for body in self.bodies:
            mesh = body.mesh
            crosses = False
            if mesh is not None and len(mesh.points):
                distances = (mesh.points - point) @ normal
                crosses = distances.max() > eps and distances.min() < -eps
            if not crosses:
                new_bodies.append(body)
                continue

            splitter = BRepAlgoAPI_Splitter()
            arguments = TopTools_ListOfShape()
            arguments.Append(body.solid)
            tools = TopTools_ListOfShape()
            tools.Append(tool_face)
            splitter.SetArguments(arguments)
            splitter.SetTools(tools)
            splitter.Build()
            if not splitter.IsDone():
                new_bodies.append(body)
                continue
            solids = list(_iter_solids(splitter.Shape()))
            if len(solids) <= 1:
                new_bodies.append(body)
                continue

            for solid in solids:
                box = Bnd_Box()
                BRepBndLib.Add_s(solid, box)
                bxmin, bymin, bzmin, bxmax, bymax, bzmax = box.Get()
                center = np.array(
                    [(bxmin + bxmax) / 2, (bymin + bymax) / 2, (bzmin + bzmax) / 2]
                )
                pin_side = float((center - point) @ normal) > 0.0
                record = BodyRecord(
                    solid=solid,
                    name=body.name,
                    color=body.color,
                    original_color=body.original_color,
                )
                if pin_side:
                    pin_side_indices.append(len(new_bodies))
                new_bodies.append(record)

        self.bodies = new_bodies
        self.remesh_all()
        return pin_side_indices

    def split_by_face_regions(
        self, regions: list[list[tuple[int, int]]], progress=None
    ) -> list[int]:
        """Split bodies along detected pin-region boundaries: for each region
        the boundary loops (edges between region and body faces) are capped
        with planar faces, and those caps drive BRepAlgoAPI_Splitter — each
        pin separates as the whole shape the edge flow found. Returns the new
        body indices that are NOT the largest piece of their split (the pin
        solids)."""
        from OCP.ShapeAnalysis import ShapeAnalysis_FreeBounds
        from OCP.TopTools import TopTools_HSequenceOfShape

        by_body: dict[int, list[set[int]]] = {}
        for region in regions:
            if not region:
                continue
            groups: dict[int, set[int]] = {}
            for body_index, face_index in region:
                groups.setdefault(body_index, set()).add(face_index)
            for body_index, faces in groups.items():
                by_body.setdefault(body_index, []).append(faces)

        new_bodies: list[BodyRecord] = []
        pin_indices: list[int] = []
        color_restores: list[tuple[BodyRecord, list, float]] = []
        for body_index, body in enumerate(self.bodies):
            region_sets = by_body.get(body_index)
            if not region_sets:
                new_bodies.append(body)
                continue

            # Per-face colours can't survive the split directly (face ids
            # change) — capture coloured-face centroids now and re-match them
            # onto the new bodies geometrically after remeshing. Faces the
            # split doesn't cut keep their exact centroid.
            colored_faces = []
            if body.mesh is not None and body.mesh.face_colors:
                centers = body.mesh.points[body.mesh.tris].mean(axis=1)
                span = body.mesh.points.max(axis=0) - body.mesh.points.min(axis=0)
                tolerance = float(np.linalg.norm(span)) * 5.0e-3
                for face_id, rgb in body.mesh.face_colors.items():
                    mask = body.mesh.tri_face_ids == face_id
                    if mask.any():
                        colored_faces.append((centers[mask].mean(axis=0), rgb))
            else:
                tolerance = 0.0

            face_map = TopTools_IndexedMapOfShape()
            TopExp.MapShapes_s(body.solid, TopAbs_FACE, face_map)
            edge_faces = TopTools_IndexedDataMapOfShapeListOfShape()
            TopExp.MapShapesAndAncestors_s(body.solid, TopAbs_EDGE, TopAbs_FACE, edge_faces)

            caps = []
            for region_number, region in enumerate(region_sets):
                if progress is not None:
                    progress("Capping pin junctions", region_number, len(region_sets))
                boundary = []
                for edge_index in range(1, edge_faces.Extent() + 1):
                    sides = set()
                    for shape in edge_faces.FindFromIndex(edge_index):
                        index = face_map.FindIndex(shape)
                        if index > 0:
                            sides.add(index in region)
                    if sides == {True, False}:
                        boundary.append(TopoDS.Edge_s(edge_faces.FindKey(edge_index)))
                if not boundary:
                    continue
                edges = TopTools_HSequenceOfShape()
                for edge in boundary:
                    edges.Append(edge)
                wires = TopTools_HSequenceOfShape()
                ShapeAnalysis_FreeBounds.ConnectEdgesToWires_s(edges, 1.0e-7, False, wires)
                for wire_index in range(1, wires.Length() + 1):
                    try:
                        maker = BRepBuilderAPI_MakeFace(
                            TopoDS.Wire_s(wires.Value(wire_index)), True
                        )
                        if maker.IsDone():
                            caps.append(maker.Face())
                    except Exception:
                        continue

            if not caps:
                new_bodies.append(body)
                continue

            if progress is not None:
                progress(f"Splitting {body.name} at {len(caps)} junction(s)", 0, 0)
            splitter = BRepAlgoAPI_Splitter()
            arguments = TopTools_ListOfShape()
            arguments.Append(body.solid)
            tools = TopTools_ListOfShape()
            for cap in caps:
                tools.Append(cap)
            splitter.SetArguments(arguments)
            splitter.SetTools(tools)
            splitter.Build()
            solids = list(_iter_solids(splitter.Shape())) if splitter.IsDone() else []
            if len(solids) <= 1:
                new_bodies.append(body)
                continue

            volumes = [shape_volume(solid) or 0.0 for solid in solids]
            largest = int(np.argmax(volumes))
            for index, solid in enumerate(solids):
                record = BodyRecord(
                    solid=solid,
                    name=body.name,
                    color=body.color,
                    original_color=body.original_color,
                )
                if index != largest:
                    pin_indices.append(len(new_bodies))
                if colored_faces:
                    color_restores.append((record, colored_faces, tolerance))
                new_bodies.append(record)

        self.bodies = new_bodies
        self.remesh_all(progress=progress)

        # Re-attach per-face colours: a new face whose centroid coincides with
        # an old coloured face's centroid is that same face.
        for record, colored_faces, tolerance in color_restores:
            mesh = record.mesh
            if mesh is None or not len(mesh.tris) or tolerance <= 0.0:
                continue
            centers = mesh.points[mesh.tris].mean(axis=1)
            anchors = np.array([c for c, _rgb in colored_faces])
            for face_id in np.unique(mesh.tri_face_ids):
                mask = mesh.tri_face_ids == face_id
                centroid = centers[mask].mean(axis=0)
                distances = np.linalg.norm(anchors - centroid, axis=1)
                nearest = int(np.argmin(distances))
                if distances[nearest] <= tolerance:
                    mesh.face_colors[int(face_id)] = colored_faces[nearest][1]
            record.original_face_colors = dict(mesh.face_colors)
        return pin_indices

    def face_smooth_adjacency(
        self, body_index: int, smooth_angle_deg: float | None = 30.0
    ) -> dict[int, set[int]]:
        """Face adjacency graph of one body. With a threshold, only edges
        whose dihedral angle (surface normals sampled at the shared edge
        midpoint) is below it are kept — region growing 'flows' across those
        and stops at discontinuities. With None, every shared edge connects."""
        solid = self.bodies[body_index].solid
        face_map = TopTools_IndexedMapOfShape()
        TopExp.MapShapes_s(solid, TopAbs_FACE, face_map)
        edge_faces = TopTools_IndexedDataMapOfShapeListOfShape()
        TopExp.MapShapesAndAncestors_s(solid, TopAbs_EDGE, TopAbs_FACE, edge_faces)

        adjacency: dict[int, set[int]] = {
            i: set() for i in range(1, face_map.Extent() + 1)
        }
        for edge_index in range(1, edge_faces.Extent() + 1):
            edge = TopoDS.Edge_s(edge_faces.FindKey(edge_index))
            faces = []
            for shape in edge_faces.FindFromIndex(edge_index):
                index = face_map.FindIndex(shape)
                if index > 0 and index not in faces:
                    faces.append(index)
            if len(faces) != 2:
                continue
            f1, f2 = faces
            if smooth_angle_deg is not None:
                angle = _edge_dihedral_deg(
                    edge,
                    TopoDS.Face_s(face_map.FindKey(f1)),
                    TopoDS.Face_s(face_map.FindKey(f2)),
                )
                if angle is None or angle > smooth_angle_deg:
                    continue
            adjacency[f1].add(f2)
            adjacency[f2].add(f1)
        return adjacency

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
