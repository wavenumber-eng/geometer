"""Colours page: a reference photo of the real part (option A, 2026-11-01).

Drop a photo (a file, or an image dragged straight out of a browser), paste one
(Ctrl+V after "Copy image") or load it from disk. The panel shows the photo and
the part's palette (app/part_photo.py); a palette swatch sets the current
colour for the normal paint tools. "Match model to photo" proposes a photo
colour for each colour group on the model, previews it live, and changes the
model only on "Apply photo" (Cancel puts every colour back).
"""

from __future__ import annotations

import urllib.request
from pathlib import Path

import numpy as np
from urllib.parse import quote

from PySide6.QtCore import QBuffer, QByteArray, QIODevice, Qt, QUrl
from PySide6.QtGui import QColor, QDesktopServices, QGuiApplication, QImage, QKeySequence, QPixmap, QShortcut
from PySide6.QtWidgets import (QComboBox, QFileDialog, QFrame, QGridLayout, QHBoxLayout, QLabel,
                               QPushButton, QSpinBox, QVBoxLayout, QWidget)

from .. import part_photo
from .base import make_apply_button, shrink_to_grid

THUMB_W, THUMB_H = 184, 120
MAX_DOWNLOAD = 15 * 1024 * 1024  # an image dragged from a browser arrives as its URL
CACHE = Path(__file__).resolve().parents[2] / "photo_cache"
IMAGE_SEARCH = "https://www.google.com/search?tbm=isch&q={}"


def search_query(document) -> str:
    """What to image-search for: manufacturer + MPN from Identity (step 2),
    else the file name. Empty when there is no document."""
    if document is None:
        return ""
    identity = getattr(document, "identity", None) or {}
    mpn = (identity.get("mpn") or "").strip()
    if not mpn:
        return Path(getattr(document, "path", "") or "").stem
    return " ".join(x for x in ((identity.get("manufacturer") or "").strip(), mpn) if x)


def image_search_url(document) -> str:
    query = search_query(document)
    return IMAGE_SEARCH.format(quote(query, safe="")) if query else ""


def qimage_to_rgb(image: QImage) -> np.ndarray:
    image = image.convertToFormat(QImage.Format.Format_RGB888)
    w, h, line = image.width(), image.height(), image.bytesPerLine()
    raw = np.frombuffer(image.constBits(), np.uint8, count=image.sizeInBytes())
    return raw.reshape(h, line)[:, :w * 3].reshape(h, w, 3).copy()


def png_bytes(image: QImage) -> bytes:
    data = QByteArray()
    buffer = QBuffer(data)
    buffer.open(QIODevice.OpenModeFlag.WriteOnly)
    image.save(buffer, "PNG")
    return bytes(data.data())


def image_from_mime(mime) -> tuple[QImage | None, bytes, str, str]:
    """(image, raw bytes, name, source) from a drop or the clipboard."""
    if mime.hasImage():
        image = QImage(mime.imageData())
        return image, png_bytes(image), "pasted image", "clipboard / dragged image"
    for url in mime.urls() if mime.hasUrls() else []:
        if url.isLocalFile():
            return image_from_file(Path(url.toLocalFile()))
        if url.scheme() in ("http", "https"):
            return image_from_url(url.toString())
    return None, b"", "", ""


def image_from_file(path: Path) -> tuple[QImage | None, bytes, str, str]:
    raw = path.read_bytes()
    image = QImage.fromData(raw)
    return (image if not image.isNull() else None), raw, path.name, f"file {path}"


def image_from_url(url: str) -> tuple[QImage | None, bytes, str, str]:
    """One download of an image the person dragged in (never a search)."""
    request = urllib.request.Request(url, headers={"User-Agent": "WN3D-STEP-Editor"})
    with urllib.request.urlopen(request, timeout=15) as response:
        raw = response.read(MAX_DOWNLOAD + 1)
    if len(raw) > MAX_DOWNLOAD:
        return None, b"", "", ""
    image = QImage.fromData(raw)
    return (image if not image.isNull() else None), raw, url.rsplit("/", 1)[-1][:80], url


class PhotoDrop(QLabel):
    """The thumbnail box: also the drop target."""

    def __init__(self, on_mime) -> None:
        super().__init__("Drop a photo of the real part here\n(file or browser image), or paste it (Ctrl+V)")
        self._on_mime = on_mime
        self.setAcceptDrops(True)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setFixedSize(THUMB_W, THUMB_H)
        self.setFrameShape(QFrame.Shape.StyledPanel)
        self.setWordWrap(True)

    def dragEnterEvent(self, event) -> None:  # noqa: N802 - Qt API
        if event.mimeData().hasImage() or event.mimeData().hasUrls():
            event.acceptProposedAction()

    def dropEvent(self, event) -> None:  # noqa: N802 - Qt API
        self._on_mime(event.mimeData())
        event.acceptProposedAction()


