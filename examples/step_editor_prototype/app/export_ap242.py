"""AP242 export: rebuild a fresh XCAF document from the editor's body table
(names + colours), write with the AP242 schema, then validate the result by
re-reading it with OCP and geometer.

The pin/hitbox metadata entity injection lands in M8; this module already owns
the writer + validation skeleton so every milestone exports through one path.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import geometer

from OCP.IFSelect import IFSelect_RetDone
from OCP.Interface import Interface_Static
from OCP.Quantity import Quantity_Color, Quantity_TOC_RGB
from OCP.STEPCAFControl import STEPCAFControl_Writer
from OCP.STEPControl import STEPControl_AsIs
from OCP.TCollection import TCollection_ExtendedString
from OCP.TDataStd import TDataStd_Name
from OCP.TDocStd import TDocStd_Document
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


def export_ap242(document: EditorDocument, out_path: Path | None = None) -> ExportReport:
    out_path = out_path or conditioned_path(document.path)

    doc = TDocStd_Document(TCollection_ExtendedString("MDTV-XCAF"))
    XCAFApp_Application.GetApplication_s().InitDocument(doc)
    shape_tool = XCAFDoc_DocumentTool.ShapeTool_s(doc.Main())
    color_tool = XCAFDoc_DocumentTool.ColorTool_s(doc.Main())

    for body in document.bodies:
        label = shape_tool.AddShape(body.solid, False)
        TDataStd_Name.Set_s(label, TCollection_ExtendedString(body.name))
        if body.color is not None:
            color = Quantity_Color(*body.color, Quantity_TOC_RGB)
            color_tool.SetColor(label, color, XCAFDoc_ColorType.XCAFDoc_ColorGen)
            color_tool.SetColor(label, color, XCAFDoc_ColorType.XCAFDoc_ColorSurf)

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

    return _validate(document, out_path)


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
