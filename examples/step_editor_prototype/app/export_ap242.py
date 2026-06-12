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

# OCCT's STEP parser rejects string literals somewhere past ~16k chars, so the
# metadata blob is split across multiple DESCRIPTIVE_REPRESENTATION_ITEMs (all
# referenced by one REPRESENTATION, concatenated in file order on read).
# 6000 raw chars stays safely under the limit even if every char is an escaped
# quote (worst case doubles to 12000).
_METADATA_CHUNK = 6000


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
    progress=None,
    source_name: str | None = None,
) -> ExportReport:
    def stage(label: str) -> None:
        if progress is not None:
            progress(label, 0, 0)

    out_path = out_path or conditioned_path(document.path)
    stage(f"Writing {out_path.name}")
    write_step(document, out_path)
    if pins is not None or journal is not None:
        from .metadata import build_metadata

        payload = build_metadata(document, pins, journal, source_name=source_name)
        stage("Embedding metadata into the AP242")
        inject_metadata(out_path, payload)
        # The conditioned STEP is the single source of truth, so PROVE the
        # embedded copy reads back identically before touching the sidecar.
        # If the round-trip ever fails, the sidecar is written as the rescue
        # copy and the export fails loudly instead of losing the metadata.
        stage("Auto-test: reading the embedded metadata back")
        sidecar = out_path.with_suffix(".metadata.json")
        extracted = extract_metadata(out_path)
        if extracted != payload:
            sidecar.write_text(json.dumps(payload, indent=2), encoding="utf-8")
            raise RuntimeError(
                "embedded metadata did not read back from the STEP — "
                f"rescue copy kept at {sidecar.name}"
            )
        if sidecar.exists():
            sidecar.unlink()
    stage("Validating: re-reading the conditioned STEP")
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
    blob = json.dumps(payload, separators=(",", ":"))
    chunks = [
        blob[i : i + _METADATA_CHUNK].replace("'", "''")
        for i in range(0, len(blob), _METADATA_CHUNK)
    ] or [""]
    lines, item_ids = [], []
    for chunk in chunks:
        lines.append(
            f"#{next_id}=DESCRIPTIVE_REPRESENTATION_ITEM"
            f"('WN3D_CONDITIONING','{chunk}');\n"
        )
        item_ids.append(next_id)
        next_id += 1
    refs = ",".join(f"#{i}" for i in item_ids)
    rep_id, prop_id, pdr_id = next_id, next_id + 1, next_id + 2
    lines.append(
        f"#{rep_id}=REPRESENTATION('WN3D_CONDITIONING',({refs}),"
        f"#{context.group(1)});\n"
    )
    lines.append(
        f"#{prop_id}=PROPERTY_DEFINITION('WN3D_CONDITIONING',"
        f"'wn3d geometric pin metadata',#{product.group(1)});\n"
    )
    lines.append(f"#{pdr_id}=PROPERTY_DEFINITION_REPRESENTATION(#{prop_id},#{rep_id});\n")
    entities = "".join(lines)
    end = text.rfind("ENDSEC;")
    if end < 0:
        raise RuntimeError("malformed STEP file (no ENDSEC)")
    step_path.write_text(text[:end] + entities + text[end:], encoding="utf-8")


