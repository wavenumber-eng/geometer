"""Step 1 Model Health + Repair: report what is wrong with each body, then fix
the mechanical defects (inside-out solids, open / unsewn shells, invalid
topology) with OpenCascade's ShapeFix. The fix is deterministic, so the
journal just records that it ran and replay re-runs it."""

from __future__ import annotations

from OCP.BRepBuilderAPI import BRepBuilderAPI_MakeSolid, BRepBuilderAPI_Sewing
from OCP.BRepCheck import BRepCheck_Analyzer
from OCP.BRepGProp import BRepGProp
from OCP.GProp import GProp_GProps
from OCP.ShapeAnalysis import ShapeAnalysis_FreeBounds
from OCP.ShapeFix import ShapeFix_Shape
from OCP.TopAbs import TopAbs_EDGE, TopAbs_FACE, TopAbs_SHELL, TopAbs_SOLID
from OCP.TopExp import TopExp, TopExp_Explorer
from OCP.TopoDS import TopoDS
from OCP.TopTools import TopTools_IndexedMapOfShape
from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QComboBox,
    QDoubleSpinBox,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QSpinBox,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)

from .base import OperationCancelled, ToolMode, make_apply_button


MM = " mm"
# The settings a person picks for each check before Preview / Fix:
# (name, label, kind, spec, role). kind "combo": [(text, value)]; "spin":
# (default, min, max, decimals, suffix, scale), the value used is shown * scale;
# "int": (default, min, max). role "detect" feeds the check, "fix" the fix,
# "both" both.
PARAMS = {
    "far_from_origin": [("target", "Move to", "combo",
                         [("geometric mean", "mean"), ("box centre", "box"), ("seat (bottom at Z 0)", "seat")], "fix")],
    "units": [("factor", "Scale", "combo", [("x 25.4 (inch)", 25.4), ("/ 25.4", 1 / 25.4),
                                            ("x 1000 (m)", 1000.0), ("/ 1000 (um)", 0.001)], "fix")],
    "surfaces_only": [("tolerance", "Sew gap", "spin", (0.0001, 0.00001, 1.0, 5, MM, 1.0), "fix")],
    "openings": [("mode", "Close by", "combo", [("matching the opening", "match"), ("sewing shut (cracks)", "sew"),
                                                 ("flat patch", "flat"), ("smooth patch", "smooth"),
                                                 ("tangent patch", "tangent")], "fix"),
                 ("tolerance", "Sew gap", "spin", (0.0001, 0.00001, 1.0, 5, MM, 1.0), "fix")],
    "inside_out": [],
    "invalid": [("tolerance", "Precision", "spin", (0.0001, 0.00001, 0.1, 5, MM, 1.0), "fix")],
    "internal_voids": [("max_share", "Fill up to", "spin", (5.0, 0.01, 100.0, 2, " % of body", 0.01), "detect")],
    "double_vertices": [("merge_mm", "Within", "spin", (0.001, 0.00001, 1.0, 5, MM, 1.0), "both")],
    "star_vertices": [("min_edges", "Hub edges >=", "int", (10, 4, 200), "detect"),
                      ("planar_deg", "Flat within", "spin", (1.0, 0.0, 30.0, 1, " deg", 1.0), "detect"),
                      ("angle_deg", "Merge angle", "spin", (0.0, 0.0, 10.0, 2, " deg", 1.0), "fix")],
    "tiny_edges": [("tolerance_mm", "Shorter than", "spin", (0.1, 0.001, 100.0, 3, " um", 0.001), "detect"),
                   ("tolerance", "Fix precision", "spin", (0.1, 0.001, 100.0, 3, " um", 0.001), "fix")],
    "z_fighting": [("distance", "Push", "spin", (0.005, 0.0001, 1.0, 4, MM, 1.0), "fix")],
    "disjoint": [("action", "Do", "combo", [("split solids into bodies", "split"), ("delete slivers", "drop"),
                                            ("split + delete slivers", "both")], "fix"),
                 ("sliver_extent", "Sliver under", "spin", (10.0, 0.1, 100.0, 1, " % of model", 0.01), "detect")],
}
PARAM_TIPS = {
    "target": "Where the model is moved: its geometric mean, its bounding-box centre, or seated (X / Y centred, "
              "bottom at Z 0)",
    "factor": "Scale the whole model by this factor",
    "mode": "matching: a crack (unsewn seam) is sewn shut at its own gap, a hole gets a flat patch\n"
            "sewing shut: sew the two sides together, no new face\n"
            "flat: a planar face on the loop (flat loops only)\nsmooth: a surface through the loop\n"
            "tangent: that surface, kept tangent to the faces around it",
    "tolerance": "Gaps up to this are closed when faces are sewn / fixed",
    "max_share": "Hollows sealed inside a body (BREP_WITH_VOIDS) are listed and filled only while\n"
                 "together they are at most this share of the body's volume: a big cavity may be meant",
    "merge_mm": "Two separate vertices closer than this are one (counted, then sewn together)",
    "min_edges": "A vertex with at least this many edges is a star hub (counted)",
    "planar_deg": "A hub is a star only if all its edges leave it in one plane, within this angle "
                  "(a flat fan; a cone tip or corner is real shape)",
    "angle_deg": "0: merge only faces on exactly one surface; more: also faces meeting at up to this angle",
    "tolerance_mm": "Edges shorter than this are counted as tiny",
    "distance": "How far the body is pushed off the coincident face",
    "action": "What Fix does: split a body made of separate solids into one body per solid (nothing is "
              "lost), or delete floating slivers (Preview first: a split-off pin can look like one)",
    "sliver_extent": "A sliver must be smaller than this in every direction (and under 0.1% of the volume)",
}


