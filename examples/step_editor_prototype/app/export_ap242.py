"""AP242 export: rebuild a fresh XCAF document from the editor's body table
(names + colours), write with the AP242 schema, then validate the result by
re-reading it with OCP and geometer.

The pin/hitbox metadata entity injection lands in M8; this module already owns
the writer + validation skeleton so every milestone exports through one path.
"""

from __future__ import annotations

import json
import re
from dataclasses import dataclass
from pathlib import Path

import geometer

from OCP.BRep import BRep_Builder
from OCP.IFSelect import IFSelect_RetDone
from OCP.Interface import Interface_Static
from OCP.TopoDS import TopoDS_Compound
from OCP.Quantity import Quantity_Color, Quantity_TOC_RGB
from OCP.STEPCAFControl import STEPCAFControl_Writer
from OCP.STEPControl import STEPControl_AsIs
from OCP.TCollection import TCollection_ExtendedString
from OCP.TDataStd import TDataStd_Name
from OCP.TDocStd import TDocStd_Document
from OCP.TopAbs import TopAbs_FACE
from OCP.TopExp import TopExp
from OCP.TopTools import TopTools_IndexedMapOfShape
from OCP.TopoDS import TopoDS
from OCP.XCAFApp import XCAFApp_Application
from OCP.XCAFDoc import XCAFDoc_ColorType, XCAFDoc_DocumentTool

from .document import EditorDocument


CONDITIONED_SUFFIX = "_AP242_conditioned"


@dataclass(frozen=True)
class ExportReport:
    path: Path
    body_count: int
    reread_body_count: int
    bounds_ok: bool

    @property
    def ok(self) -> bool:
        return self.body_count == self.reread_body_count and self.bounds_ok

    def summary(self) -> str:
        state = "OK" if self.ok else "MISMATCH"
        return (
            f"{state}: wrote {self.path.name} | bodies {self.body_count} -> "
            f"re-read {self.reread_body_count} | geometer bounds "
            f"{'ok' if self.bounds_ok else 'FAILED'}"
        )


def conditioned_path(input_path: Path) -> Path:
    return input_path.with_name(f"{input_path.stem}{CONDITIONED_SUFFIX}.step")


def export_ap242(
    document: EditorDocument,
    out_path: Path | None = None,
    pins=None,
    journal=None,
) -> ExportReport:
    out_path = out_path or conditioned_path(document.path)
    write_step(document, out_path)
    if pins is not None or journal is not None:
        from .metadata import build_metadata

        payload = build_metadata(document, pins, journal)
        inject_metadata(out_path, payload)
        sidecar = out_path.with_suffix(".metadata.json")
        sidecar.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    return _validate(document, out_path)


def inject_metadata(step_path: Path, payload: dict) -> None:
    """Embed the conditioning JSON as legal AP242 entities: a
    DESCRIPTIVE_REPRESENTATION_ITEM carrying the payload, wired through
    REPRESENTATION -> PROPERTY_DEFINITION_REPRESENTATION to the file's
    PRODUCT_DEFINITION — readable by any STEP parser, queryable from the 3D
    file alone."""
    text = step_path.read_text(encoding="utf-8", errors="replace")
    ids = [int(match) for match in re.findall(r"#(\d+)\s*=", text)]
    next_id = (max(ids) if ids else 0) + 1
    product = re.search(r"#(\d+)\s*=\s*PRODUCT_DEFINITION\(", text)
    context = re.search(
        r"#(\d+)\s*=\s*\(?[^;]{0,200}GEOMETRIC_REPRESENTATION_CONTEXT", text
    )
    if product is None or context is None:
        raise RuntimeError("could not locate PRODUCT_DEFINITION / context entities")
    blob = json.dumps(payload, separators=(",", ":")).replace("'", "''")
    a, b, c, d = next_id, next_id + 1, next_id + 2, next_id + 3
    entities = (
        f"#{a}=DESCRIPTIVE_REPRESENTATION_ITEM('WN3D_CONDITIONING','{blob}');\n"
        f"#{b}=REPRESENTATION('WN3D_CONDITIONING',(#{a}),#{context.group(1)});\n"
        f"#{c}=PROPERTY_DEFINITION('WN3D_CONDITIONING',"
        f"'wn3d geometric pin metadata',#{product.group(1)});\n"
        f"#{d}=PROPERTY_DEFINITION_REPRESENTATION(#{c},#{b});\n"
    )
    end = text.rfind("ENDSEC;")
    if end < 0:
        raise RuntimeError("malformed STEP file (no ENDSEC)")
    step_path.write_text(text[:end] + entities + text[end:], encoding="utf-8")