def verify_metadata_text(text: str) -> dict:
    """Pure-text structural audit of the injected conditioning block.

    Re-derives every invariant inject_metadata() promises, from the raw STEP
    text alone (no OCCT): chunking, entity-id hygiene, the REPRESENTATION ->
    PROPERTY_DEFINITION -> PRODUCT_DEFINITION wiring, placement flush against
    ENDSEC, and that the reassembled JSON parses. Returns a report dict with
    ok/errors plus the facts found, so chain tests can assert on specifics.
    """
    errors: list[str] = []
    report: dict = {"ok": False, "errors": errors}

    items = list(
        re.finditer(
            r"#(\d+)=DESCRIPTIVE_REPRESENTATION_ITEM"
            r"\('WN3D_CONDITIONING','((?:[^']|'')*)'\);",
            text,
        )
    )
    if not items:
        errors.append("no WN3D_CONDITIONING items found")
        return report
    item_ids = [int(m.group(1)) for m in items]
    report["chunks"] = len(items)
    report["chunk_max"] = max(len(m.group(2)) for m in items)
    if item_ids != list(range(item_ids[0], item_ids[0] + len(item_ids))):
        errors.append(f"item ids not sequential: {item_ids}")
    oversize = [i for i, m in enumerate(items) if len(m.group(2)) > _METADATA_CHUNK]
    if oversize:
        errors.append(f"chunks over {_METADATA_CHUNK} chars at indices {oversize}")

    reps = list(
        re.finditer(
            r"#(\d+)=REPRESENTATION\('WN3D_CONDITIONING',\(([^)]*)\),#(\d+)\);", text
        )
    )
    if len(reps) != 1:
        errors.append(f"expected exactly 1 WN3D REPRESENTATION, found {len(reps)}")
        return report
    rep = reps[0]
    refs = [int(r) for r in re.findall(r"#(\d+)", rep.group(2))]
    if refs != item_ids:
        errors.append("REPRESENTATION does not reference exactly the item ids in order")
    context_id = int(rep.group(3))
    if not re.search(
        rf"#{context_id}\s*=\s*\(?[^;]{{0,200}}GEOMETRIC_REPRESENTATION_CONTEXT", text
    ):
        errors.append(f"context #{context_id} is not a GEOMETRIC_REPRESENTATION_CONTEXT")

    prop = re.search(
        r"#(\d+)=PROPERTY_DEFINITION\('WN3D_CONDITIONING',"
        r"'wn3d geometric pin metadata',#(\d+)\);",
        text,
    )
    if prop is None:
        errors.append("PROPERTY_DEFINITION missing")
        return report
    product_id = int(prop.group(2))
    if not re.search(rf"#{product_id}\s*=\s*PRODUCT_DEFINITION\(", text):
        errors.append(f"#{product_id} is not a PRODUCT_DEFINITION")

    pdr = re.search(r"#(\d+)=PROPERTY_DEFINITION_REPRESENTATION\(#(\d+),#(\d+)\);", text)
    if pdr is None:
        errors.append("PROPERTY_DEFINITION_REPRESENTATION missing")
        return report
    if int(pdr.group(2)) != int(prop.group(1)) or int(pdr.group(3)) != int(rep.group(1)):
        errors.append("PDR does not wire PROPERTY_DEFINITION to REPRESENTATION")

    block = text[items[0].start() : pdr.end()]
    if len(re.findall(r"#\d+=", block)) != len(items) + 3:
        errors.append("foreign entities interleaved inside the metadata block")
    endsec = text.rfind("ENDSEC;")
    if endsec < 0:
        errors.append("no ENDSEC")
    elif text[pdr.end() : endsec].strip():
        errors.append("metadata block is not flush against ENDSEC")
    report["tail_gap"] = max(0, endsec - pdr.end()) if endsec >= 0 else None

    all_ids = [int(i) for i in re.findall(r"#(\d+)\s*=", text)]
    if len(all_ids) != len(set(all_ids)):
        errors.append("duplicate entity ids in file")

    blob = "".join(m.group(2).replace("''", "'") for m in items)
    try:
        payload = json.loads(blob)
        report["schema"] = payload.get("schema")
        report["json_bytes"] = len(blob)
    except json.JSONDecodeError as ex:
        errors.append(f"reassembled JSON does not parse: {ex}")

    report["block_span"] = (items[0].start(), pdr.end())
    report["ok"] = not errors
    return report


def extract_metadata(step_path: Path) -> dict | None:
    """Read the embedded conditioning JSON back out of a STEP file."""
    text = step_path.read_text(encoding="utf-8", errors="replace")
    parts = re.findall(
        r"DESCRIPTIVE_REPRESENTATION_ITEM\('WN3D_CONDITIONING','((?:[^']|'')*)'\)",
        text,
    )
    if not parts:
        return None
    return json.loads("".join(part.replace("''", "'") for part in parts))


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