def _param_widget(kind: str, spec):
    if kind == "combo":
        widget = QComboBox()
        for text, value in spec:
            widget.addItem(text, value)
        return widget, widget.currentIndexChanged
    if kind == "int":
        widget = QSpinBox()
        widget.setRange(spec[1], spec[2])
        widget.setValue(spec[0])
        return widget, widget.valueChanged
    default, low, high, decimals, suffix, _scale = spec
    widget = QDoubleSpinBox()
    widget.setRange(low, high)
    widget.setDecimals(decimals)
    widget.setValue(default)
    widget.setSuffix(suffix)
    return widget, widget.valueChanged


def _param_value(widget, kind: str, spec):
    if kind == "combo":
        return widget.currentData()
    if kind == "int":
        return int(widget.value())
    return float(widget.value()) * spec[5]


def _closing(op: dict, item: dict) -> dict:
    """Resolve "match" for one opening: a crack is sewn at 1.5x its own gap
    (at least the sew gap setting), a hole gets a flat patch. The journal
    records what was actually done."""
    gap = item.get("gap")
    mode = op.get("mode", "match")
    if mode == "match":
        mode = "sew" if gap is not None else "flat"
    tolerance = op.get("tolerance", 1e-4)
    if mode == "sew" and gap is not None:
        tolerance = max(tolerance, 1.5 * gap)
    return {"mode": mode, "tolerance": tolerance}


def _count(shape, kind) -> int:
    m = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(shape, kind, m)
    return m.Extent()


def _volume(shape) -> float:
    g = GProp_GProps()
    BRepGProp.VolumeProperties_s(shape, g)
    return g.Mass()


def _free_edges(shape) -> int:
    fb = ShapeAnalysis_FreeBounds(shape, 1.0e-6, False, False)
    return _count(fb.GetClosedWires(), TopAbs_EDGE) + _count(fb.GetOpenWires(), TopAbs_EDGE)


def body_health(solid) -> dict:
    """Plain-language health of one body."""
    solids, shells, faces = _count(solid, TopAbs_SOLID), _count(solid, TopAbs_SHELL), _count(solid, TopAbs_FACE)
    vol = _volume(solid) if solids else 0.0
    free = _free_edges(solid)
    problems = []
    if not BRepCheck_Analyzer(solid).IsValid():
        problems.append("invalid topology")
    if solids == 0:
        problems.append("no solid (surfaces only)")
    if solids and vol < 0:
        problems.append("inside-out (negative volume)")
    if free:
        problems.append(f"{free} open (free) edges")
    return {"solids": solids, "shells": shells, "faces": faces, "volume": vol,
            "free_edges": free, "problems": problems}