def _chip(hex_color: str, text: str = "", width: int = 26) -> QPushButton:
    button = QPushButton(text)
    button.setFixedSize(width, 20)
    dark = QColor(hex_color).lightness() < 128
    button.setStyleSheet(f"QPushButton {{background: {hex_color}; border: 1px solid #00000060; font-size: 8px;"
                         f" color: {'#ffffff' if dark else '#101010'}; padding: 0;}}")
    return button


def _chip_tip(entry: dict) -> str:
    share = f"{entry['share']:.0%} of the part"
    if entry.get("swatch"):
        return (f"Metal: {entry['swatch']} {entry['swatch_hex']} ({entry['swatch_meaning']}), {share}.\n"
                f"The photo shows {entry['hex']}: a metal's photo colour is tinted by what it\n"
                f"reflects, so it snaps to the plating swatch nearest its lit side.\n"
                f"Click: make it the current colour")
    if entry.get("shade_of"):
        return (f"{entry['hex']}  {share}: shading of {entry['shade_of']} (same tint, lit / in shadow)\n"
                f"Click: make it the current colour")
    return f"{entry['hex']}  {share}\nClick: make it the current colour"


def match_options(palette: list[dict]) -> list[tuple[str, str | None]]:
    """The choices for one model colour: keep, each photo colour (metal as its
    plating swatch), then every plating swatch so a metal can be overridden."""
    options: list[tuple[str, str | None]] = [("keep", None)]
    for entry in palette:
        if entry.get("swatch"):
            options.append((f"{entry['swatch']} {entry['swatch_hex']} (photo metal)", entry["swatch_hex"]))
        else:
            options.append((f"{entry['hex']} (photo)", entry["hex"]))
    seen = {hex_color for _t, hex_color in options}
    for name, hex_color, meaning in part_photo.METAL_SWATCHES:
        if hex_color not in seen:
            options.append((f"{name} {hex_color} ({meaning})", hex_color))
    return options


