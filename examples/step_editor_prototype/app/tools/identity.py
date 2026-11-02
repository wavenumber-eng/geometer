"""Step 2 Identity + Provenance: what this part is and where the model came
from. Plain form fields; saved on the document as `identity` and embedded in
the conditioned file's metadata. Fields are prefilled from the file name and
the STEP header when possible."""

from __future__ import annotations

from urllib.parse import quote

from PySide6.QtCore import QUrl
from PySide6.QtGui import QDesktopServices
from PySide6.QtWidgets import (
    QComboBox,
    QFormLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from .base import ToolMode, make_apply_button

FIELDS = [
    ("mpn", "Part number (MPN)"),
    ("manufacturer", "Manufacturer"),
    ("package", "Package / case"),
    ("pin_count", "Pin count (expected)"),
    ("source", "Model source (site / URL)"),
    ("notes", "Notes"),
]
# Part-number lookups: (button label, search URL, color).
LOOKUPS = [
    ("LCSC", "https://www.lcsc.com/search?q={}", "#1f5fd6"),
    ("DigiKey", "https://www.digikey.com/en/products/result?keywords={}", "#cc2027"),
    ("Octopart", "https://octopart.com/search?q={}", "#13294b"),
]


def lookup_url(template: str, mpn: str) -> str:
    """A site's search URL for a part number (empty string when there is none)."""
    mpn = (mpn or "").strip()
    return template.format(quote(mpn, safe="")) if mpn else ""


CATEGORIES = ["", "connector", "IC", "passive", "discrete", "electromechanical", "magnetics",
              "crystal / oscillator", "optical", "sensor", "module", "mechanical", "thermal / shield", "other"]


def apply_identity(document, values: dict) -> None:
    document.identity = {k: v for k, v in values.items() if v not in ("", None)}


class IdentityTool(ToolMode):
    id = "identity"
    title = "2 Identity + Provenance"
    accent = "#2e86c1"
    auto_method = None  # identity is typed in; AUTO has nothing to do here

    def build_actions_widget(self) -> QWidget:
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(8, 8, 8, 8)
        form = QFormLayout()
        self.edits = {}
        for key, label in FIELDS:
            edit = QLineEdit()
            self.edits[key] = edit
            form.addRow(label, edit)
            if key == "mpn":
                # Look the typed part number up on a distributor site.
                row = QHBoxLayout()
                self.lookup_buttons = []
                for name, template, color in LOOKUPS:
                    button = QPushButton(name)
                    button.setStyleSheet(
                        f"QPushButton {{background-color: {color}; color: #ffffff; font-weight: 700;"
                        f" padding: 4px 6px; border-radius: 3px;}}"
                        "QPushButton:disabled {background-color: #5a5e66; color: #b9bcc2;}")
                    button.setToolTip(f"Search {name} for this part number in your web browser")
                    button.clicked.connect(lambda _c=False, t=template: self._open_lookup(t))
                    row.addWidget(button)
                    self.lookup_buttons.append(button)
                form.addRow("Look up", row)
                edit.textChanged.connect(self._update_lookups)
        self.category = QComboBox()
        self.category.addItems(CATEGORIES)
        form.addRow("Category", self.category)
        layout.addLayout(form)
        self.apply_button = make_apply_button("Save Identity")
        self.apply_button.clicked.connect(self.apply)
        layout.addWidget(self.apply_button)
        self.saved_label = QLabel("")
        layout.addWidget(self.saved_label)
        layout.addStretch(1)
        return widget

    def _update_lookups(self, text: str) -> None:
        for button in self.lookup_buttons:
            button.setEnabled(bool(text.strip()))

    def _open_lookup(self, template: str) -> None:
        url = lookup_url(template, self.edits["mpn"].text())
        if url:
            QDesktopServices.openUrl(QUrl(url))

    def enter(self) -> None:
        self.status("Identity: fill in what you know, then Save Identity")
        self._load()

    def on_document_changed(self) -> None:
        if self._actions_widget is not None:
            self._load()

    def _load(self) -> None:
        doc = self.ctx.document
        if doc is None or self._actions_widget is None:
            return
        current = getattr(doc, "identity", None) or {}
        for key, edit in self.edits.items():
            edit.setText(str(current.get(key, "")))
        # blank until typed: a file name is not a part number
        self.edits["mpn"].setPlaceholderText("type the part number")
        self.category.setCurrentText(current.get("category", ""))
        pins = len(self.ctx.window.pins.pins)
        if not current.get("pin_count") and pins:
            self.edits["pin_count"].setPlaceholderText(f"{pins} pins detected")
        self.saved_label.setText("Saved." if current else "Not saved yet.")

    def has_unsaved(self) -> bool:
        """The form differs from what is saved (Tab saves it)."""
        doc = self.ctx.document
        if doc is None or self._actions_widget is None:
            return False
        saved = getattr(doc, "identity", None) or {}
        typed = {key: edit.text().strip() for key, edit in self.edits.items()}
        typed["category"] = self.category.currentText()
        return any(value != str(saved.get(key, "") or "") for key, value in typed.items())

    def apply(self) -> None:
        doc = self.ctx.document
        if doc is None:
            return
        values = {key: edit.text().strip() for key, edit in self.edits.items()}
        values["category"] = self.category.currentText()
        apply_identity(doc, values)
        self.ctx.journal.record("identity", dict(doc.identity), {}, {})
        self.saved_label.setText("Saved.")
        self.status("Identity saved (embedded on export)")