def extract_metadata(step_path: Path) -> dict | None:
    """Read the embedded conditioning JSON back out of a STEP file."""
    text = step_path.read_text(encoding="utf-8", errors="replace")
    match = re.search(
        r"DESCRIPTIVE_REPRESENTATION_ITEM\('WN3D_CONDITIONING','((?:[^']|'')*)'\)",
        text,
    )
    if match is None:
        return None
    return json.loads(match.group(1).replace("''", "'"))


def write_step(document: EditorDocument, out_path: Path) -> None:
    """Write the live body table (names + colours) as AP242, no validation.
    Also used to feed the current in-memory geometry to geometer for HLR
    footprint projections."""
    doc = TDocStd_Document(TCollection_ExtendedString("MDTV-XCAF"))
    XCAFApp_Application.GetApplication_s().InitDocument(doc)
    shape_tool = XCAFDoc_DocumentTool.ShapeTool_s(doc.Main())
    color_tool = XCAFDoc_DocumentTool.ColorTool_s(doc.Main())

    # One assembly root, bodies as components: viewers (incl. the wn3d
    # browser) only resolve XCAF colours reliably for a single free shape —
    # N top-level shapes made them fall back to uncoloured.
    builder = BRep_Builder()
    compound = TopoDS_Compound()
    builder.MakeCompound(compound)
    for body in document.bodies:
        builder.Add(compound, body.solid)
    root = shape_tool.AddShape(compound, False)  # one free shape, sub-shape colours
    TDataStd_Name.Set_s(root, TCollection_ExtendedString(document.path.stem))

    for body in document.bodies:
        label = shape_tool.AddSubShape(root, body.solid)
        if label is None or label.IsNull():
            continue
        TDataStd_Name.Set_s(label, TCollection_ExtendedString(body.name))
        if body.color is not None:
            color = Quantity_Color(*body.color, Quantity_TOC_RGB)
            color_tool.SetColor(label, color, XCAFDoc_ColorType.XCAFDoc_ColorGen)
            color_tool.SetColor(label, color, XCAFDoc_ColorType.XCAFDoc_ColorSurf)
        # Per-face colours (originals and paintbrush edits) as sub-shapes.
        if body.mesh is not None and body.mesh.face_colors:
            face_map = TopTools_IndexedMapOfShape()
            TopExp.MapShapes_s(body.solid, TopAbs_FACE, face_map)
            for face_id, rgb in body.mesh.face_colors.items():
                if not (1 <= face_id <= face_map.Extent()):
                    continue
                try:
                    face = TopoDS.Face_s(face_map.FindKey(face_id))
                    sub_label = shape_tool.AddSubShape(root, face)
                    if sub_label.IsNull():
                        continue
                    color_tool.SetColor(
                        sub_label,
                        Quantity_Color(*rgb, Quantity_TOC_RGB),
                        XCAFDoc_ColorType.XCAFDoc_ColorSurf,
                    )
                except Exception:
                    continue

    # The schema parameter only exists once a STEP controller has been
    # instantiated, so the writer must be created before SetCVal.
    writer = STEPCAFControl_Writer()
    if not Interface_Static.SetCVal_s("write.step.schema", "AP242DIS"):
        raise RuntimeError("could not select the AP242 write schema")
    writer.SetColorMode(True)
    writer.SetNameMode(True)
    if not writer.Transfer(doc, STEPControl_AsIs):
        raise RuntimeError("XCAF -> STEP transfer failed")
    status = writer.Write(str(out_path))
    if status != IFSelect_RetDone:
        raise RuntimeError(f"STEP write failed for {out_path}")


def document_step_bytes(document: EditorDocument) -> bytes:
    """Current in-memory geometry as STEP bytes (for geometer projections)."""
    import tempfile

    with tempfile.TemporaryDirectory(prefix="step_editor_proj_") as temp:
        path = Path(temp) / "current.step"
        write_step(document, path)
        return path.read_bytes()


def _validate(document: EditorDocument, out_path: Path) -> ExportReport:
    reread = EditorDocument.load(out_path)
    try:
        geometer.model_bounds(out_path)
        bounds_ok = True
    except Exception:
        bounds_ok = False
    return ExportReport(
        path=out_path,
        body_count=len(document.bodies),
        reread_body_count=len(reread.bodies),
        bounds_ok=bounds_ok,
    )