class PhotoPanel(QWidget):
    """Reference photo section of the Colours page (owned by ColorsTool)."""

    def __init__(self, tool) -> None:
        super().__init__()
        self.tool = tool
        self.rgb = None  # the photo as an array
        self.raw, self.name, self.source = b"", "", ""
        self.palette: list[dict] = []
        self.groups: list[dict] = []
        self.combos: list[QComboBox] = []
        self._saved = None  # model colours before the preview
        self._build()

    # ----------------------------------------------------------------- layout

    def _build(self) -> None:
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 6, 0, 0)
        layout.addWidget(QLabel("<b>Reference photo</b> (the real part)"))
        self.drop = PhotoDrop(self.load_mime)
        top = QHBoxLayout()
        top.setSpacing(4)
        top.addWidget(self.drop)
        top.addLayout(self._source_row())
        layout.addLayout(top)
        layout.addLayout(self._settings_row())
        self.palette_row = QHBoxLayout()
        self.palette_row.setSpacing(2)
        layout.addLayout(self.palette_row)
        self.match_button = QPushButton("Match model to photo")
        shrink_to_grid(self.match_button)
        self.match_button.setToolTip("Propose a photo colour for each colour on the model:\n"
                                     "pins -> the photo's metal, the main body -> the photo's body.\n"
                                     "Previewed live; nothing changes until Apply photo.")
        self.match_button.setEnabled(False)
        self.match_button.clicked.connect(self.match)
        layout.addWidget(self.match_button)
        self.table = QGridLayout()
        self.table.setSpacing(3)
        layout.addLayout(self.table)
        layout.addLayout(self._commit_row())
        shortcut = QShortcut(QKeySequence(QKeySequence.StandardKey.Paste), self)
        shortcut.setContext(Qt.ShortcutContext.WidgetWithChildrenShortcut)
        shortcut.activated.connect(self.paste)

    def _source_row(self) -> QVBoxLayout:
        """Search / Load / Paste in a column to the right of the photo."""
        row = QVBoxLayout()
        row.setSpacing(3)
        load = QPushButton("Load ...")
        load.setToolTip("Load a photo of the real part from disk")
        load.clicked.connect(self.load_dialog)
        paste = QPushButton("Paste")
        paste.setToolTip("Paste an image copied from a browser (right-click > Copy image)")
        paste.clicked.connect(self.paste)
        search = QPushButton("Search")
        search.setToolTip("Open Google Images for this part (manufacturer + MPN from step 2,\n"
                          "else the file name) in your browser; drag a photo back here.")
        search.clicked.connect(self.search_images)
        for button in (search, load, paste):
            shrink_to_grid(button)
            row.addWidget(button)
        row.addStretch(1)
        return row

    def search_images(self) -> None:
        url = image_search_url(self.tool.ctx.document)
        if not url:
            self.tool.status("Reference photo: open a model first")
            return
        QDesktopServices.openUrl(QUrl(url))
        self.tool.status(f"Reference photo: searching images for '{search_query(self.tool.ctx.document)}'")

    def _settings_row(self) -> QGridLayout:
        """Colours / Backdrop as a two-row grid (side by side is too wide)."""
        grid = QGridLayout()
        grid.setSpacing(3)
        self.colours_spin = self._spin(2, 8, part_photo.DEFAULT_COLOURS,
                                       "How many colours to look for in the photo")
        self.tolerance_spin = self._spin(0, 120, part_photo.DEFAULT_TOLERANCE,
                                         "How far a pixel must be from the backdrop to count as the part\n"
                                         "(raise it if backdrop shading shows up in the palette)")
        for row, (text, spin) in enumerate((("Colours", self.colours_spin), ("Backdrop", self.tolerance_spin))):
            label = QLabel(text)
            label.setToolTip(spin.toolTip())
            grid.addWidget(label, row, 0)
            grid.addWidget(spin, row, 1)
        grid.setColumnStretch(1, 1)
        return grid

    def _spin(self, low: int, high: int, value: int, tip: str) -> QSpinBox:
        spin = QSpinBox()
        spin.setRange(low, high)
        spin.setValue(value)
        spin.setToolTip(tip)
        spin.valueChanged.connect(lambda _v: self.extract())
        return spin

    def _commit_row(self) -> QHBoxLayout:
        row = QHBoxLayout()
        self.apply_button = make_apply_button("Apply photo")
        self.apply_button.setToolTip("Apply the matched photo colours (journaled, undoable)")
        self.apply_button.setEnabled(False)
        self.apply_button.clicked.connect(self.apply)
        self.cancel_button = QPushButton("Cancel")
        self.cancel_button.setEnabled(False)
        self.cancel_button.clicked.connect(self.cancel)
        row.addWidget(self.apply_button, 1)
        row.addWidget(self.cancel_button)
        return row

    # ------------------------------------------------------------- loading

    def load_dialog(self) -> None:
        path, _ = QFileDialog.getOpenFileName(self, "Photo of the real part", "",
                                              "Images (*.png *.jpg *.jpeg *.webp *.bmp *.gif)")
        if path:
            self.set_image(*image_from_file(Path(path)))

    def paste(self) -> None:
        self.load_mime(QGuiApplication.clipboard().mimeData())

    def load_mime(self, mime) -> None:
        try:
            self.set_image(*image_from_mime(mime))
        except Exception as exc:  # noqa: BLE001 - a bad drop must not break the page
            self.tool.status(f"Reference photo: couldn't read that ({exc})")

    def set_image(self, image, raw: bytes, name: str, source: str) -> None:
        if image is None or image.isNull():
            self.tool.status("Reference photo: that isn't an image the editor can read")
            return
        self.cancel()
        self.rgb, self.raw, self.name, self.source = qimage_to_rgb(image), raw, name, source
        self.drop.setPixmap(QPixmap.fromImage(image).scaled(
            THUMB_W, THUMB_H, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation))
        self._cache()
        self.extract()

    def _cache(self) -> None:
        """Keep a copy by hash so the photo can be found again (ML corpus, review)."""
        record = part_photo.photo_record(self.raw, self.name, self.source, [], 0, 0)
        CACHE.mkdir(exist_ok=True)
        target = CACHE / f"{record['sha256']}{Path(self.name).suffix.lower() or '.png'}"
        if not target.exists():
            target.write_bytes(self.raw)

    # ------------------------------------------------------------- palette

    def extract(self) -> None:
        if self.rgb is None:
            return
        self.palette = part_photo.extract_palette(self.rgb, self.colours_spin.value(), self.tolerance_spin.value())
        self._show_palette()
        document = self.tool.ctx.document
        if document is not None:
            document.reference_photo = part_photo.photo_record(
                self.raw, self.name, self.source, self.palette,
                self.colours_spin.value(), self.tolerance_spin.value())
        self.match_button.setEnabled(document is not None and bool(self.palette))
        if self._saved is not None:
            self.match()  # settings changed mid-preview: re-propose
        roles = part_photo.suggest_roles(self.palette)
        self.tool.status(f"Reference photo: {len(self.palette)} colour(s); body {roles['body']}, "
                         f"metal {roles['metal'] or 'none found'} - click a swatch to paint with it")

    def _show_palette(self) -> None:
        while self.palette_row.count():
            item = self.palette_row.takeAt(0)
            if item.widget():
                item.widget().deleteLater()
        for p in self.palette:
            chip = _chip(part_photo.paint_hex(p), p.get("swatch") or f"{p['share']:.0%}", 38)
            chip.setToolTip(_chip_tip(p))
            chip.clicked.connect(lambda _c=False, h=part_photo.paint_hex(p): self.tool.set_color(h))
            self.palette_row.addWidget(chip)
        self.palette_row.addStretch(1)

    # ------------------------------------------------------- match + preview

    def match(self) -> None:
        document = self.tool.ctx.document
        if document is None or not self.palette:
            return
        self._restore()
        self.groups = part_photo.colour_groups(document, self.tool.ctx.window.pins)
        plan = part_photo.plan_mapping(self.groups, part_photo.suggest_roles(self.palette))
        self._show_table(plan)
        self.preview()

    def _show_table(self, plan) -> None:
        while self.table.count():
            item = self.table.takeAt(0)
            if item.widget():
                item.widget().deleteLater()
        self.combos = []
        for row, (group, proposed) in enumerate(zip(self.groups[:8], plan)):
            self.table.addWidget(_chip(group["hex"]), row, 0)
            where = "pins" if group["on_pins"] >= 0.5 else "body"
            self.table.addWidget(QLabel(f"{where} {group['share']:.0%}  ->"), row, 1)
            combo = QComboBox()
            for text, hex_color in match_options(self.palette):
                combo.addItem(text, hex_color)
            combo.setCurrentIndex(max(0, combo.findData(proposed)))
            combo.currentIndexChanged.connect(lambda _i: self.preview())
            self.table.addWidget(combo, row, 2)
            self.combos.append(combo)

    def _choices(self) -> list[tuple[str, list]]:
        return [(combo.currentData(), group["keys"]) for combo, group in zip(self.combos, self.groups)
                if combo.currentData()]

    def preview(self) -> None:
        self._restore()
        document = self.tool.ctx.document
        self._saved = [(b.color, dict(b.mesh.face_colors) if b.mesh is not None else None)
                       for b in document.bodies]
        for hex_color, keys in self._choices():
            self.tool._paint_many(keys, hex_color)
        self.tool.ctx.scene.plotter.render()
        self.apply_button.setEnabled(bool(self._choices()))
        self.cancel_button.setEnabled(True)
        self.tool.status("Reference photo: previewing - Apply photo to keep it, Cancel to undo")

    def _restore(self) -> bool:
        """Put back the colours from before the preview (True if there was one)."""
        if self._saved is None:
            return False
        for body, (color, faces) in zip(self.tool.ctx.document.bodies, self._saved):
            body.color = color
            if body.mesh is not None and faces is not None:
                body.mesh.face_colors = faces
        self._saved = None
        return True

    def cancel(self) -> None:
        if self._restore():
            self.tool.ctx.window.document_mutated()
            self.tool.status("Reference photo: preview cancelled, colours restored")
        self.apply_button.setEnabled(False)
        self.cancel_button.setEnabled(False)

    def apply(self) -> None:
        choices = self._choices()
        if self._saved is None or not choices:
            return
        self._saved = None
        for hex_color, keys in choices:  # one journal entry per colour: replays like Apply Color
            self.tool.ctx.journal.record(
                tool=self.tool.id, params={"action": "photo_palette", "target": "Faces"},
                inputs={"keys": [list(k) for k in keys], "photo_sha256": self.tool.ctx.document.reference_photo["sha256"]},
                result={"color": hex_color})
        self.tool.ctx.document.reference_photo["applied"] = [h for h, _k in choices]
        self.apply_button.setEnabled(False)
        self.cancel_button.setEnabled(False)
        self.tool.ctx.window.document_mutated()
        self.tool.status(f"Reference photo: applied {len(choices)} photo colour(s)")

    def document_changed(self) -> None:
        """A different / reloaded document: drop the preview state."""
        self._saved = None
        self.groups, self.combos = [], []
        self.apply_button.setEnabled(False)
        self.cancel_button.setEnabled(False)
        self.match_button.setEnabled(self.rgb is not None and self.tool.ctx.document is not None)