def _solids_from_shells(shape, tolerance: float):
    """Sew loose faces, then make ONE solid per closed shell (outward); open
    shells and stray faces are kept as they are. (Putting every shell into
    one solid makes the others cavities, and ShapeFix then drops them.)"""
    from OCP.BRep import BRep_Builder, BRep_Tool
    from OCP.ShapeFix import ShapeFix_Solid
    from OCP.TopoDS import TopoDS_Compound

    sew = BRepBuilderAPI_Sewing(tolerance)
    sew.Add(shape)
    sew.Perform()
    sewed = sew.SewedShape()
    builder, compound = BRep_Builder(), TopoDS_Compound()
    builder.MakeCompound(compound)
    closed = 0
    in_shells = TopTools_IndexedMapOfShape()
    for shell in _shapes(sewed, TopAbs_SHELL):
        TopExp.MapShapes_s(shell, TopAbs_FACE, in_shells)
        shell = TopoDS.Shell_s(shell)
        if BRep_Tool.IsClosed_s(shell):
            builder.Add(compound, ShapeFix_Solid().SolidFromShell(shell))
            closed += 1
        else:
            builder.Add(compound, shell)
    for face in _shapes(sewed, TopAbs_FACE):
        if not in_shells.Contains(face):
            builder.Add(compound, face)
    return compound, closed


def _shapes(shape, kind) -> list:
    found = TopTools_IndexedMapOfShape()
    TopExp.MapShapes_s(shape, kind, found)
    return [found.FindKey(i) for i in range(1, found.Extent() + 1)]


def _outward(shape):
    """Flip each inside-out solid on its own (not the whole compound)."""
    from OCP.BRep import BRep_Builder
    from OCP.TopoDS import TopoDS_Compound

    solids = _shapes(shape, TopAbs_SOLID)
    if not any(_volume(s) < 0 for s in solids):
        return shape, False
    builder, compound = BRep_Builder(), TopoDS_Compound()
    builder.MakeCompound(compound)
    for solid in solids:
        builder.Add(compound, solid.Reversed() if _volume(solid) < 0 else solid)
    return compound, True


def repair_solid(solid, tolerance: float = 1.0e-4):
    """Sew loose faces, close shells into solids, fix orientation and topology.
    Never loses geometry: a step that would drop faces is not taken.
    Returns (new_shape, list of what was done)."""
    done = []
    shape = solid
    faces_in = _count(solid, TopAbs_FACE)
    if _count(shape, TopAbs_SOLID) == 0:
        shape, closed = _solids_from_shells(shape, tolerance)
        if closed:
            done.append(f"closed {closed} shell(s) into solids")
    fixer = ShapeFix_Shape(shape)
    fixer.SetPrecision(tolerance)
    fixer.Perform()
    fixed = fixer.Shape()
    if not fixed.IsNull() and _count(fixed, TopAbs_FACE) >= _count(shape, TopAbs_FACE):
        if not BRepCheck_Analyzer(shape).IsValid() and BRepCheck_Analyzer(fixed).IsValid():
            done.append("fixed invalid topology")
        shape = fixed
    shape, flipped = _outward(shape)
    if flipped:
        done.append("flipped inside-out solid(s)")
    if _count(shape, TopAbs_FACE) < faces_in:
        return solid, ["repair skipped: it would have lost faces"]
    return shape, done


def repair_document(document) -> dict[int, list[str]]:
    """Repair every body that has a problem; returns {body index: actions}."""
    changes = {}
    for index, body in enumerate(document.bodies):
        if not body_health(body.solid)["problems"]:
            continue
        new_shape, done = repair_solid(body.solid)
        if done:
            body.solid = new_shape
            changes[index] = done
    if changes:
        document.remesh_all()
    document.repair_log = changes
    return changes


class RepairTool(ToolMode):
    id = "repair"
    title = "1 Model Health + Repair"
    accent = "#5d6d7e"
    auto_method = "check"

    def __init__(self, ctx) -> None:
        super().__init__(ctx)
        self.found: dict[str, list[dict]] = {}
        self.rows: dict[str, dict] = {}
        self._touched: set[int] = set()  # bodies the running fix changed

    def build_actions_widget(self) -> QWidget:
        from .. import health

        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(4)
        check = QPushButton("Check")
        check.setToolTip("Count every problem again")
        check.clicked.connect(self.check)
        layout.addWidget(check)
        for key, label, _detector in health.CHECKS:
            layout.addLayout(self._row(key, label))
        self.report = QTextEdit()
        self.report.setReadOnly(True)
        self.report.setMaximumHeight(90)
        layout.addWidget(self.report)
        self.apply_button = make_apply_button("Repair all")
        self.apply_button.setToolTip("Sew, close, flip and fix every body that needs it (CONFIRM)")
        self.apply_button.clicked.connect(self.apply)
        layout.addWidget(self.apply_button)
        layout.addStretch(1)
        return widget

    def _row(self, key: str, label: str) -> QVBoxLayout:
        """[count] name [Preview] [Fix], then (indented) the instance and
        this check's settings."""
        row = QHBoxLayout()
        extra = QGridLayout()  # settings, one per line: the panel reflow leaves grids alone
        extra.setContentsMargins(34, 0, 0, 0)
        extra.setHorizontalSpacing(6)
        extra.setVerticalSpacing(2)
        count = QPushButton("0")
        count.setFixedSize(30, 22)
        count.setStyleSheet("border: 1px solid #555; font-weight: 700;")
        count.setToolTip("Show every instance in the view (edges and vertices show through the model)")
        count.clicked.connect(lambda _c=False, k=key: self.show_found(k))
        row.addWidget(count)
        row.addWidget(QLabel(label), 1)
        instance = QComboBox()
        instance.setMinimumContentsLength(6)
        instance.setSizeAdjustPolicy(QComboBox.SizeAdjustPolicy.AdjustToMinimumContentsLengthWithIcon)
        instance.setToolTip("Look at one instance (it lights up in the view)")
        instance.activated.connect(lambda index, k=key: self.show_found(k, index))
        extra.addWidget(instance, 0, 0, 1, 2)
        widgets = {"count": count, "instance": instance, "params": {}}
        for name, text, kind, spec, role in PARAMS[key]:
            widget, changed = _param_widget(kind, spec)
            widget.setToolTip(f"{text}: {PARAM_TIPS.get(name, '')}" + ("  (re-counts)" if role != "fix" else ""))
            changed.connect(lambda *_a, k=key, r=role: self._param_changed(k, r))
            line = extra.rowCount()
            extra.addWidget(QLabel(text), line, 0)
            widget.setMaximumWidth(220)
            extra.addWidget(widget, line, 1)
            widgets["params"][name] = (widget, kind, spec, role)
        if key == "z_fighting":
            arrow = QPushButton("<->")
            arrow.setToolTip("Which body is pushed off along its own face normal\n"
                             "(default: the smaller one, usually the pin)")
            arrow.setFixedWidth(40)
            arrow.clicked.connect(self._swap_mover)
            line = extra.rowCount()
            extra.addWidget(QLabel("Moves"), line, 0)
            extra.addWidget(arrow, line, 1)
        preview = QPushButton("Preview")
        preview.setToolTip("Show in green what Fix would change, with these settings (nothing is changed yet)")
        preview.clicked.connect(lambda _c=False, k=key: self.preview(k))
        row.addWidget(preview)
        fix = QPushButton("Fix")
        fix.setToolTip(f"Fix every {label.lower()} instance with these settings (undoable)")
        fix.clicked.connect(lambda _c=False, k=key: self.fix(k))
        row.addWidget(fix)
        widgets.update(preview=preview, fix=fix)
        self.rows[key] = widgets
        lines = QVBoxLayout()
        lines.setSpacing(2)
        lines.addLayout(row)
        extra.setColumnStretch(2, 1)
        lines.addLayout(extra)
        return lines

    def _params(self, key: str, role: str) -> dict:
        """This check's setting values for the check ("detect") or the fix."""
        return {name: _param_value(widget, kind, spec)
                for name, (widget, kind, spec, r) in self.rows[key]["params"].items() if r in (role, "both")}

    def _param_changed(self, key: str, role: str) -> None:
        """A setting changed: any preview is stale; a counting setting
        re-counts this check."""
        self.ctx.scene.end_flash()
        if role != "fix" and self.found and self.ctx.document is not None:
            self._check_row(key)
            self._refresh_report()

    def enter(self) -> None:
        self.status("Model Health: press Check to count each problem; Fix them one by one, "
                    "or CONFIRM to repair everything")

    def on_document_changed(self) -> None:
        """A new model starts unchecked: every count back to 0 until Check
        (a fix re-checks by itself)."""
        self.found = {}
        if self._actions_widget is None:
            return
        for widgets in self.rows.values():
            widgets["count"].setText("0")
            widgets["count"].setStyleSheet("border: 1px solid #555; font-weight: 700;")
            widgets["instance"].clear()
            widgets["instance"].setVisible(False)
            widgets["fix"].setEnabled(False)
            widgets["preview"].setEnabled(False)
        self.report.setPlainText("Not checked yet: press Check.")
        self.ctx.scene.clear_highlight()
        self.ctx.scene.end_flash()

    def _check_row(self, key: str) -> int:
        from .. import health

        detector = {k: d for k, _l, d in health.CHECKS}[key]
        try:
            items = detector(self.ctx.document, **self._params(key, "detect"))
        except Exception as exc:  # noqa: BLE001 - one check must not hide the rest
            items = []
            self.status(f"Model Health: {key} check failed ({exc})")
        self.found[key] = items
        widgets = self.rows[key]
        widgets["count"].setText(str(len(items)))
        widgets["count"].setStyleSheet(
            "border: 1px solid #555; font-weight: 700;"
            + (" background: #8a2b2b; color: #ffffff;" if items else " color: #6fbf6f;"))
        widgets["instance"].clear()
        for item in items:
            widgets["instance"].addItem(item["label"])
        widgets["instance"].setVisible(len(items) > 1)
        widgets["fix"].setEnabled(bool(items))
        widgets["preview"].setEnabled(bool(items))
        if key == "units" and items:
            factor = widgets["params"]["factor"][0]
            factor.setCurrentIndex(max(factor.findData(items[0]["factor"]), 0))
        return len(items)

    def _refresh_report(self) -> None:
        from .. import health

        lines = [f"{label}: {item['label']}" for key, label, _d in health.CHECKS for item in self.found.get(key, [])]
        self.report.setPlainText("\n".join(lines) if lines else
                                 f"All {len(self.ctx.document.bodies)} bodies are healthy.")

    def check(self) -> int:
        from .. import health

        if self.ctx.document is None:
            return 0
        self.found = {}
        total = sum(self._check_row(key) for key, _label, _detector in health.CHECKS)
        self._refresh_report()
        return total

    def show_found(self, key: str, index: int | None = None) -> None:
        """Light up one instance (index) or every instance of a finding, edges
        and vertices drawn on top so the ones inside a body show through."""
        from .. import health

        doc = self.ctx.document
        items = self.found.get(key) or []
        if doc is None or not items:
            self.ctx.scene.clear_highlight()
            self.ctx.scene.plotter.render()
            return
        chosen = items if index is None else items[index:index + 1]
        lines, points = [], []
        for item in chosen:
            more_lines, more_points = health.item_marks(doc, item)
            lines += more_lines
            points += more_points
        self.ctx.scene.show_marks(lines, points)
        what = f"all {len(items)}" if index is None else chosen[0]["label"]
        self.status(f"Model Health: showing {what}" + ("" if lines or points else " (nothing to draw: whole model)"))

    def _swap_mover(self) -> None:
        items = self.found.get("z_fighting") or []
        index = self.rows["z_fighting"]["instance"].currentIndex()
        if 0 <= index < len(items):
            items[index]["mover"] = 1 - items[index]["mover"]
            moving = items[index]["body_a" if items[index]["mover"] == 0 else "body_b"]
            self.status(f"Z-fighting: body {moving} will be pushed off")

    def has_unsaved(self) -> bool:
        return False  # findings are not staged work: Tab must never run Repair all

    def staged_rows(self) -> list[tuple[str, str, str]]:
        from .. import health

        return [(label, item["label"], "") for key, label, _d in health.CHECKS
                for item in self.found.get(key, [])][:60]

    def _fix_op(self, key: str, item: dict, movers: dict) -> dict:
        """The journal op that fixes one instance with the row's settings
        (replayable by health.apply_fix)."""
        params = self._params(key, "fix")
        params.pop("action", None)
        op = {"action": key, **params}
        if key == "openings":
            op.update(body=item["body"], wire=item["wire"])
            op.update(_closing(op, item))
        elif key == "z_fighting":
            mover = movers.get((item["body_a"], item["body_b"]), item["mover"])
            op.update(body=item["body_a" if mover == 0 else "body_b"],
                      normal=item["normal_a" if mover == 0 else "normal_b"],
                      partner=item["body_b" if mover == 0 else "body_a"])
        elif key == "disjoint":
            op.update(body=item["body"], sliver=bool(item.get("sliver")))
        elif "body" in item:
            op["body"] = item["body"]
        return op

    def _fixable(self, key: str, items: list[dict]) -> list[dict]:
        """The instances this row's Fix acts on (Disjoint: split, delete, or both)."""
        action = self._params(key, "fix").get("action", "split")
        return [i for i in items if action == "both" or bool(i.get("sliver")) == (action == "drop")]

    def _flash_changes(self, before: list) -> None:
        """Flash what the fix changed green for 5 s; a change that moves no
        geometry (a flip) flashes the bodies it touched."""
        self.ctx.scene.flash_geometry(self._changes(before, self.ctx.document), seconds=5.0)

    def _changes(self, before: list, doc) -> list:
        from .. import health
        from ..document import _face_triangles, changed_geometry

        pieces = changed_geometry(before, [body.mesh for body in doc.bodies], health.model_size(doc))
        if not pieces:
            meshes = [doc.bodies[i].mesh for i in sorted(self._touched) if i < len(doc.bodies)]
            pieces = [_face_triangles(m, range(1, m.face_count + 1)) for m in meshes if m is not None]
        return pieces

    def _fix_one(self, key: str, item: dict, movers: dict, doc, record: bool) -> None:
        from .. import health

        if key == "far_from_origin":
            op = {"action": key, "matrix": health.recentre(doc, self._params(key, "fix")["target"]).tolist()}
            if record:
                from ..pins import transform_pins

                transform_pins(self.ctx.window.pins.pins, op["matrix"])
        else:
            op = self._fix_op(key, item, movers)
            if record:  # the live model: pins keep pointing at the same geometry
                from ..pins import fix_keeping_pins

                fix_keeping_pins(doc, self.ctx.window.pins.pins, lambda: health.apply_fix(doc, op))
            else:
                health.apply_fix(doc, op)
            if "body" in op:
                self._touched.add(op["body"])
        if record:
            self.ctx.journal.record("repair", op, {}, {"fixed": item["label"]})

    def _fix_all(self, key: str, items: list[dict], doc, record: bool) -> int:
        """Fix instances until none are left (or the rest won't fix); returns
        how many went away. The check re-runs after each fix: bodies and
        loops renumber as they are fixed."""
        self._touched = set()
        if key in ("far_from_origin", "units"):  # one model-wide fix
            self._fix_one(key, items[0], {}, doc, record)
            return 1
        return self._fix_loop(key, items, doc, record)

    def _fix_loop(self, key: str, items: list[dict], doc, record: bool) -> int:
        from .. import health

        detector = {k: d for k, _l, d in health.CHECKS}[key]
        movers = {(i["body_a"], i["body_b"]): i["mover"] for i in items if "mover" in i}
        stuck: set[str] = set()
        items = self._fixable(key, items)
        start = len(items)
        for _attempt in range(2 * start + 10):
            todo = [i for i in items if i["label"] not in stuck]
            if not todo:
                break
            self._tick(f"{self._progress_label} {start - len(items) + 1}/{start}", start - len(items), start)
            self._fix_one(key, todo[0], movers, doc, record)
            remaining = self._fixable(key, detector(doc, **self._params(key, "detect")))
            if len(remaining) >= len(items):
                stuck.add(todo[0]["label"])
            items = remaining
        return max(start - len(items), 0)

    def preview(self, key: str) -> None:
        """Run the fix on a scratch copy and show in green what it would
        change; the model itself is untouched until Fix."""
        import copy

        from ..history import _copy_body

        doc = self.ctx.document
        items = self.found.get(key) or []
        if doc is None or not items:
            return
        scratch = copy.copy(doc)
        scratch.bodies = [_copy_body(body) for body in doc.bodies]
        self._progress_label = f"Previewing {key.replace('_', ' ')}"
        try:
            fixed = self._fix_all(key, items, scratch, record=False)
        except OperationCancelled:
            return
        finally:
            self._bar_done()
        pieces = self._changes([body.mesh for body in doc.bodies], scratch)
        self.ctx.scene.flash_geometry(pieces, seconds=None)
        faces = sum(len(t) for _p, t in pieces)
        self.status(f"Preview {key.replace('_', ' ')}: {fixed} of {len(items)} instance(s) would be fixed; "
                    f"bodies {len(doc.bodies)} -> {len(scratch.bodies)}; green = what changes "
                    f"({faces} triangles). Fix to apply, or change the settings.")

    def fix(self, key: str) -> None:
        """Fix every instance with the row's settings (each journaled,
        undoable); what changed flashes green for 5 s. An instance a fix
        can't clear is skipped, not retried."""
        doc = self.ctx.document
        items = self.found.get(key) or []
        if doc is None or not items:
            return
        before = [body.mesh for body in doc.bodies]
        self._progress_label = f"Fixing {key.replace('_', ' ')}"
        try:
            fixed = self._fix_all(key, items, doc, record=True)
        except OperationCancelled:
            fixed = 0
        finally:
            self._bar_done()
        self.ctx.window.document_mutated()
        self._flash_changes(before)
        self.check()
        left = len(self.found.get(key) or [])
        self.status(f"Model Health: fixed {fixed} {key.replace('_', ' ')} instance(s)"
                    + (f"; {left} could not be fixed" if left else ""))

    _progress_label = "Repairing"

    def _tick(self, label: str, value: int, maximum: int) -> None:
        """Report on the window's progress bar (top left) so a long repair
        never looks hung; Cancel there stops at the next instance."""
        progress = getattr(self.ctx.window, "progress", None)
        if progress is not None:
            progress(label, value, maximum)

    def apply(self) -> None:
        """Repair all: every row's Fix, in the panel's order (surfaces and
        openings before z-fighting and disjoint), each with its row's
        settings (each journaled, undoable), then one green flash of
        everything that changed. Progress shows on the top-left bar; a
        cancel keeps the fixes already made (each is journaled)."""
        doc = self.ctx.document
        if doc is None:
            return
        before = [body.mesh for body in doc.bodies]
        try:
            done = self._repair_rows(doc)
        except OperationCancelled:
            done = None
        finally:
            self._bar_done()
        self._report_repair(done, before)

    def _bar_done(self) -> None:
        done_bar = getattr(self.ctx.window, "progress_done", None)
        if done_bar is not None:
            done_bar()

    def _repair_rows(self, doc) -> list[str]:
        from .. import health

        done = []
        for row, (key, label, _detector) in enumerate(health.CHECKS):
            self._progress_label = f"Repair all: {label}"
            self._tick(f"Repair all: checking {label}", row, len(health.CHECKS))
            self._check_row(key)  # counted on the model as the rows above left it
            items = self.found.get(key) or []
            if items:
                done.append(f"{label} {self._fix_all(key, items, doc, record=True)}/{len(items)}")
        return done

    def _report_repair(self, done, before) -> None:
        if done == []:
            self.status("Repair all: nothing to fix")
            return
        self.ctx.window.document_mutated()
        self._flash_changes(before)
        left = self.check()
        if done is None:
            self.status("Repair all: cancelled; the fixes already made are kept (each is in the journal)")
            return
        self.status("Repair all: fixed " + ", ".join(done) + (f"; {left} still counted" if left else ""))
